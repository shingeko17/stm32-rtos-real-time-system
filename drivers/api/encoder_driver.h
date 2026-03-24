/**
 * @file encoder_driver.h
 * @brief Encoder Driver - Motor Speed Feedback
 * @details API để đọc encoder (rotary sensor) từ motor feedback
 *          - Dùng TIM3 capture input từ encoder output
 *          - Tính RPM từ pulse frequency
 * @author STM32 Encoder Team
 * @date 2026-03-22
 * 
 * ============================================================================
 * MỤC ĐÍCH: Cung cấp giao diện đơn giản để đọc motor speed từ encoder
 * 
 * Ví dụ sử dụng:
 *   Encoder_Init();                    // Khởi tạo TIM3 capture
 *   uint16_t rpm = Encoder_GetRPM();   // Đọc RPM
 *   uint32_t pulses = Encoder_GetCount();  // Đọc tổng số pulse
 * 
 * ============================================================================
 */

#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ENCODER CONFIGURATION
 * ============================================================================ */

/** Timer 3 được dùng cho Encoder Input Capture */
#define ENCODER_TIMER          TIM3

/** Encoder pulses per revolution (PPR) */
#define ENCODER_PPR            20    /* 20 pulses per revolution (tuning needed) */

/** Sampling interval (ms) để calculate RPM */
#define ENCODER_SAMPLE_RATE_MS 100   /* Calculate RPM every 100ms */

/* ============================================================================
 * ENCODER PIN MAPPING
 * ============================================================================ */

/**
 * Encoder Output (Motor Feedback):
 *   - Signal: Square wave pulse (one pulse per shaft revolution or fraction)
 *   - STM32 pin: PA6 (TIM3_CH1) - Input capture from encoder
 *   - Frequency: Proportional to motor speed
 *   - Example: 500 RPM = 500/60 = 8.33 Hz = 167 pulses/2 seconds
 */
#define ENCODER_PIN            GPIO_Pin_6   // PA6
#define ENCODER_PORT           GPIOA
#define ENCODER_CHANNEL        TIM_Channel_1
#define ENCODER_TIM_CHANNEL    1

/** Alternate function for timer capture */
#define ENCODER_AF             GPIO_AF_TIM3

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo Encoder Input Capture (TIM3)
 * 
 * @details Hàm này:
 *  1. Bật clock cho GPIOA + TIM3 (APB1)
 *  2. Cấu hình GPIO:
 *     - PA6: Input alternate function (TIM3_CH1)
 *     - Mode: Input with pull-up
 *  3. Cấu hình TIM3:
 *     - Prescaler: để cho khoảng cách pulse đủ lớn để không bị miss
 *     - Counting mode: Normal counting mode
 *     - Input capture channel CH1:
 *       * Capture on every rising edge (hoặc falling, tùy encoder)
 *       * DMA hoặc interrupt để capture pulse
 *  4. Bật TIM3 + interrupt
 * 
 * @return void
 * @note Phải gọi trước khi dùng Encoder_GetRPM()
 */
void Encoder_Init(void);

/**
 * @brief Đọc RPM từ encoder data
 * 
 * @return uint16_t RPM (rotations per minute)
 *         Range: 0 - 5000 RPM (hoặc tuỳ motor max speed)
 * 
 * @details Hàm này:
 *  1. Đọc pulse count từ ~100ms trước đến bây giờ
 *  2. Tính tần số: Hz = pulses / 0.1s
 *  3. Tính RPM: RPM = (Hz * 60) / ENCODER_PPR
 *  4. Trả về RPM value
 * 
 * @example
 *  uint16_t speed = Encoder_GetRPM();
 *  if (speed == 0) {
 *      // Motor không quay (stall detection)
 *  }
 *  if (speed > 3000) {
 *      // Motor chạy quá nhanh
 *  }
 */
uint16_t Encoder_GetRPM(void);

/**
 * @brief Trả về tổng số pulse từ lần init
 * 
 * @return uint32_t Total pulse count
 * 
 * @details Giá trị này tăng mỗi khi có pulse từ encoder
 *          Dùng cho diagnostic/debugging
 */
uint32_t Encoder_GetCount(void);

/**
 * @brief Reset pulse counter về 0
 * 
 * @return void
 * 
 * @details Dùng khi cần restart counting (ví dụ: sau reset motor)
 */
void Encoder_ResetCount(void);

/**
 * @brief Kiểm tra nếu encoder bị stuck (không xoay)
 * 
 * @return uint8_t  1 nếu stuck (không có pulse trong ENCODER_SAMPLE_RATE_MS)
 *                  0 nếu bình thường
 * 
 * @details Nguyên lý:
 *  1. Kiểm tra nếu pulse count không thay đổi trong ENCODER_SAMPLE_RATE_MS
 *  2. Nếu không thay đổi, motor stuck hoặc không có tải
 *  3. Dùng để detect stall condition
 * 
 * @example
 *  if (Encoder_IsStuck()) {
 *      // Motor stuck detected - stop PWM
 *      Motor_All_Stop();
 *  }
 */
uint8_t Encoder_IsStuck(void);

/**
 * @brief Trả về raw capture value (debug purpose)
 * 
 * @return uint16_t Raw capture counter value
 * 
 * @example
 *  uint16_t raw = Encoder_GetRaw();
 *  printf("Encoder raw: %d\n", raw);
 */
uint16_t Encoder_GetRaw(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_DRIVER_H */
