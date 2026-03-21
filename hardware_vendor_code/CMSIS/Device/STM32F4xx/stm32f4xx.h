/**
 * @file stm32f4xx.h
 * @brief STM32F407 Device Header - Minimal but Complete
 * @date 2026-03-14
 */

#ifndef STM32F4XX_H
#define STM32F4XX_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * INTERRUPT ENUM (MUST BE BEFORE core_cm4.h)
 * ============================================================================ */

typedef enum {
    NonMaskableInt_IRQn         = -14,
    MemoryManagement_IRQn       = -12,
    BusFault_IRQn               = -11,
    UsageFault_IRQn             = -10,
    SVCall_IRQn                 = -5,
    DebugMonitor_IRQn           = -4,
    PendSV_IRQn                 = -2,
    SysTick_IRQn                = -1,
    WWDG_IRQn                   = 0,
    PVD_IRQn                    = 1,
    TAMP_STAMP_IRQn             = 2,
    RTC_WKUP_IRQn               = 3,
    FLASH_IRQn                  = 4,
    RCC_IRQn                    = 5,
    EXTI0_IRQn                  = 6,
    EXTI1_IRQn                  = 7,
    EXTI2_IRQn                  = 8,
    EXTI3_IRQn                  = 9,
    EXTI4_IRQn                  = 10,
    DMA1_Stream0_IRQn           = 11,
    DMA1_Stream1_IRQn           = 12,
    DMA1_Stream2_IRQn           = 13,
    DMA1_Stream3_IRQn           = 14,
    DMA1_Stream4_IRQn           = 15,
    DMA1_Stream5_IRQn           = 16,
    DMA1_Stream6_IRQn           = 17,
    ADC_IRQn                    = 18,
    CAN1_TX_IRQn                = 19,
    CAN1_RX0_IRQn               = 20,
    CAN1_RX1_IRQn               = 21,
    CAN1_SCE_IRQn               = 22,
    EXTI9_5_IRQn                = 23,
    TIM1_BRK_TIM9_IRQn          = 24,
    TIM1_UP_TIM10_IRQn          = 25,
    TIM1_TRG_COM_TIM11_IRQn     = 26,
    TIM1_CC_IRQn                = 27,
    TIM2_IRQn                   = 28,
    TIM3_IRQn                   = 29,
    TIM4_IRQn                   = 30,
    I2C1_EV_IRQn                = 31,
    I2C1_ER_IRQn                = 32,
    I2C2_EV_IRQn                = 33,
    I2C2_ER_IRQn                = 34,
    SPI1_IRQn                   = 35,
    SPI2_IRQn                   = 36,
    USART1_IRQn                 = 37,
    USART2_IRQn                 = 38,
    USART3_IRQn                 = 39,
    EXTI15_10_IRQn              = 40,
    RTC_Alarm_IRQn              = 41,
    OTG_FS_WKUP_IRQn            = 42,
    TIM8_BRK_TIM12_IRQn         = 43,
    TIM8_UP_TIM13_IRQn          = 44,
    TIM8_TRG_COM_TIM14_IRQn     = 45,
    TIM8_CC_IRQn                = 46,
    DMA1_Stream7_IRQn           = 47,
    FSMC_IRQn                   = 48,
    SDIO_IRQn                   = 49,
    TIM5_IRQn                   = 50,
    SPI3_IRQn                   = 51,
    USART4_IRQn                 = 52,
    USART5_IRQn                 = 53,
    TIM6_DAC_IRQn               = 54,
    TIM7_IRQn                   = 55,
    DMA2_Stream0_IRQn           = 56,
    DMA2_Stream1_IRQn           = 57,
    DMA2_Stream2_IRQn           = 58,
    DMA2_Stream3_IRQn           = 59,
    DMA2_Stream4_IRQn           = 60,
    ETH_IRQn                    = 61,
    ETH_WKUP_IRQn               = 62,
    CAN2_TX_IRQn                = 63,
    CAN2_RX0_IRQn               = 64,
    CAN2_RX1_IRQn               = 65,
    CAN2_SCE_IRQn               = 66,
    OTG_FS_IRQn                 = 67,
    DMA2_Stream5_IRQn           = 68,
    DMA2_Stream6_IRQn           = 69,
    DMA2_Stream7_IRQn           = 70,
    USART6_IRQn                 = 71,
    I2C3_EV_IRQn                = 72,
    I2C3_ER_IRQn                = 73,
    OTG_HS_EP1_OUT_IRQn         = 74,
    OTG_HS_EP1_IN_IRQn          = 75,
    OTG_HS_WKUP_IRQn            = 76,
    OTG_HS_IRQn                 = 77,
    DCMI_IRQn                   = 78,
    CRYP_IRQn                   = 79,
    HASH_RNG_IRQn               = 80
} IRQn_Type;

/* ============================================================================
 * NVIC PRIORITY BITS (MUST BE BEFORE core_cm4.h)
 * ============================================================================ */

#define __NVIC_PRIO_BITS   4

/* ============================================================================
 * INCLUDE CMSIS CORE (NOW THAT IRQn_Type IS DEFINED)
 * ============================================================================ */

#include "core_cm4.h"
#include "cmsis_gcc.h"

/* ============================================================================
 * DEVICE DEFINITIONS
 * ============================================================================ */

#define STM32F407xx
#define SYSCLK_FREQ_168MHz  168000000U
#define AHB_CLK_FREQ        168000000U
#define APB1_CLK_FREQ       42000000U
#define APB2_CLK_FREQ       84000000U
#define HSE_VALUE           8000000U

/* ============================================================================
 * GPIO DEFINITIONS (MINIMAL)
 * ============================================================================ */

typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t LCKR;
    __IO uint32_t AFR[2];
} GPIO_TypeDef;

#define GPIOA_BASE      0x40020000U
#define GPIOB_BASE      0x40020400U
#define GPIOC_BASE      0x40020800U
#define GPIOD_BASE      0x40020C00U
#define GPIOE_BASE      0x40021000U
#define GPIOF_BASE      0x40021400U
#define GPIOG_BASE      0x40021800U
#define GPIOH_BASE      0x40021C00U
#define GPIOI_BASE      0x40022000U

#define GPIOA       ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB       ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC       ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD       ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE       ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOF       ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG       ((GPIO_TypeDef *) GPIOG_BASE)
#define GPIOH       ((GPIO_TypeDef *) GPIOH_BASE)
#define GPIOI       ((GPIO_TypeDef *) GPIOI_BASE)

/* GPIO Pins */
#define GPIO_Pin_0      0x0001
#define GPIO_Pin_1      0x0002
#define GPIO_Pin_2      0x0004
#define GPIO_Pin_3      0x0008
#define GPIO_Pin_4      0x0010
#define GPIO_Pin_5      0x0020
#define GPIO_Pin_6      0x0040
#define GPIO_Pin_7      0x0080
#define GPIO_Pin_8      0x0100
#define GPIO_Pin_9      0x0200
#define GPIO_Pin_10     0x0400
#define GPIO_Pin_11     0x0800
#define GPIO_Pin_12     0x1000
#define GPIO_Pin_13     0x2000
#define GPIO_Pin_14     0x4000
#define GPIO_Pin_15     0x8000
#define GPIO_Pin_All    0xFFFF

/* GPIO Modes */
#define GPIO_Mode_IN    0x00
#define GPIO_Mode_OUT   0x01
#define GPIO_Mode_AF    0x02
#define GPIO_Mode_AN    0x03

#define GPIO_OType_PP   0x00
#define GPIO_OType_OD   0x01

#define GPIO_Speed_2MHz     0x00
#define GPIO_Speed_25MHz    0x01
#define GPIO_Speed_50MHz    0x02
#define GPIO_Speed_100MHz   0x03

#define GPIO_PuPd_NOPULL    0x00
#define GPIO_PuPd_UP        0x01
#define GPIO_PuPd_DOWN      0x02

typedef struct {
    uint32_t GPIO_Pin;
    uint8_t GPIO_Mode;
    uint8_t GPIO_Speed;
    uint8_t GPIO_OType;
    uint8_t GPIO_PuPd;
} GPIO_InitTypeDef;

/* ============================================================================
 * RCC DEFINITIONS
 * ============================================================================ */

#define RCC_BASE    0x40023800U

typedef struct {
    __IO uint32_t CR;
    __IO uint32_t PLLCFGR;
    __IO uint32_t CFGR;
    __IO uint32_t CIR;
    __IO uint32_t AHB1RSTR;
    __IO uint32_t AHB2RSTR;
    __IO uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    __IO uint32_t APB1RSTR;
    __IO uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
    __IO uint32_t AHB3ENR;
    uint32_t RESERVED2;
    __IO uint32_t APB1ENR;
    __IO uint32_t APB2ENR;
    uint32_t RESERVED3[2];
    __IO uint32_t AHB1LPENR;
    __IO uint32_t AHB2LPENR;
    __IO uint32_t AHB3LPENR;
    uint32_t RESERVED4;
    __IO uint32_t APB1LPENR;
    __IO uint32_t APB2LPENR;
    uint32_t RESERVED5[2];
    __IO uint32_t BDCR;
    __IO uint32_t CSR;
    uint32_t RESERVED6[2];
    __IO uint32_t SSCGR;
    __IO uint32_t PLLI2SCFGR;
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *) RCC_BASE)

#define RCC_CR_HSION           0x00000001
#define RCC_CR_HSIRDY          0x00000002
#define RCC_CR_HSEON           0x00010000
#define RCC_CR_HSERDY          0x00020000
#define RCC_CR_PLLON           0x01000000
#define RCC_CR_PLLRDY          0x02000000

#define RCC_AHB1Periph_GPIOA   0x00000001
#define RCC_AHB1Periph_GPIOB   0x00000002
#define RCC_AHB1Periph_GPIOC   0x00000004
#define RCC_AHB1Periph_GPIOD   0x00000008
#define RCC_AHB1Periph_GPIOE   0x00000010
#define RCC_AHB1Periph_GPIOF   0x00000020
#define RCC_AHB1Periph_GPIOG   0x00000040
#define RCC_AHB1Periph_GPIOH   0x00000080
#define RCC_AHB1Periph_GPIOI   0x00000100

#define RCC_APB2Periph_TIM1    0x00000001
#define RCC_APB2Periph_TIM8    0x00000002
#define RCC_APB2Periph_USART1  0x00000010
#define RCC_APB2Periph_USART6  0x00000020
#define RCC_APB2Periph_ADC1    0x00000100
#define RCC_APB2Periph_SPI1    0x00001000
#define RCC_APB2Periph_SYSCFG  0x00004000

#define RCC_APB1Periph_TIM2    0x00000001
#define RCC_APB1Periph_TIM3    0x00000002
#define RCC_APB1Periph_TIM4    0x00000004
#define RCC_APB1Periph_TIM5    0x00000008
#define RCC_APB1Periph_USART2  0x00020000
#define RCC_APB1Periph_I2C1    0x00200000
#define RCC_APB1Periph_I2C2    0x00400000
#define RCC_APB1Periph_I2C3    0x00800000

/* ============================================================================
 * TIMER DEFINITIONS
 * ============================================================================ */

#define TIM1_BASE   0x40010000U
#define TIM2_BASE   0x40000000U
#define TIM3_BASE   0x40000400U
#define TIM4_BASE   0x40000800U
#define TIM5_BASE   0x40000C00U

typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMCR;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    __IO uint32_t CCMR1;
    __IO uint32_t CCMR2;
    __IO uint32_t CCER;
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
    __IO uint32_t RCR;
    __IO uint32_t CCR1;
    __IO uint32_t CCR2;
    __IO uint32_t CCR3;
    __IO uint32_t CCR4;
    __IO uint32_t BDTR;
    __IO uint32_t DCR;
    __IO uint32_t DMAR;
} TIM_TypeDef;

#define TIM1   ((TIM_TypeDef *) TIM1_BASE)
#define TIM2   ((TIM_TypeDef *) TIM2_BASE)
#define TIM3   ((TIM_TypeDef *) TIM3_BASE)
#define TIM4   ((TIM_TypeDef *) TIM4_BASE)
#define TIM5   ((TIM_TypeDef *) TIM5_BASE)

/* ============================================================================
 * FLASH DEFINITIONS
 * ============================================================================ */

#define FLASH_BASE  0x40023C00U

typedef struct {
    __IO uint32_t ACR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t OPTCR;
} FLASH_TypeDef;

#define FLASH   ((FLASH_TypeDef *) FLASH_BASE)

/* ============================================================================
 * MISC DEFINITIONS
 * ============================================================================ */

#define ENABLE      1
#define DISABLE     0

/* ============================================================================
 * SYSTICK CONFIGURATION (MISSING FROM CMSIS CORE)
 * ============================================================================ */

#define SYSTICK_MAXCOUNT    0x00FFFFFFUL

__STATIC_INLINE uint32_t SysTick_Config(uint32_t ticks)
{
    if ((ticks - 1UL) > SYSTICK_MAXCOUNT)
    {
        return (1UL);  /* Reload value impossible */
    }
    
    SysTick->LOAD  = (uint32_t)(ticks - 1UL);
    NVIC_SetPriority(SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
    SysTick->VAL   = 0UL;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    return (0UL);
}

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_H */
