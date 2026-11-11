/*
 * systemsteuerung.c
 *
 *  Created on: 2026 Jun 10 11:23:31
 *      Author: oskib
 *
 * Treiberschicht-Modul mit zwei uebergreifenden Funktionen (Not-Stopp und
 * Status-LED), die nicht an eine einzelne Hardwarekomponente gebunden sind.
 * So muss die Applikationsschicht keine HAL-Module direkt aufrufen.
 */
#include "systemsteuerung.h"
#include "motor.h"
#include "status_led.h"

/* Not-Stopp: schaltet alle Motoren ab */
void Fahrzeug_Stop(void)
{
    Motor_Stop();
}

/* Zeigt den Betriebszustand ueber die Status-LED: FEHLER = an, sonst aus */
void Diagnose_SetStatus(Betriebszustand_t zustand)
{
    if (zustand == AUTOMAT_FEHLER)
    {
        StatusLED_On();
    }
    else
    {
        StatusLED_Off();
    }
}
