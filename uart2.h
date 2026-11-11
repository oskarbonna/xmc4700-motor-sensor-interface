/*
 * uart2.h
 *
 *  Created on: 2026 Jun 05 14:03:29
 *      Author: oskib
 */

#ifndef UART2_H
#define UART2_H

#include "uart_hal.h"
#include "motor.h"
#include "platform.h"

/* Geschwindigkeitslimit – aus platform.h (UART_MAX_KMH_10 abgeleitet) */
#define UART_MAX_KMH                PLATFORM_MAX_KMH
#define UART_MAX_KMH_10             (UART_MAX_KMH * 10U)  /* km/h x10 */

/* Fahrzeugparameter – aus platform.h */
#define UART_MAX_PWM                PLATFORM_MAX_PWM
#define UART_SPURBREITE_HALB_MM     PLATFORM_SPURBREITE_HALB
#define UART_MAX_PWM_CURVE          PLATFORM_MAX_PWM_CURVE
#define UART_R_MIN_MM               PLATFORM_R_MIN_MM

/* Binaeres Protokoll – Frame-Konstanten */
/* Empfangsframe (RX-Parser) */
#define UART_RX_STARTBYTE           'B'       /* 0x42  Empfangsframe-Start  */
#define UART_RX_FRAME_LEN           4U        /* B + Speed + Steering + CS  */

/* Lenk-Schwellen (Binaer_Befehl_Verarbeiten) */
#define UART_STEER_MAX_KURVE        126       /* |Steering| fuer engste Kurve   */
#define UART_STEER_ROTATE           127       /* +/-127 = Drehen auf der Stelle */

/* Timeout fuer unvollstaendigen Empfangsframe */
#define UART_TIMEOUT_US             500000U   /* 500 ms */

/* Befehls-Watchdog: kommt laenger kein gueltiger Befehl -> sicherer Stopp */
#define UART_CMD_TIMEOUT_US         500000U   /* 500 ms (bei 10 ms Senden: ~50 Frames Toleranz) */

/* Betriebsmodus: 1 = UART steuert, 0 = Joystick steuert (in main.c gesetzt) */
extern uint8_t uart_aktiv;

/* Sensorwerte (von main.c gesetzt) */
extern uint32_t sensor_d0;    /* Frontsensor links  [cm] */
extern uint32_t sensor_d1;    /* Bodensensor        [cm] */
extern uint32_t sensor_d2;    /* Frontsensor rechts [cm] */

/* 1 = Hindernis aktiv -> nur Rueckwaerts + Rotation erlaubt */
extern uint8_t sensor_hindernis_aktiv;

/* 1 = Kabelbruch -> UART-Flag 'K' (Vorrang vor Hindernis) */
extern uint8_t sensor_kabelbruch_aktiv;

/* Richtungswechsel-Schutz: Wartezeit nach Richtungswechsel */
#define UART_STOP_TIME_US               PLATFORM_RICHTUNGSWECHSEL_STOP_TIME_US

/* Oeffentliche Schnittstelle */
void MyUART_Init(void);
void MyUART_Process(void);
uint8_t MyUART_BytePending(void);          /* Startmodus: Byte im Empfang? */
void UART_DirectionStopDone(void* args);   /* SYSTIMER-Callback */

#endif /* UART2_H */
