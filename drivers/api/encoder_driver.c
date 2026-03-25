/**
 * @file encoder_driver.c
 * @brief Encoder Driver Implementation - Motor Speed Feedback
 * @date 2026-03-22
 */

#include "encoder_driver.h"
#include "misc_drivers.h"

/* TIM register helper bits */
#define TIM_CR1_CEN            (1UL << 0)
#define TIM_DIER_CC1IE         (1UL << 1)
#define TIM_SR_CC1IF           (1UL << 1)
#define TIM_CCER_CC1E          (1UL << 0)
#define TIM_CCMR1_CC1S_TI1     (1UL << 0)

/* Total pulse count */
static volatile uint32_t encoder_pulse_count = 0;

/* Previous pulse count (for change detection) */
static uint32_t encoder_prev_count = 0;

/* Current RPM (cached value) */
static uint16_t encoder_current_rpm = 0;

/* Flag to indicate if encoder is stuck */
static uint8_t encoder_stuck = 0;

/* Raw capture value */
static volatile uint16_t encoder_raw_value = 0;

void Encoder_Init(void)
{
    GPIO_InitTypeDef gpio_cfg;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    gpio_cfg.GPIO_Pin = ENCODER_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AF;
    gpio_cfg.GPIO_OType = GPIO_OType_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_cfg.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(ENCODER_PORT, &gpio_cfg);

    GPIO_PinAFConfig(ENCODER_PORT, GPIO_PinSource6, ENCODER_AF);

    ENCODER_TIMER->CR1 = 0U;
    ENCODER_TIMER->PSC = 83U;           /* 84MHz/84 = 1MHz timer clock */
    ENCODER_TIMER->ARR = 0xFFFFU;
    ENCODER_TIMER->CNT = 0U;

    ENCODER_TIMER->CCMR1 &= ~0x3U;
    ENCODER_TIMER->CCMR1 |= TIM_CCMR1_CC1S_TI1;
    ENCODER_TIMER->CCER &= ~((1UL << 1) | (1UL << 3)); /* Rising edge, non-inverted */
    ENCODER_TIMER->CCER |= TIM_CCER_CC1E;

    ENCODER_TIMER->DIER |= TIM_DIER_CC1IE;
    ENCODER_TIMER->CR1 |= TIM_CR1_CEN;

    NVIC_SetPriority(TIM3_IRQn, 2U);
    NVIC_EnableIRQ(TIM3_IRQn);

    encoder_pulse_count = 0U;
    encoder_prev_count = 0U;
    encoder_current_rpm = 0U;
    encoder_stuck = 0U;
    encoder_raw_value = 0U;
}

uint16_t Encoder_GetRPM(void)
{
    uint32_t pulse_delta;
    uint32_t frequency_hz;
    uint32_t rpm;

    pulse_delta = encoder_pulse_count - encoder_prev_count;
    encoder_prev_count = encoder_pulse_count;

    frequency_hz = pulse_delta * (1000U / ENCODER_SAMPLE_RATE_MS);
    rpm = (frequency_hz * 60U) / ENCODER_PPR;

    if (rpm > 65535U) {
        rpm = 65535U;
    }

    encoder_current_rpm = (uint16_t)rpm;
    encoder_stuck = (pulse_delta == 0U) ? 1U : 0U;

    return encoder_current_rpm;
}

uint32_t Encoder_GetCount(void)
{
    return encoder_pulse_count;
}

void Encoder_ResetCount(void)
{
    encoder_pulse_count = 0U;
    encoder_prev_count = 0U;
    encoder_current_rpm = 0U;
    encoder_stuck = 0U;
}

uint8_t Encoder_IsStuck(void)
{
    return encoder_stuck;
}

uint16_t Encoder_GetRaw(void)
{
    return encoder_raw_value;
}

void TIM3_IRQHandler(void)
{
    if ((ENCODER_TIMER->SR & TIM_SR_CC1IF) != 0U) {
        encoder_raw_value = (uint16_t)ENCODER_TIMER->CCR1;
        encoder_pulse_count++;
        ENCODER_TIMER->SR &= ~TIM_SR_CC1IF;
    }
}
