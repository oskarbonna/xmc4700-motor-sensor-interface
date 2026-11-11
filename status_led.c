/*
 * status_led.c
 *
 *  Created on: 2026 Jun 10 16:48:12
 *      Author: oskib
 *
 * Hardware Abstraction Layer fuer die Statusanzeige-LED.
 * Kein anderes Modul greift direkt auf die DIGITAL_IO-APP "LED1" zu.
 */
#include "status_led.h"

/* Schaltet die Status-LED ein (Ausgang High). */
void StatusLED_On(void)
{
    DIGITAL_IO_SetOutputHigh(&LED1);
}

/* Schaltet die Status-LED aus (Ausgang Low). */
void StatusLED_Off(void)
{
    DIGITAL_IO_SetOutputLow(&LED1);
}
