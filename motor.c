/*
 * motor.c
 *
 *  Created on: 2026 Apr 20 16:04:32
 *      Author: oskib
 *
 * Konvention: speed_outer = kurvenaeussere (schnellere) Seite,
 *             speed_inner = kurveninnere (langsamere) Seite.
 * MOTOR_VORWAERTS = 1, MOTOR_RUECKWAERTS = 0.
 */
#include "motor.h"

#define MOTOR_DUTY_MAX  10000U

/* Fahrtrichtung fuer Motor_SetRight / Motor_SetLeft */
#define MOTOR_RUECKWAERTS  0U
#define MOTOR_VORWAERTS    1U

/* Begrenzt den Tastgrad auf hoechstens MOTOR_DUTY_MAX (100 %). */
static uint32_t duty_begrenzen(uint32_t s)
{
    if (s > MOTOR_DUTY_MAX)
    {
        s = MOTOR_DUTY_MAX;
    }
    return s;
}

/* Rechte Motorseite (PWM_0/1 = vorne, PWM_2/3 = hinten) */
static void Motor_SetRight(uint32_t speed, uint8_t vorwaerts)
{
    uint32_t duty = duty_begrenzen(speed);
    if (vorwaerts)
    {
        PWM_SetDutyCycle(&PWM_0, duty);
        PWM_Start(&PWM_0);
        PWM_Stop(&PWM_1);
        PWM_SetDutyCycle(&PWM_3, duty);
        PWM_Start(&PWM_3);
        PWM_Stop(&PWM_2);
    }
    else
    {
        PWM_SetDutyCycle(&PWM_1, duty);
        PWM_Start(&PWM_1);
        PWM_Stop(&PWM_0);
        PWM_SetDutyCycle(&PWM_2, duty);
        PWM_Start(&PWM_2);
        PWM_Stop(&PWM_3);
    }
}

/* Linke Motorseite (PWM_4/5 = hinten, PWM_6/7 = vorne) */
static void Motor_SetLeft(uint32_t speed, uint8_t vorwaerts)
{
    uint32_t duty = duty_begrenzen(speed);
    if (vorwaerts)
    {
        PWM_SetDutyCycle(&PWM_4, duty);
        PWM_Start(&PWM_4);
        PWM_Stop(&PWM_5);
        PWM_SetDutyCycle(&PWM_7, duty);
        PWM_Start(&PWM_7);
        PWM_Stop(&PWM_6);
    }
    else
    {
        PWM_SetDutyCycle(&PWM_5, duty);
        PWM_Start(&PWM_5);
        PWM_Stop(&PWM_4);
        PWM_SetDutyCycle(&PWM_6, duty);
        PWM_Start(&PWM_6);
        PWM_Stop(&PWM_7);
    }
}

/* Linkskurve vorwaerts: rechts = aussen (schnell), links = innen (langsam) */
void Motor_TurnLeft(uint32_t speed_outer, uint32_t speed_inner)
{
    Motor_SetRight(speed_outer, MOTOR_VORWAERTS);
    Motor_SetLeft(speed_inner, MOTOR_VORWAERTS);
}

/* Rechtskurve vorwaerts: links = aussen (schnell), rechts = innen (langsam) */
void Motor_TurnRight(uint32_t speed_outer, uint32_t speed_inner)
{
    Motor_SetRight(speed_inner, MOTOR_VORWAERTS);
    Motor_SetLeft(speed_outer, MOTOR_VORWAERTS);
}

/* Linkskurve rueckwaerts */
void Motor_TurnLeftBackward(uint32_t speed_outer, uint32_t speed_inner)
{
    Motor_SetRight(speed_outer, MOTOR_RUECKWAERTS);
    Motor_SetLeft(speed_inner, MOTOR_RUECKWAERTS);
}

/* Rechtskurve rueckwaerts */
void Motor_TurnRightBackward(uint32_t speed_outer, uint32_t speed_inner)
{
    Motor_SetRight(speed_inner, MOTOR_RUECKWAERTS);
    Motor_SetLeft(speed_outer, MOTOR_RUECKWAERTS);
}

/* Drehung auf der Stelle nach rechts: rechte Seite rueckwaerts, linke Seite vorwaerts */
void Motor_RotateRight(uint32_t speed)
{
    Motor_SetRight(speed, MOTOR_RUECKWAERTS);
    Motor_SetLeft(speed, MOTOR_VORWAERTS);
}

/* Drehung auf der Stelle nach links: rechte Seite vorwaerts, linke Seite rueckwaerts */
void Motor_RotateLeft(uint32_t speed)
{
    Motor_SetRight(speed, MOTOR_VORWAERTS);
    Motor_SetLeft(speed, MOTOR_RUECKWAERTS);
}

/* Haelt das Fahrzeug an: schaltet alle acht PWM-Kanaele ab */
void Motor_Stop(void)
{
    PWM_Stop(&PWM_0);
    PWM_Stop(&PWM_1);
    PWM_Stop(&PWM_2);
    PWM_Stop(&PWM_3);
    PWM_Stop(&PWM_4);
    PWM_Stop(&PWM_5);
    PWM_Stop(&PWM_6);
    PWM_Stop(&PWM_7);
}
