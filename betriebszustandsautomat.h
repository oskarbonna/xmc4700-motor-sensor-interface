/*
 * betriebszustandsautomat.h
 *
 *  Created on: 2026 Jun 10 14:05:48
 *      Author: oskib
 */
#ifndef BETRIEBSZUSTANDSAUTOMAT_H_
#define BETRIEBSZUSTANDSAUTOMAT_H_

#include <DAVE.h>

/* Betriebszustaende */
typedef enum
{
    AUTOMAT_BEREIT = 0U,   /* normale Fahrt */
    AUTOMAT_FEHLER = 1U    /* Kabelbruch -> Aktorik gesperrt (Motor_Stop) */
} Betriebszustand_t;

void              Automat_Init(void);        /* in den Ausgangszustand BEREIT setzen         */
void              Automat_Update(void);      /* zyklisch aufrufen: Fehlerbedingungen pruefen */
uint8_t           Automat_IstFehler(void);   /* 1 = Fehlerzustand, sonst 0                   */

#endif /* BETRIEBSZUSTANDSAUTOMAT_H_ */
