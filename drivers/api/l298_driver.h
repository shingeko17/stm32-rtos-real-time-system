/**
 * @file l298_driver.h
 * @brief L298 Dual H-Bridge Driver
 * @details Driver cho module L298 điều khiển 2 DC motor độc lập
 *          với các kỹ thuật C nâng cao (typedef, struct, union, volatile)
 * @author Advanced STM32 Driver Team
 * @date 2026-03-31
 * 
 * ============================================================================
 * KIẾN THỨC C NÂNG CAO ĐƯỢC SỬ DỤNG:
 * ============================================================================
 * 1. typedef struct  - Định nghĩa cấu trúc dữ liệu cho từng channel
 * 2. volatile        - Đánh dấu các biến thay đổi từ bên ngoài (ISR, hardware)
 * 3. union           - Cho phép truy cập giá trị theo nhiều cách khác nhau
 * 4. bit fields      - Packed storage cho flags và control bits
 * 5. enum            - Type-safe constants cho các state
 * 6. macro functions - Bit manipulation, configuration
 * 7. function ptr    - Callbacks cho interrupt handlers
 * 8. extern & static - Scope management (public API vs internal functions)
 * 
 * ============================================================================
 * KIỂM SOÁT MOTOR LOGIC L298:
 * ============================================================================
 * | EN | IN1 | IN2 | Hành động    | Ý nghĩa                               |
 * |----|-----|-----|--------------|--------------------------------------|
 * | 0  | X   | X   | Free run     | Motor quay tự do (không có động lực) |
 * | 1  | 0   | 0   | Brake        | Hãm: cả hai output kết nối âm        |
 * | 1  | 1   | 0   | Forward      | Tiến: OUT1=+, OUT2=-                |
 * | 1  | 0   | 1   | Reverse      | Lùi: OUT1=-, OUT2=+                 |
 * | 1  | 1   | 1   | Brake        | Hãm: cả hai output kết nối âm        |
 * ============================================================================
 */

#ifndef L298_DRIVER_H
#define L298_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * STEP 1: ENUM TYPE - Kiểu Safe Constants (Enumeration)
 * ============================================================================
 * 
 * Enum là cách an toàn hơn #define vì:
 * - Có type checking
 * - Dễ debug
 * - IDE tự động complete
 */

/**
 * @enum L298_MotorState
 * @brief Trạng thái của motor
 * 
 * Sử dụng enum thay vì magic numbers:
 * - MOTOR_FREE_RUN (0)  : Motor quay tự do
 * - MOTOR_FORWARD (1)   : Tiến
 * - MOTOR_REVERSE (2)   : Lùi
 * - MOTOR_BRAKE (3)     : Hãm
 */
typedef enum {
    MOTOR_FREE_RUN = 0,    /* EN=0, motor quay không điều khiển          */
    MOTOR_FORWARD  = 1,    /* IN1=1, IN2=0, motor tiến                  */
    MOTOR_REVERSE  = 2,    /* IN1=0, IN2=1, motor lùi                   */
    MOTOR_BRAKE    = 3     /* IN1=1 hoặc 0, IN2 ngược, motor hãm         */
} L298_MotorState;

/**
 * @enum L298_ChannelID
 * @brief ID của channel motor (Channel A hoặc B)
 */
typedef enum {
    L298_CHANNEL_A = 0,    /* Bridge A: IN1, IN2, ENA, OUT1, OUT2       */
    L298_CHANNEL_B = 1     /* Bridge B: IN3, IN4, ENB, OUT3, OUT4       */
} L298_ChannelID;

/**
 * @enum L298_Status
 * @brief Status return code của các hàm driver
 */
typedef enum {
    L298_OK         = 0,   /* Thành công                                 */
    L298_ERROR      = 1,   /* Lỗi chung                                  */
    L298_INVALID_CH = 2,   /* Channel ID không hợp lệ                    */
    L298_NOT_INIT   = 3    /* Driver chưa được init                      */
} L298_Status;

/* ============================================================================
 * STEP 2: STRUCT & UNION - Advanced Data Structures
 * ============================================================================
 * 
 * Struct:
 * - Cho phép nhóm các dữ liệu liên quan lại
 * - Dễ hơn truyền nhiều parameters riêng lẻ
 * 
 * Union:
 * - Cho phép giá trị có thể được truy cập theo nhiều cách
 * - Tiết kiệm bộ nhớ (nhưng chỉ dùng 1 member tại 1 thời điểm)
 * - Hữu ích cho hardware control (bit-level vs byte-level)
 */

/**
 * @struct L298_ControlBits
 * @brief Union để điều khiển IN1, IN2, EN bằng cả bit-level và byte-level
 * 
 * Ví dụ:
 *   L298_ControlBits ctrl;
 *   ctrl.byte = 0x05;      // Thiết lập byte (bit-wise)
 *   if (ctrl.bits.en) ...  // Kiểm tra EN bit
 *   ctrl.bits.in1 = 1;     // Thiết lập IN1 bit
 */
typedef union {
    uint8_t byte;          /* Toàn bộ 8 bits (cho truyền SPI/I2C) */
    struct {
        uint8_t in1 : 1;   /* Bit 0: IN1 control (1-bit field)    */
        uint8_t in2 : 1;   /* Bit 1: IN2 control (1-bit field)    */
        uint8_t en  : 1;   /* Bit 2: Enable control (1-bit field) */
        uint8_t reserved : 5; /* Bits 3-7: dành riêng           */
    } bits;                /* Access individual bits              */
} L298_ControlBits;

/**
 * @struct L298_ChannelConfig
 * @brief Cấu hình cho 1 channel (A hoặc B)
 * 
 * Chứa pinout GPIO và các thông số cấu hình cho từng channel
 */
typedef struct {
    /* GPIO Pins */
    uint32_t in1_pin;           /* Pin IN1 GPIO (VD: GPIO_PIN_0)    */
    uint32_t in2_pin;           /* Pin IN2 GPIO (VD: GPIO_PIN_1)    */
    uint32_t enable_pin;        /* Pin EN GPIO (VD: GPIO_PIN_2)     */
    
    /* PWM Configuration */
    uint16_t pwm_frequency_hz;  /* Tần số PWM (VD: 1000 Hz)        */
    uint16_t pwm_max_duty;      /* Max duty cycle (VD: 999 cho 16-bit TIM) */
    
    /* Calibration */
    uint8_t deadzone_percent;   /* Deadzone % dưới đó motor không động */
    float   speed_correction;   /* Hệ số hiệu chỉnh tốc độ (1.0 = 100%) */
} L298_ChannelConfig;

/**
 * @struct L298_ChannelState
 * @brief Runtime state của channel (thay đổi trong quá trình chạy)
 * 
 * volatile: Báo cho compiler rằng giá trị này có thể thay đổi 
 *          từ ISR hoặc hardware event
 */
typedef struct {
    volatile L298_MotorState state;    /* Trạng thái hiện tại (FORWARD/REVERSE/...) */
    volatile uint16_t current_duty;    /* Duty cycle hiện tại (0..pwm_max_duty)    */
    volatile uint16_t current_speed;   /* Tốc độ hiện tại (0..100%)                */
    volatile uint32_t timestamp_ms;    /* Timestamp lần cập nhật cuối cùng        */
} L298_ChannelState;

/**
 * @struct L298_Driver
 * @brief Cấu trúc chính để quản lý cả L298 (2 channels)
 * 
 * Đây là "driver context" - chứa tất cả thông tin về driver
 * Cho phép có nhiều instances của L298 driver nếu cần (multi-motor systems)
 */
typedef struct {
    /* Channel A Config & State */
    L298_ChannelConfig config_a;
    L298_ChannelState  state_a;
    
    /* Channel B Config & State */
    L298_ChannelConfig config_b;
    L298_ChannelState  state_b;
    
    /* Driver Status */
    volatile bool initialized;        /* Đã khởi tạo? */
    volatile bool error_occurred;     /* Có lỗi nào không? */
    volatile uint32_t error_code;     /* Error code nếu có */
    
    /* Statistics */
    volatile uint32_t total_commands; /* Tổng số command được gửi */
    volatile uint32_t total_errors;   /* Tổng số errors xảy ra */
} L298_Driver;

/* ============================================================================
 * STEP 3: BIT MANIPULATION MACROS - Advance Bitwise Operations
 * ============================================================================
 * 
 * Macros để thao tác bit một cách rõ ràng và hiệu quả
 * Thường dùng trong driver hardware-level
 */

#define BIT_SET(byte, bit_pos)     ((byte) |= (1 << (bit_pos)))
#define BIT_CLR(byte, bit_pos)     ((byte) &= ~(1 << (bit_pos)))
#define BIT_TOG(byte, bit_pos)     ((byte) ^= (1 << (bit_pos)))
#define BIT_READ(byte, bit_pos)    (((byte) >> (bit_pos)) & 1)

/* ============================================================================
 * STEP 4: CALLBACK FUNCTION POINTERS - For Future ISR Handling
 * ============================================================================
 * 
 * Function pointers cho phép user code để handle events callback
 * Ví dụ: khi motor đạt tốc độ target
 */

/**
 * @typedef L298_EventCallback
 * @brief Function pointer type cho event callbacks
 * 
 * @param[in] driver    Pointer đến L298_Driver instance
 * @param[in] channel   Channel ID (A hoặc B)
 * @param[in] user_ctx  User context pointer (VD: task struct)
 */
typedef void (*L298_EventCallback)(L298_Driver *driver, 
                                    L298_ChannelID channel, 
                                    void *user_ctx);

/* ============================================================================
 * STEP 5: PUBLIC API - Driver Functions
 * ============================================================================
 */

/**
 * @brief Khởi tạo L298 driver
 * 
 * @param[in,out] driver    Pointer đến L298_Driver struct (cần allocate sẵn)
 * @param[in]     config_a  Config cho channel A (NULL = dùng default)
 * @param[in]     config_b  Config cho channel B (NULL = dùng default)
 * 
 * @return L298_Status      L298_OK nếu thành công, else error code
 * 
 * @details Hàm này:
 *  1. Kiểm tra pointer driver
 *  2. Khởi tạo GPIO pins
 *  3. Khởi tạo PWM timer
 *  4. Set initialized flag
 *  5. Reset state A & B
 */
extern L298_Status L298_Init(L298_Driver *driver,
                              const L298_ChannelConfig *config_a,
                              const L298_ChannelConfig *config_b);

/**
 * @brief Dừng hoạt động L298 (cleanup)
 * 
 * @param[in,out] driver    Pointer đến L298_Driver struct
 * 
 * @return L298_Status      L298_OK nếu thành công
 * 
 * @details Hàm này:
 *  1. Stop all PWM
 *  2. Reset tất cả pins
 *  3. Deinitialize GPIO
 *  4. Clear initialized flag
 */
extern L298_Status L298_Deinit(L298_Driver *driver);

/**
 * @brief Điều khiển motor speed (tốc độ % từ -100 đến +100)
 * 
 * @param[in,out] driver    Pointer đến L298_Driver struct
 * @param[in]     channel   Channel ID (L298_CHANNEL_A hoặc L298_CHANNEL_B)
 * @param[in]     speed     Tốc độ: [-100, +100]
 *                          - Âm: lùi
 *                          - 0: dừng
 *                          - Dương: tiến
 * 
 * @return L298_Status      L298_OK nếu thành công
 * 
 * @details Hàm này:
 *  1. Kiểm tra channel validity
 *  2. Kiểm tra driver initialized
 *  3. Xác định hướng (FORWARD/REVERSE) từ dấu speed
 *  4. Chuyển |speed| → duty cycle
 *  5. Cập nhật IN1, IN2, ENA pins
 *  6. Update state struct
 *  7. Gọi PWM timer update
 */
extern L298_Status L298_SetSpeed(L298_Driver *driver,
                                  L298_ChannelID channel,
                                  int16_t speed);

/**
 * @brief Đặt motor ở trạng thái freerun (không có điều khiển)
 * 
 * @param[in,out] driver    Pointer đến L298_Driver struct
 * @param[in]     channel   Channel ID
 * 
 * @return L298_Status      L298_OK nếu thành công
 * 
 * @details Set EN=0, motor coasts freely
 */
extern L298_Status L298_FreeRun(L298_Driver *driver, L298_ChannelID channel);

/**
 * @brief Đặt motor ở trạng thái brake (hãm)
 * 
 * @param[in,out] driver    Pointer đến L298_Driver struct
 * @param[in]     channel   Channel ID
 * 
 * @return L298_Status      L298_OK nếu thành công
 * 
 * @details Set IN1=IN2 (cùng mức), motor dừng ngay
 */
extern L298_Status L298_Brake(L298_Driver *driver, L298_ChannelID channel);

/**
 * @brief Lấy trạng thái hiện tại của channel
 * 
 * @param[in] driver    Pointer đến L298_Driver struct
 * @param[in] channel   Channel ID
 * 
 * @return L298_MotorState Trạng thái hiện tại hoặc MOTOR_FREE_RUN nếu error
 * 
 * @note Hàm này chỉ đọc, không thay đổi state
 */
extern L298_MotorState L298_GetState(const L298_Driver *driver,
                                      L298_ChannelID channel);

/**
 * @brief Lấy tốc độ hiện tại (%)
 * 
 * @param[in] driver    Pointer đến L298_Driver struct
 * @param[in] channel   Channel ID
 * 
 * @return uint16_t     Tốc độ (0..100%) hoặc 0 nếu error
 */
extern uint16_t L298_GetSpeed(const L298_Driver *driver,
                               L298_ChannelID channel);

/**
 * @brief Đọc dòng điện hiện tại đi qua channel (nếu có SENSE pin)
 * 
 * @param[in] driver    Pointer đến L298_Driver struct
 * @param[in] channel   Channel ID
 * 
 * @return uint16_t     Giá trị ADC (0..4095 cho 12-bit) hoặc 0 nếu error
 * 
 * @note Yêu cầu: SENSE pin phải được cấu hình và ADC phải hoạt động
 */
extern uint16_t L298_ReadCurrent(const L298_Driver *driver,
                                  L298_ChannelID channel);

/**
 * @brief Cập nhật duty cycle bằng cách trực tiếp (nâng cao)
 * 
 * @param[in,out] driver    Pointer đến L298_Driver struct
 * @param[in]     channel   Channel ID
 * @param[in]     duty      Duty cycle (0..pwm_max_duty)
 * 
 * @return L298_Status      L298_OK nếu thành công
 * 
 * @note Hàm này cho advanced user - bypass tính toán speed %
 */
extern L298_Status L298_SetDuty(L298_Driver *driver,
                                 L298_ChannelID channel,
                                 uint16_t duty);

/* ============================================================================
 * STEP 6: MACRO HELPERS - Inline convenience functions
 * ============================================================================
 */

#define L298_IsInitialized(driver)  ((driver)->initialized)
#define L298_HasError(driver)       ((driver)->error_occurred)
#define L298_GetErrorCode(driver)   ((driver)->error_code)

#ifdef __cplusplus
}
#endif

#endif /* L298_DRIVER_H */
