/**
 * @file uart_driver.h
 * @brief UART Driver - Watchdog Heartbeat Communication (F4 → F1)
 * @details API để gửi heartbeat pulse từ F4 sang F1 watchdog qua UART
 *          - Gửi byte 0xAA mỗi 50-100ms
 *          - F1 dùng để phát hiện crash F4
 * @author STM32 UART Team
 * @date 2026-03-22
 * 
 * ============================================================================
 * MỤC ĐÍCH: Giao tiếp UART đơn giản F4 ↔ F1 cho watchdog
 * 
 * Protocol:
 *   - Baud rate: 115200
 *   - Data bits: 8
 *   - Stop bits: 1
 *   - Parity: None
 *   - Flow control: None
 * 
 * Heartbeat format:
 *   - F4 → F1: 0xAA byte (pulse để báo F4 còn sống)
 *   - Tần suất: 50-100ms (mục tiêu: 100ms)
 *   - Timeout F1: 200-300ms (nếu không nhận được 2+ pulses → reset F4)
 * 
 * Ví dụ sử dụng:
 *   UART_Init();                  // Khởi tạo UART1 (F4→F1)
 *   UART_SendHeartbeat();         // Gửi 0xAA
 *   UART_SendByte(0xFF);          // Gửi 1 byte
 *   UART_SendString("Hello");     // Gửi string (debug)
 * 
 * ============================================================================
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "heartbeat_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART CONFIGURATION
 * ============================================================================ */

/** UART1 được dùng cho F4 ↔ F1 Watchdog Communication */
#define WATCHDOG_UART          USART1

/** UART Baud Rate */
#define UART_BAUDRATE          115200

/** Heartbeat byte value */
#define HEARTBEAT_PULSE        HEARTBEAT_PULSE_VALUE

/** Heartbeat interval (ms) */
#define HEARTBEAT_INTERVAL_MS  HEARTBEAT_PERIOD_MS

/* ============================================================================
 * UART PIN MAPPING
 * ============================================================================ */

/**
 * UART1 TX Pin (F4 → F1):
 *   - Pin: PA9 (USART1_TX)
 *   - Alternate Function: AF7
 */
#define UART_TX_PIN            GPIO_Pin_9   // PA9
#define UART_TX_PORT           GPIOA
#define UART_TX_AF             GPIO_AF_USART1

/**
 * UART1 RX Pin (F1 → F4) - Optional, dùng cho debug/commands:
 *   - Pin: PA10 (USART1_RX)
 *   - Alternate Function: AF7
 */
#define UART_RX_PIN            GPIO_Pin_10  // PA10
#define UART_RX_PORT           GPIOA
#define UART_RX_AF             GPIO_AF_USART1

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo UART1 cho Watchdog Heartbeat Communication
 * 
 * @details Hàm này:
 *  1. Bật clock cho GPIOA + USART1 (APB2)
 *  2. Cấu hình GPIO:
 *     - PA9: Output alternate function (UART_TX)
 *     - PA10: Input alternate function (UART_RX) - optional
 *  3. Cấu hình UART1:
 *     - Baud rate: 115200
 *     - Data bits: 8
 *     - Stop bits: 1
 *     - Parity: None
 *     - Hardware flow control: Disabled
 *     - Mode: TX + RX
 *  4. Bật UART1
 *  5. Bật UART interrupt (nếu cần RX callback)
 * 
 * @return void
 * @note Phải gọi hàm này trước khi dùng UART_SendHeartbeat()
 */
void UART_Init(void);

/**
 * @brief Gửi Heartbeat Pulse (0xAA byte) tới F1
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Gửi byte 0xAA (HEARTBEAT_PULSE) qua UART1_TX
 *  2. Chờ cho đến khi transmission complete
 *  3. Dùng cho Watchdog Heartbeat Task (mỗi 100ms)
 * 
 * @example
 *  // Trong Watchdog Heartbeat Task (Priority 4 - cao nhất)
 *  void HeartbeatTask(void *pvParameters) {
 *      for (;;) {
 *          UART_SendHeartbeat();
 *          osDelay(100);  // Gửi mỗi 100ms
 *      }
 *  }
 * 
 * @note Blocking call - chờ đến khi transmission xong
 * @note Phải gọi UART_Init() trước
 */
void UART_SendHeartbeat(void);

/**
 * @brief Gửi 1 byte qua UART (generic)
 * 
 * @param[in] byte  Byte value (0x00 - 0xFF)
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Gửi byte data qua UART1_TX
 *  2. Chờ cho đến khi transmission complete
 *  3. Dùng cho debug hoặc custom commands
 * 
 * @example
 *  UART_SendByte(0xFF);    // Gửi 0xFF (command)
 *  UART_SendByte(0x01);    // Gửi 0x01 (data)
 * 
 * @note Blocking call
 */
void UART_SendByte(uint8_t byte);

/**
 * @brief Gửi string (for debug)
 * 
 * @param[in] str  Con trỏ tới null-terminated string
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Gửi từng byte của string
 *  2. Dừng khi gặp '\0'
 *  3. Dùng cho debug output
 * 
 * @example
 *  UART_SendString("Motor A: Error!\n");
 *  UART_SendString("Speed: 256 RPM");
 * 
 * @note Blocking call - tốc độ phụ thuộc vào độ dài string
 */
void UART_SendString(const char *str);

/**
 * @brief Đọc 1 byte từ UART RX (nếu có data)
 * 
 * @param[out] pByte  Con trỏ tới biến để lưu byte nhận được
 * 
 * @return uint8_t  1 nếu có data, 0 nếu không có data trong buffer
 * 
 * @details Hàm này:
 *  1. Kiểm tra UART RX buffer có data không
 *  2. Nếu có, lưu vào *pByte và trả về 1
 *  3. Nếu không có, trả về 0 (non-blocking)
 *  4. Dùng cho receive command từ F1 hoặc debug
 * 
 * @example
 *  uint8_t received_byte;
 *  if (UART_ReadByte(&received_byte)) {
 *      // Có data từ F1
 *      if (received_byte == 0xFF) {
 *          // Handle F1 command
 *      }
 *  }
 * 
 * @note Non-blocking - trả về ngay lập tức
 */
uint8_t UART_ReadByte(uint8_t *pByte);

/**
 * @brief Trả về status UART (ready/busy/error)
 * 
 * @return uint8_t  0 = Ready, 1 = Busy (transmitting), 2 = Error
 * 
 * @details Dùng để check xem UART có ready để gửi dữ liệu không
 */
uint8_t UART_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H */
