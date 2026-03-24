/**
 * @file encoder_driver.c
 * @brief Encoder Driver Implementation - Motor Speed Feedback
 * @date 2026-03-22
 */

#include "encoder_driver.h"
#include "misc_drivers.h"

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================ */

/** Total pulse count */
static volatile uint32_t encoder_pulse_count = 0;

/** Previous pulse count (for change detection) */
static uint32_t encoder_prev_count = 0;

/** Current RPM (cached value) */
static uint16_t encoder_current_rpm = 0;

/** Flag to indicate if encoder is stuck */
static uint8_t encoder_stuck = 0;

/** Raw capture value */
static volatile uint16_t encoder_raw_value = 0;

/* ============================================================================
 * PUBLIC FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

/**
 * @brief Encoder_Init - Khởi tạo TIM3 Input Capture
 */
void Encoder_Init(void)
{
    /*
     * TODO: Implement encoder initialization
     * 
     * Steps:
     * 1. Enable RCC clocks:
     *    - RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE)
     *    - RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE)
     * 
     * 2. Configure GPIO (PA6) as input capture:
     *    - GPIO_Mode = GPIO_Mode_AF
     *    - GPIO_OType = GPIO_OType_IN
     *    - GPIO_PuPd = GPIO_PuPd_UP (pull-up)
     *    - GPIO_Ospeed = GPIO_Speed_50MHz
     *    - GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_TIM3)
     *    - GPIO_Init(GPIOA, &GPIO_InitStructure)
     * 
     * 3. Configure TIM3:
     *    - Prescaler: Adjust based on pulse frequency
     *      For 500 RPM motor with 20 PPR:
     *      Pulse frequency ≈ 500 * 20 / 60 ≈ 167 Hz
     *      To capture accurately: Prescaler ≈ 168/167 ≈ 1 (minimal)
     *    - Period: TIM3_ARR = 0xFFFF (16-bit max)
     *    - CounterMode = TIM_CounterMode_Up
     *    - TIM_Init(TIM3, &TIM_InitStructure)
     * 
     * 4. Configure Input Capture Channel 1:
     *    - TIM_ICInitStructure.TIM_Channel = TIM_Channel_1
     *    - TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising
     *    - TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI
     *    - TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1
     *    - TIM_ICInitStructure.TIM_ICFilter = 0 (no filter)
     *    - TIM_ICInit(TIM3, &TIM_ICInitStructure)
     * 
     * 5. Enable TIM3 + Capture interrupt:
     *    - TIM_Cmd(TIM3, ENABLE)
     *    - TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE)
     *    - NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn
     *    - NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2
     *    - NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0
     *    - NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE
     *    - NVIC_Init(&NVIC_InitStructure)
     */
}

/**
 * @brief Encoder_GetRPM - Đọc RPM
 */
uint16_t Encoder_GetRPM(void)
{
    /*
     * TODO: Implement RPM calculation
     * 
     * 1. Calculate pulse difference since last sample:
     *    pulse_delta = encoder_pulse_count - encoder_prev_count
     *    encoder_prev_count = encoder_pulse_count
     * 
     * 2. Calculate frequency (Hz):
     *    Pulse rate per 100ms → multiply by 10 to get Hz
     *    frequency_hz = pulse_delta * 10
     * 
     * 3. Calculate RPM:
     *    rpm = (frequency_hz * 60) / ENCODER_PPR
     *    Example: 167 pulses/s, PPR=20
     *    RPM = (167 * 60) / 20 = 501 RPM ✓
     * 
     * 4. Cache result and return
     *    encoder_current_rpm = rpm
     *    return rpm
     * 
     * Note: This should be called periodically (e.g., every 100ms from task)
     */
    
    // Placeholder calculation
    return encoder_current_rpm;
}

/**
 * @brief Encoder_GetCount - Đọc tổng pulse
 */
uint32_t Encoder_GetCount(void)
{
    /*
     * TODO: Return current pulse count
     * 
     * This is a simple counter that increments with encoder pulse
     */
    
    return encoder_pulse_count;
}

/**
 * @brief Encoder_ResetCount - Reset counter
 */
void Encoder_ResetCount(void)
{
    /*
     * TODO: Reset pulse counter
     */
    
    encoder_pulse_count = 0;
    encoder_prev_count = 0;
    encoder_current_rpm = 0;
}

/**
 * @brief Encoder_IsStuck - Phát hiện motor stuck
 */
uint8_t Encoder_IsStuck(void)
{
    /*
     * TODO: Detect if encoder is stuck (motor not rotating)
     * 
     * Logic:
     * 1. If pulse count hasn't changed in ENCODER_SAMPLE_RATE_MS, stuck = 1
     * 2. If pulse count changed, stuck = 0
     * 3. Return stuck flag
     * 
     * This should be called periodically to check
     */
    
    return encoder_stuck;
}

/**
 * @brief Encoder_GetRaw - Trả về raw capture
 */
uint16_t Encoder_GetRaw(void)
{
    /*
     * TODO: Return raw capture value
     * 
     * Read from TIM3->CCR1 (Capture Compare Register)
     */
    
    return encoder_raw_value;
}

/* ============================================================================
 * INTERRUPT HANDLERS
 * ============================================================================ */

/**
 * @brief TIM3_IRQHandler - Handle encoder pulse capture interrupt
 * 
 * Called when rising edge is detected on PA6 (TIM3_CH1)
 */
/*
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET) {
        // Capture interrupt triggered
        
        // Read capture value
        encoder_raw_value = TIM_GetCapture1(TIM3);
        
        // Increment pulse counter
        encoder_pulse_count++;
        
        // Check for stuck condition
        if (encoder_pulse_count == encoder_prev_count) {
            encoder_stuck = 1;  // No new pulses
        } else {
            encoder_stuck = 0;  // Motor is rotating
        }
        
        // Clear interrupt
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);
    }
}
*/
