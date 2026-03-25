/**
 * @file uart_driver.c
 * @brief UART Driver Implementation - Watchdog Heartbeat
 * @date 2026-03-22
 */

#include "uart_driver.h"
#include "misc_drivers.h"
#include <stddef.h>

/* USART1 registers base address */
#define USART1_BASE_ADDR 0x40011000UL

/* GPIO alternate function for USART1 */
#define GPIO_AF_USART1 0x07U

/* USART register offsets */
#define USART_SR_OFFSET   0x00UL
#define USART_DR_OFFSET   0x04UL
#define USART_BRR_OFFSET  0x08UL
#define USART_CR1_OFFSET  0x0CUL
#define USART_CR2_OFFSET  0x10UL
#define USART_CR3_OFFSET  0x14UL

/* USART_SR flags */
#define USART_FLAG_RXNE ((uint16_t)0x0020)
#define USART_FLAG_TC   ((uint16_t)0x0040)
#define USART_FLAG_TXE  ((uint16_t)0x0080)

/* USART CR1 bits */
#define USART_CR1_RE  (1UL << 2)
#define USART_CR1_TE  (1UL << 3)
#define USART_CR1_UE  (1UL << 13)

/* Status constants */
#define SET   1
#define RESET 0

/* Inline: Get USART flag status */
static inline uint16_t USART_GetFlagStatus(uint32_t USARTx, uint16_t USART_FLAG)
{
    return (*(volatile uint32_t *)(USARTx + USART_SR_OFFSET) & USART_FLAG);
}

/* Inline: Send data via USART */
static inline void USART_SendData(uint32_t USARTx, uint16_t Data)
{
    *(volatile uint32_t *)(USARTx + USART_DR_OFFSET) = (Data & 0xFFU);
}

/* Inline: Receive data from USART */
static inline uint16_t USART_ReceiveData(uint32_t USARTx)
{
    return (*(volatile uint32_t *)(USARTx + USART_DR_OFFSET) & 0xFFU);
}

/* USART1 address */
#define USART1 USART1_BASE_ADDR

/* UART status flags */
static volatile uint8_t uart_status = 0;  /* 0=ready, 1=busy, 2=error */

void UART_Init(void)
{
    GPIO_InitTypeDef gpio_cfg;
    uint32_t brr;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    gpio_cfg.GPIO_Pin = UART_TX_PIN | UART_RX_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AF;
    gpio_cfg.GPIO_OType = GPIO_OType_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_cfg.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio_cfg);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    *(volatile uint32_t *)(USART1 + USART_CR1_OFFSET) = 0U;
    *(volatile uint32_t *)(USART1 + USART_CR2_OFFSET) = 0U;
    *(volatile uint32_t *)(USART1 + USART_CR3_OFFSET) = 0U;

    brr = (APB2_CLK_FREQ + (UART_BAUDRATE / 2U)) / UART_BAUDRATE;
    *(volatile uint32_t *)(USART1 + USART_BRR_OFFSET) = brr;

    *(volatile uint32_t *)(USART1 + USART_CR1_OFFSET) = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
    uart_status = 0U;
}

void UART_SendHeartbeat(void)
{
    UART_SendByte((uint8_t)HEARTBEAT_PULSE);
}

void UART_SendByte(uint8_t byte)
{
    uart_status = 1U;

    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
    }

    USART_SendData(USART1, byte);

    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
    }

    uart_status = 0U;
}

void UART_SendString(const char *str)
{
    if (str == NULL) {
        return;
    }

    while (*str != '\0') {
        UART_SendByte((uint8_t)*str);
        str++;
    }
}

uint8_t UART_ReadByte(uint8_t *pByte)
{
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {
        if (pByte != NULL) {
            *pByte = (uint8_t)USART_ReceiveData(USART1);
        }
        return 1U;
    }
    return 0U;
}

uint8_t UART_GetStatus(void)
{
    return uart_status;
}
