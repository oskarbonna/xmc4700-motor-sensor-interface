/*
 * status_led.h
 *
 *  Created on: 2026 Jun 10 16:47:55
 *      Author: oskib
 */
#ifndef STATUS_LED_H_
#define STATUS_LED_H_

#include <DAVE.h>

void StatusLED_On(void);    /* Status-LED einschalten (P5.9 High) */
void StatusLED_Off(void);   /* Status-LED ausschalten (P5.9 Low) */

#endif /* STATUS_LED_H_ */
