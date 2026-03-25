/**
 * @file system_stm32f4xx.h
 * @brief STM32F407VET6 - Hệ thống khởi tạo và quản lý clock
 * @details File header chứa khai báo hàm SystemInit() và biến toàn cục
 * @author STM32 ARM Cortex-M4
 * @date 2026-03-14
 * 
 * ============================================================================
 * MỤC ĐÍCH: Khởi tạo clock 168MHz từ HSE 8MHz qua PLL
 * ============================================================================
 */

#ifndef SYSTEM_STM32F4XX_H
#define SYSTEM_STM32F4XX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {  /* Nếu biên dịch bằng C++, sử dụng C linkage */
#endif

/* ============================================================================
 * KHAI BÁO HẠNG SỐ TOÀN CỤC
 * ============================================================================ */

/* Tần số HSE (External High Speed oscillator) từ crystal 8MHz */
#ifndef HSE_VALUE
#define HSE_VALUE    ((uint32_t)8000000)
#endif

/* Timeout cho HSE startup - số vòng lặp tối đa chờ HSE sẵn sàng */
#define HSE_STARTUP_TIMEOUT ((uint16_t)0x0500)

/* Tần số HSI (Internal High Speed oscillator) - 16MHz nếu không có HSE */
#ifndef HSI_VALUE
#define HSI_VALUE    ((uint32_t)16000000)
#endif

/* ============================================================================
 * KHAI BÁO BIẾN TOÀN CỤC
 * ============================================================================ */

/* Biến toàn cục lưu tần số clock hiện tại (Hz) - KHAI BÁO EXTERN */
extern uint32_t SystemCoreClock;           /* Tần số core = 168MHz */

/* Tần số AHB clock (Cortex-M4 System Bus, DMA, FPU) */
extern uint32_t AHBClock;                  /* AHB = 168MHz */

/* Tần số APB1 clock (Timer, USART2-5, SPI2-3, I2C) */
extern uint32_t APB1Clock;                 /* APB1 = 42MHz (prescaler /4) */

/* Tần số APB2 clock (Timer1, USART1/6, SPI1, ADC) */
extern uint32_t APB2Clock;                 /* APB2 = 84MHz (prescaler /2) */

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo hệ thống STM32F407VET6
 * @details Hàm này được gọi tự động bởi Reset_Handler trước main()
 *          - Bật clock HSE (8MHz external crystal)
 *          - Cấu hình PLL: (8MHz / 8) * 336 = 336MHz → /2 = 168MHz
 *          - Cấu hình AHB prescaler = 1 (168MHz)
 *          - Cấu hình APB1 prescaler = 4 (42MHz)
 *          - Cấu hình APB2 prescaler = 2 (84MHz)
 *          - Cấu hình Flash latency = 5 cycles
 *          - Cập nhật SystemCoreClock variable
 * @return void
 */
extern void SystemInit(void);

/**
 * @brief Cập nhật biến SystemCoreClock
 * @details Hàm này đọc RCC register và cập nhật biến toàn cục
 *          SystemCoreClock, AHBClock, APB1Clock, APB2Clock
 * @return void
 */
extern void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}  /* Kết thúc C linkage */
#endif

#endif /* SYSTEM_STM32F4XX_H */
