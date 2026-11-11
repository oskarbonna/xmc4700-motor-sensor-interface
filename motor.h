/*
 * motor.h
 *
 *  Created on: 2026 Apr 20 16:04:18
 *      Author: oskib
 *
 * Oeffentliche Schnittstelle des Motor-Moduls (HAL).
 * Kapselt die vier Antriebsmotoren ueber acht PWM-Kanaele und stellt
 * fertige Fahrmanoever bereit. Die PWM-Kanalzuordnung ist in motor.c
 * dokumentiert (siehe Kapitel 5.3).
 *
 * Die Geschwindigkeitswerte sind PWM-Tastgrade. Sie werden in der HAL auf
 * den zulaessigen Hoechstwert begrenzt (Funktion duty_begrenzen):
 *   speed_outer  Tastgrad fuer die kurvenaeussere Seite
 *   speed_inner  Tastgrad fuer die kurveninnere Seite
 *   speed        Tastgrad fuer beide Seiten (Drehung auf der Stelle)
 */
#ifndef MOTOR_H_
#define MOTOR_H_

#include <DAVE.h>

/* Kurvenfahrt vorwaerts */
void Motor_TurnLeft(uint32_t speed_outer, uint32_t speed_inner);
void Motor_TurnRight(uint32_t speed_outer, uint32_t speed_inner);

/* Kurvenfahrt rueckwaerts */
void Motor_TurnLeftBackward(uint32_t speed_outer, uint32_t speed_inner);
void Motor_TurnRightBackward(uint32_t speed_outer, uint32_t speed_inner);

/* Drehung auf der Stelle (beide Seiten gegenlaeufig) */
void Motor_RotateRight(uint32_t speed);
void Motor_RotateLeft(uint32_t speed);

/* Alle Motoren anhalten */
void Motor_Stop(void);

#endif /* MOTOR_H_ */
