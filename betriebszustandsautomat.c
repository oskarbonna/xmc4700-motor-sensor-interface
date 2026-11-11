/*
 * betriebszustandsautomat.c
 *
 *  Created on: 2026 Jun 10 14:06:02
 *      Author: oskib
 *
 * Betriebszustandsautomat der Applikationsschicht mit zwei Zustaenden
 * (AUTOMAT_BEREIT / AUTOMAT_FEHLER). Prueft zyklisch die Joystick-
 * Fehlerbedingung und zeigt den Zustand ueber die Status-LED. Im Fehlerzustand
 * haelt main.c das Fahrzeug an; faellt die Ursache weg, geht der Automat
 * selbsttaetig auf BEREIT zurueck.
 */
#include "betriebszustandsautomat.h"
#include "joystick.h"
#include "ir_sensor.h"
#include "systemsteuerung.h"

static Betriebszustand_t automat_zustand = AUTOMAT_BEREIT;

/* Setzt den Automaten in den Ausgangszustand BEREIT und zeigt ihn an */
void Automat_Init(void)
{
    automat_zustand = AUTOMAT_BEREIT;
    Diagnose_SetStatus(automat_zustand);
}

/* Zyklisch aufrufen: prueft die Joystick-Fehlerbedingung, aktualisiert Zustand
 * und LED nur bei Aenderung */
void Automat_Update(void)
{
    Betriebszustand_t neuer_zustand;

    /* IR-Kabelbruch deaktiviert - nur der Joystick loest FEHLER aus */
    if (Joystick_IstAusserBereich() == 1U /* || (IR_Sensor_IstKabelbruch() == 1U) */)
    {
        neuer_zustand = AUTOMAT_FEHLER;
    }
    else
    {
        neuer_zustand = AUTOMAT_BEREIT;
    }

    if (neuer_zustand != automat_zustand)
    {
        automat_zustand = neuer_zustand;
        Diagnose_SetStatus(automat_zustand);
    }
}

/* 1, wenn der Automat im Fehlerzustand ist, sonst 0 */
uint8_t Automat_IstFehler(void)
{
    if (automat_zustand == AUTOMAT_FEHLER)
    {
        return 1U;
    }
    return 0U;
}
