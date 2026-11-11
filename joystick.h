/*
 * joystick.h
 *
 *  Created on: 2026 Apr 20 15:42:04
 *      Author: oskib
 */
#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include <DAVE.h>
#include "motor.h"
#include "platform.h"

/* Joystick-Kalibrierung (Hardware-abhaengig, per Messung ermittelt) */
#define JOYSTICK_CENTER_FWD             1847U   /* ADC-Ruhewert Laengsachse */
#define JOYSTICK_CENTER_TURN            1974U   /* ADC-Ruhewert Querachse */
#define JOYSTICK_DEADZONE               200U    /* Totzone um den Ruhewert */
/* Max. Auslenkung je Achse = Abstand vom Ruhewert bis zur Plausibilitaets-
 * bzw. Kabelbruchgrenze (gueltiger Rohwertbereich 150..3700, siehe THRESHOLD_LOW/HIGH). */
#define JOYSTICK_MAX_DEV_FWD            1697U   /* = 1847 - 150  */
#define JOYSTICK_MAX_DEV_BWD            1853U   /* = 3700 - 1847 */
#define JOYSTICK_MAX_DEV_LEFT           1824U   /* = 1974 - 150  */
#define JOYSTICK_MAX_DEV_RIGHT          1726U   /* = 3700 - 1974 */
#define JOYSTICK_STOP_TIME_US           PLATFORM_RICHTUNGSWECHSEL_STOP_TIME_US  /* Sperrzeit bei Richtungswechsel */
#define JOYSTICK_ROTATE_FWD_TOLERANCE   1200  /* darunter -> Drehung auf der Stelle statt Kurve */
#define JOYSTICK_THRESHOLD_LOW          150U    /* darunter -> Kabelbruch (weit unter Normalgrenze 580, wegen Rohwertschwankung) */
#define JOYSTICK_THRESHOLD_HIGH         3700U   /* darueber -> Kabelbruch (weit ueber Normalgrenze 3480, wegen Rohwertschwankung) */

/* Fahrzeugparameter – aus platform.h */
#define JOYSTICK_MAX_PWM                PLATFORM_MAX_PWM
#define JOYSTICK_SPURBREITE_HALB        PLATFORM_SPURBREITE_HALB
#define JOYSTICK_R_MIN_MM               PLATFORM_R_MIN_MM

void Joystick_Process(void);
void Direction_StopDone(void* args);
uint8_t Joystick_IstAusserBereich(void);
uint8_t Joystick_StartAusgelenkt(void);

#endif /* JOYSTICK_H_ */
