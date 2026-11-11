/*
 * uart2.c
 *
 *  Created on: 2026 Jun 05 14:03:46
 *      Author: oskib
 *
 * UART-Treiber: binaere Protokollverarbeitung und Motorsteuerung.
 *
 * Empfang (4 Byte):  'B' | Speed(int8) | Steering(int8) | XOR-Checksum
 * Senden  (8 Byte):  'D' | L | M | R | 'x' | 'x' | Flag('F'/'S'/'K') | XOR
 *
 * Ablauf von MyUART_Process():
 *   Befehls-Watchdog (sicherer Zustand bei Verbindungsabbruch) ->
 *   Hindernis-Sofortreaktion -> gepufferten Befehl nachziehen ->
 *   Frame-Timeout (haengenden Frame verwerfen) -> Bytes einlesen und auf 'B'
 *   synchronisieren -> bei vollstaendigem Frame Checksum pruefen ->
 *   Befehl auswerten -> Antwort senden.
 * Ein gueltiger Fahrbefehl durchlaeuft dabei:
 *   Binaer_Befehl_Verarbeiten -> Befehl_Ausfuehren -> Motor_Ausfuehren.
 * Die Hilfsfunktionen unten stehen in dieser (aufrufnahen) Reihenfolge.
 */
#include "uart2.h"

uint8_t uart_aktiv = 0U;   /* Betriebsmodus, in main.c gesetzt */

/* Sensorwerte (von main.c gesetzt) */
uint32_t sensor_d0          = 100U;
uint32_t sensor_d1          = 100U;
uint32_t sensor_d2          =  59U;

/* 1 = Hindernis aktiv -> nur Rueckwaerts + Rotation erlaubt */
uint8_t  sensor_hindernis_aktiv = 0U;

/* 1 = Kabelbruch -> UART meldet 'K', Vorrang vor Hindernis */
uint8_t  sensor_kabelbruch_aktiv = 0U;

/* Interne Zustandsvariablen */
static uint8_t  rx_frame[UART_RX_FRAME_LEN];        /* aktueller Empfangsframe   */
static uint8_t  rx_idx              = 0U;           /* 0 = warte auf Startbyte   */
static uint32_t rx_start_us         = 0U;           /* Zeitstempel Startbyte     */

static uint8_t          uart_last_dir         = 0U; /* 0=neutral,1=vor,2=zurueck,3=rot.re,4=rot.li */
static volatile uint8_t uart_direction_change = 0U; /* 1 = Stopp-Phase aktiv     */
static uint32_t         uart_timer_id         = 0U; /* SYSTIMER-Handle           */
static uint8_t          ausstehend_valid      = 0U; /* 1 = Befehl wartet auf Ausfuehrung */
static int32_t          ausstehend_v          = 0;  /* gepufferter Speed  [km/h x10]   */
static int32_t          ausstehend_r          = 0;  /* gepufferter Radius [mm]     */
static int32_t          ausstehend_d          = 0;  /* gepufferte Rotation[km/h x10]   */

static uint32_t last_cmd_us = 0U;   /* Zeitstempel des letzten gueltigen Befehls  */
static uint8_t  cmd_aktiv   = 0U;   /* 1 = Befehl empfangen -> Watchdog scharf    */

/* SYSTIMER-Callback: Stopp-Phase nach Richtungswechsel beendet. */
void UART_DirectionStopDone(void* args)
{
    (void)args;
    uart_direction_change = 0U;
}

/* Betrag eines vorzeichenbehafteten Wertes */
static int32_t betrag(int32_t x)
{
    if (x < 0)
    {
        return -x;
    }
    return x;
}

/* 1 = Vorwaertsfahrt bei aktivem Hindernis gesperrt (Rueckwaerts/Rotation bleiben erlaubt) */
static uint8_t Vorwaerts_Gesperrt(uint8_t ist_vorwaerts)
{
    if (sensor_hindernis_aktiv == 1U && ist_vorwaerts == 1U)
    {
        return 1U;
    }
    return 0U;
}

/* Laufenden Richtungswechsel-Timer loeschen (falls vorhanden) */
static void Timer_Stoppen(void)
{
    if (uart_timer_id != 0U)
    {
        SYSTIMER_DeleteTimer(uart_timer_id);
        uart_timer_id = 0U;
    }
}

/* Motoransteuerung (bereits validierte Werte)
 *   v_kmh10 : Laengstempo [km/h x10] (+ vor / - zurueck)
 *   r_mm  : Kurvenradius [mm] (+ rechts / - links)
 *   d_kmh10 : Drehtempo [km/h x10] (nur bei v=0, r=0) */
static void Motor_Ausfuehren(int32_t v_kmh10, int32_t r_mm, int32_t d_kmh10)
{
    /* Rotation auf der Stelle */
    if (d_kmh10 != 0 && v_kmh10 == 0)
    {
        uint32_t d_abs   = (uint32_t)betrag(d_kmh10);
        uint32_t rot_pwm = d_abs * UART_MAX_PWM / UART_MAX_KMH_10;
        /* d>0 rechts, d<0 links */
        if (d_kmh10 > 0)
        {
            Motor_RotateRight(rot_pwm);
        }
        else
        {
            Motor_RotateLeft(rot_pwm);
        }
        return;
    }

    uint32_t v_abs = (uint32_t)betrag(v_kmh10);
    uint32_t pwm   = v_abs * UART_MAX_PWM / UART_MAX_KMH_10;
    uint32_t r_abs = (uint32_t)betrag(r_mm);

    if (r_mm == 0)
    {
        if (v_kmh10 > 0)
        {
            Motor_TurnRight(pwm, pwm);
        }
        else
        {
            Motor_TurnRightBackward(pwm, pwm);
        }
        return;
    }

    /* Kurvenfahrt (Differentialantrieb): d_R_mag ist der Drehzahlunterschied.
     * v_innen unterlaeuft nie, da R >= R_min > halbe Spurbreite. */
    uint32_t d_R_mag  = pwm * UART_SPURBREITE_HALB_MM / r_abs;
    uint32_t v_aussen = pwm + d_R_mag;   /* kurvenaeusseres Rad: v + omega*d */
    uint32_t v_innen  = pwm - d_R_mag;   /* kurveninneres  Rad: v - omega*d */

    /* kurvenaeusseres Rad auf den Kurven-Hoechstwert begrenzen */
    if (v_aussen > UART_MAX_PWM_CURVE)
    {
        v_aussen = UART_MAX_PWM_CURVE;
    }

    /* Kurve (v_aussen = schnelleres/kurvenaeusseres Rad):
     *   v>0,r>0 -> Motor_TurnRight          v>0,r<0 -> Motor_TurnLeft
     *   v<0,r>0 -> Motor_TurnRightBackward  v<0,r<0 -> Motor_TurnLeftBackward */
    if (v_kmh10 > 0)
    {
        if (r_mm > 0)
        {
            Motor_TurnRight(v_aussen, v_innen);
        }
        else
        {
            Motor_TurnLeft(v_aussen, v_innen);
        }
    }
    else
    {
        if (r_mm > 0)
        {
            Motor_TurnRightBackward(v_aussen, v_innen);
        }
        else
        {
            Motor_TurnLeftBackward(v_aussen, v_innen);
        }
    }
}

/* Befehl mit Sicherheitslogik (Stopp / Hindernis / Richtungswechsel-Schutz) */
static void Befehl_Ausfuehren(int32_t v_kmh10, int32_t r_mm, int32_t d_kmh10)
{
    /* 1. Stillstand: v=0 UND d=0 -> anhalten (Lenkwert egal); reine Drehung (d!=0) laeuft weiter */
    if (v_kmh10 == 0 && d_kmh10 == 0)
    {
        Motor_Stop();
        uart_last_dir         = 0U;
        uart_direction_change = 0U;
        ausstehend_valid      = 0U;
        Timer_Stoppen();
        return;
    }

    /* 2. Hindernis aktiv -> Vorwaertsfahrt gesperrt (Rueckwaerts/Rotation erlaubt) */
    if (Vorwaerts_Gesperrt((uint8_t)(v_kmh10 > 0)))
    {
        Motor_Stop();
        return;
    }

    /* 3. Aktuelle Fahrtrichtung bestimmen
     *    0=neutral, 1=vorwaerts, 2=rueckwaerts, 3=rot.rechts, 4=rot.links */
    uint8_t curr_dir = 0U;
    if (v_kmh10 > 0)
    {
        curr_dir = 1U;             /* vorwaerts  */
    }
    else if (v_kmh10 < 0)
    {
        curr_dir = 2U;             /* rueckwaerts */
    }
    else if (d_kmh10 > 0)
    {
        curr_dir = 3U;             /* Drehung rechts (v==0 durch else if) */
    }
    else if (d_kmh10 < 0)
    {
        curr_dir = 4U;             /* Drehung links  */
    }

    /* 4. Richtungswechsel erkannt -> Motor stoppen, Stopp-Phase starten,
     *    Befehl fuer danach puffern */
    if (curr_dir != 0U && uart_last_dir != 0U && curr_dir != uart_last_dir)
    {
        Motor_Stop();
        uart_last_dir         = 0U;
        uart_direction_change = 1U;
        Timer_Stoppen();
        uart_timer_id = (uint32_t)SYSTIMER_CreateTimer(UART_STOP_TIME_US,
                                                        SYSTIMER_MODE_ONE_SHOT,
                                                        UART_DirectionStopDone, NULL);
        if (uart_timer_id != 0U)
        {
            SYSTIMER_StartTimer(uart_timer_id);
        }
        else
        {
            uart_direction_change = 0U;
        }
        ausstehend_v     = v_kmh10;
        ausstehend_r     = r_mm;
        ausstehend_d     = d_kmh10;
        ausstehend_valid = 1U;
        return;
    }

    /* 5. Stopp-Phase laeuft -> eingehende Befehle ignorieren */
    if (uart_direction_change == 1U)
    {
        Motor_Stop();
        return;
    }

    /* 6. Ausfuehren */
    uart_last_dir = curr_dir;
    Motor_Ausfuehren(v_kmh10, r_mm, d_kmh10);
}

/* Binaeren Fahrbefehl interpretieren und ausfuehren
 *   speed    : -128..127 -> +/- Hoechsttempo
 *   steering : 0=geradeaus, +/-1..126=Kurve (126=R_min),
 *              +127=Drehen rechts, -127=Drehen links, -128=ungenutzt */
static void Binaer_Befehl_Verarbeiten(int8_t speed, int8_t steering)
{
    /* Speed-Byte linear auf km/h x10 abbilden */
    int32_t v_kmh10 = (int32_t)speed * (int32_t)UART_MAX_KMH_10 / 127;

    int32_t r_mm  = 0;   /* Kurvenradius [mm], + rechts / - links */
    int32_t d_kmh10 = 0;   /* Rotationsgeschwindigkeit [km/h x10]       */

    if (steering == UART_STEER_ROTATE || steering == -UART_STEER_ROTATE)
    {
        /* Drehen auf der Stelle: Betrag aus Speed, Vorzeichen aus Steering;
         * danach v_kmh10 = 0 (keine Laengsfahrt) */
        if (steering > 0)
        {
            d_kmh10 =  betrag(v_kmh10);   /* +127 rechts */
        }
        else
        {
            d_kmh10 = -betrag(v_kmh10);   /* -127 links  */
        }
        v_kmh10 = 0;
    }
    else if (steering != 0 && steering >= -UART_STEER_MAX_KURVE && steering <= UART_STEER_MAX_KURVE)
    {
        /* Kurve: R = R_min * 126 / Steering ; Vorzeichen = Lenkrichtung (+ rechts, - links) */
        r_mm = (int32_t)UART_R_MIN_MM * UART_STEER_MAX_KURVE / steering;
    }
    /* steering == 0     -> geradeaus (r_mm = 0)
     * steering == -128  -> ungenutzt, wird wie geradeaus behandelt */

    Befehl_Ausfuehren(v_kmh10, r_mm, d_kmh10);
}

/* Antwort senden: 8 feste Bytes (binaer) */
static void Antwort_Senden(void)
{
    uint8_t L = (uint8_t)sensor_d0;   /* Front links  (idx 0) */
    uint8_t M = (uint8_t)sensor_d1;   /* Boden        (idx 1) */
    uint8_t R = (uint8_t)sensor_d2;   /* Front rechts (idx 2) */

    uint8_t flag;
    if (sensor_kabelbruch_aktiv == 1U)
    {
        flag = (uint8_t)'K';   /* Kabelbruch erkannt -> Vorrang */
    }
    else if (sensor_hindernis_aktiv == 1U)
    {
        flag = (uint8_t)'S';   /* Hindernis -> Stopp */
    }
    else
    {
        flag = (uint8_t)'F';   /* frei */
    }

    uint8_t cs = (uint8_t)('D' ^ L ^ M ^ R ^ 'x' ^ 'x' ^ flag);

    UART_HAL_SendByte((uint8_t)'D');   /* Startbyte      */
    UART_HAL_SendByte(L);
    UART_HAL_SendByte(M);
    UART_HAL_SendByte(R);
    UART_HAL_SendByte((uint8_t)'x');   /* reserviert     */
    UART_HAL_SendByte((uint8_t)'x');   /* reserviert     */
    UART_HAL_SendByte(flag);           /* 'F'/'S'/'K'    */
    UART_HAL_SendByte(cs);             /* XOR-Pruefsumme */
}

/* Empfang und Fahrzustand zuruecksetzen */
void MyUART_Init(void)
{
    rx_idx                = 0U;
    uart_last_dir         = 0U;
    uart_direction_change = 0U;
    ausstehend_valid      = 0U;
    cmd_aktiv             = 0U;
}

/* Startmodus-Hilfe: 1 = Byte wartet im Empfang (bleibt im FIFO), 0 = keins */
uint8_t MyUART_BytePending(void)
{
    return UART_HAL_RXAvailable();
}

/* Hauptverarbeitung: einmal pro Zyklus aus main.c aufgerufen (UART-Modus). */
void MyUART_Process(void)
{
    uint8_t byte = 0U;

    /* Befehls-Watchdog: bleibt laenger als UART_CMD_TIMEOUT_US ein gueltiger
     * Befehl aus (Verbindungsabbruch), in den sicheren Zustand (Stopp) gehen. */
    if (cmd_aktiv == 1U)
    {
        uint32_t seit_befehl = (uint32_t)SYSTIMER_GetTime() - last_cmd_us;
        if (seit_befehl > UART_CMD_TIMEOUT_US)
        {
            Motor_Stop();
            uart_last_dir         = 0U;
            uart_direction_change = 0U;
            ausstehend_valid      = 0U;
            cmd_aktiv             = 0U;
        }
    }

    /* Hindernis-Sofortreaktion: laufende Vorwaertsfahrt stoppen */
    if (Vorwaerts_Gesperrt((uint8_t)(uart_last_dir == 1U)))
    {
        Motor_Stop();
    }

    /* Gepufferten Befehl nach Ablauf der Stopp-Phase automatisch ausfuehren */
    if (uart_direction_change == 0U && ausstehend_valid == 1U)
    {
        ausstehend_valid = 0U;
        Befehl_Ausfuehren(ausstehend_v, ausstehend_r, ausstehend_d);
    }

    /* Frame-Timeout: nur den haengenden (unvollstaendigen) Frame verwerfen,
     * damit der Parser beim naechsten 'B' wieder aufsetzt. */
    if (rx_idx > 0U)
    {
        uint32_t jetzt = (uint32_t)SYSTIMER_GetTime();
        if ((jetzt - rx_start_us) > UART_TIMEOUT_US)
        {
            rx_idx = 0U;
        }
    }

    /* Empfangene Bytes verarbeiten */
    while (UART_HAL_RXByte(&byte))
    {
        if (rx_idx == 0U)
        {
            /* Auf Startbyte 'B' synchronisieren, alles davor verwerfen */
            if (byte == UART_RX_STARTBYTE)
            {
                rx_frame[0] = byte;
                rx_idx      = 1U;
                rx_start_us = (uint32_t)SYSTIMER_GetTime();
            }
        }
        else
        {
            rx_frame[rx_idx] = byte;
            rx_idx++;

            if (rx_idx >= UART_RX_FRAME_LEN)
            {
                /* Checksum = XOR(Startbyte, Speed, Steering) */
                uint8_t cs = (uint8_t)(rx_frame[0] ^ rx_frame[1] ^ rx_frame[2]);
                if (cs == rx_frame[3])
                {
                    /* gueltiger Befehl -> Watchdog-Uhr neu stellen */
                    last_cmd_us = (uint32_t)SYSTIMER_GetTime();
                    cmd_aktiv   = 1U;
                    Binaer_Befehl_Verarbeiten((int8_t)rx_frame[1], (int8_t)rx_frame[2]);
                    Antwort_Senden();   /* genau eine Antwort pro gueltigem Befehl */
                }
                /* ungueltige Checksum -> Frame verwerfen, naechstes 'B' resynchronisiert */
                rx_idx = 0U;
            }
        }
    }
}
