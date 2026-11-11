/*
 * joystick.c
 *
 *  Created on: 2026 Apr 20 15:42:19
 *      Author: oskib
 *
 * Ablauf von Joystick_Process():
 *   Einlesen -> Plausibilitaet -> Richtungswechsel-Sperre -> Flags/Geschwindigkeit
 *   -> Hindernisschutz -> Richtung klassifizieren -> Richtungswechsel-Schutz
 *   -> Drehung auf der Stelle ODER Differentialantrieb -> Motoren ansteuern.
 * Die Hilfsfunktionen unten stehen in genau dieser Reihenfolge.
 */
#include "joystick.h"
#include "adc.h"
#include "uart2.h"

static volatile uint8_t direction_change = 0U;
static uint32_t         timer_id         = 0U;
static uint8_t          last_dir         = 0U;

/* Timer-Callback: gibt die Richtungswechsel-Sperre wieder frei. */
void Direction_StopDone(void* args)
{
    (void)args;
    direction_change = 0U;
}

/* Betrag der Differenz |a - b| (vorzeichenlos) */
static uint32_t betrag_diff(uint32_t a, uint32_t b)
{
    if (a >= b)
    {
        return a - b;
    }
    return b - a;
}

/* Laengsgeschwindigkeit als Betrag (Richtung steckt in fwd/bwd), sonst 0. */
static uint32_t laengs_geschwindigkeit(uint8_t fwd, uint8_t bwd, uint32_t dev_laengs)
{
    if (fwd)
    {
        return (dev_laengs * JOYSTICK_MAX_PWM) / JOYSTICK_MAX_DEV_FWD;
    }
    if (bwd)
    {
        return (dev_laengs * JOYSTICK_MAX_PWM) / JOYSTICK_MAX_DEV_BWD;
    }
    return 0U;
}

/* Quergeschwindigkeit (Lenkbetrag), sonst 0. */
static uint32_t quer_geschwindigkeit(uint8_t rght, uint8_t lft, uint32_t dev_turn)
{
    if (rght)
    {
        return (dev_turn * JOYSTICK_MAX_PWM) / JOYSTICK_MAX_DEV_RIGHT;
    }
    if (lft)
    {
        return (dev_turn * JOYSTICK_MAX_PWM) / JOYSTICK_MAX_DEV_LEFT;
    }
    return 0U;
}

/* Fahrtrichtung klassifizieren: 0=Stillstand, 1=vorwaerts, 2=rueckwaerts,
 * 3=Drehung rechts, 4=Drehung links. */
static uint8_t richtung_bestimmen(uint8_t fwd, uint8_t bwd, uint8_t rght, uint8_t lft,
                                  uint32_t v_abs_laengs)
{
    if ((rght || lft) && v_abs_laengs <= JOYSTICK_ROTATE_FWD_TOLERANCE)
    {
        if (rght)
        {
            return 3U;
        }
        return 4U;
    }

    if (fwd)
    {
        return 1U;
    }

    if (bwd)
    {
        return 2U;
    }

    return 0U;
}

/* Richtungswechsel-Schutz: bei Wechsel erst stoppen und JOYSTICK_STOP_TIME_US sperren.
 *   1 = Wechsel erkannt (Fahrzeug bleibt stehen), 0 = kein Wechsel. */
static uint8_t richtungswechsel_behandeln(uint8_t curr_dir)
{
    if (curr_dir != 0U && last_dir != 0U && curr_dir != last_dir)
    {
        Motor_Stop();
        last_dir = 0U;
        direction_change = 1U;

        if (timer_id != 0U)
        {
            SYSTIMER_DeleteTimer(timer_id);
            timer_id = 0U;
        }

        timer_id = (uint32_t)SYSTIMER_CreateTimer(JOYSTICK_STOP_TIME_US,
                                                   SYSTIMER_MODE_ONE_SHOT,
                                                   Direction_StopDone, NULL);
        if (timer_id != 0U)
        {
            SYSTIMER_StartTimer(timer_id);
        }
        else
        {
            direction_change = 0U;
        }
        return 1U;
    }

    last_dir = curr_dir;
    return 0U;
}

/* Drehung auf der Stelle je nach curr_dir. 1 = ausgefuehrt, 0 = keine Drehung. */
static uint8_t drehung_ausfuehren(uint8_t curr_dir, uint32_t speed_turn)
{
    if (curr_dir == 3U)
    {
        Motor_RotateRight(speed_turn);
        return 1U;
    }
    if (curr_dir == 4U)
    {
        Motor_RotateLeft(speed_turn);
        return 1U;
    }
    return 0U;
}

/* Differentialantrieb: aus Laengsbetrag + Lenkbetrag die Sollwerte je Seite.
 * rght -> rechte Seite langsamer (innen), linke schneller (aussen) -> Rechtskurve. */
static void differential_berechnen(uint8_t rght, uint8_t lft, uint32_t v_abs_laengs,
                                   uint32_t speed_turn,
                                   uint32_t* v_rechts, uint32_t* v_links)
{
    *v_rechts = v_abs_laengs;
    *v_links  = v_abs_laengs;

    if ((rght || lft) && v_abs_laengs != 0U)
    {
        uint32_t d_R_mag = (uint32_t)((uint64_t)v_abs_laengs * speed_turn * JOYSTICK_SPURBREITE_HALB
                                      / ((uint64_t)JOYSTICK_MAX_PWM * JOYSTICK_R_MIN_MM));
        if (rght)
        {
            *v_rechts = v_abs_laengs - d_R_mag;
            *v_links  = v_abs_laengs + d_R_mag;
        }
        else
        {
            *v_rechts = v_abs_laengs + d_R_mag;
            *v_links  = v_abs_laengs - d_R_mag;
        }
    }
}

/* Sollwerte an die Motor-Funktion verteilen. */
static void motoren_ansteuern(uint32_t r, uint32_t l, uint8_t fwd)
{
    if (fwd == 1U)
    {
        if (l > r)
        {
            Motor_TurnRight(l, r);
        }
        else
        {
            Motor_TurnLeft(r, l);
        }
    }
    else
    {
        if (l > r)
        {
            Motor_TurnRightBackward(l, r);
        }
        else
        {
            Motor_TurnLeftBackward(r, l);
        }
    }
}

/* Joystick ausserhalb des gueltigen ADC-Bereichs? Nur bei uart_aktiv == 0. */
uint8_t Joystick_IstAusserBereich(void)
{
    ADC_JoystickValues_t werte;

    if (uart_aktiv == 1U)
    {
        return 0U;
    }

    werte = ADC_ReadJoystick();

    if ((werte.fwd  < JOYSTICK_THRESHOLD_LOW) || (werte.fwd  > JOYSTICK_THRESHOLD_HIGH) ||
        (werte.turn < JOYSTICK_THRESHOLD_LOW) || (werte.turn > JOYSTICK_THRESHOLD_HIGH))
    {
        return 1U;
    }

    return 0U;
}

/* Startmodus-Hilfe: Joystick aktuell ausserhalb der Totzone? (fuer main.c-Moduswahl) */
uint8_t Joystick_StartAusgelenkt(void)
{
    ADC_JoystickValues_t j = ADC_ReadJoystick();

    uint32_t d_fwd  = betrag_diff(j.fwd,  JOYSTICK_CENTER_FWD);
    uint32_t d_turn = betrag_diff(j.turn, JOYSTICK_CENTER_TURN);

    if ((d_fwd > JOYSTICK_DEADZONE) || (d_turn > JOYSTICK_DEADZONE))
    {
        return 1U;
    }
    return 0U;
}

/* Hauptverarbeitung: einmal pro Zyklus aus main.c aufgerufen (Joystick-Modus). */
void Joystick_Process(void)
{
    ADC_JoystickValues_t joystick = ADC_ReadJoystick();
    uint32_t raw_fwd  = joystick.fwd;
    uint32_t raw_turn = joystick.turn;

    /* Plausibilitaet / Kabelbruch -> anhalten */
    if (raw_fwd  < JOYSTICK_THRESHOLD_LOW  || raw_fwd  > JOYSTICK_THRESHOLD_HIGH ||
        raw_turn < JOYSTICK_THRESHOLD_LOW  || raw_turn > JOYSTICK_THRESHOLD_HIGH)
    {
        Motor_Stop();
        return;
    }

    /* Waehrend einer laufenden Richtungswechsel-Sperre bleibt das Fahrzeug stehen */
    if (direction_change == 1U)
    {
        Motor_Stop();
        return;
    }

    /* Auslenkungs-Flags (Joystick invertiert): fwd = Rohwert unter der Mitte,
     * bwd = darueber, rght = ueber der Mitte, lft = darunter. */
    uint8_t fwd  = (raw_fwd  < JOYSTICK_CENTER_FWD  - JOYSTICK_DEADZONE);
    uint8_t bwd  = (raw_fwd  > JOYSTICK_CENTER_FWD  + JOYSTICK_DEADZONE);
    uint8_t rght = (raw_turn > JOYSTICK_CENTER_TURN + JOYSTICK_DEADZONE);
    uint8_t lft  = (raw_turn < JOYSTICK_CENTER_TURN - JOYSTICK_DEADZONE);

    uint32_t dev_laengs = betrag_diff(raw_fwd,  JOYSTICK_CENTER_FWD);
    uint32_t dev_turn   = betrag_diff(raw_turn, JOYSTICK_CENTER_TURN);

    uint32_t v_abs_laengs = laengs_geschwindigkeit(fwd, bwd, dev_laengs);
    uint32_t speed_turn   = quer_geschwindigkeit(rght, lft, dev_turn);

    /* Hindernisschutz: Vorwaertsfahrt sperren wenn Hindernis aktiv. */
    if (sensor_hindernis_aktiv == 1U && fwd)
    {
        Motor_Stop();
        return;
    }

    /* Richtung klassifizieren und Richtungswechsel-Schutz */
    uint8_t curr_dir = richtung_bestimmen(fwd, bwd, rght, lft, v_abs_laengs);
    if (richtungswechsel_behandeln(curr_dir))
    {
        return;
    }

    /* Drehung auf der Stelle */
    if (drehung_ausfuehren(curr_dir, speed_turn))
    {
        return;
    }

    /* Geradeaus- / Kurvenfahrt ueber Differentialantrieb */
    uint32_t v_rechts;
    uint32_t v_links;
    differential_berechnen(rght, lft, v_abs_laengs, speed_turn, &v_rechts, &v_links);
    motoren_ansteuern(v_rechts, v_links, fwd);
}
