/*
 * ir_sensor.c
 *
 *  Created on: 2026 May 19 10:32:07
 *      Author: oskib
 *
 * Sensor 0 (Channel C, "Distance L"): Front links
 * Sensor 1 (Channel D, "Distance M"): Boden (diagonal)
 * Sensor 2 (Channel E, "Distance R"): Front rechts
 */
#include <stdint.h>
#include "ir_sensor.h"
#include "adc.h"

static volatile uint8_t  sensor_bereit = 0U;
static volatile uint32_t distanz_cm[IR_SENSOR_ANZAHL] = { 150U, 150U, 150U };

/* hindernis[idx]: 1 = Hindernis, 0 = frei */
static volatile uint8_t  hindernis[IR_SENSOR_ANZAHL]  = { 0U, 0U, 0U };

/* kabelbruch[idx]: 1 = Kabelbruch, 0 = ok (IR-Kabelbruch derzeit deaktiviert) */
static volatile uint8_t  kabelbruch[IR_SENSOR_ANZAHL] = { 0U, 0U, 0U };

/* Startwerte: erste Messung ans Ende der Anlaufzeit, Sensoren je 10 ms versetzt */
static volatile uint32_t letzte_messung_us[IR_SENSOR_ANZAHL] =
{
    (IR_STARTUP_DELAY_US - IR_MESSINTERVALL_US) + 0U * IR_VERSATZ_US,
    (IR_STARTUP_DELAY_US - IR_MESSINTERVALL_US) + 1U * IR_VERSATZ_US,
    (IR_STARTUP_DELAY_US - IR_MESSINTERVALL_US) + 2U * IR_VERSATZ_US
};

/* Timer-Callback: gibt die Sensoren nach der Anlaufzeit frei */
static void Startup_Fertig(void* args)
{
    (void)args;
    sensor_bereit = 1U;
}

/* IR-Kabelbruch deaktiviert */
/*
static uint8_t Kabelbruch_Erkannt(uint32_t adc_wert)
{
    if (adc_wert < IR_ADC_KABELBRUCH_MIN || adc_wert > IR_ADC_KABELBRUCH_MAX)
    {
        return 1U;
    }
    return 0U;
}
*/

/* Rechnet einen ADC-Rohwert in eine Distanz [cm] um */
static uint32_t ADC_Zu_Distanz(uint32_t adc_wert)
{
    uint32_t spannung_mv = (adc_wert * IR_VREF_MV) / IR_ADC_MAX;
    uint32_t distanz     = IR_FORMEL_K / spannung_mv;

    /* auf Messbereich begrenzen */
    if (distanz < IR_DISTANZ_MIN_CM)
    {
        distanz = IR_DISTANZ_MIN_CM;
    }
    if (distanz > IR_DISTANZ_MAX_CM)
    {
        distanz = IR_DISTANZ_MAX_CM;
    }
    return distanz;
}

/* Misst und verarbeitet einen Sensor, nur alle IR_MESSINTERVALL_US */
static void Sensor_Verarbeiten(uint8_t idx)
{
    uint32_t jetzt_us     = (uint32_t)SYSTIMER_GetTime();
    uint32_t vergangen_us = jetzt_us - letzte_messung_us[idx];

    if (vergangen_us < IR_MESSINTERVALL_US)
    {
        return;
    }
    letzte_messung_us[idx] = jetzt_us;

    /* ADC-Rohwert lesen und umrechnen */
    uint32_t adc = ADC_GetKanal(idx);
    uint32_t dist;

    /* Kabelbruch deaktiviert: kabelbruch[idx] bleibt 0 */
    /*
    if (Kabelbruch_Erkannt(adc) == 1U)
    {
        kabelbruch[idx] = 1U;
        dist            = 0U;
    }
    else
    {
        kabelbruch[idx] = 0U;
        dist            = ADC_Zu_Distanz(adc);
    }
    */
    kabelbruch[idx] = 0U;
    dist            = ADC_Zu_Distanz(adc);
    distanz_cm[idx] = dist;

    /* Frontsensoren (idx 0 = links, idx 2 = rechts): naeher als Schwelle -> Hindernis */
    if (idx == 0U || idx == 2U)
    {
        if (dist < IR_DISTANZ_SCHWELLE_CM)
        {
            hindernis[idx] = 1U;
        }
        else
        {
            hindernis[idx] = 0U;
        }
    }

    /* Bodensensor (idx 1): frei nur im gueltigen Band, sonst STOPP (Loch/Kante/Stufe) */
    if (idx == 1U)
    {
        if ((dist > IR_BODEN_SCHWELLE_AUFWAERTS_CM) &&
            (dist < IR_BODEN_SCHWELLE_ABWAERTS_CM))
        {
            hindernis[1U] = 0U;
        }
        else
        {
            hindernis[1U] = 1U;
        }
    }

    ADC_StartKonversion();
}

/* Startet die erste ADC-Wandlung und den Anlauf-Timer (SYSTIMER, One-Shot) */
void IR_Sensor_Init(void)
{
    uint32_t timer_id = 0U;
    ADC_StartKonversion();
    timer_id = (uint32_t)SYSTIMER_CreateTimer(
                    IR_STARTUP_DELAY_US, SYSTIMER_MODE_ONE_SHOT,
                    Startup_Fertig, NULL);
    if (timer_id != 0U)
    {
        SYSTIMER_StartTimer(timer_id);
    }
}

/* Zyklisch aufrufen: verarbeitet alle drei Sensoren */
void IR_Sensor_Process(void)
{
    if (sensor_bereit == 0U)
    {
        return;
    }
    Sensor_Verarbeiten(0U);
    Sensor_Verarbeiten(1U);
    Sensor_Verarbeiten(2U);
}

/* Letzte Distanz [cm] von Sensor idx (0..2) */
uint32_t IR_Sensor_GetDistanz(uint8_t idx)
{
    return distanz_cm[idx];
}

/* 1 = mindestens ein Sensor meldet ein Hindernis */
uint8_t IR_Sensor_IstStopp(void)
{
    return (hindernis[0U] | hindernis[1U] | hindernis[2U]);
}

/* IstKabelbruch DEAKTIVIERT (siehe oben): 1 = mind. ein Sensor meldet Kabelbruch */
/*
uint8_t IR_Sensor_IstKabelbruch(void)
{
    return (kabelbruch[0U] | kabelbruch[1U] | kabelbruch[2U]);
}
*/
