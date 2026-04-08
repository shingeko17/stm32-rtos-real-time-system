/**
 * @file uart_driver_eventdriven.c
 * @brief UART Non-blocking Event-driven Driver (Debug version)
 * @description
 *   Fixed version with:
 *   - Circular ring buffer (RX buffer)
 *   - Non-blocking send with queue
 *   - Interrupt-driven RX
 *   - Event-based instead of polling loops
 * 
 * Problem in old version:
 *   - UART_SendByte() has blocking while() loops
 *   - Baudrate 115200 data arrives too fast
 *   - If CPU is blocked, RX interrupt misses frames
 *   - Result: garbled data in PuTTY
 * 
 * Solution:
 *   - No blocking loops
 *   - Ring buffer catches all RX data
 *   - TX queue if TX busy
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * RING BUFFER CONFIGURATION
 * ============================================================================ */

#define RX_BUFFER_SIZE    256   /* Circular RX buffer */
#define TX_BUFFER_SIZE    256   /* Circular TX queue */

/* ============================================================================
 * RING BUFFER STRUCTURE
 * ============================================================================ */

typedef struct {
    uint8_t buffer[RX_BUFFER_SIZE];
    volatile uint16_t head;       /* Write pointer (ISR updates) */
    volatile uint16_t tail;       /* Read pointer (app reads) */
    volatile uint16_t count;      /* Number of bytes in buffer */
} ring_buffer_t;

typedef struct {
    uint8_t buffer[TX_BUFFER_SIZE];
    volatile uint16_t head;       /* Write pointer (app writes) */
    volatile uint16_t tail;       /* Read pointer (ISR sends) */
    volatile uint16_t count;      /* Number of bytes in queue */
    volatile uint8_t busy;        /* If TX is in progress */
} tx_queue_t;

/* ============================================================================
 * REGISTER DEFINITIONS
 * ============================================================================ */

#define USART1_BASE_ADDR  0x40011000UL

#define USART_SR_OFFSET   0x00UL
#define USART_DR_OFFSET   0x04UL
#define USART_BRR_OFFSET  0x08UL
#define USART_CR1_OFFSET  0x0CUL
#define USART_CR2_OFFSET  0x10UL
#define USART_CR3_OFFSET  0x14UL

#define USART_SR_RXNE     (1 << 5)    /* RX not empty */
#define USART_SR_TC       (1 << 6)    /* TX complete */
#define USART_SR_TXE      (1 << 7)    /* TX register empty */

#define USART_CR1_UE      (1 << 13)   /* UART enable */
#define USART_CR1_RE      (1 << 2)    /* Receiver enable */
#define USART_CR1_TE      (1 << 3)    /* Transmitter enable */
#define USART_CR1_RXNEIE  (1 << 5)    /* RX interrupt enable */
#define USART_CR1_TXEIE   (1 << 7)    /* TX interrupt enable */
#define USART_CR1_TCIE    (1 << 6)    /* TX complete interrupt enable */

#define USART1            USART1_BASE_ADDR

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static ring_buffer_t rx_buffer = {0};
static tx_queue_t tx_queue = {0};

static volatile uint32_t uart_rx_count = 0;   /* Debug: RX count */
static volatile uint32_t uart_tx_count = 0;   /* Debug: TX count */
static volatile uint32_t uart_drop_count = 0; /* Debug: Dropped bytes */

/* ============================================================================
 * RING BUFFER OPERATIONS (Non-blocking)
 * ============================================================================ */

/**
 * Add byte to RX ring buffer
 * Called from RX ISR (non-blocking)
 */
static void RX_Buffer_Put(uint8_t byte)
{
    uint16_t next_head = (rx_buffer.head + 1) % RX_BUFFER_SIZE;
    
    if (next_head == rx_buffer.tail) {
        /* Buffer full - drop byte */
        uart_drop_count++;
        return;
    }
    
    rx_buffer.buffer[rx_buffer.head] = byte;
    rx_buffer.head = next_head;
    rx_buffer.count++;
}

/**
 * Get byte from RX ring buffer
 * Called from app (non-blocking)
 */
static uint8_t RX_Buffer_Get(uint8_t *byte)
{
    if (rx_buffer.tail == rx_buffer.head) {
        return 0;  /* Buffer empty */
    }
    
    *byte = rx_buffer.buffer[rx_buffer.tail];
    rx_buffer.tail = (rx_buffer.tail + 1) % RX_BUFFER_SIZE;
    rx_buffer.count--;
    return 1;
}

/**
 * Get RX buffer count
 */
static uint16_t RX_Buffer_Count(void)
{
    return rx_buffer.count;
}

/* ============================================================================
 * TX QUEUE OPERATIONS (Non-blocking)
 * ============================================================================ */

/**
 * Add byte to TX queue
 * Called from app (non-blocking)
 */
static uint8_t TX_Queue_Put(uint8_t byte)
{
    uint16_t next_head = (tx_queue.head + 1) % TX_BUFFER_SIZE;
    
    if (next_head == tx_queue.tail && tx_queue.busy) {
        /* Queue full */
        return 0;
    }
    
    tx_queue.buffer[tx_queue.head] = byte;
    tx_queue.head = next_head;
    tx_queue.count++;
    
    return 1;
}

/**
 * Get next byte from TX queue
 * Called from TX ISR
 */
static uint8_t TX_Queue_Get(uint8_t *byte)
{
    if (tx_queue.tail == tx_queue.head || tx_queue.count == 0) {
        return 0;  /* Queue empty */
    }
    
    *byte = tx_queue.buffer[tx_queue.tail];
    tx_queue.tail = (tx_queue.tail + 1) % TX_BUFFER_SIZE;
    tx_queue.count--;
    
    return 1;
}

/**
 * Get TX queue status
 */
static uint16_t TX_Queue_Count(void)
{
    return tx_queue.count;
}

/* ============================================================================
 * UART CONTROL (Low-level register access)
 * ============================================================================ */

static inline uint16_t UART_GetFlagStatus(uint16_t flag)
{
    return (*(volatile uint32_t *)(USART1 + USART_SR_OFFSET)) & flag;
}

static inline void UART_SendByte_Direct(uint8_t byte)
{
    *(volatile uint32_t *)(USART1 + USART_DR_OFFSET) = byte;
}

static inline uint8_t UART_ReceiveByte_Direct(void)
{
    return (uint8_t)(*(volatile uint32_t *)(USART1 + USART_DR_OFFSET));
}

static inline void UART_EnableTXInterrupt(void)
{
    *(volatile uint32_t *)(USART1 + USART_CR1_OFFSET) |= USART_CR1_TXEIE;
}

static inline void UART_DisableTXInterrupt(void)
{
    *(volatile uint32_t *)(USART1 + USART_CR1_OFFSET) &= ~USART_CR1_TXEIE;
}

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

void UART_Init_EventDriven(void)
{
    /* Reset buffers */
    rx_buffer.head = 0;
    rx_buffer.tail = 0;
    rx_buffer.count = 0;
    
    tx_queue.head = 0;
    tx_queue.tail = 0;
    tx_queue.count = 0;
    tx_queue.busy = 0;
    
    uart_rx_count = 0;
    uart_tx_count = 0;
    uart_drop_count = 0;
    
    /* Enable RX interrupt in UART control register */
    /* NOTE: Assumes UART clock, GPIO pins already configured */
    uint32_t cr1 = *(volatile uint32_t *)(USART1 + USART_CR1_OFFSET);
    cr1 |= USART_CR1_RXNEIE;  /* Enable RX interrupt */
    cr1 |= USART_CR1_TCIE;    /* Enable TX complete interrupt */
    *(volatile uint32_t *)(USART1 + USART_CR1_OFFSET) = cr1;
}

/* ============================================================================
 * PUBLIC API - NON-BLOCKING SEND
 * ============================================================================ */

/**
 * Send character (non-blocking)
 * Queues byte for transmission
 * Returns: 1 if queued, 0 if queue full
 */
uint8_t UART_SendChar_NonBlocking(char c)
{
    return TX_Queue_Put((uint8_t)c);
}

/**
 * Send string (non-blocking)
 * Queues all characters
 * Returns: Number of characters queued
 */
uint16_t UART_SendString_NonBlocking(const char *str)
{
    uint16_t count = 0;
    
    if (!str) return 0;
    
    while (*str && count < TX_BUFFER_SIZE) {
        if (TX_Queue_Put((uint8_t)*str)) {
            count++;
            str++;
        } else {
            break;  /* Queue full */
        }
    }
    
    /* Kick-start TX if not already running */
    if (count > 0 && !tx_queue.busy) {
        UART_EnableTXInterrupt();
    }
    
    return count;
}

/**
 * Send character blocking (for emergencies only)
 * WARNING: This blocks! Use only if absolutely necessary
 */
void UART_SendChar_Blocking(char c)
{
    while (UART_GetFlagStatus(USART_SR_TXE) == 0);
    UART_SendByte_Direct((uint8_t)c);
    while (UART_GetFlagStatus(USART_SR_TC) == 0);
}

/* ============================================================================
 * PUBLIC API - NON-BLOCKING RECEIVE
 * ============================================================================ */

/**
 * Receive character (non-blocking)
 * Returns: Character if available, 0 if none
 */
uint8_t UART_RecvChar_NonBlocking(void)
{
    uint8_t byte;
    if (RX_Buffer_Get(&byte)) {
        return byte;
    }
    return 0;
}

/**
 * Check if data available in RX buffer
 */
uint8_t UART_DataAvailable(void)
{
    return (RX_Buffer_Count() > 0) ? 1 : 0;
}

/**
 * Get RX buffer count
 */
uint16_t UART_RXCount(void)
{
    return RX_Buffer_Count();
}

/**
 * Get TX queue count (debug)
 */
uint16_t UART_TXCount(void)
{
    return TX_Queue_Count();
}

/* ============================================================================
 * INTERRUPT HANDLERS (Event-driven core)
 * ============================================================================ */

/**
 * RX Complete Interrupt Handler
 * Call this from USART1_IRQHandler when RXNE flag set
 * ✅ Non-blocking (just put data in buffer)
 */
void UART_RX_ISR(void)
{
    uint8_t byte = UART_ReceiveByte_Direct();
    RX_Buffer_Put(byte);
    uart_rx_count++;
}

/**
 * TX Complete Interrupt Handler
 * Call this from USART1_IRQHandler when TC or TXE flag set
 * Gets next byte from TX queue if available
 * ✅ Non-blocking (just send next byte if queue has data)
 */
void UART_TX_ISR(void)
{
    uint8_t byte;
    
    if (TX_Queue_Get(&byte)) {
        /* Queue has data - send it */
        UART_SendByte_Direct(byte);
        uart_tx_count++;
        tx_queue.busy = 1;
    } else {
        /* Queue empty - disable interrupt */
        tx_queue.busy = 0;
        UART_DisableTXInterrupt();
    }
}

/* ============================================================================
 * DEBUG STATISTICS
 * ============================================================================ */

void UART_PrintStats(void)
{
    UART_SendString_NonBlocking("\r\n========== UART STATS ==========\r\n");
    
    char buf[64];
    snprintf(buf, sizeof(buf), "RX Count:    %lu\r\n", uart_rx_count);
    UART_SendString_NonBlocking(buf);
    
    snprintf(buf, sizeof(buf), "TX Count:    %lu\r\n", uart_tx_count);
    UART_SendString_NonBlocking(buf);
    
    snprintf(buf, sizeof(buf), "Drop Count:  %lu\r\n", uart_drop_count);
    UART_SendString_NonBlocking(buf);
    
    snprintf(buf, sizeof(buf), "RX Buffer:   %u/%u\r\n", RX_Buffer_Count(), RX_BUFFER_SIZE);
    UART_SendString_NonBlocking(buf);
    
    snprintf(buf, sizeof(buf), "TX Queue:    %u/%u\r\n", TX_Queue_Count(), TX_BUFFER_SIZE);
    UART_SendString_NonBlocking(buf);
    
    UART_SendString_NonBlocking("================================\r\n");
}

/**
 * Reset statistics (for benchmarking)
 */
void UART_ResetStats(void)
{
    uart_rx_count = 0;
    uart_tx_count = 0;
    uart_drop_count = 0;
}
