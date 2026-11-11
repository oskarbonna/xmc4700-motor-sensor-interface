/*
 * main.c
 *
 *  Created on: 2026 Apr 20 15:40:19
 *      Author: oskib
 *
 * Integriertes Gesamtsystem: Joystick + Motor + UART + IR-Abstandssensoren.
 * Der Betriebsmodus wird beim Start einmalig gewaehlt (Joystick oder UART) und
 * in uart_aktiv verriegelt. Danach laeuft die Hauptschleife mit Sensorabfrage,
 * Zustandspruefung und der aktiven Steuerung.
 *
 * DAVE ADC_MEASUREMENT_0:
 *   Joystick   Channel_A = fwd, Channel_B = turn
 *   IR-Sensor  Channel_C = Front links, Channel_D = Boden, Channel_E = Front rechts
 */
#include "DAVE.h"
#include "adc.h"
#include "joystick.h"
#include "uart2.h"
#include "ir_sensor.h"
#include "betriebszustandsautomat.h"
#include "systemsteuerung.h"

/* Betriebsmodus einmalig bestimmen: blockiert bis Joystick oder UART reagiert.
 * Rueckgabe: 1 = UART-Modus, 0 = Joystick-Modus (UART wird zuerst geprueft). */
static uint8_t Startmodus_Bestimmen(void)
{
    for (;;)
    {
        /* Byte nur pruefen, nicht lesen -> bleibt im FIFO, damit MyUART_Process
         * den ersten Frame komplett (mit Startbyte 'B') bekommt */
        if (MyUART_BytePending())
        {
            return 1U;
        }

        if (Joystick_StartAusgelenkt())
        {
            return 0U;
        }
    }
}

/* Sensoren auslesen und daraus das modusunabhaengige Hindernis-Flag ableiten */
static void Sensorwerte_Aktualisieren(void)
{
    IR_Sensor_Process();

    sensor_d0    = IR_Sensor_GetDistanz(0U);
    sensor_d1    = IR_Sensor_GetDistanz(1U);
    sensor_d2    = IR_Sensor_GetDistanz(2U);
    uint8_t sensor_stopp = IR_Sensor_IstStopp();

    if (sensor_stopp == 1U)
    {
        sensor_hindernis_aktiv = 1U;
    }
    else
    {
        sensor_hindernis_aktiv = 0U;
    }
}

/**
 * @brief main() - Application entry point
 *
 * <b>Details of function</b><br>
 * This routine is the application entry point. It is invoked by the device startup code. It is responsible for
 * invoking the APP initialization dispatcher routine - DAVE_Init() and hosting the place-holder for user application
 * code.
 */
int main(void)
{
    DAVE_STATUS_t status;

    status = DAVE_Init();           /* Initialization of DAVE APPs  */

    if (status != DAVE_STATUS_SUCCESS)
    {
        /* Placeholder for error handler code. The while loop below can be replaced with an user error handler. */
        XMC_DEBUG("DAVE APPs initialization failed\n");

        while (1U)
        {

        }
    }

    /* Alle Module in einen sicheren Ausgangszustand bringen */
    Fahrzeug_Stop();
    IR_Sensor_Init();
    MyUART_Init();
    Automat_Init();

    /* Betriebsmodus einmalig festlegen (danach kein Umschalten) */
    uart_aktiv = Startmodus_Bestimmen();

    while (1U)
    {
        Sensorwerte_Aktualisieren();

        Automat_Update();

        /* Kabelbruch-Flag fuer die UART-Rueckmeldung bereitstellen */
        if (Automat_IstFehler() == 1U)
        {
            sensor_kabelbruch_aktiv = 1U;
        }
        else
        {
            sensor_kabelbruch_aktiv = 0U;
        }

        if (uart_aktiv == 1U)
        {
            /* UART antwortet auch im Fehlerfall; bei Kabelbruch bleiben die
             * Motoren danach zwangsweise gestoppt */
            MyUART_Process();
            if (sensor_kabelbruch_aktiv == 1U)
            {
                Fahrzeug_Stop();
            }
        }
        else
        {
            /* Joystick-Modus: bei Kabelbruch stoppen, sonst Joystick steuern */
            if (sensor_kabelbruch_aktiv == 1U)
            {
                Fahrzeug_Stop();
            }
            else
            {
                Joystick_Process();
            }
        }
    }
}
