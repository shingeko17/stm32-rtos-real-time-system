/**
 * @file misc_drivers.h
 * @brief Minimal STM32 StdPeriph Library Function Declarations
 * @date 2026-03-14
 */

#ifndef MISC_DRIVERS_H
#define MISC_DRIVERS_H

#include "stm32f4xx.h"

/* ============================================================================
 * GPIO PIN SOURCE DEFINITIONS
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
#define GPIO_AF_TIM8           ((uint8_t)0x03)

/* ============================================================================
 * TIMER STRUCTURE DEFINITIONS
 * ============================================================================ */

typedef struct {
    uint16_t TIM_Prescaler;
    uint16_t TIM_CounterMode;
    uint32_t TIM_Period;
    uint16_t TIM_ClockDivision;
    uint8_t  TIM_RepetitionCounter;
} TIM_TimeBaseInitTypeDef;

typedef struct {
    uint16_t TIM_OCMode;
    uint16_t TIM_OutputState;
    uint16_t TIM_OutputNState;
    uint32_t TIM_Pulse;
    uint16_t TIM_OCPolarity;
    uint16_t TIM_OCNPolarity;
    uint16_t TIM_OCIdleState;
    uint16_t TIM_OCNIdleState;
} TIM_OCInitTypeDef;

/* Timer modes */
#define TIM_CounterMode_Up               ((uint16_t)0x0000)
#define TIM_CounterMode_Down             ((uint16_t)0x0010)
#define TIM_CounterMode_CenterAligned1   ((uint16_t)0x0020)
#define TIM_CounterMode_CenterAligned2   ((uint16_t)0x0040)
#define TIM_CounterMode_CenterAligned3   ((uint16_t)0x0060)

/* Output compare modes */
#define TIM_OCMode_Timing                ((uint16_t)0x0000)
#define TIM_OCMode_Active                ((uint16_t)0x0010)
#define TIM_OCMode_Inactive              ((uint16_t)0x0020)
#define TIM_OCMode_Toggle                ((uint16_t)0x0030)
#define TIM_OCMode_PWM1                  ((uint16_t)0x0060)
#define TIM_OCMode_PWM2                  ((uint16_t)0x0070)
#define TIM_OCMode_Retriggerable_OPM1    ((uint16_t)0x0080)
#define TIM_OCMode_Retriggerable_OPM2    ((uint16_t)0x0090)

/* Output states */
#define TIM_OutputState_Disable          ((uint16_t)0x0000)
#define TIM_OutputState_Enable           ((uint16_t)0x0001)

/* Polarity */
#define TIM_OCPolarity_High              ((uint16_t)0x0000)
#define TIM_OCPolarity_Low               ((uint16_t)0x0002)

/* Output Compare Preload */
#define TIM_OCPreload_Disable            ((uint16_t)0x0000)
#define TIM_OCPreload_Enable             ((uint16_t)0x0008)

/* ============================================================================
 * TIM BIT MASKS FOR REGISTER OPERATIONS
 * ============================================================================ */

/* CR1 reg bits */
#define TIM_CR1_CKD                      ((uint16_t)0x0300)
#define TIM_CR1_ARPE                     ((uint16_t)0x0080)
#define TIM_CR1_CMS                      ((uint16_t)0x0060)
#define TIM_CR1_DIR                      ((uint16_t)0x0010)

/* CCMR2 reg bits */
#define TIM_CCMR2_OC3M                   ((uint32_t)0x0070)
#define TIM_CCMR2_OC3PE                  ((uint32_t)0x0008)
#define TIM_CCMR2_OC4M                   ((uint32_t)0xF000)
#define TIM_CCMR2_OC4PE                  ((uint32_t)0x0800)

/* CCER reg bits */
#define TIM_CCER_CC3E                    ((uint32_t)0x0100)
#define TIM_CCER_CC3P                    ((uint32_t)0x0200)
#define TIM_CCER_CC4E                    ((uint32_t)0x1000)
#define TIM_CCER_CC4P                    ((uint32_t)0x2000)

/* EGR reg bits */
#define TIM_EGR_UG                       ((uint16_t)0x0001)

/* BDTR reg bits */
#define TIM_BDTR_MOE                     ((uint32_t)0x8000)

/* Clock division */
#define TIM_CKD_DIV1                     ((uint16_t)0x0000)
#define TIM_CKD_DIV2                     ((uint16_t)0x0100)
#define TIM_CKD_DIV4                     ((uint16_t)0x0200)

/* ============================================================================
 * RCC FUNCTIONS
 * ============================================================================ */

void RCC_AHB1PeriphClockCmd(uint32_t RCC_AHB1Periph, uint8_t NewState);
void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, uint8_t NewState);
void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, uint8_t NewState);

/* ============================================================================
 * GPIO FUNCTIONS
 * ============================================================================ */

void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct);
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_Write(GPIO_TypeDef* GPIOx, uint16_t PortVal);
void GPIO_PinAFConfig(GPIO_TypeDef* GPIOx, uint16_t GPIO_PinSource, uint8_t GPIO_AF);

/* ============================================================================
 * TIMER FUNCTIONS
 * ============================================================================ */

void TIM_TimeBaseInit(TIM_TypeDef* TIMx, TIM_TimeBaseInitTypeDef* TIM_TimeBaseInitStruct);
void TIM_OC3Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
void TIM_OC4Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
void TIM_OC3PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload);
void TIM_OC4PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload);
void TIM_ARRPreloadConfig(TIM_TypeDef* TIMx, uint8_t NewState);
void TIM_CtrlPWMOutputs(TIM_TypeDef* TIMx, uint8_t NewState);
void TIM_Cmd(TIM_TypeDef* TIMx, uint8_t NewState);
void TIM_SetCompare3(TIM_TypeDef* TIMx, uint32_t Compare3);
void TIM_SetCompare4(TIM_TypeDef* TIMx, uint32_t Compare4);

#endif /* MISC_DRIVERS_H */
