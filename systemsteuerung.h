/*
 * systemsteuerung.h
 *
 *  Created on: 2026 Jun 10 11:23:17
 *      Author: oskib
 */
#ifndef SYSTEMSTEUERUNG_H_
#define SYSTEMSTEUERUNG_H_

#include <DAVE.h>
#include "betriebszustandsautomat.h"

void Fahrzeug_Stop(void);                            /* alle Motoren abschalten */
void Diagnose_SetStatus(Betriebszustand_t zustand);  /* Betriebszustand -> Status-LED */

#endif /* SYSTEMSTEUERUNG_H_ */
