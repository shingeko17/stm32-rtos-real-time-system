/**
 * @file misc_drivers.c
 * @brief Minimal STM32 StdPeriph Library Function Stubs
 * @details Basic implementations of RCC, GPIO, TIM functions
 * @date 2026-03-14
 */

#include "stm32f4xx.h"
#include "misc_drivers.h"

/* ============================================================================
 * RCC CLOCK ENABLE/DISABLE FUNCTIONS
 * ============================================================================ */

/**
 * @brief Enable AHB1 Peripheral Clock
 */
void RCC_AHB1PeriphClockCmd(uint32_t RCC_AHB1Periph, uint8_t NewState)
{
    if (NewState != DISABLE) {
        RCC->AHB1ENR |= RCC_AHB1Periph;
    } else {
        RCC->AHB1ENR &= ~RCC_AHB1Periph;
    }
}

/**
 * @brief Enable APB2 Peripheral Clock
 */
void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, uint8_t NewState)
{
    if (NewState != DISABLE) {
        RCC->APB2ENR |= RCC_APB2Periph;
    } else {
        RCC->APB2ENR &= ~RCC_APB2Periph;
    }
}

/**
 * @brief Enable APB1 Peripheral Clock
 */
void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, uint8_t NewState)
{
    if (NewState != DISABLE) {
        RCC->APB1ENR |= RCC_APB1Periph;
    } else {
        RCC->APB1ENR &= ~RCC_APB1Periph;
    }
}

/* ============================================================================
 * GPIO FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize GPIO pins
 */
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct)
{
    uint32_t pinpos = 0x00, pos = 0x00;
    uint32_t currentpin = 0x00;
    
    for (pinpos = 0x00; pinpos < 0x10; pinpos++) {
        pos = ((uint32_t)1) << pinpos;
        currentpin = (GPIO_InitStruct->GPIO_Pin) & pos;
        
        if (currentpin == pos) {
            /* Configure pin mode, speed, type, and pull-up/down */
            uint32_t tmp = GPIOx->MODER;
            tmp &= ~(3 << (pinpos * 2));
            tmp |= (GPIO_InitStruct->GPIO_Mode << (pinpos * 2));
            GPIOx->MODER = tmp;
            
            /* Configure output type */
            tmp = GPIOx->OTYPER;
            tmp &= ~((GPIO_InitStruct->GPIO_OType) << pinpos);
            tmp |= (GPIO_InitStruct->GPIO_OType << pinpos);
            GPIOx->OTYPER = tmp;
            
            /* Configure speed */
            tmp = GPIOx->OSPEEDR;
            tmp &= ~(3 << (pinpos * 2));
            tmp |= (GPIO_InitStruct->GPIO_Speed << (pinpos * 2));
            GPIOx->OSPEEDR = tmp;
            
            /* Configure pull-up/pull-down */
            tmp = GPIOx->PUPDR;
            tmp &= ~(3 << (pinpos * 2));
            tmp |= (GPIO_InitStruct->GPIO_PuPd << (pinpos * 2));
            GPIOx->PUPDR = tmp;
        }
    }
}

/**
 * @brief Set GPIO output bits high
 */
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->BSRR = (uint32_t)GPIO_Pin;
}

/**
 * @brief Reset GPIO output bits low
 */
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16;
}

/**
 * @brief Write GPIO output data
 */
void GPIO_Write(GPIO_TypeDef* GPIOx, uint16_t PortVal)
{
    GPIOx->ODR = PortVal;
}

/**
 * @brief Configure GPIO pin alternate function
 */
void GPIO_PinAFConfig(GPIO_TypeDef* GPIOx, uint16_t GPIO_PinSource, uint8_t GPIO_AF)
{
    uint32_t tmp = 0x00;
    uint32_t temp = 0x00;
    
    temp = ((uint32_t)(GPIO_AF) << ((uint32_t)((GPIO_PinSource & (uint16_t)0x07) * 4)));
    tmp = GPIOx->AFR[GPIO_PinSource >> 0x03];
    tmp &= ~((uint32_t)0xF << ((uint32_t)((GPIO_PinSource & (uint16_t)0x07) * 4)));
    tmp |= temp;
    GPIOx->AFR[GPIO_PinSource >> 0x03] = tmp;
}

/* ============================================================================
 * TIMER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize Timer Time Base
 */
void TIM_TimeBaseInit(TIM_TypeDef* TIMx, TIM_TimeBaseInitTypeDef* TIM_TimeBaseInitStruct)
{
    uint32_t tmp = 0;
    
    /* Set prescaler */
    TIMx->PSC = TIM_TimeBaseInitStruct->TIM_Prescaler;
    
    /* Set counter mode */
    tmp = TIMx->CR1;
    tmp &= ~(TIM_CR1_DIR | TIM_CR1_CMS);
    tmp |= TIM_TimeBaseInitStruct->TIM_CounterMode;
    TIMx->CR1 = tmp;
    
    /* Set period (auto-reload register) */
    TIMx->ARR = TIM_TimeBaseInitStruct->TIM_Period;
    
    /* Set clock division */
    tmp = TIMx->CR1;
    tmp &= ~TIM_CR1_CKD;
    tmp |= TIM_TimeBaseInitStruct->TIM_ClockDivision;
    TIMx->CR1 = tmp;
    
    /* Generate update event to load PSC and ARR */
    TIMx->EGR = TIM_EGR_UG;
}

/**
 * @brief Initialize Timer Output Compare Channel 3
 */
void TIM_OC3Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
    uint32_t tmp = 0;
    
    /* Output compare mode */
    tmp = TIMx->CCMR2;
    tmp &= ~((uint32_t)TIM_CCMR2_OC3M);
    tmp |= (uint32_t)(TIM_OCInitStruct->TIM_OCMode << 4);
    TIMx->CCMR2 = tmp;
    
    /* Output compare polarity */
    tmp = TIMx->CCER;
    tmp &= ~TIM_CCER_CC3P;
    tmp |= (TIM_OCInitStruct->TIM_OCPolarity << 8);
    TIMx->CCER = tmp;
    
    /* Set pulse value */
    TIMx->CCR3 = TIM_OCInitStruct->TIM_Pulse;
    
    /* Enable output */
    tmp = TIMx->CCER;
    tmp |= (TIM_CCER_CC3E);
    TIMx->CCER = tmp;
}

/**
 * @brief Initialize Timer Output Compare Channel 4
 */
void TIM_OC4Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
    uint32_t tmp = 0;
    
    /* Output compare mode */
    tmp = TIMx->CCMR2;
    tmp &= ~((uint32_t)TIM_CCMR2_OC4M);
    tmp |= (uint32_t)(TIM_OCInitStruct->TIM_OCMode << 12);
    TIMx->CCMR2 = tmp;
    
    /* Output compare polarity */
    tmp = TIMx->CCER;
    tmp &= ~TIM_CCER_CC4P;
    tmp |= (TIM_OCInitStruct->TIM_OCPolarity << 12);
    TIMx->CCER = tmp;
    
    /* Set pulse value */
    TIMx->CCR4 = TIM_OCInitStruct->TIM_Pulse;
    
    /* Enable output */
    tmp = TIMx->CCER;
    tmp |= (TIM_CCER_CC4E);
    TIMx->CCER = tmp;
}

/**
 * @brief Configure Output Compare Channel 3 Preload
 */
void TIM_OC3PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
    uint32_t tmp = TIMx->CCMR2;
    tmp &= ~TIM_CCMR2_OC3PE;
    tmp |= (TIM_OCPreload << 3);
    TIMx->CCMR2 = tmp;
}

/**
 * @brief Configure Output Compare Channel 4 Preload
 */
void TIM_OC4PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
    uint32_t tmp = TIMx->CCMR2;
    tmp &= ~TIM_CCMR2_OC4PE;
    tmp |= (TIM_OCPreload << 11);
    TIMx->CCMR2 = tmp;
}

/**
 * @brief Configure Auto-Reload Register Preload
 */
void TIM_ARRPreloadConfig(TIM_TypeDef* TIMx, uint8_t NewState)
{
    if (NewState != DISABLE) {
        TIMx->CR1 |= TIM_CR1_ARPE;
    } else {
        TIMx->CR1 &= ~TIM_CR1_ARPE;
    }
}

/**
 * @brief Enable PWM outputs on Advanced Timers (TIM1/TIM8)
 */
void TIM_CtrlPWMOutputs(TIM_TypeDef* TIMx, uint8_t NewState)
{
    if (NewState != DISABLE) {
        TIMx->BDTR |= TIM_BDTR_MOE;
    } else {
        TIMx->BDTR &= ~TIM_BDTR_MOE;
    }
}

/**
 * @brief Enable Timer
 */
void TIM_Cmd(TIM_TypeDef* TIMx, uint8_t NewState)
{
    if (NewState != DISABLE) {
        TIMx->CR1 |= 0x0001;
    } else {
        TIMx->CR1 &= ~0x0001;
    }
}

/**
 * @brief Set Timer Compare Value
 */
void TIM_SetCompare3(TIM_TypeDef* TIMx, uint32_t Compare3)
{
    TIMx->CCR3 = Compare3;
}

/**
 * @brief Set Timer Compare Value
 */
void TIM_SetCompare4(TIM_TypeDef* TIMx, uint32_t Compare4)
{
    TIMx->CCR4 = Compare4;
}

/* ============================================================================
 * GPIO PIN SOURCE DEFINITIONS (for alternate function config)
 * ============================================================================ */

#define GPIO_PinSource0        ((uint16_t)0x0000)
#define GPIO_PinSource1        ((uint16_t)0x0001)
#define GPIO_PinSource2        ((uint16_t)0x0002)
#define GPIO_PinSource3        ((uint16_t)0x0003)
#define GPIO_PinSource4        ((uint16_t)0x0004)
#define GPIO_PinSource5        ((uint16_t)0x0005)
#define GPIO_PinSource6        ((uint16_t)0x0006)
#define GPIO_PinSource7        ((uint16_t)0x0007)
#define GPIO_PinSource8        ((uint16_t)0x0008)
#define GPIO_PinSource9        ((uint16_t)0x0009)
#define GPIO_PinSource10       ((uint16_t)0x000A)
#define GPIO_PinSource11       ((uint16_t)0x000B)
#define GPIO_PinSource12       ((uint16_t)0x000C)
#define GPIO_PinSource13       ((uint16_t)0x000D)
#define GPIO_PinSource14       ((uint16_t)0x000E)
#define GPIO_PinSource15       ((uint16_t)0x000F)

/* Alternate functions */
#define GPIO_AF_TIM1           ((uint8_t)0x01)
#define GPIO_AF_TIM2           ((uint8_t)0x01)
#define GPIO_AF_TIM3           ((uint8_t)0x02)
#define GPIO_AF_TIM4           ((uint8_t)0x02)
#define GPIO_AF_TIM5           ((uint8_t)0x02)
