/**
 * @file uart_driver_eventdriven.h
 * @brief UART Non-blocking Event-driven Driver Header
 */

#ifndef UART_DRIVER_EVENTDRIVEN_H
#define UART_DRIVER_EVENTDRIVEN_H

#include <stdint.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * Initialize UART with event-driven mode
 * (Must be called after basic UART init)
 */
void UART_Init_EventDriven(void);

/**
 * Send character non-blocking
 * Returns: 1 if queued, 0 if queue full
 */
uint8_t UART_SendChar_NonBlocking(char c);

/**
 * Send string non-blocking
 * Returns: Number of characters queued (may be less than string length if queue full)
 */
uint16_t UART_SendString_NonBlocking(const char *str);

/**
 * Send character blocking (use only if necessary)
 * WARNING: This can block! Avoid in RTOS tasks
 */
void UART_SendChar_Blocking(char c);

/**
 * Receive character non-blocking
 * Returns: Character if available, 0 if none
 */
uint8_t UART_RecvChar_NonBlocking(void);

/**
 * Check if data available
 * Returns: 1 if data available, 0 if not
 */
uint8_t UART_DataAvailable(void);

/**
 * Get number of bytes in RX buffer
 */
uint16_t UART_RXCount(void);

/**
 * Get number of bytes queued in TX buffer
 */
uint16_t UART_TXCount(void);

/**
 * RX Interrupt handler
 * Call from USART ISR when RXNE flag is set
 */
void UART_RX_ISR(void);

/**
 * TX Interrupt handler
 * Call from USART ISR when TC/TXE flag is set
 */
void UART_TX_ISR(void);

/**
 * Print UART statistics
 * Shows RX/TX counts, drop count, buffer usage
 */
void UART_PrintStats(void);

/**
 * Reset statistics counters
 */
void UART_ResetStats(void);

#endif /* UART_DRIVER_EVENTDRIVEN_H */
