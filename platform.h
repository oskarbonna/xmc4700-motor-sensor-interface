/*
 * platform.h
 *
 *  Created on: 2026 Jun 15 10:52:07
 *      Author: oskib
 *
 * Zentrale Fahrzeugparameter.
 * NUR DIESE DATEI ANPASSEN, um Geschwindigkeit, Kurvenverhalten,
 * Motorverhalten und die Erkennungsschwellen einzustellen.
 * Gilt fuer Joystick, UART und die Sensorik.
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

/* ================================================================
 * ANWENDERPARAMETER – hier aendern
 * ================================================================ */

#define PLATFORM_MAX_KMH           3U    /* Hoechstgeschwindigkeit [km/h]          */
                                         /* Beliebig aenderbar – MAX_PWM skaliert  */
                                         /* automatisch mit (Hardware-Max ca. 7,6) */

#define PLATFORM_R_MIN_MM        300U    /* Mindestkurvenradius [mm]               */
                                         /* Kleiner = schraefere Kurven moeglich   */

#define PLATFORM_RICHTUNGSWECHSEL_STOP_TIME_US  1000000U
                                         /* Stopp-Phase vor Richtungswechsel [us]  */
                                         /* gilt fuer Joystick und UART            */

/* ================================================================
 * ERKENNUNGSSCHWELLEN (Sicherheit) – hier aendern
 * Gelten fuer ir_sensor (Front- und Bodensensor).
 * ================================================================ */
#define PLATFORM_HINDERNIS_CM         50U   /* Frontsensoren: Stopp wenn naeher [cm]          */

#define PLATFORM_STUFE_ABWAERTS_CM    10U   /* Bodensensor: Mindest-Stufenhoehe abwaerts [cm] */
                                            /* Bordstein/Treppe (RASt 06: ab 10 cm)           */

#define PLATFORM_STUFE_AUFWAERTS_CM   14U   /* Bodensensor: Mindest-Hindernishoehe aufwaerts  */
                                            /* Aufwaertsstufe (DIN 18065: s_min = 14 cm)      */

/* ================================================================
 * KALIBRIERUNG – nur nach Messung aendern
 * Gemessen: 20% PWM (2000 von 10000) -> 31 U/min -> 1,52 km/h
 *           -> 2000 / 1,52 = 1316 PWM pro km/h (Max 100% ~ 7,6 km/h)
 * ================================================================ */
#define PLATFORM_PWM_PER_KMH    1316U    /* PWM-Einheiten pro km/h (kalibriert)   */

/* ================================================================
 * GEOMETRIE – Fahrzeugmasse, nicht aendern
 * ================================================================ */
#define PLATFORM_SPURBREITE_HALB 258U    /* Halbe Spurbreite [mm], fuer den        */
                                         /* Differentialantrieb (Drehzahldifferenz */
                                         /* der beiden Fahrzeugseiten)             */

/* ================================================================
 * INTERNE ABLEITUNGEN – nicht aendern
 * Aus den Parametern oben berechnet, kein eigener Anpasswert.
 * ================================================================ */
/* Stellgroessen-Maximum bei Geradeausfahrt (beide Seiten gleich) */
#define PLATFORM_MAX_PWM         (PLATFORM_MAX_KMH * PLATFORM_PWM_PER_KMH)
/* Obergrenze fuer das kurvenaeussere Rad: es darf in engen Kurven
 * ueber PLATFORM_MAX_PWM hinaus, daher der verdoppelte Wert         */
#define PLATFORM_MAX_PWM_CURVE   (PLATFORM_MAX_PWM * 2U)

#endif /* PLATFORM_H_ */
