/**
 * @file l298_example.c
 * @brief L298 Driver - Practical Example with STM32 HAL Integration
 * @details Ví dụ hoàn chỉnh cách sử dụng L298 driver trong STM32 project
 * @author Advanced STM32 Driver Team
 * @date 2026-03-31
 * 
 * ============================================================================
 * FILE NÀY CHỨA:
 * 1. HAL Integration Functions (GPIO, PWM Timer setup)
 * 2. Concrete Implementation của helper functions
 * 3. Example usage trong main()
 * 4. ISR handlers cho PWM interrupts
 * ============================================================================
 */

#include "l298_driver.h"
#include "stm32f4xx_hal.h"  /* STM32F4 HAL Library */

/* ============================================================================
 * STEP 1: HARDWARE CONFIGURATION (STM32F4 specific)
 * ============================================================================
 * 
 * Giả sử:
 * - Motor A: 
 *   - IN1 = PA0 (GPIO)
 *   - IN2 = PA1 (GPIO)
 *   - EN = PA2 (PWM - Timer 2 Channel 3)
 *   - SENSE = PA3 (ADC1 Channel 3)
 * 
 * - Motor B:
 *   - IN3 = PA4 (GPIO)
 *   - IN4 = PA5 (GPIO)
 *   - EN = PA6 (PWM - Timer 2 Channel 3)
 *   - SENSE = PA7 (ADC1 Channel 7)
 */

#define L298_PORT_A                GPIOA
#define L298_PORT_B                GPIOA

/* Motor A pins */
#define L298_A_IN1_PIN             GPIO_PIN_0
#define L298_A_IN2_PIN             GPIO_PIN_1
#define L298_A_EN_PIN              GPIO_PIN_2
#define L298_A_SENSE_PIN           GPIO_PIN_3

/* Motor B pins */
#define L298_B_IN3_PIN             GPIO_PIN_4
#define L298_B_IN4_PIN             GPIO_PIN_5
#define L298_B_EN_PIN              GPIO_PIN_6
#define L298_B_SENSE_PIN           GPIO_PIN_7

/* PWM Timer Configuration */
#define L298_PWM_TIMER             TIM2
#define L298_PWM_FREQ_HZ           1000  /* 1 kHz */
#define L298_PWM_MAX_DUTY          999   /* 16-bit timer (1000-1) */

/* ADC for current sensing */
#define L298_ADC_HANDLE             hadc1

/* ============================================================================
 * STEP 2: GLOBAL INSTANCES (External from HAL)
 * ============================================================================
 * 
 * Những handler này được tạo bởi STM32CubeMX (hoặc manual).
 * Chúng là global variables của STM32 HAL system.
 */

extern TIM_HandleTypeDef htim2;       /* Timer 2 for PWM */
extern ADC_HandleTypeDef hadc1;       /* ADC 1 for current sensing */

/* ============================================================================
 * STEP 3: IMPLEMENT HELPER FUNCTIONS (Thay thế TODO comments)
 * ============================================================================
 * 
 * Bây giờ chúng ta implement cái cụ thể bằng STM32 HAL calls
 */

/**
 * @brief Set physical GPIO pins (Channel A or B)
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Conditional compilation: Compile khác nhau cho A vs B
 * - Type casting: (uint32_t) cast
 * - Ternary operator: inline conditions
 */
static void l298_set_pins(const L298_Driver *driver,
                           L298_ChannelID channel,
                           uint8_t in1_state, uint8_t in2_state, uint8_t en_state)
{
    if (driver == NULL) return;
    
    /* Select port & pins based on channel */
    GPIO_TypeDef *port = (channel == L298_CHANNEL_A) ? L298_PORT_A : L298_PORT_B;
    
    uint32_t in1_pin = (channel == L298_CHANNEL_A) ? L298_A_IN1_PIN : L298_B_IN3_PIN;
    uint32_t in2_pin = (channel == L298_CHANNEL_A) ? L298_A_IN2_PIN : L298_B_IN4_PIN;
    uint32_t en_pin  = (channel == L298_CHANNEL_A) ? L298_A_EN_PIN : L298_B_EN_PIN;
    
    /* Set IN1 pin state */
    HAL_GPIO_WritePin(port, in1_pin, 
                      in1_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    /* Set IN2 pin state */
    HAL_GPIO_WritePin(port, in2_pin, 
                      in2_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    /*
     * KIẾN THỨC C NÂNG CAO: EN pin handling
     * EN pin được điều khiển bằng PWM (Timer), không phải GPIO
     * Nếu en_state = 0: Stop PWM
     * Nếu en_state = 1: Start PWM
     */
    
    uint32_t timer_channel = (channel == L298_CHANNEL_A) ? TIM_CHANNEL_1 : TIM_CHANNEL_2;
    
    if (en_state) {
        /* Start PWM */
        HAL_TIM_PWM_Start(&htim2, timer_channel);
    } else {
        /* Stop PWM */
        HAL_TIM_PWM_Stop(&htim2, timer_channel);
    }
}

/**
 * @brief Set PWM duty cycle using HAL
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Macro expansion: __HAL_TIM_SET_COMPARE là macro (compiled inline)
 * - Register manipulation: Thực tế macro write tới CCR register
 */
static void l298_set_pwm_duty(const L298_Driver *driver,
                               L298_ChannelID channel,
                               uint16_t duty)
{
    if (driver == NULL) return;
    
    /*
     * MACRO: __HAL_TIM_SET_COMPARE(htim, channel, compare)
     * 
     * Thực tế, macro này expand thành:
     *   (*(__IO uint32_t *)&(htim)->Instance->CCR[channel]) = compare;
     * 
     * Nghĩa là nó write trực tiếp vào CCR (Capture/Compare Register) của timer
     */
    
    uint32_t timer_channel = (channel == L298_CHANNEL_A) ? TIM_CHANNEL_1 : TIM_CHANNEL_2;
    __HAL_TIM_SET_COMPARE(&htim2, timer_channel, duty);
}

/**
 * @brief Initialize GPIO pins for L298
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - struct initialization: Cách tạo struct in-line
 * - Multiple initializations: Setup tất cả pins đều lúc
 */
static void l298_gpio_init(void)
{
    /*
     * KIẾN THỨC C NÂNG CAO: Struct inline initialization
     * 
     * Thay vì:
     *   GPIO_InitTypeDef GPIO_InitStruct;
     *   GPIO_InitStruct.Pin = GPIO_PIN_0;
     *   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     *   GPIO_InitStruct.Pull = GPIO_NOPULL;
     *   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
     * 
     * Dùng:
     *   GPIO_InitTypeDef GPIO_InitStruct = {
     *       .Pin = GPIO_PIN_0,
     *       .Mode = GPIO_MODE_OUTPUT_PP,
     *       ...
     *   };
     */
    
    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* Configure Motor A pins */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = L298_A_IN1_PIN | L298_A_IN2_PIN | L298_B_IN3_PIN | L298_B_IN4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;      /* Output push-pull */
    GPIO_InitStruct.Pull = GPIO_NOPULL;              /* No pull resistor */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    /* High speed */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Configure PWM pins (EN pins) - handled by PWM timer */
    GPIO_InitStruct.Pin = L298_A_EN_PIN | L298_B_EN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;         /* Alternate function */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;      /* Timer 2 AF */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Configure ADC pins for current sensing */
    GPIO_InitStruct.Pin = L298_A_SENSE_PIN | L298_B_SENSE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;        /* Analog */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief Initialize PWM Timer for L298 (1 kHz, 16-bit)
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Timer prescaler calculation: Để achieve 1 kHz frequency
 * - PWM period setting: Để achieve 16-bit resolution
 */
static void l298_timer_init(void)
{
    /*
     * STM32F4 APB1 clock: 84 MHz
     * Target PWM frequency: 1 kHz
     * Timer resolution: 16-bit (0..999)
     * 
     * Formula:
     *   PWM_clock = APB1_clock / (PSC + 1)
     *   PWM_freq = PWM_clock / (ARR + 1)
     * 
     * We want: 1000 Hz với ARR = 999 (1000 counts)
     *   1000 = (84,000,000 / (PSC + 1)) / 1000
     *   1000 * (PSC + 1) = 84,000
     *   PSC + 1 = 84
     *   PSC = 83
     */
    
    /* Enable TIM2 clock */
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    /* Initialize timer */
    TIM_HandleTypeDef htim2_config = {0};
    htim2_config.Instance = TIM2;
    htim2_config.Init.Prescaler = 83;           /* PSC = 83 */
    htim2_config.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2_config.Init.Period = 999;             /* ARR = 999 (1000-1) */
    htim2_config.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    
    HAL_TIM_PWM_Init(&htim2_config);
    
    /* Configure PWM channels */
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;                        /* Initial duty = 0 */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH; /* Active high */
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    /* Channel 1 (Motor A EN) */
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    
    /* Channel 2 (Motor B EN) */
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    
    /* Copy back global handle (should be done by SM_CubeMX but we do manually) */
    htim2 = htim2_config;
}

/**
 * @brief Initialize ADC for current sensing
 */
static void l298_adc_init(void)
{
    /* TODO: ADC initialization code */
    /* This would include:
     * 1. Enable ADC clock
     * 2. Configure ADC channels (PA3, PA7)
     * 3. Set sampling time
     * 4. Enable ADC
     */
}

/* ============================================================================
 * STEP 4: WRAPPER FUNCTIONS (Từ l298_driver.c vào HAL layer)
 * ============================================================================
 * 
 * Những hàm này được gọi từ l298_driver.c khi chúng ta cần HAL operations
 */

/**
 * @brief HAL wrapper để initialize L298 driver (được gọi từ L298_Init)
 */
L298_Status l298_hal_init(void)
{
    l298_gpio_init();
    l298_timer_init();
    l298_adc_init();
    return L298_OK;
}

/**
 * @brief HAL wrapper để deinitialize (cleanup)
 */
L298_Status l298_hal_deinit(void)
{
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    return L298_OK;
}

/**
 * @brief Read ADC value cho current sensing
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - ADC blocking read: Chờ cho kết quả
 * - Timeout handling: ADC timeout safety
 */
uint16_t l298_read_adc(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;                    /* ADC_CHANNEL_3 hoặc _7 */
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    /* Start ADC conversion */
    HAL_ADC_Start(&hadc1);
    
    /* Wait for conversion complete (timeout 1000ms) */
    if (HAL_ADC_PollForConversion(&hadc1, 1000) != HAL_OK) {
        return 0;  /* Timeout! */
    }
    
    /* Get ADC value (0..4095 for 12-bit) */
    uint16_t adc_value = HAL_ADC_GetValue(&hadc1);
    
    HAL_ADC_Stop(&hadc1);
    
    return adc_value;
}

/* ============================================================================
 * STEP 5: MAIN EXAMPLE - Cách sử dụng L298 driver
 * ============================================================================
 */

/**
 * @brief Example main function demonstrating L298 usage
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Error handling: Check return codes
 * - Control flow: State machine sử dụng if/switch
 * - Infinite loop: Typical RTOS pattern
 */
void l298_example_main(void)
{
    /* ========================================================================
     * STEP 5.1: DECLARE & INITIALIZE
     * ======================================================================== */
    
    L298_Driver motor_driver;
    
    /* Channel A configuration */
    L298_ChannelConfig config_a = {
        .in1_pin           = L298_A_IN1_PIN,
        .in2_pin           = L298_A_IN2_PIN,
        .enable_pin        = L298_A_EN_PIN,
        .pwm_frequency_hz  = 1000,
        .pwm_max_duty      = 999,
        .deadzone_percent  = 5,
        .speed_correction  = 1.0f
    };
    
    /* Channel B configuration */
    L298_ChannelConfig config_b = {
        .in1_pin           = L298_B_IN3_PIN,
        .in2_pin           = L298_B_IN4_PIN,
        .enable_pin        = L298_B_EN_PIN,
        .pwm_frequency_hz  = 1000,
        .pwm_max_duty      = 999,
        .deadzone_percent  = 5,
        .speed_correction  = 1.0f
    };
    
    /* Initialize driver */
    L298_Status status = L298_Init(&motor_driver, &config_a, &config_b);
    if (status != L298_OK) {
        /* Initialization failed - blink error LED? */
        while (1) {
            /* Error loop */
        }
    }
    
    /* ========================================================================
     * STEP 5.2: CONTROL MOTORS
     * ======================================================================== */
    
    /* Motor A: 75% forward */
    L298_SetSpeed(&motor_driver, L298_CHANNEL_A, 75);
    
    /* Motor B: 50% reverse */
    L298_SetSpeed(&motor_driver, L298_CHANNEL_B, -50);
    
    /* ========================================================================
     * STEP 5.3: MONITOR LOOP (Typical RTOS task)
     * ======================================================================== */
    
    uint32_t last_time = HAL_GetTick();
    
    while (1) {
        uint32_t current_time = HAL_GetTick();
        
        /* Run every 100ms */
        if ((current_time - last_time) >= 100) {
            last_time = current_time;
            
            /* Read motor states */
            uint16_t speed_a = L298_GetSpeed(&motor_driver, L298_CHANNEL_A);
            uint16_t speed_b = L298_GetSpeed(&motor_driver, L298_CHANNEL_B);
            
            printf("Motor A: %d%%, Motor B: %d%%\n", speed_a, speed_b);
            
            /* Optionally read currents */
            uint16_t current_a = L298_ReadCurrent(&motor_driver, L298_CHANNEL_A);
            uint16_t current_b = L298_ReadCurrent(&motor_driver, L298_CHANNEL_B);
            
            printf("Current A: %d, Current B: %d\n", current_a, current_b);
            
            /* Simple state machine for testing */
            static int state = 0;
            
            switch (state) {
                case 0:
                    /* Ramp up Motor A */
                    L298_SetSpeed(&motor_driver, L298_CHANNEL_A, 50);
                    state = 1;
                    break;
                
                case 1:
                    /* Ramp up Motor B */
                    L298_SetSpeed(&motor_driver, L298_CHANNEL_B, 50);
                    state = 2;
                    break;
                
                case 2:
                    /* Reverse Motor A */
                    L298_SetSpeed(&motor_driver, L298_CHANNEL_A, -75);
                    state = 3;
                    break;
                
                case 3:
                    /* Brake Motor A */
                    L298_Brake(&motor_driver, L298_CHANNEL_A);
                    state = 0;
                    break;
            }
        }
        
        /* Yield to other tasks (RTOS) */
        HAL_Delay(10);
    }
    
    /* Cleanup (normally never reached) */
    L298_Deinit(&motor_driver);
}

/* ============================================================================
 * STEP 6: EXAMPLE ISR - How to integrate with interrupts
 * ============================================================================
 */

/**
 * @brief Example: UART command handler for motor control
 * 
 * Allows remote control via UART (e.g., PC sends commands)
 */
void uart_motor_command_handler(uint8_t cmd_byte)
{
    extern L298_Driver g_motor_driver;  /* Global driver instance */
    
    /*
     * Command format: 1 byte
     * Bit 7-6: Channel (0=A, 1=B)
     * Bit 5-0: Speed (0-63 maps to 0-100%)
     */
    
    uint8_t channel_select = (cmd_byte >> 6) & 0x03;
    uint8_t speed_raw = cmd_byte & 0x3F;
    
    /* Convert 0-63 → 0-100 */
    int16_t speed_percent = (speed_raw * 100) / 63;
    
    /* Map channel */
    L298_ChannelID channel = (channel_select == 0) ? L298_CHANNEL_A : L298_CHANNEL_B;
    
    /* Set motor speed */
    L298_Status status = L298_SetSpeed(&g_motor_driver, channel, speed_percent);
    
    if (status == L298_OK) {
        printf("Motor %c speed set to %d%%\n", 
               (channel == L298_CHANNEL_A) ? 'A' : 'B',
               speed_percent);
    } else {
        printf("Failed to set motor speed\n");
    }
}

/**
 * @brief Example: Timer interrupt for periodic speed update
 * 
 * Called every 10ms by timer interrupt
 */
void timer_motor_update_isr(void)
{
    extern L298_Driver g_motor_driver;
    
    /* 
     * KIẾN THỨC C NÂNG CAO: ISR best practices
     * - Không dùng printf (too slow in ISR)
     * - Các operations phải nhanh
     * - Update persistent variables (volatile)
     */
    
    /* Update speed based on sensor feedback (optional PID) */
    static int16_t target_speed_a = 50;
    L298_SetSpeed(&g_motor_driver, L298_CHANNEL_A, target_speed_a);
}

/* ============================================================================
 * END OF EXAMPLE FILE
 * ============================================================================ */
