/**
 * @file uart2_driver.h
 * @brief STM32F103 UART2 Driver (PA2=TX, PA3=RX)
 * @date 2026-03-29
 */

#ifndef UART2_DRIVER_H
#define UART2_DRIVER_H

#include <stdint.h>

/* ============================================================================
 * UART2 Configuration
 * ============================================================================ */

#define UART2_BAUDRATE      115200   /* 115200 baud */
#define UART2_RX_BUFFER_SIZE   64   /* Receive buffer */

/* ============================================================================
 * Function Declarations
 * ============================================================================ */

/**
 * Initialize UART2 (PA2=TX, PA3=RX)
 * Baud rate: 9600, 8N1 (8 bits, no parity, 1 stop bit)
 */
void UART2_Init(void);

/**
 * Send single character via UART2
 */
void UART2_SendChar(char c);

/**
 * Send string via UART2
 */
void UART2_SendString(const char *str);

/**
 * Send formatted string (printf-like)
 * Example: UART2_Printf("LED%d=%s\r\n", led_num, "ON");
 */
void UART2_Printf(const char *format, ...);

/**
 * Receive single character (blocking)
 */
char UART2_RecvChar(void);

/**
 * Check if data available in receive buffer
 */
uint8_t UART2_DataAvailable(void);

/**
 * Send data array
 */
void UART2_SendData(const uint8_t *data, uint32_t length);

#endif /* UART2_DRIVER_H */
