/**
 * @file uart_driver.c
 * @brief UART Driver Implementation - Watchdog Heartbeat
 * @date 2026-03-22
 */

#include "uart_driver.h"
#include "misc_drivers.h"
#include <stddef.h>

/* ============================================================================
 * USART INLINE FUNCTIONS (Basic register access wrappers)
 * ============================================================================ */

/* USART1 registers base address */
#define USART1_BASE 0x40011000

/* USART_SR flags */
#define USART_FLAG_RXNE ((uint16_t)0x0020)  // Receive buffer not empty
#define USART_FLAG_TC   ((uint16_t)0x0040)  // Transmission complete
#define USART_FLAG_TXE  ((uint16_t)0x0080)  // Transmit buffer empty

/* Status constants */
#define SET   1
#define RESET 0

/* Inline: Get USART flag status */
static inline uint16_t USART_GetFlagStatus(uint32_t USARTx, uint16_t USART_FLAG)
{
    return (*(volatile uint32_t *)(USARTx + 0x00) & USART_FLAG);  // Read SR register
}

/* Inline: Send data via USART */
static inline void USART_SendData(uint32_t USARTx, uint16_t Data)
{
    *(volatile uint32_t *)(USARTx + 0x04) = (Data & 0xFF);  // Write to DR register
}

/* Inline: Receive data from USART */
static inline uint16_t USART_ReceiveData(uint32_t USARTx)
{
    return (*(volatile uint32_t *)(USARTx + 0x04) & 0xFF);  // Read from DR register
}

/* USART1 address */
#define USART1 USART1_BASE

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================ */

/** UART RX buffer (circular) - For future interrupt mode */
/* static uint8_t uart_rx_buffer[256];
static uint16_t uart_rx_head = 0;
static uint16_t uart_rx_tail = 0; */

/** UART status flags */
static volatile uint8_t uart_status = 0;  // 0=ready, 1=busy, 2=error

/* ============================================================================
 * PUBLIC FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

/**
 * @brief UART_Init - Khởi tạo UART1
 */
void UART_Init(void)
{
    /*
     * TODO: Implement UART initialization
     * 
     * Steps:
     * 1. Enable RCC clocks:
     *    - RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE)
     *    - RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE)
     * 
     * 2. Configure GPIO (PA9-TX, PA10-RX) as alternate function:
     *    - GPIO_Mode = GPIO_Mode_AF
     *    - GPIO_OType = GPIO_OType_PP (push-pull)
     *    - GPIO_PuPd = GPIO_PuPd_NOPULL
     *    - GPIO_Ospeed = GPIO_Speed_25MHz
     *    - GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1)
     *    - GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1)
     *    - GPIO_Init(GPIOA, &GPIO_InitStructure)
     * 
     * 3. Configure USART1:
     *    - USART_InitStructure.USART_BaudRate = 115200
     *    - USART_InitStructure.USART_WordLength = USART_WordLength_8b
     *    - USART_InitStructure.USART_StopBits = USART_StopBits_1
     *    - USART_InitStructure.USART_Parity = USART_Parity_No
     *    - USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx
     *    - USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None
     *    - USART_Init(USART1, &USART_InitStructure)
     * 
     * 4. Enable USART1:
     *    - USART_Cmd(USART1, ENABLE)
     * 
     * 5. Enable USART1 RX interrupt (optional):
     *    - USART_ITConfig(USART1, USART_IT_RXNE, ENABLE)
     *    - NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn
     *    - NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2
     *    - NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0
     *    - NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE
     *    - NVIC_Init(&NVIC_InitStructure)
     */
}

/**
 * @brief UART_SendHeartbeat - Gửi 0xAA pulse
 */
void UART_SendHeartbeat(void)
{
    /*
     * TODO: Implement heartbeat send
     * 
     * 1. Set uart_status = 1 (busy)
     * 2. Send HEARTBEAT_PULSE (0xAA) via USART1_DR
     * 3. Wait until USART_GetFlagStatus(USART1, USART_FLAG_TC) == SET
     * 4. Set uart_status = 0 (ready)
     * 
     * Note: TC (Transmission Complete) flag means transmission finished
     */
}

/**
 * @brief UART_SendByte - Gửi 1 byte
 */
void UART_SendByte(uint8_t byte)
{
    (void)byte;  // Mark as unused for now
    /*
     * TODO: Implement byte send
     * 
     * 1. Wait for USART_FLAG_TXE (transmit buffer empty)
     * 2. Send byte via USART_SendData(USART1, byte)
     * 3. Wait for USART_FLAG_TC (transmission complete)
     */
}

/**
 * @brief UART_SendString - Gửi string
 */
void UART_SendString(const char *str)
{
    (void)str;  // Mark as unused for now
    /*
     * TODO: Implement string send
     * 
     * 1. Loop through each character in str
     * 2. Call UART_SendByte() for each character
     * 3. Stop when encountering '\0' (null terminator)
     * 4. Optional: Handle '\n' (newline) if needed
     * 
     * Example loop:
     *   while (*str != '\0') {
     *       UART_SendByte(*str);
     *       str++;
     *   }
     */
}

/**
 * @brief UART_ReadByte - Đọc 1 byte
 */
uint8_t UART_ReadByte(uint8_t *pByte)
{
    /*
     * TODO: Implement byte read
     * 
     * 1. Check if USART_FLAG_RXNE (receive buffer not empty)
     * 2. If yes:
     *    - Read byte via USART_ReceiveData(USART1)
     *    - Store to *pByte
     *    - Return 1 (data available)
     * 3. If no:
     *    - Return 0 (no data)
     * 
     * Non-blocking implementation
     */
    
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {
        if (pByte != NULL) {
            *pByte = USART_ReceiveData(USART1);
        }
        return 1;  // Data received
    }
    return 0;  // No data
}

/**
 * @brief UART_GetStatus - Trả về UART status
 */
uint8_t UART_GetStatus(void)
{
    /*
     * TODO: Implement status return
     * 
     * Return values:
     *   0 = Ready (can send)
     *   1 = Busy (transmitting)
     *   2 = Error (framing/parity error)
     */
    
    return uart_status;
}

/* ============================================================================
 * INTERRUPT HANDLERS (Optional - for RX interrupt mode)
 * ============================================================================ */

/**
 * @brief USART1_IRQHandler - Handle UART1 interrupts
 * 
 * Note: Define this only if using interrupt mode
 * Otherwise, can use polling with UART_ReadByte()
 */
/*
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        // UART RX byte received
        uint8_t data = USART_ReceiveData(USART1);
        
        // Store to circular buffer
        uart_rx_buffer[uart_rx_head] = data;
        uart_rx_head = (uart_rx_head + 1) % 256;
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
    
    if (USART_GetITStatus(USART1, USART_IT_TC) != RESET) {
        // Transmission complete
        uart_status = 0;  // Ready
        USART_ClearITPendingBit(USART1, USART_IT_TC);
    }
    
    if (USART_GetITStatus(USART1, USART_IT_FE) != RESET) {
        // Framing error
        uart_status = 2;  // Error
        USART_ClearITPendingBit(USART1, USART_IT_FE);
    }
}
*/
