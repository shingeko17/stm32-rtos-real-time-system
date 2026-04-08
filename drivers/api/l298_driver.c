/**
 * @file l298_driver.c
 * @brief L298 Driver - Implementation
 * @details Triển khai chi tiết API driver với các kỹ thuật C nâng cao
 * @author Advanced STM32 Driver Team
 * @date 2026-03-31
 * 
 * ============================================================================
 * KIẾN THỨC C NÂNG CAO TRONG IMPLEMENTATION:
 * ============================================================================
 * 1. Static Global Variables - Quản lý state toàn cục nhưng "private"
 * 2. Static Functions - Hàm nội bộ (internal helper functions)
 * 3. Pointer Dereference - Truy cập thành viên struct qua con trỏ
 * 4. Bitwise Operations - AND, OR, XOR cho bit-level control
 * 5. Conditional Compilation - #if, #ifdef cho optional features
 * 6. Const Correctness - const pointers và const parameters
 * 7. Inline Functions - Optimization cho small functions
 * 8. Error Propagation - Return error codes, không dùng exceptions
 * 
 * ============================================================================
 */

#include "l298_driver.h"
#include <string.h>  /* memset, memcpy */

/* ============================================================================
 * STEP 1: STATIC GLOBAL VARIABLES - Quản lý driver instance
 * ============================================================================
 * 
 * static:
 * - Giới hạn scope của variable chỉ trong file này
 * - Giống private keyword trong C++
 * - Cho phép có multiple instances nếu cần
 */

/** Default configuration cho channel - sử dụng khi user không cung cấp */
static const L298_ChannelConfig L298_DEFAULT_CONFIG = {
    .in1_pin             = 0,        /* Cần set bởi L298_Init() */
    .in2_pin             = 1,
    .enable_pin          = 2,
    .pwm_frequency_hz    = 1000,     /* 1 kHz PWM */
    .pwm_max_duty        = 999,      /* 16-bit timer max */
    .deadzone_percent    = 5,        /* Deadzone 5% */
    .speed_correction    = 1.0f      /* No correction */
};

/* ============================================================================
 * STEP 2: STATIC FORWARD DECLARATIONS - Internal Functions
 * ============================================================================
 * 
 * Khai báo trước các hàm nội bộ (static functions)
 * Có thể định nghĩa sau nhưng cần declare trước để compiler biết
 */

/**
 * @brief Helper: Chuyển speed (%) sang duty cycle
 * 
 * @param[in] driver    Driver struct
 * @param[in] channel   Channel ID
 * @param[in] speed     Speed %, chỉ phần dương (0..100)
 * 
 * @return uint16_t     Duty cycle
 */
static uint16_t l298_speed_to_duty(const L298_Driver *driver,
                                    L298_ChannelID channel,
                                    uint16_t speed);

/**
 * @brief Helper: Set physical GPIO pins (IN1, IN2, EN) để điều khiển motor
 * 
 * @param[in] driver    Driver struct
 * @param[in] channel   Channel ID
 * @param[in] in1_state IN1 pin state (0 or 1)
 * @param[in] in2_state IN2 pin state (0 or 1)
 * @param[in] en_state  EN pin state (0 or 1)
 */
static void l298_set_pins(const L298_Driver *driver,
                           L298_ChannelID channel,
                           uint8_t in1_state, uint8_t in2_state, uint8_t en_state);

/**
 * @brief Helper: Set PWM duty cycle trên timer
 * 
 * @param[in] driver    Driver struct
 * @param[in] channel   Channel ID
 * @param[in] duty      Duty cycle value (0..max_duty)
 */
static void l298_set_pwm_duty(const L298_Driver *driver,
                               L298_ChannelID channel,
                               uint16_t duty);

/**
 * @brief Helper: Lấy config struct cho channel
 * 
 * @param[in] driver    Driver struct
 * @param[in] channel   Channel ID
 * 
 * @return Pointer tới config struct, hoặc NULL nếu channel invalid
 */
static L298_ChannelConfig* l298_get_config(L298_Driver *driver,
                                            L298_ChannelID channel);

/**
 * @brief Helper: Lấy state struct cho channel (const version)
 * 
 * @param[in] driver    Driver struct
 * @param[in] channel   Channel ID
 * 
 * @return const Pointer tới state struct
 */
static const L298_ChannelState* l298_get_state_const(const L298_Driver *driver,
                                                      L298_ChannelID channel);

/**
 * @brief Helper: Lấy state struct cho channel (mutable version)
 * 
 * @param[in] driver    Driver struct
 * @param[in] channel   Channel ID
 * 
 * @return Pointer tới state struct
 */
static L298_ChannelState* l298_get_state_mut(L298_Driver *driver,
                                              L298_ChannelID channel);

/**
 * @brief Helper: Kiểm tra channel validity
 * 
 * @param[in] channel   Channel ID
 * 
 * @return true nếu channel valid (0 hoặc 1), false otherwise
 */
static inline bool l298_is_valid_channel(L298_ChannelID channel);

/* ============================================================================
 * STEP 3: IMPLEMENTATION HÀM HELPER (STATIC FUNCTIONS)
 * ============================================================================
 */

/**
 * KIẾN THỨC C NÂNG CAO:
 * - static inline: Hàm nhỏ, compiler sẽ inline (no function call overhead)
 * - Bitwise check: (channel <= 1) tương với (channel == A || channel == B)
 */
static inline bool l298_is_valid_channel(L298_ChannelID channel)
{
    /* Chỉ có 2 channels: 0 (A) hoặc 1 (B) */
    return (channel <= L298_CHANNEL_B);
}

/**
 * KIẾN THỨC C NÂNG CAO:
 * - Pointer dereferencing: driver->config_a để truy cập struct member qua con trỏ
 * - Conditional return: Return địa chỉ của config, NULL nếu invalid
 * - const correctness: Parameter "L298_Driver *" để cho phép modify
 */
static L298_ChannelConfig* l298_get_config(L298_Driver *driver,
                                            L298_ChannelID channel)
{
    /* Nếu channel không valid, return NULL */
    if (!l298_is_valid_channel(channel)) {
        return NULL;
    }
    
    /* Return address của config_a hoặc config_b dựa theo channel */
    return (channel == L298_CHANNEL_A) ? &driver->config_a : &driver->config_b;
}

/**
 * KIẾN THỨC C NÂNG căng:
 * - const pointer: Không thể modify giá trị qua pointer này
 * - Ternary operator: Inline condition
 */
static const L298_ChannelState* l298_get_state_const(const L298_Driver *driver,
                                                      L298_ChannelID channel)
{
    if (!l298_is_valid_channel(channel)) {
        return NULL;
    }
    
    return (channel == L298_CHANNEL_A) ? &driver->state_a : &driver->state_b;
}

/**
 * KIẾN THỨC C NÂNG CAO:
 * - Mutable version của hàm: cho phép modify state
 * - Mutable pointer: Không có const keyword
 */
static L298_ChannelState* l298_get_state_mut(L298_Driver *driver,
                                              L298_ChannelID channel)
{
    if (!l298_is_valid_channel(channel)) {
        return NULL;
    }
    
    return (channel == L298_CHANNEL_A) ? &driver->state_a : &driver->state_b;
}

/**
 * chuyển conversion từ speed (%) sang duty cycle
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Clamping: Đảm bảo giá trị nằm trong range
 * - Scaling: Chuyển đổi từ 1 range (0..100) sang range khác (0..max_duty)
 * - Công thức: duty = (speed * max_duty) / 100
 * 
 * Ví dụ:
 *   speed = 50%, max_duty = 999
 *   duty = (50 * 999) / 100 = 49950 / 100 = 499 (≈ 50%)
 */
static uint16_t l298_speed_to_duty(const L298_Driver *driver,
                                    L298_ChannelID channel,
                                    uint16_t speed)
{
    const L298_ChannelConfig *config = l298_get_config((L298_Driver *)driver, channel);
    
    /* Nếu config NULL, return 0 */
    if (config == NULL) {
        return 0;
    }
    
    /* Clamp speed vào [0, 100] */
    if (speed > 100) {
        speed = 100;
    }
    
    /* Scaling: (speed * max_duty) / 100 */
    uint32_t duty = ((uint32_t)speed * config->pwm_max_duty) / 100;
    
    return (uint16_t)duty;
}

/**
 * @brief Set physical GPIO pins
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - GPIO được biểu diễn bởi structure HAL
 * - Chúng ta cần gọi HAL functions để set pins
 * - Thực tế, cái này phụ thuộc vào STM32Cube/HAL layer mà bạn dùng
 * 
 * NOTE: Implementation thực tế phụ thuộc vào STM32 HAL version
 * Ở đây chỉ là placeholder - bạn cần thay bằng call HAL thực tế
 * VD: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, in1_state ? GPIO_PIN_SET : GPIO_PIN_RESET)
 */
static void l298_set_pins(const L298_Driver *driver,
                           L298_ChannelID channel,
                           uint8_t in1_state, uint8_t in2_state, uint8_t en_state)
{
    L298_ChannelConfig *config = l298_get_config((L298_Driver *)driver, channel);
    
    if (config == NULL) return;
    
    /* TODO: Implement actual GPIO setting based on STM32 HAL
     * 
     * Example for STM32F4:
     * GPIO_TypeDef *gpio_port = GPIOA;  // Đơn giản hóa
     * 
     * if (in1_state) {
     *     HAL_GPIO_WritePin(gpio_port, config->in1_pin, GPIO_PIN_SET);
     * } else {
     *     HAL_GPIO_WritePin(gpio_port, config->in1_pin, GPIO_PIN_RESET);
     * }
     * 
     * // Tương tự cho in2_state, en_state...
     */
}

/**
 * @brief Set PWM duty cycle
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - PWM được control bằng Timer peripheral
 * - Mỗi channel có 1 compare register (TIMx_CCR)
 * - Chúng ta update CCR để thay đổi duty cycle
 * 
 * NOTE: Implementation phụ thuộc vào STM32 HAL
 */
static void l298_set_pwm_duty(const L298_Driver *driver,
                               L298_ChannelID channel,
                               uint16_t duty)
{
    /* TODO: Implement actual PWM update based on STM32 HAL
     * 
     * Example:
     * TIM_HandleTypeDef *htim = &htim2;  // Using Timer 2
     * uint32_t channel_hal = (channel == L298_CHANNEL_A) ? TIM_CHANNEL_1 : TIM_CHANNEL_2;
     * 
     * __HAL_TIM_SET_COMPARE(htim, channel_hal, duty);
     */
}

/* ============================================================================
 * STEP 4: PUBLIC API IMPLEMENTATIONS
 * ============================================================================
 */

/**
 * @brief Khởi tạo L298 driver
 * 
 * KIẾN THỨC C NÂNG CAO:
 * 1. Parameter validation: Checking NULL pointers trước khi dùng
 * 2. memset: Xóa bộ nhớ struct
 * 3. Conditional copy: Nếu config == NULL, dùng default
 * 4. Error codes: Return L298_Status instead of using exceptions
 * 5. State initialization: Set up tất cả fields
 */
L298_Status L298_Init(L298_Driver *driver,
                       const L298_ChannelConfig *config_a,
                       const L298_ChannelConfig *config_b)
{
    /* ========================================================================
     * STEP 4.1: INPUT VALIDATION
     * ======================================================================== */
    
    if (driver == NULL) {
        return L298_ERROR;  /* Cannot init NULL pointer */
    }
    
    /* ========================================================================
     * STEP 4.2: CLEAR DRIVER STRUCT (Zero initialization)
     * ========================================================================
     * 
     * KIẾN THỨC C NÂNG CAO:
     * - memset: Điền 0 vào toàn bộ memory block
     * - sizeof(L298_Driver): Lấy size của struct (compile-time)
     * - Null coalescing: Dùng default nếu config == NULL
     */
    memset(driver, 0, sizeof(L298_Driver));
    
    /* ========================================================================
     * STEP 4.3: COPY CONFIGURATIONS
     * ======================================================================== */
    
    if (config_a != NULL) {
        /*
         * KIẾN THỨC C NÂNG CAO: memcpy
         * - Sao chép byte-by-byte từ source (config_a) tới destination (&driver->config_a)
         * - sizeof(...) là compile-time constant
         */
        memcpy(&driver->config_a, config_a, sizeof(L298_ChannelConfig));
    } else {
        /* Nếu config_a NULL, dùng default */
        memcpy(&driver->config_a, &L298_DEFAULT_CONFIG, sizeof(L298_ChannelConfig));
    }
    
    if (config_b != NULL) {
        memcpy(&driver->config_b, config_b, sizeof(L298_ChannelConfig));
    } else {
        memcpy(&driver->config_b, &L298_DEFAULT_CONFIG, sizeof(L298_ChannelConfig));
    }
    
    /* ========================================================================
     * STEP 4.4: INITIALIZE GPIO PINS
     * ======================================================================== */
    
    /*
     * TODO: Thêm HAL calls để:
     * 1. Enable GPIO port clocking
     * 2. Configure pins thành output mode
     * 3. Set initial pin states
     */
    
    /* ========================================================================
     * STEP 4.5: INITIALIZE PWM TIMER
     * ======================================================================== */
    
    /*
     * TODO: Thêm HAL calls để:
     * 1. Enable timer clock
     * 2. Configure timer frequency
     * 3. Start timer PWM
     */
    
    /* ========================================================================
     * STEP 4.6: SET INITIAL STATE
     * ======================================================================== */
    
    /*
     * KIẾN THỨC C NÂNG CAO: volatile access
     * - Báo compiler rằng giá trị này có thể thay đổi ngoài expected flow
     * - Compiler sẽ không optimize dead code
     */
    driver->state_a.state = MOTOR_FREE_RUN;
    driver->state_a.current_duty = 0;
    driver->state_a.current_speed = 0;
    driver->state_a.timestamp_ms = 0;
    
    driver->state_b.state = MOTOR_FREE_RUN;
    driver->state_b.current_duty = 0;
    driver->state_b.current_speed = 0;
    driver->state_b.timestamp_ms = 0;
    
    /* ========================================================================
     * STEP 4.7: SET DRIVER STATUS FLAGS
     * ======================================================================== */
    
    driver->initialized = true;
    driver->error_occurred = false;
    driver->error_code = 0;
    driver->total_commands = 0;
    driver->total_errors = 0;
    
    return L298_OK;
}

/**
 * @brief Deinit L298 driver
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Cleanup: Đặt tất cả pins về trạng thái safe
 * - Deinitialize: Tắt timer, GPIO
 * - Error handling: Cơ chế rollback nếu deinit fail
 */
L298_Status L298_Deinit(L298_Driver *driver)
{
    if (driver == NULL) {
        return L298_ERROR;
    }
    
    if (!driver->initialized) {
        return L298_NOT_INIT;
    }
    
    /* TODO: 
     * 1. Stop PWM timers
     * 2. Reset GPIO pins to default state
     * 3. Disable GPIO port clocking
     */
    
    driver->initialized = false;
    driver->error_code = 0;
    
    return L298_OK;
}

/**
 * @brief Điều khiển motor speed
 * 
 * KIẾN THỨC C NÂNG CAO:
 * 1. Input validation: Kiểm tra channel, driver state
 * 2. Sign handling: Tách "dấu" (direction) từ "độ lớn" (speed)
 * 3. State machine: Chuyển từ state này sang state khác
 * 4. Duty calculation: Từ speed % → duty cycle
 * 5. Pin control: Set IN1, IN2, EN pins dựa trên state
 * 6. volatile update: Update runtime state
 */
L298_Status L298_SetSpeed(L298_Driver *driver,
                           L298_ChannelID channel,
                           int16_t speed)
{
    /* ========================================================================
     * VALIDATION
     * ======================================================================== */
    
    if (driver == NULL) {
        return L298_ERROR;
    }
    
    if (!driver->initialized) {
        return L298_NOT_INIT;
    }
    
    if (!l298_is_valid_channel(channel)) {
        return L298_INVALID_CH;
    }
    
    /* ========================================================================
     * DETERMINE MOTOR STATE BASED ON SPEED SIGN
     * ======================================================================== */
    
    L298_MotorState next_state;
    uint16_t abs_speed;
    
    /*
     * KIẾN THỨC C NÂNG CAO: Conditional state machine
     * - if speed > 0  → FORWARD
     * - if speed < 0  → REVERSE
     * - if speed == 0 → BRAKE
     */
    
    if (speed > 0) {
        next_state = MOTOR_FORWARD;
        abs_speed = (uint16_t)speed;
    } else if (speed < 0) {
        next_state = MOTOR_REVERSE;
        abs_speed = (uint16_t)(-speed);  /* Lấy giá trị tuyệt đối */
    } else {
        next_state = MOTOR_BRAKE;
        abs_speed = 0;
    }
    
    /* ========================================================================
     * CLAMP SPEED TO 0..100
     * ======================================================================== */
    
    if (abs_speed > 100) {
        abs_speed = 100;
    }
    
    /* ========================================================================
     * CONVERT SPEED % TO DUTY CYCLE
     * ======================================================================== */
    
    uint16_t duty = l298_speed_to_duty(driver, channel, abs_speed);
    
    /* ========================================================================
     * SET PINS DỰA TRÊN STATE
     * ======================================================================== */
    
    /*
     * L298 H-Bridge logic:
     * FORWARD:  IN1=1, IN2=0, EN=1
     * REVERSE:  IN1=0, IN2=1, EN=1
     * BRAKE:    IN1=1, IN2=1, EN=1 (hoặc IN1=0, IN2=0)
     * FREERUN:  EN=0 (IN1, IN2 không quan trọng)
     */
    
    switch (next_state) {
        case MOTOR_FORWARD:
            l298_set_pins(driver, channel, 1, 0, 1);  /* IN1=1, IN2=0, EN=1 */
            break;
        
        case MOTOR_REVERSE:
            l298_set_pins(driver, channel, 0, 1, 1);  /* IN1=0, IN2=1, EN=1 */
            break;
        
        case MOTOR_BRAKE:
            l298_set_pins(driver, channel, 1, 1, 1);  /* IN1=1, IN2=1, EN=1 */
            abs_speed = 0;  /* Brake speed = 0 */
            break;
        
        case MOTOR_FREE_RUN:
            l298_set_pins(driver, channel, 0, 0, 0);  /* EN=0 */
            break;
        
        default:
            return L298_ERROR;
    }
    
    /* ========================================================================
     * SET PWM DUTY CYCLE
     * ======================================================================== */
    
    l298_set_pwm_duty(driver, channel, duty);
    
    /* ========================================================================
     * UPDATE RUNTIME STATE
     * ======================================================================== */
    
    /*
     * KIẾN THỨC C NÂNG CAO: Pointer to mutable member
     * - Lấy pointer đến state struct
     * - Thông qua volatile member, update giá trị
     */
    L298_ChannelState *state = l298_get_state_mut(driver, channel);
    if (state != NULL) {
        state->state = next_state;
        state->current_duty = duty;
        state->current_speed = abs_speed;
        /* state->timestamp_ms = HAL_GetTick(); */ /* TODO: Update timestamp */
    }
    
    /* ========================================================================
     * UPDATE STATISTICS
     * ======================================================================== */
    
    driver->total_commands++;
    
    return L298_OK;
}

/**
 * @brief Set motor to free run state
 */
L298_Status L298_FreeRun(L298_Driver *driver, L298_ChannelID channel)
{
    if (driver == NULL || !driver->initialized || !l298_is_valid_channel(channel)) {
        return L298_ERROR;
    }
    
    /*
     * KIẾN THỨC C NÂNG CAO: Delegation
     * - Gọi L298_SetSpeed với speed=0 sẽ không phải brake
     * - Thay vào đó, gọi hàm này trực tiếp để set EN=0
     */
    
    l298_set_pins(driver, channel, 0, 0, 0);  /* Just disable EN */
    
    L298_ChannelState *state = l298_get_state_mut(driver, channel);
    if (state != NULL) {
        state->state = MOTOR_FREE_RUN;
        state->current_duty = 0;
        state->current_speed = 0;
    }
    
    driver->total_commands++;
    return L298_OK;
}

/**
 * @brief Set motor to brake state
 */
L298_Status L298_Brake(L298_Driver *driver, L298_ChannelID channel)
{
    /*
     * KIẾN THỨC C NÂNG CAO: Delegation
     * - Brake = speed 0 (positive 0, không âm)
     */
    return L298_SetSpeed(driver, channel, 0);
}

/**
 * @brief Lấy trạng thái hiện tại
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - const parameter: Không modify driver
 * - Early return: Nếu error, return default value
 * - Const correctness: Dùng const version của helper
 */
L298_MotorState L298_GetState(const L298_Driver *driver,
                               L298_ChannelID channel)
{
    if (driver == NULL || !driver->initialized) {
        return MOTOR_FREE_RUN;
    }
    
    const L298_ChannelState *state = l298_get_state_const(driver, channel);
    if (state == NULL) {
        return MOTOR_FREE_RUN;
    }
    
    return state->state;
}

/**
 * @brief Lấy tốc độ hiện tại (%)
 */
uint16_t L298_GetSpeed(const L298_Driver *driver,
                        L298_ChannelID channel)
{
    if (driver == NULL || !driver->initialized) {
        return 0;
    }
    
    const L298_ChannelState *state = l298_get_state_const(driver, channel);
    if (state == NULL) {
        return 0;
    }
    
    return state->current_speed;
}

/**
 * @brief Đọc dòng điện (ADC value từ SENSE pin)
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Stub for future ADC integration
 * - Return type là uint16_t (ADC resolution)
 */
uint16_t L298_ReadCurrent(const L298_Driver *driver,
                           L298_ChannelID channel)
{
    if (driver == NULL || !driver->initialized) {
        return 0;
    }
    
    /* TODO: 
     * 1. Read ADC value từ SENSE pin (channel A hoặc B)
     * 2. Return value (0..4095 cho 12-bit ADC)
     */
    
    return 0;
}

/**
 * @brief Set duty cycle trực tiếp (advanced)
 * 
 * KIẾN THỨC C NÂNG CAO:
 * - Bypass normal speed % calculation
 * - Direct hardware control
 * - Advanced user API
 */
L298_Status L298_SetDuty(L298_Driver *driver,
                          L298_ChannelID channel,
                          uint16_t duty)
{
    if (driver == NULL || !driver->initialized || !l298_is_valid_channel(channel)) {
        return L298_ERROR;
    }
    
    /* Clamp duty to valid range */
    L298_ChannelConfig *config = l298_get_config(driver, channel);
    if (config == NULL) return L298_ERROR;
    
    if (duty > config->pwm_max_duty) {
        duty = config->pwm_max_duty;
    }
    
    /* Set PWM */
    l298_set_pwm_duty(driver, channel, duty);
    
    /* Update state */
    L298_ChannelState *state = l298_get_state_mut(driver, channel);
    if (state != NULL) {
        state->current_duty = duty;
        /* Calculate speed % từ duty */
        state->current_speed = (uint16_t)((duty * 100) / config->pwm_max_duty);
    }
    
    driver->total_commands++;
    return L298_OK;
}

/* ============================================================================
 * END OF L298_DRIVER.C
 * ============================================================================ */
