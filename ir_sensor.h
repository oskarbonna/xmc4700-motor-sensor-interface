/*
 * ir_sensor.h
 *
 *  Created on: 2026 May 19 10:31:52
 *      Author: oskib
 */
#ifndef IR_SENSOR_H_
#define IR_SENSOR_H_

#include <DAVE.h>
#include "platform.h"

#define IR_SENSOR_ANZAHL            3U      /* Anzahl Sensoren */

#define IR_DISTANZ_MIN_CM           20U     /* Messbereich GP2Y0A02YK unten [cm] */
#define IR_DISTANZ_MAX_CM          150U     /* Messbereich oben [cm] */

#define IR_DISTANZ_SCHWELLE_CM      PLATFORM_HINDERNIS_CM   /* Front-Stopp wenn naeher [cm] */

/* Bodensensor: Einbau und Ausloesehoehen */
#define IR_BODEN_H_CM                25U    /* Montagehoehe [cm] */
#define IR_BODEN_ALPHA_SIN_10000   4226U    /* sin(25 Grad) x 10000 */
#define IR_BODEN_STOPP_ABWAERTS_CM   PLATFORM_STUFE_ABWAERTS_CM   /* min. Abwaertsstufe [cm] */
#define IR_BODEN_STOPP_AUFWAERTS_CM  PLATFORM_STUFE_AUFWAERTS_CM  /* min. Aufwaertshindernis [cm] */

/* Boden frei nur zwischen AUFWAERTS und ABWAERTS, sonst Stopp [cm] */
/* +1U: die Ganzzahldivision schneidet ab (82,82 -> 82). Die obere Grenze muss
 * aber aufgerundet werden, sonst loest die Erkennung schon bei 9,65 cm statt
 * bei 10 cm Stufenhoehe aus. Ergibt 83 cm und trifft damit Tab. 5.11. */
#define IR_BODEN_SCHWELLE_ABWAERTS_CM \
    (((IR_BODEN_H_CM + IR_BODEN_STOPP_ABWAERTS_CM) * 10000U) / IR_BODEN_ALPHA_SIN_10000 + 1U)
#define IR_BODEN_SCHWELLE_AUFWAERTS_CM \
    (((IR_BODEN_H_CM - IR_BODEN_STOPP_AUFWAERTS_CM) * 10000U) / IR_BODEN_ALPHA_SIN_10000)

#define IR_FORMEL_K                 58502U  /* Kennlinie: d [cm] = K / U [mV] */
#define IR_VREF_MV                  3300U   /* ADC-Referenzspannung [mV] */
#define IR_ADC_MAX                  4095U   /* ADC-Maximum (12 Bit) */

/* Kabelbruchschwellen [ADC] - deaktiviert */
/* #define IR_ADC_KABELBRUCH_MIN       310U */
/* #define IR_ADC_KABELBRUCH_MAX      3537U */

#define IR_STARTUP_DELAY_US         50000U  /* Anlaufzeit [us] */
#define IR_MESSINTERVALL_US         50000U  /* Messintervall je Sensor [us] */
#define IR_VERSATZ_US               10000U  /* Versatz zwischen Sensoren [us] */

void     IR_Sensor_Init(void);
void     IR_Sensor_Process(void);
uint32_t IR_Sensor_GetDistanz(uint8_t idx);
uint8_t  IR_Sensor_IstStopp(void);
/* uint8_t  IR_Sensor_IstKabelbruch(void); */  /* deaktiviert */

#endif /* IR_SENSOR_H_ */
