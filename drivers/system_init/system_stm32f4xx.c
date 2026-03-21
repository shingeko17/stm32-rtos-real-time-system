/**
 * @file system_stm32f4xx.c
 * @brief STM32F407VET6 - Hệ thống khởi tạo và quản lý clock chi tiết
 * @details Triển khai SystemInit() - khởi tạo clock 168MHz từ HSE
 * @author STM32 ARM Cortex-M4
 * @date 2026-03-14
 * 
 * ============================================================================
 * SƠ ĐỒ CLOCK:
 *   HSE (8MHz) → PLL → 336MHz → /2 → SYSCLK (168MHz)
 *                                  ├─ AHB /1 → 168MHz (GPIO, DMA, Flash)
 *                                  ├─ APB2 /2 → 84MHz (TIM1, SPI1, USART1)
 *                                  └─ APB1 /4 → 42MHz (TIM2-7, USART2-5, SPI2-3)
 * ============================================================================
 */

/* Khai báo wrapper cho stm32f4xx.h - register map ST */
#include "stm32f4xx.h"
/* Khai báo hàm và biến system */
#include "system_stm32f4xx.h"

/* ============================================================================
 * Khởi tạo biến toàn cục - định nghĩa extern ở .h, gán giá trị ở .c
 * ============================================================================ */

/* Biến lưu tần số core hiện tại (được cập nhật mỗi lần clock thay đổi) */
uint32_t SystemCoreClock = 168000000;

/* Biến lưu tần số AHB (Advanced High performance Bus) */
uint32_t AHBClock = 168000000;

/* Biến lưu tần số APB1 (Advanced Peripheral Bus 1 - chậu nhất) */
uint32_t APB1Clock = 42000000;

/* Biến lưu tần số APB2 (Advanced Peripheral Bus 2 - nhanh hơn) */
uint32_t APB2Clock = 84000000;

/* ============================================================================
 * HÀM KHỞI TẠO HỆ THỐNG
 * ============================================================================ */

/**
 * @brief Khởi tạo hệ thống STM32F407VET6 - gọi tự động bởi startup code
 * 
 * @details Quy trình:
 *  1. Bật HSI (16MHz internal oscillator) - sẵn có mặc định
 *  2. Bật HSE (8MHz external crystal)
 *  3. Chờ HSE sẵn sàng (timeout = 5000 vòng lặp)
 *  4. Cấu hình PLL:
 *     - Input:  HSE 8MHz / PLLM(8)  = 1MHz
 *     - Output: 1MHz * PLLN(336)   = 336MHz
 *     - SYSCLK: 336MHz / PLLP(2)   = 168MHz (Cortex-M4 max)
 *     - USB:    336MHz / PLLQ(7)   = 48MHz (USB FS)
 *  5. Cấu hình Flash latency = 5 cycles (cần cho tần số 168MHz)
 *  6. Cấu hình AHB/APB prescaler
 *  7. Chuyển SYSCLK sang PLL
 *  8. Cập nhật SystemCoreClock
 * 
 * @return void
 * @note Hàm này được gọi bởi Reset_Handler TRƯỚC khi gọi main()
 */
void SystemInit(void)
{
    /* ========================================================================
     * BƯỚC 1: Bật HSI (Internal 16MHz oscillator) - sẵn có
     * ======================================================================== */
    
    /* Đọc RCC->CR - RCC Control Register */
    uint32_t reg_value = RCC->CR;
    
    /* Bật HSI oscillator: set bit 0 (HSION) = 1 */
    reg_value |= (uint32_t)0x00000001;
    
    /* Ghi lại vào RCC->CR để kích hoạt HSI */
    RCC->CR = reg_value;
    
    /* ========================================================================
     * BƯỚC 2: Đợi HSI sẵn sàng (bit 1 - HSIRDY)
     * ======================================================================== */
    
    /* Vòng lặp chờ HSI ready flag */
    while ((RCC->CR & RCC_CR_HSIRDY) == 0)
    {
        /* Chờ đến khi HSIRDY = 1 (HSI đã ổn định) */
    }
    
    /* ========================================================================
     * BƯỚC 3: Cấu hình Flash (Memory Interface) cho tần số cao
     * ======================================================================== */
    
    /* Đọc thanh ghi ACR của Flash (Access Control Register) */
    reg_value = FLASH->ACR;
    
    /* Xóa các bit latency cũ (bits 2-0) */
    reg_value &= (uint32_t)0xFFFFFFF8;
    
    /* Đặt Flash latency = 5 cycles cho tần số 168MHz
       - 0 cycles : 0-30MHz
       - 1 cycle  : 30-60MHz
       - 2 cycles : 60-90MHz
       - 3 cycles : 90-120MHz
       - 4 cycles : 120-150MHz
       - 5 cycles : 150-180MHz (168MHz → cần 5 cycles)
    */
    reg_value |= (uint32_t)0x00000005;
    
    /* Bật prefetch buffer (bit 8) - tăng tốc độ fetch lệnh */
    reg_value |= (uint32_t)0x00000100;
    
    /* Bật instruction cache (bit 9) */
    reg_value |= (uint32_t)0x00000200;
    
    /* Bật data cache (bit 10) */
    reg_value |= (uint32_t)0x00000400;
    
    /* Ghi ACR configuration vào Flash */
    FLASH->ACR = reg_value;
    
    /* ========================================================================
     * BƯỚC 4: Cấu hình RCC - Reset and Clock Control
     * ======================================================================== */
    
    /* Đọc thanh ghi CFGR (Clock Configuration Register) */
    reg_value = RCC->CFGR;
    
    /* Xóa các bit cũ về prescaler (bits 4-0: AHB, bits 12-10: APB2, bits 14-12: APB1) */
    reg_value &= (uint32_t)0xFFF0F8AC;
    
    /* Cấu hình AHB prescaler = 1 (không chia, 168MHz)
       Bits 7-4: HPRE (AHB prescaler)
       0000 = /1, 0001 = /2, ..., 1111 = /512
    */
    reg_value |= (uint32_t)0x00000000;  /* AHB prescaler = /1 → 168MHz */
    
    /* Cấu hình APB2 prescaler = 2 (chia đôi để 84MHz)
       Bits 15-13: PPRE2 (APB2 prescaler)
       0xx = /1, 100 = /2, 101 = /4, 110 = /8, 111 = /16
    */
    reg_value |= (uint32_t)0x00008000;  /* APB2 prescaler = /2 → 84MHz */
    
    /* Cấu hình APB1 prescaler = 4 (chia 4 để 42MHz)
       Bits 12-10: PPRE1 (APB1 prescaler)
       0xx = /1, 100 = /2, 101 = /4, 110 = /8, 111 = /16
    */
    reg_value |= (uint32_t)0x00001400;  /* APB1 prescaler = /4 → 42MHz */
    
    /* Ghi CFGR configuration */
    RCC->CFGR = reg_value;
    
    /* ========================================================================
     * BƯỚC 5: Cấu hình PLL (Phase Locked Loop)
     * ======================================================================== */
    
    /* Đọc thanh ghi PLLCFGR (PLL Configuration Register) */
    reg_value = RCC->PLLCFGR;
    
    /* Xóa các bit PLL cũ để reset về 0 */
    reg_value &= (uint32_t)0xF0BC7FFF;
    
    /* Cấu hình PLL source = HSE (bit 22: PLLSRC)
       0 = HSI, 1 = HSE
    */
    reg_value |= (uint32_t)0x00400000;  /* PLLSRC = HSE */
    
    /* Cấu hình PLLM (input prescaler) = 8
       PLL input = HSE / PLLM = 8MHz / 8 = 1MHz
       Bits 5-0: PLLM divider (1..63)
    */
    reg_value |= (uint32_t)0x00000008;  /* PLLM = 8 */
    
    /* Cấu hình PLLN (multiplier) = 336
       PLL intermediate = 1MHz * 336 = 336MHz
       Bits 14-6: PLLN (50..432)
    */
    reg_value |= (uint32_t)0x0000A000;  /* PLLN = 336 (binary: 101010000) */
    
    /* Cấu hình PLLP (output divider) = 2 để lấy SYSCLK
       SYSCLK = 336MHz / 2 = 168MHz
       Bits 17-16: PLLP (00 = /2, 01 = /4, 10 = /6, 11 = /8)
    */
    reg_value |= (uint32_t)0x00000000;  /* PLLP = /2 → SYSCLK = 168MHz */
    
    /* Cấu hình PLLQ (USB clock divider) = 7
       USB clock = 336MHz / 7 = 48MHz (tiêu chuẩn USB FS)
       Bits 27-24: PLLQ (2..15)
    */
    reg_value |= (uint32_t)0x00700000;  /* PLLQ = 7 → USB = 48MHz */
    
    /* Ghi PLL configuration */
    RCC->PLLCFGR = reg_value;
    
    /* ========================================================================
     * BƯỚC 6: Bật PLL oscillator
     * ======================================================================== */
    
    /* Đọc RCC->CR */
    reg_value = RCC->CR;
    
    /* Bật PLL: set bit 24 (PLLON) = 1 */
    reg_value |= (uint32_t)0x01000000;
    
    /* Ghi RCC->CR để kích hoạt PLL */
    RCC->CR = reg_value;
    
    /* ========================================================================
     * BƯỚC 7: Chờ PLL sẵn sàng (bit 25 - PLLRDY)
     * ======================================================================== */
    
    /* Vòng lặp chờ PLL ready flag */
    while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
        /* Chờ đến khi PLLRDY = 1 (PLL đã ổn định) */
    }
    
    /* ========================================================================
     * BƯỚC 8: Chuyển SYSCLK từ HSI sang PLL
     * ======================================================================== */
    
    /* Đọc CFGR lần nữa để đặt clock source */
    reg_value = RCC->CFGR;
    
    /* Xóa bit clock source cũ (bits 1-0: SW) */
    reg_value &= (uint32_t)0xFFFFFFFC;
    
    /* Đặt SW = 10 (binary) = chọn PLL làm SYSCLK
       00 = HSI, 01 = HSE, 10 = PLL, 11 = dành riêng
    */
    reg_value |= (uint32_t)0x00000002;  /* SW = PLL */
    
    /* Ghi CFGR */
    RCC->CFGR = reg_value;
    
    /* ========================================================================
     * BƯỚC 9: Chờ SYSCLK thật sự chuyển sang PLL
     * ======================================================================== */
    
    /* Đọc SWS (System Clock Switch Status) - bits 3-2 */
    while ((RCC->CFGR & (uint32_t)0x0C) != (uint32_t)0x08)
    {
        /* Chờ SWS = 10 (PLL) có nghĩa là PLL đã là SYSCLK */
        /* Nếu SWS = 00 → HSI, 01 → HSE, 10 → PLL */
    }
    
    /* ========================================================================
     * BƯỚC 10: Cập nhật SystemCoreClock variable
     * ======================================================================== */
    
    /* Gặp lại hàm SystemCoreClockUpdate() để đọc RCC và cập nhật */
    SystemCoreClockUpdate();
}

/* ============================================================================
 * HÀM CẬP NHẬT CLOCK
 * ============================================================================ */

/**
 * @brief Cập nhật biến SystemCoreClock từ RCC registers
 * 
 * @details Hàm này đọc RCC->CFGR để xác định:
 *  - SYSCLK (từ bit 1-0 SWS)
 *  - AHB prescaler (từ bits 7-4 HPRE)
 *  - APB1 prescaler (từ bits 12-10 PPRE1)
 *  - APB2 prescaler (từ bits 15-13 PPRE2)
 *  Sau đó gán giá trị vào SystemCoreClock, AHBClock, APB1Clock, APB2Clock
 * 
 * @return void
 */
void SystemCoreClockUpdate(void)
{
    /* Khai báo biến cục bộ */
    uint32_t tmp = 0;                      /* Biến tạm để đọc CFGR */
    uint32_t pllm = 0;                     /* PLLM divider */
    uint32_t plln = 0;                     /* PLLN multiplier */
    uint32_t pllp = 0;                     /* PLLP divider */
    uint32_t pll_input = 0;                /* PLL input frequency */
    uint32_t pll_output = 0;               /* PLL output frequency */
    uint32_t ahb_prescaler = 1;            /* AHB prescaler (bắt đầu = 1) */
    uint32_t apb1_prescaler = 1;           /* APB1 prescaler */
    uint32_t apb2_prescaler = 1;           /* APB2 prescaler */
    
    /* Đọc CFGR để kiểm tra clock source hiện tại */
    tmp = RCC->CFGR;
    
    /* ========================================================================
     * Xác định SYSCLK từ SWS bits (3-2)
     * ======================================================================== */
    
    /* Lấy bit 3-2 SWS - xem hiện tại dùng clock nào */
    uint32_t sws = (tmp >> 2) & 0x03;      /* SWS = bits 3-2 */
    
    /* Biến lưu tần số SYSCLK (Hz) */
    uint32_t sysclk_freq = HSI_VALUE;      /* Mặc định = HSI 16MHz */
    
    /* Xét từng trường hợp SWS */
    if (sws == 0x00)  /* SWS = 00 → SYSCLK từ HSI */
    {
        sysclk_freq = HSI_VALUE;           /* SYSCLK = 16MHz (HSI) */
    }
    else if (sws == 0x01)  /* SWS = 01 → SYSCLK từ HSE */
    {
        sysclk_freq = HSE_VALUE;           /* SYSCLK = 8MHz (HSE) */
    }
    else if (sws == 0x02)  /* SWS = 10 → SYSCLK từ PLL */
    {
        /* Đọc PLLCFGR để lấy tham số PLL */
        uint32_t pllcfgr = RCC->PLLCFGR;
        
        /* Lấy PLLSRC (bit 22) để xác định input PLL là HSI hay HSE */
        if ((pllcfgr & 0x00400000) == 0)   /* PLLSRC = 0 → HSI input */
        {
            pll_input = HSI_VALUE;         /* PLL input = 16MHz (HSI) */
        }
        else  /* PLLSRC = 1 → HSE input */
        {
            pll_input = HSE_VALUE;         /* PLL input = 8MHz (HSE) */
        }
        
        /* Lấy PLLM (bits 5-0) - input predivider */
        pllm = pllcfgr & 0x3F;             /* PLLM = bits 5-0 */
        
        /* Tính input frequency vào PLL */
        pll_input = pll_input / pllm;      /* PLL input = 8MHz/8 = 1MHz */
        
        /* Lấy PLLN (bits 14-6) - multiplier */
        plln = (pllcfgr >> 6) & 0x1FF;     /* PLLN = bits 14-6 */
        
        /* Tính tần số PLL (post-multiplier) */
        pll_output = pll_input * plln;     /* PLL = 1MHz * 336 = 336MHz */
        
        /* Lấy PLLP (bits 17-16) - output divider */
        pllp = (pllcfgr >> 16) & 0x03;     /* PLLP = bits 17-16 */
        
        /* Chuyển PLLP giá trị register sang divisor thực tế */
        if (pllp == 0)
            pllp = 2;                      /* PLLP = 00 → /2 divisor */
        else if (pllp == 1)
            pllp = 4;                      /* PLLP = 01 → /4 divisor */
        else if (pllp == 2)
            pllp = 6;                      /* PLLP = 10 → /6 divisor */
        else
            pllp = 8;                      /* PLLP = 11 → /8 divisor */
        
        /* Tính SYSCLK từ PLL */
        sysclk_freq = pll_output / pllp;   /* SYSCLK = 336MHz/2 = 168MHz */
    }
    
    /* ========================================================================
     * Xác định AHB prescaler từ HPRE (bits 7-4)
     * ======================================================================== */
    
    /* Lấy HPRE bits (7-4) */
    uint32_t hpre = (tmp >> 4) & 0x0F;     /* HPRE = bits 7-4 */
    
    /* Bảng AHB prescaler: 0xxx = /1, 1000 = /2, 1001 = /4, ..., 1111 = /512 */
    if (hpre < 8)                          /* 0000-0111 */
        ahb_prescaler = 1;                 /* AHB = /1 (không chia) */
    else if (hpre == 8)
        ahb_prescaler = 2;                 /* 1000 = /2 */
    else if (hpre == 9)
        ahb_prescaler = 4;                 /* 1001 = /4 */
    else if (hpre == 10)
        ahb_prescaler = 8;                 /* 1010 = /8 */
    else if (hpre == 11)
        ahb_prescaler = 16;                /* 1011 = /16 */
    else if (hpre == 12)
        ahb_prescaler = 64;                /* 1100 = /64 */
    else if (hpre == 13)
        ahb_prescaler = 128;               /* 1101 = /128 */
    else if (hpre == 14)
        ahb_prescaler = 256;               /* 1110 = /256 */
    else
        ahb_prescaler = 512;               /* 1111 = /512 */
    
    /* ========================================================================
     * Xác định APB1 prescaler từ PPRE1 (bits 12-10)
     * ======================================================================== */
    
    /* Lấy PPRE1 bits (12-10) */
    uint32_t ppre1 = (tmp >> 10) & 0x07;   /* PPRE1 = bits 12-10 */
    
    /* Bảng APB1 prescaler: 0xx = /1, 100 = /2, 101 = /4, ..., 111 = /16 */
    if (ppre1 < 4)                         /* 000-011 */
        apb1_prescaler = 1;                /* APB1 = /1 */
    else if (ppre1 == 4)
        apb1_prescaler = 2;                /* 100 = /2 */
    else if (ppre1 == 5)
        apb1_prescaler = 4;                /* 101 = /4 */
    else if (ppre1 == 6)
        apb1_prescaler = 8;                /* 110 = /8 */
    else
        apb1_prescaler = 16;               /* 111 = /16 */
    
    /* ========================================================================
     * Xác định APB2 prescaler từ PPRE2 (bits 15-13)
     * ======================================================================== */
    
    /* Lấy PPRE2 bits (15-13) */
    uint32_t ppre2 = (tmp >> 13) & 0x07;   /* PPRE2 = bits 15-13 */
    
    /* Bảng APB2 prescaler: 0xx = /1, 100 = /2, 101 = /4, ..., 111 = /16 */
    if (ppre2 < 4)                         /* 000-011 */
        apb2_prescaler = 1;                /* APB2 = /1 */
    else if (ppre2 == 4)
        apb2_prescaler = 2;                /* 100 = /2 */
    else if (ppre2 == 5)
        apb2_prescaler = 4;                /* 101 = /4 */
    else if (ppre2 == 6)
        apb2_prescaler = 8;                /* 110 = /8 */
    else
        apb2_prescaler = 16;               /* 111 = /16 */
    
    /* ========================================================================
     * Tính toán và cập nhật các biến toàn cục
     * ======================================================================== */
    
    /* Gán SYSCLK vào SystemCoreClock */
    SystemCoreClock = sysclk_freq;
    
    /* Gán AHB clock = SYSCLK / AHB_prescaler */
    AHBClock = sysclk_freq / ahb_prescaler;
    
    /* Gán APB1 clock = AHB / APB1_prescaler */
    APB1Clock = (sysclk_freq / ahb_prescaler) / apb1_prescaler;
    
    /* Gán APB2 clock = AHB / APB2_prescaler */
    APB2Clock = (sysclk_freq / ahb_prescaler) / apb2_prescaler;
    
    /* 
     * Kết quả cuối cùng (với cấu hình mặc định):
     * SystemCoreClock = 168MHz
     * AHBClock = 168MHz
     * APB1Clock = 42MHz
     * APB2Clock = 84MHz
     */
}
