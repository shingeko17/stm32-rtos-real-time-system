/**
 * @file main.c
 * @brief STM32F407VET6 + L298N Motor Control - Main Application
 * @details Chương trình chính điều khiển 2 động cơ DC qua L298N H-bridge
 *          Sử dụng tầng HAL (CMSIS → BSP → Driver)
 * @author STM32 Motor Control Team
 * @date 2026-03-14
 * 
 * ============================================================================
 * KIẾN TRÚC PHẦN MỀM:
 * 
 *  main.c
 *    ↓
 *  motor_driver.c (API tốc độ -100..+100)
 *    ↓
 *  bsp_motor.c (GPIO + PWM control)
 *    ↓
 *  system_stm32f4xx.c (Clock 168MHz)
 *    ↓
 *  STM32F407VET6 Hardware
 * 
 * ============================================================================
 * CHƯƠNG TRÌNH MIMIC:
 *  1. Khởi tạo clock 168MHz
 *  2. Khởi tạo GPIO + PWM cho motor
 *  3. Chạy motor A tiến 75%
 *  4. Chạy motor B tiến 75%
 *  5. Delay 2 giây
 *  6. Chạy motor A lùi 50%
 *  7. Chạy motor B lùi 50%
 *  8. Delay 2 giây
 *  9. Lặp lại vô hạn
 * 
 * ============================================================================
 */

/* ============================================================================
 * INCLUDE HÃY CÁC FILE HEADER CẦN THIẾT
 * ============================================================================ */

/* Khai báo header STM32F4xx register map + config */
#include "stm32f4xx.h"

/* Khai báo system init function (clock 168MHz) */
#include "system_stm32f4xx.h"

/* Khai báo BSP motor init + GPIO/PWM low-level functions */
#include "bsp_motor.h"

/* Khai báo Motor driver API (high-level -100..+100) */
#include "motor_driver.h"

/* ============================================================================
 * BIẾN TOÀN CỤC
 * ============================================================================ */

/* Biến đếm millisecond để implement delay
   - Được cập nhật mỗi 1ms bởi SysTick interrupt
   - Dùng để implement Delay(ms) function
*/
volatile uint32_t g_delay_ms = 0;

/* ============================================================================
 * HÀM SUPPORT - SysTick Interrupt Handler
 * ============================================================================ */

/**
 * @brief SysTick Interrupt Handler - gọi mỗi 1ms
 * 
 * @details Hàm này được gọi tự động mỗi 1 millisecond
 *          Dùng để:
 *          1. Giảm biến g_delay_ms (nếu > 0)
 *          2. Cập nhật timestamp hệ thống
 *          3. Chạy task định kỳ 1ms
 * 
 * @return void
 * @note Tên hàm phải là "SysTick_Handler" (định sẵn)
 */
void SysTick_Handler(void)
{
    /* Kiểm tra nếu g_delay_ms > 0 */
    if (g_delay_ms > 0)
    {
        /* Giảm g_delay_ms xuống 1ms */
        g_delay_ms--;
    }
}

/* ============================================================================
 * HÀM SUPPORT - Configure SysTick Timer
 * ============================================================================ */

/**
 * @brief Cấu hình SysTick timer để interrupt mỗi 1ms
 * 
 * @details Hàm này:
 *  1. Gọi SysTick_Config() với SystemCoreClock / 1000
 *     Điều này sẽ cấu hình SysTick để tick mỗi 1 millisecond
 *  2. Bật SysTick interrupt
 *  3. Sau hàm này, SysTick_Handler() sẽ được gọi mỗi 1ms
 * 
 * @return void
 * @note Phải gọi hàm này TRƯỚC khi gọi Delay()
 */
static void SysTick_Configuration(void)
{
    /* Cấu hình SysTick timer
       Tham số: SystemCoreClock / 1000 = 168,000,000 / 1000 = 168,000
       Nghĩa là SysTick sẽ đếm từ 0 đến 168,000 rồi reset
       Tần số tick = 168MHz / 168,000 = 1kHz = 1 tick/ms
       Mỗi khi counter đạt 168,000 sẽ gọi SysTick_Handler()
    */
    SysTick_Config(SystemCoreClock / 1000);
}

/* ============================================================================
 * HÀM SUPPORT - Delay Function
 * ============================================================================ */

/**
 * @brief Delay (halt) trong số milliseconds nhất định
 * 
 * @param[in] ms  Số milliseconds cần chờ
 * 
 * @details Hàm này:
 *  1. Gán g_delay_ms = ms
 *  2. Chờ trong vòng lặp while(g_delay_ms != 0)
 *  3. Mỗi 1ms, SysTick_Handler() sẽ giảm g_delay_ms đi 1
 *  4. Khi g_delay_ms = 0, vòng lặp thoát
 * 
 * @example
 *  Delay(1000);  // Chờ 1 giây
 *  Delay(500);   // Chờ 500 milliseconds
 * 
 * @return void
 * @note Hàm này BLOCKING - CPU bị chiếm trong lúc delay
 */
static void Delay(uint32_t ms)
{
    /* Gán giá trị delay cần chờ vào biến toàn cục */
    g_delay_ms = ms;
    
    /* Vòng lặp chờ đến khi g_delay_ms = 0
       - Mỗi 1ms, SysTick_Handler() giảm g_delay_ms đi 1
       - Khi g_delay_ms = 0, thoát vòng lặp
    */
    while (g_delay_ms != 0)
    {
        /* Chờ... */
    }
}

/* ============================================================================
 * HÀM CHÍNH - MAIN FUNCTION
 * ============================================================================ */

/**
 * @brief Hàm chính - điểm vào chương trình
 * 
 * @details Chường trình chính thực hiện:
 *  1. Khởi tạo clock 168MHz (SystemInit được gọi tự động trước main)
 *  2. Cấu hình SysTick untuk delay
 *  3. Khởi tạo GPIO + PWM cho motor
 *  4. Vòng lặp vô hạn: chạy motor theo mô hình định sẵn
 * 
 * @return int  (Không sử dụng - chương trình embedded không trả về)
 * 
 * @note Hàm main() được gọi bởi reset handler sau khi SystemInit()
 */
int main(void)
{
    /* ========================================================================
     * BƯỚC 1: KHỞI TẠO SYSTICK TIMER
     * ======================================================================== */
    
    /* Cấu hình SysTick để interrupt mỗi 1ms
       - Sau hàm này, SysTick_Handler() sẽ được gọi mỗi 1ms
       - Cho phép Delay() function hoạt động
    */
    SysTick_Configuration();
    
    /* ========================================================================
     * BƯỚC 2: KHỞI TẠO MOTOR SYSTEM
     * ======================================================================== */
    
    /* Gọi hàm BSP_Motor_Init() để khởi tạo:
       - Clock cho GPIO Port D (IN1-IN4) và Port E (PWM)
       - GPIO PD0-PD3 làm output (direction control)
       - GPIO PE13-PE14 làm TIM1 alternate function (PWM)
       - Timer1 PWM 1kHz mode
       - Reset motor = STOP
    */
    BSP_Motor_Init();
    
    /* Tại điểm này, motor system sẵn sàng để chạy */
    
    /* ========================================================================
     * BƯỚC 3: VÒNG LẶP CHÍNH - CHẠY MOTOR THEO MẪU
     * ======================================================================== */
    
    /* Vòng lặp vô hạn - chương trình điều nhận không bao giờ kết thúc */
    while (1)
    {
        /* ====================================================================
         * ANIMATION 1: Cả 2 motor tiến 75%
         * ====================================================================
         */
        
        /* Motor A tiến 75% tốc độ tối đa
           - Hướng: FORWARD (IN1=1, IN2=0)
           - Tốc độ: 75% (duty=749 của 999)
        */
        Motor_A_Run(75);
        
        /* Motor B tiến 75% tốc độ tối đa
           - Tương tự Motor A
        */
        Motor_B_Run(75);
        
        /* Delay 2 giây (2000 milliseconds) để xem cảnh này
           - CPU chờ trong SysTick interrupt
           - Mỗi 1ms, g_delay_ms giảm 1
           - Khi g_delay_ms = 0, Delay() trả về
        */
        Delay(2000);
        
        /* ====================================================================
         * ANIMATION 2: Cả 2 motor lùi 50%
         * ====================================================================
         */
        
        /* Motor A lùi 50% tốc độ tối đa
           - Hướng: BACKWARD (IN1=0, IN2=1)
           - Tốc độ: 50% (duty=499 của 999)
        */
        Motor_A_Run(-50);
        
        /* Motor B lùi 50% tốc độ tối đa */
        Motor_B_Run(-50);
        
        /* Delay 2 giây */
        Delay(2000);
        
        /* ====================================================================
         * ANIMATION 3: Motor A tiến, Motor B lùi (Robot quay)
         * ====================================================================
         */
        
        /* Motor A tiến 100% (tối đa) */
        Motor_A_Run(100);
        
        /* Motor B lùi 100% (tối đa) */
        Motor_B_Run(-100);
        
        /* Delay 1 giây */
        Delay(1000);
        
        /* ====================================================================
         * ANIMATION 4: Dừng tất cả (coasting)
         * ====================================================================
         */
        
        /* Dừng cả 2 motor - coasting mode
           - IN1=0, IN2=0 cho cả hai
           - Motor sẽ coasting (trượt) nếu còn quán tính
        */
        Motor_All_Stop();
        
        /* Delay 1 giây */
        Delay(1000);
        
        /* ====================================================================
         * ANIMATION 5: Hãm tất cả (dynamic braking)
         * ====================================================================
         */
        
        /* Hãm cả 2 motor - braking mode
           - IN1=1, IN2=1 cho cả hai
           - Motor sẽ hãm (dừng ngay) bằng short-circuit
        */
        Motor_All_Brake();
        
        /* Delay 1 giây */
        Delay(1000);
        
        /* Sau delay 1s, while(1) tiếp tục lặp từ ANIMATION 1 */
    }
    
    /* Không bao giờ để chạy tới đây */
    return 0;
}

/* ============================================================================
 * HIM MYNH - RESET HANDLER + STARTUP CODE
 * ============================================================================
 * 
 * Khi STM32 reset:
 * 1. Reset_Handler (trong startup_stm32f4xx.s) được gọi
 * 2. Reset_Handler gọi SystemInit() (cấu hình clock 168MHz)
 * 3. Reset_Handler gọi main() (chương trình chính)
 * 4. main() thực hiện 2 khối khởi tạo, rồi chạy while(1)
 * 
 * Chi tiết hơn:
 * - SystemInit() được gọi BỘ MNH TRƯỚC main() (auto)
 * - Khi main() kết thúc (không xảy ra ở đây), Reset_Handler có thể loop
 * 
 * ============================================================================
 */
