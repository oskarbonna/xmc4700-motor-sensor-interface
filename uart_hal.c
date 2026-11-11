/*
 * uart_hal.c
 *
 *  Created on: 2026 Jun 05 09:18:01
 *      Author: oskib
 *
 * Hardware Abstraction Layer fuer die UART-Schnittstelle.
 * Kein anderes Modul greift direkt auf UART_0 zu.
 *
 * Physikalische Verbindung:
 *   XMC4700 P1.5 (TX) -> CP2102 RXD
 *   XMC4700 P1.4 (RX) -> CP2102 TXD
 *   XMC4700 GND       -> CP2102 GND
 *   Baudrate: 115200, 8N1
 */
#include "uart_hal.h"

/* Wartet, bis Platz im TX-FIFO ist */
static void waitTxReady(void)
{
    while (UART_IsTXFIFOFull(&UART_0))
    {
        /* warten, bis Platz im Sendepuffer ist */
    }
}

/* Sendet ein einzelnes Byte (Basis aller Sendefunktionen) */
void UART_HAL_SendByte(uint8_t byte)
{
    waitTxReady();
    UART_TransmitWord(&UART_0, byte);
}

/* Liest ein Byte aus dem RX-FIFO (muss in DAVE aktiv bleiben).
 * Byte -> *byte_out; Rueckgabe 1 = Byte geholt, 0 = FIFO leer */
uint8_t UART_HAL_RXByte(uint8_t *byte_out)
{
    if (UART_IsRXFIFOEmpty(&UART_0) == 0U)
    {
        *byte_out = (uint8_t)UART_GetReceivedWord(&UART_0);
        return 1U;
    }
    return 0U;
}

/* Prueft nur, ob ein Byte wartet (liest keins); 1 = ja, 0 = nein */
uint8_t UART_HAL_RXAvailable(void)
{
    if (UART_IsRXFIFOEmpty(&UART_0) == 0U)
    {
        return 1U;
    }
    return 0U;
}
