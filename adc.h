/*
 * adc.h
 *
 *  Created on: 2026 May 27 13:09:41
 *      Author: oskib
 */
#ifndef ADC_H_
#define ADC_H_

#include <DAVE.h>

typedef struct
{
    uint32_t fwd;    /* Laengsrichtung (0-4095) */
    uint32_t turn;   /* Lenkrichtung   (0-4095) */
} ADC_JoystickValues_t;

ADC_JoystickValues_t ADC_ReadJoystick(void);   /* liest beide Joystick-Achsen (fwd, turn) */

void     ADC_StartKonversion(void);   /* startet eine Wandlung aller Kanaele */
uint32_t ADC_GetKanal(uint8_t idx);   /* idx 0/1/2 = Front links/Boden/Front rechts, Rueckgabe 0..4095 */

#endif /* ADC_H_ */
