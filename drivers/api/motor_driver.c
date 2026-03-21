/**
 * @file motor_driver.c
 * @brief Motor Driver - Implementation của cao cấp hơn BSP
 * @details Triển khai các hàm điều khiển motor với API tốc độ phần trăm
 * @author STM32 Motor Control Team
 * @date 2026-03-14
 */

/* Khai báo extern từ motor_driver.h */
#include "motor_driver.h"

/* ============================================================================
 * HÀM HỖ TRỢ: Chuyển tốc độ phần trăm thành duty cycle PWM
 * ============================================================================ */

/**
 * @brief Chuyển tốc độ phần trăm (0..100%) sang giá trị PWM duty (0..999)
 * 
 * @param[in] speed  Tốc độ từ 0 đến 100 (% công suất)
 * 
 * @return uint16_t  Giá trị duty cycle (0..999)
 * 
 * @details Hàm này:
 *  1. Kiểm tra input speed (nếu > 100 thì cắt xuống 100)
 *  2. Tính duty = (speed * MOTOR_PWM_PERIOD) / 100
 *     Ví dụ:
 *       speed = 0   → duty = 0 × 999 / 100 = 0 (0%)
 *       speed = 50  → duty = 50 × 999 / 100 = 499 (50%)
 *       speed = 100 → duty = 100 × 999 / 100 = 999 (100%)
 *  3. Trả về duty value để feed vào TIM_SetCompare()
 * 
 * @note Công thức: duty = (speed * MOTOR_PWM_PERIOD) / 100
 *       MOTOR_PWM_PERIOD = 999, nên công thức là:
 *       duty = (speed * 999) / 100
 */
static uint16_t speed_to_duty(int16_t speed)
{
    /* Kiểm tra nếu speed vượt quá 100 */
    if (speed < 0)
    {
        /* Nếu speed âm, lấy giá trị tuyệt đối (bỏ dấu) */
        speed = -speed;
    }
    
    /* Giới hạn speed ≤ 100 */
    if (speed > 100)
    {
        /* Nếu speed > 100, cắt xuống 100 để tránh overflow */
        speed = 100;
    }
    
    /* Chuyển đổi từ % (0..100) sang duty value (0..999)
       Công thức: duty = (speed × MOTOR_PWM_PERIOD) / 100
       = (speed × 999) / 100
    */
    return (uint16_t)((speed * MOTOR_PWM_PERIOD) / 100);
}

/* ============================================================================
 * HÀM ĐIỀU KHIỂN MOTOR A
 * ============================================================================ */

/**
 * @brief Chạy motor A với tốc độ phần trăm (-100 đến +100)
 * 
 * @param[in] speed  Tốc độ (-100..+100)
 *                   Âm = lùi, Dương = tiến, 0 = dừng
 * 
 * @return void
 */
void Motor_A_Run(int16_t speed)
{
    /* ========================================================================
     * TRƯỜNG HỢP 1: speed = 0 → DỪNG
     * ======================================================================== */
    
    if (speed == 0)
    {
        /* Gọi hàm BSP để đặt hướng = STOP (IN1=0, IN2=0) */
        BSP_Motor_A_SetDirection(MOTOR_STOP);
        
        /* Đặt PWM speed = 0 (0% công suất) */
        BSP_Motor_A_SetSpeed(0);
        
        /* Tại điểm này, motor A sẽ coasting (tắt PWM) */
        return;  /* Thoát hàm */
    }
    
    /* ========================================================================
     * TRƯỜNG HỢP 2: speed > 0 → TIẾN (FORWARD)
     * ======================================================================== */
    
    if (speed > 0)
    {
        /* Gọi hàm BSP để đặt hướng = FORWARD (IN1=1, IN2=0) */
        BSP_Motor_A_SetDirection(MOTOR_FORWARD);
        
        /* Chuyển tốc độ phần trăm thành duty cycle PWM
           Ví dụ: speed=75 → duty=749 (75×999/100)
        */
        uint16_t duty = speed_to_duty(speed);
        
        /* Gọi hàm BSP để đặt PWM speed */
        BSP_Motor_A_SetSpeed(duty);
        
        /* Tại điểm này, motor A sẽ quay tiến với tốc độ speed% */
        return;
    }
    
    /* ========================================================================
     * TRƯỜNG HỢP 3: speed < 0 → LÙI (BACKWARD)
     * ======================================================================== */
    
    /* Nếu code chạy tới đây, nghĩa là speed < 0 */
    
    /* Gọi hàm BSP để đặt hướng = BACKWARD (IN1=0, IN2=1) */
    BSP_Motor_A_SetDirection(MOTOR_BACKWARD);
    
    /* Chuyển tốc độ phần trăm thành duty cycle PWM
       Ví dụ: speed=-75 → |-75|=75 → duty=749 (75×999/100)
    */
    uint16_t duty = speed_to_duty(speed);  /* Hàm tự xử lý dấu âm */
    
    /* Gọi hàm BSP để đặt PWM speed */
    BSP_Motor_A_SetSpeed(duty);
    
    /* Tại điểm này, motor A sẽ quay lùi với tốc độ |speed|% */
}

/* ============================================================================
 * HÀM ĐIỀU KHIỂN MOTOR B
 * ============================================================================ */

/**
 * @brief Chạy motor B với tốc độ phần trăm (-100 đến +100)
 * 
 * @param[in] speed  Tốc độ (-100..+100)
 * 
 * @return void
 */
void Motor_B_Run(int16_t speed)
{
    /* ========================================================================
     * TRƯỜNG HỢP 1: speed = 0 → DỪNG
     * ======================================================================== */
    
    if (speed == 0)
    {
        /* Dừng motor B */
        BSP_Motor_B_SetDirection(MOTOR_STOP);
        BSP_Motor_B_SetSpeed(0);
        return;
    }
    
    /* ========================================================================
     * TRƯỜNG HỢP 2: speed > 0 → TIẾN (FORWARD)
     * ======================================================================== */
    
    if (speed > 0)
    {
        /* Motor B tiến */
        BSP_Motor_B_SetDirection(MOTOR_FORWARD);
        uint16_t duty = speed_to_duty(speed);
        BSP_Motor_B_SetSpeed(duty);
        return;
    }
    
    /* ========================================================================
     * TRƯỜNG HỢP 3: speed < 0 → LÙI (BACKWARD)
     * ======================================================================== */
    
    /* Motor B lùi */
    BSP_Motor_B_SetDirection(MOTOR_BACKWARD);
    uint16_t duty = speed_to_duty(speed);
    BSP_Motor_B_SetSpeed(duty);
}

/* ============================================================================
 * HÀM DỪNG TẤT CẢ MOTOR (COASTING)
 * ============================================================================ */

/**
 * @brief Dừng tất cả motor
 * 
 * @return void
 */
void Motor_All_Stop(void)
{
    /* Gọi Motor_A_Run(0) để dừng motor A */
    Motor_A_Run(0);
    
    /* Gọi Motor_B_Run(0) để dừng motor B */
    Motor_B_Run(0);
    
    /* Tại điểm này, cả hai motor được đặt ở chế độ STOP (coasting) */
}

/* ============================================================================
 * HÀM HÃM TẤT CẢ MOTOR (DYNAMIC BRAKING)
 * ============================================================================ */

/**
 * @brief Hãm tất cả motor (dừng ngay lập tức)
 * 
 * @return void
 */
void Motor_All_Brake(void)
{
    /* Gọi hàm BSP để đặt motor A ở chế độ BRAKE (IN1=1, IN2=1) */
    BSP_Motor_A_SetDirection(MOTOR_BRAKE);
    
    /* Đặt speed = 0 cho motor A (PWM = 0%) */
    BSP_Motor_A_SetSpeed(0);
    
    /* Gọi hàm BSP để đặt motor B ở chế độ BRAKE */
    BSP_Motor_B_SetDirection(MOTOR_BRAKE);
    
    /* Đặt speed = 0 cho motor B */
    BSP_Motor_B_SetSpeed(0);
    
    /* Tại điểm này, cả hai motor ở chế độ BRAKE (hãm điện) */
}
