/*
 * uart_hal.h
 *
 *  Created on: 2026 Jun 05 09:17:44
 *      Author: oskib
 */

#ifndef UART_HAL_H
#define UART_HAL_H

#include "DAVE.h"

/* Senden: einzelne Bytes */
void    UART_HAL_SendByte(uint8_t byte);

/* Ein Byte aus dem RX-FIFO lesen; 1 = Byte geholt, 0 = leer */
uint8_t UART_HAL_RXByte(uint8_t *byte_out);

/* Prueft nur, ob ein Byte wartet (nicht-konsumierend); 1 = ja, 0 = nein */
uint8_t UART_HAL_RXAvailable(void);

#endif /* UART_HAL_H */
