/**
 * @file motor_driver.h
 * @brief Motor Driver - Higher Level Abstraction Layer (trên BSP)
 * @details Header file cung cấp API đơn giản để điều khiển motor
 *          với tốc độ từ -100 đến +100 (% power)
 * @author STM32 Motor Control Team
 * @date 2026-03-14
 * 
 * ============================================================================
 * MỤC ĐÍCH: Cung cấp giao diện tốc độ trực quan (-100..+100)
 * thay vì phải tính duty cycle từ 0..999
 * 
 * Ví dụ sử dụng:
 *   Motor_A_Run(75);     // Motor A tiến 75% tốc độ tối đa
 *   Motor_B_Run(-50);    // Motor B lùi 50% tốc độ tối đa
 *   Motor_All_Stop();    // Dừng tất cả motor
 *   Motor_All_Brake();   // Hãm tất cả motor
 * 
 * ============================================================================
 */

#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

/* Khai báo extern từ bsp_motor.h - sử dụng hàm BSP */
#include "bsp_motor.h"

/* ============================================================================
 * MỤC BẢO VỀ C++ LINKAGE
 * ============================================================================ */

#ifdef __cplusplus
extern "C" {  /* Nếu compile bằng C++, sử dụng C linkage */
#endif

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Chạy motor A với tốc độ được chỉ định (dạng phần trăm)
 * 
 * @param[in] speed  Tốc độ từ -100 đến +100 (% công suất)
 *                   - Âm (-100 đến -1): quay lùi (BACKWARD), tốc độ tăng theo |speed|
 *                   - 0: dừng (STOP)
 *                   - Dương (+1 đến +100): quay tiến (FORWARD), tốc độ = speed
 *   Ví dụ:
 *     Motor_A_Run(0);    // Dừng (0%)
 *     Motor_A_Run(50);   // Tiến 50%
 *     Motor_A_Run(100);  // Tiến 100% (tối đa)
 *     Motor_A_Run(-50);  // Lùi 50%
 *     Motor_A_Run(-100); // Lùi 100% (tối đa)
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Kiểm tra dấu của speed
 *  2. Gọi BSP_Motor_A_SetDirection() để đặt hướng quay
 *  3. Tính duty cycle từ |speed| (0..999)
 *  4. Gọi BSP_Motor_A_SetSpeed() để thiết lập PWM
 * 
 * @note Hàm tự động handle -100 nếu input > 100 hoặc < -100
 */
extern void Motor_A_Run(int16_t speed);

/**
 * @brief Chạy motor B với tốc độ được chỉ định (dạng phần trăm)
 * 
 * @param[in] speed  Tốc độ từ -100 đến +100 (% công suất)
 * 
 * @return void
 * 
 * @details Tương tự Motor_A_Run(), nhưng áp dụng cho motor B
 */
extern void Motor_B_Run(int16_t speed);

/**
 * @brief Dừng tất cả motor (coasting - không có sức hút)
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Gọi Motor_A_Run(0) - dừng motor A
 *  2. Gọi Motor_B_Run(0) - dừng motor B
 *  3. Kết quả: cả hai motor ở chế độ STOP (IN1=0, IN2=0)
 * 
 * @note Động cơ sẽ coasting (trượt) nếu còn quán tính
 *       Để hãm ngay lập tức, dùng Motor_All_Brake()
 */
extern void Motor_All_Stop(void);

/**
 * @brief Hãm tất cả motor (điện trở - dừng ngay lập tức)
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Gọi BSP_Motor_A_SetDirection(MOTOR_BRAKE) - hãm motor A
 *  2. Gọi BSP_Motor_B_SetDirection(MOTOR_BRAKE) - hãm motor B
 *  3. Đặt speed = 0 cho cả hai
 *  4. Kết quả: cả hai motor ở chế độ BRAKE (IN1=1, IN2=1)
 * 
 * @note Chế độ hãm sử dụng short-circuit để dừng ngay
 *       Tiêu hao năng lượng hơn STOP nhưng dừng nhanh hơn
 * 
 * @see Motor_All_Stop() - Để dừng mềm mại (coasting)
 */
extern void Motor_All_Brake(void);

/* ============================================================================
 * MỤC BẢO VỀ C++ LINKAGE - KẾT THÚC
 * ============================================================================ */

#ifdef __cplusplus
}  /* Kết thúc C linkage */
#endif

#endif  /* MOTOR_DRIVER_H */
