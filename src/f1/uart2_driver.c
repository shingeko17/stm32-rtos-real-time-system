/**
 * @file uart2_driver.c
 * @brief STM32F103 UART2 Driver Implementation
 * @date 2026-03-29
 *
 * UART2 Configuration:
 * - TX: PA2 (Alternate Function)
 * - RX: PA3 (Alternate Function)
 * - Clock: APB1 (42MHz / 9600 baud)
 * - Baud rate: 9600
 * - Data bits: 8
 * - Stop bits: 1
 * - Parity: None
 */

#include "uart2_driver.h"

/* ============================================================================
 * REGISTER DEFINITIONS (STM32F103)
 * ============================================================================ */

#define RCC_BASE            0x40021000UL
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x1C))

#define GPIOA_BASE          0x40010800UL
#define GPIOA_CRL           (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_CRH           (*(volatile uint32_t *)(GPIOA_BASE + 0x04))

#define UART2_BASE          0x40004400UL
#define UART2_SR            (*(volatile uint32_t *)(UART2_BASE + 0x00))
#define UART2_DR            (*(volatile uint32_t *)(UART2_BASE + 0x04))
#define UART2_BRR           (*(volatile uint32_t *)(UART2_BASE + 0x08))
#define UART2_CR1           (*(volatile uint32_t *)(UART2_BASE + 0x0C))
#define UART2_CR2           (*(volatile uint32_t *)(UART2_BASE + 0x10))
#define UART2_CR3           (*(volatile uint32_t *)(UART2_BASE + 0x14))

/* UART SR flags */
#define UART_SR_TXE         (1 << 7)   /* Transmit data register empty */
#define UART_SR_TC          (1 << 6)   /* Transmission complete */
#define UART_SR_RXNE        (1 << 5)   /* Read data register not empty */

/* UART CR1 bits */
#define UART_CR1_UE         (1 << 13)  /* UART enable */
#define UART_CR1_M          (1 << 12)  /* Word length (0=8bit, 1=9bit) */
#define UART_CR1_PCE        (1 << 10)  /* Parity control enable */
#define UART_CR1_PS         (1 << 9)   /* Parity selection */
#define UART_CR1_PEIE       (1 << 8)   /* Parity error interrupt enable */
#define UART_CR1_TXEIE      (1 << 7)   /* TX empty interrupt enable */
#define UART_CR1_TCIE       (1 << 6)   /* TX complete interrupt enable */
#define UART_CR1_RXNEIE     (1 << 5)   /* RX not empty interrupt enable */
#define UART_CR1_IDLEIE     (1 << 4)   /* Idle interrupt enable */
#define UART_CR1_TE         (1 << 3)   /* Transmitter enable */
#define UART_CR1_RE         (1 << 2)   /* Receiver enable */

/* System clock */
#define PCLK1_FREQ          8000000   /* 8MHz APB1 clock (STM32F103 with 8MHz xtal, no PLL) */

/* ============================================================================
 * Circular Receive Buffer
 * ============================================================================ */

static uint8_t uart2_rx_buffer[UART2_RX_BUFFER_SIZE];
static volatile uint16_t uart2_rx_head = 0;
static volatile uint16_t uart2_rx_tail = 0;

/* ============================================================================
 * Calculate Baud Rate Divisor
 * ============================================================================ */

static inline uint16_t UART_BRR_Calculate(uint32_t pclk, uint32_t baudrate)
{
    /* BRR = PCLK1 / (16 * baudrate) */
    return (uint16_t)(pclk / (16 * baudrate));
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

void UART2_Init(void)
{
    /* Enable GPIOA clock (APB2) */
    RCC_APB2ENR |= (1 << 2);

    /* Enable UART2 clock (APB1) */
    RCC_APB1ENR |= (1 << 17);

    /* Configure PA2 as TX (Alternate function push-pull, 50MHz) */
    /* PA2 uses bits 8-11 in CRL */
    GPIOA_CRL &= ~(0xF << 8);      /* Clear PA2 */
    GPIOA_CRL |=  (0xB << 8);      /* Mode=10 (50MHz), CNF=10 (AF push-pull) */

    /* Configure PA3 as RX (Float input) */
    /* PA3 uses bits 12-15 in CRL */
    GPIOA_CRL &= ~(0xF << 12);     /* Clear PA3 */
    GPIOA_CRL |=  (0x4 << 12);     /* Mode=00 (input), CNF=01 (floating) */

    /* Calculate baud rate divisor */
    uint16_t brr = UART_BRR_Calculate(PCLK1_FREQ, UART2_BAUDRATE);

    /* Set baud rate */
    UART2_BRR = brr;

    /* Configure UART2:
     * - 8 data bits
     * - 1 stop bit
     * - No parity
     * - TX/RX enabled
     */
    UART2_CR1 = UART_CR1_TE |      /* TX enable */
                UART_CR1_RE |      /* RX enable */
                UART_CR1_UE;       /* UART enable */

    UART2_CR2 = 0;                 /* 1 stop bit */
    UART2_CR3 = 0;                 /* No flow control */
}

/* ============================================================================
 * Character I/O
 * ============================================================================ */

void UART2_SendChar(char c)
{
    /* Wait until TX register is empty */
    while (!(UART2_SR & UART_SR_TXE));

    /* Send character */
    UART2_DR = (uint8_t)c;
}

char UART2_RecvChar(void)
{
    /* Wait until RX register has data */
    while (!(UART2_SR & UART_SR_RXNE));

    /* Receive character */
    return (char)(UART2_DR & 0xFF);
}

/* ============================================================================
 * String I/O
 * ============================================================================ */

void UART2_SendString(const char *str)
{
    if (!str) return;

    while (*str) {
        UART2_SendChar(*str);
        str++;
    }
}

/* ============================================================================
 * Formatted Output (Simple - without vsnprintf)
 * ============================================================================ */

void UART2_Printf(const char *format, ...)
{
    /* Simple version without vsnprintf - just send the format string */
    UART2_SendString(format);
}

/* ============================================================================
 * Data Array I/O
 * ============================================================================ */

void UART2_SendData(const uint8_t *data, uint32_t length)
{
    if (!data) return;

    for (uint32_t i = 0; i < length; i++) {
        UART2_SendChar((char)data[i]);
    }
}

/* ============================================================================
 * Buffer Status
 * ============================================================================ */

uint8_t UART2_DataAvailable(void)
{
    return (UART2_SR & UART_SR_RXNE) ? 1 : 0;
}
