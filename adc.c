/*
 * adc.c
 *
 *  Created on: 2026 May 27 13:10:16
 *      Author: oskib
 *
 * Einzige Datei mit direktem Zugriff auf DAVE-ADC-Funktionen.
 *
 * ADC_MEASUREMENT_0  ->  alle Kanaele
 *   Channel_A  ->  Joystick Laengsachse (fwd)
 *   Channel_B  ->  Joystick Lenkachse   (turn)
 *   Channel_C  ->  Sensor 0  Front links   ("Distance L")
 *   Channel_D  ->  Sensor 1  Boden         ("Distance M", diagonal)
 *   Channel_E  ->  Sensor 2  Front rechts  ("Distance R")
 */
#include "adc.h"

/* Liest beide Joystick-Achsen (fwd, turn) als Rohwerte 0..4095 */
ADC_JoystickValues_t ADC_ReadJoystick(void)
{
    ADC_JoystickValues_t values;
    ADC_MEASUREMENT_StartConversion(&ADC_MEASUREMENT_0);
    values.fwd  = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_Channel_A);
    values.turn = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_Channel_B);
    return values;
}

void ADC_StartKonversion(void)
{
    ADC_MEASUREMENT_StartConversion(&ADC_MEASUREMENT_0);
}

/* Liefert den Rohwert eines Abstandssensors (0 = Front links, 1 = Boden, 2 = Front rechts) */
uint32_t ADC_GetKanal(uint8_t idx)
{
    uint32_t ergebnis = 0U;

    switch (idx)
    {
        case 0U:
            ergebnis = (uint32_t)ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_Channel_C);
            break;
        case 1U:
            ergebnis = (uint32_t)ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_Channel_D);
            break;
        case 2U:
            ergebnis = (uint32_t)ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_Channel_E);
            break;
        default:
            ergebnis = 0U;
            break;
    }

    return ergebnis;
}
