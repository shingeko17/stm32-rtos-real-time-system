/**
 * @file bsp_motor.h
 * @brief Board Support Package - L298N Motor Driver Configuration
 * @details Header file khai báo hàm khởi tạo và điều khiển làm động cơ DC
 *          qua L298N H-bridge sử dụng STM32F407VET6 GPIO + PWM
 * @author STM32 L298N Driver Team
 * @date 2026-03-14
 * 
 * ============================================================================
 * SƠ ĐỒ KẾT NỐI L298N ↔ STM32F407VET6:
 * 
 *  L298N Pin  │  STM32 Pin      │  Chức năng
 *  ───────────┼─────────────────┼──────────────────────────
 *  ENA        │  PE13 (TIM1_CH3)│  PWM điều khiển tốc độ motor A
 *  IN1        │  PD0 (GPIO out) │  Hướng motor A bit 0
 *  IN2        │  PD1 (GPIO out) │  Hướng motor A bit 1
 *  ENB        │  PE14 (TIM1_CH4)│  PWM điều khiển tốc độ motor B
 *  IN3        │  PD2 (GPIO out) │  Hướng motor B bit 0
 *  IN4        │  PD3 (GPIO out) │  Hướng motor B bit 1
 *  GND        │  GND            │  Chung đất
 *  +12V/+5V   │  External PSU   │  Cấp nguồn cho motor
 * 
 * ============================================================================
 * CHẾ ĐỘ ĐIỀU KHIỂN:
 * 
 *  IN1 │ IN2 │ Chế độ          │ Mô tả
 *  ────┼─────┼─────────────────┼──────────────────────────
 *   0  │  0  │ MOTOR_STOP      │ Động cơ dừng (coasting)
 *   1  │  0  │ MOTOR_FORWARD   │ Động cơ tiến (chiều dương)
 *   0  │  1  │ MOTOR_BACKWARD  │ Động cơ lùi (chiều âm)
 *   1  │  1  │ MOTOR_BRAKE     │ Động cơ hãm (điện trở)
 * 
 * ============================================================================
 */

#ifndef BSP_MOTOR_H
#define BSP_MOTOR_H

/* ============================================================================
 * KHAI BÁO CÁC FILE HEADER CẦN THIẾT
 * ============================================================================ */

/* Khai báo ngoài từ CMSIS - STM32F4xx device register */
#include "stm32f4xx.h"

/* Khai báo hàm STM32 StdPeriph */
#include "misc_drivers.h"

/* ============================================================================
 * MỤC BẢO VỆ C++ LINKAGE
 * ============================================================================ */

#ifdef __cplusplus
extern "C" {  /* Nếu compile bằng C++, dùng C linkage */
#endif

/* ============================================================================
 * ĐỊNH NGHĨA PIN MAPPING - TÙY CHỈNH THEO BOARD CỤ THỂ
 * ============================================================================ */

/* === Motor A - IN1 Pin (PD0) === */
#define MOTOR_A_IN1_PORT             GPIOD    /* GPIO Port D */
#define MOTOR_A_IN1_PIN              GPIO_Pin_0  /* Pin 0 */

/* === Motor A - IN2 Pin (PD1) === */
#define MOTOR_A_IN2_PORT             GPIOD    /* GPIO Port D */
#define MOTOR_A_IN2_PIN              GPIO_Pin_1  /* Pin 1 */

/* === Motor B - IN3 Pin (PD2) === */
#define MOTOR_B_IN3_PORT             GPIOD    /* GPIO Port D */
#define MOTOR_B_IN3_PIN              GPIO_Pin_2  /* Pin 2 */

/* === Motor B - IN4 Pin (PD3) === */
#define MOTOR_B_IN4_PORT             GPIOD    /* GPIO Port D */
#define MOTOR_B_IN4_PIN              GPIO_Pin_3  /* Pin 3 */

/* === PWM Timer Configuration === */
#define MOTOR_PWM_TIM                TIM1     /* Timer 1 (APB2) */
#define MOTOR_A_CHANNEL              3        /* TIM1_CH3 → PE13 */
#define MOTOR_B_CHANNEL              4        /* TIM1_CH4 → PE14 */

/* === PWM Configuration === */
#define MOTOR_PWM_PERIOD             999      /* Period count (0..999) */
#define MOTOR_PWM_FREQUENCY          1000     /* 1kHz PWM frequency */

/* Prescaler để tạo 1kHz: 168MHz / (168*1000) = 1kHz */
#define MOTOR_PWM_PRESCALER          (168-1)  /* Prescaler value */

/* ============================================================================
 * ĐỊNH NGHĨA KIỂU DỮ LIỆU - LIỆT KÊ CHẾ ĐỘ MOTOR
 * ============================================================================ */

/**
 * @enum MotorDirection_t
 * @brief Liệt kê các chế độ hoạt động của động cơ
 */
typedef enum 
{
    MOTOR_STOP      = 0,    /* Dừng - IN1=0, IN2=0 - động cơ coasting */
    MOTOR_FORWARD   = 1,    /* Tiến - IN1=1, IN2=0 - động cơ quay chiều dương */
    MOTOR_BACKWARD  = 2,    /* Lùi - IN1=0, IN2=1 - động cơ quay chiều âm */
    MOTOR_BRAKE     = 3     /* Hãm - IN1=1, IN2=1 - động cơ hãm điện */
} MotorDirection_t;

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo toàn bộ hệ thống motor (GPIO + PWM)
 * 
 * @details Hàm này thực hiện:
 *  1. Bật clock cho GPIO Port D (IN pins) và Port E (PWM pins)
 *  2. Khởi tạo GPIO PD0-PD3 làm output push-pull (direction)
 *  3. Bật clock cho TIM1
 *  4. Cấu hình GPIO PE13, PE14 làm alternate function TIM1
 *  5. Cấu hình TIM1 PWM 1kHz mode
 *  6. Bật TIM1 PWM output
 *  7. Reset tất cả motor về chế độ STOP
 * 
 * @return void
 * @note Hàm này phải được gọi TRƯỚC bất kỳ hàm điều khiển motor khác
 */
extern void BSP_Motor_Init(void);

/**
 * @brief Thiết lập hướng quay motor A (IN1, IN2 pins)
 * 
 * @param[in] dir  Chế độ motor (MOTOR_STOP, FORWARD, BACKWARD, BRAKE)
 * 
 * @details Hàm điều khiển IN1 (PD0) và IN2 (PD1):
 *  - STOP:     IN1=0, IN2=0 → động cơ dừng (coasting)
 *  - FORWARD:  IN1=1, IN2=0 → quay chiều dương
 *  - BACKWARD: IN1=0, IN2=1 → quay chiều âm
 *  - BRAKE:    IN1=1, IN2=1 → hãm điện
 * 
 * @return void
 * @note Tốc độ được điều khiển riêng bằng PRE signal thông qua PWM
 */
extern void BSP_Motor_A_SetDirection(MotorDirection_t dir);

/**
 * @brief Thiết lập hướng quay motor B (IN3, IN4 pins)
 * 
 * @param[in] dir  Chế độ motor (MOTOR_STOP, FORWARD, BACKWARD, BRAKE)
 * 
 * @details Hàm điều khiển IN3 (PD2) và IN4 (PD3):
 *  - Hành động tương tự như BSP_Motor_A_SetDirection()
 * 
 * @return void
 */
extern void BSP_Motor_B_SetDirection(MotorDirection_t dir);

/**
 * @brief Thiết lập tốc độ (PWM duty cycle) motor A
 * 
 * @param[in] duty  Giá trị duty cycle (0..MOTOR_PWM_PERIOD)
 *                  0 = 0% (dừng), 999 = 100% (tối đa)
 * 
 * @details Hàm này điều chỉnh pulse width của TIM1_CH3:
 *  - Kích hoạt ENA pin PE13 qua PWM
 *  - PWM frequency = 1kHz (không đổi)
 *  - Duty = (duty / MOTOR_PWM_PERIOD) * 100%
 * 
 * @return void
 * @note Hướng quay được thiết lập riêng bằng hàm BSP_Motor_A_SetDirection()
 */
extern void BSP_Motor_A_SetSpeed(uint16_t duty);

/**
 * @brief Thiết lập tốc độ (PWM duty cycle) motor B
 * 
 * @param[in] duty  Giá trị duty cycle (0..MOTOR_PWM_PERIOD)
 * 
 * @details Tương tự BSP_Motor_A_SetSpeed, nhưng điều khiển TIM1_CH4 (PE14)
 * 
 * @return void
 */
extern void BSP_Motor_B_SetSpeed(uint16_t duty);

/* ============================================================================
 * MỤC BẢO VỀ C++ LINKAGE - KẾT THÚC
 * ============================================================================ */

#ifdef __cplusplus
}  /* Kết thúc C linkage */
#endif

#endif  /* BSP_MOTOR_H */
