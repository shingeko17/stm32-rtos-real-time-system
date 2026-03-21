/**
 * @file bsp_motor.c
 * @brief Board Support Package - L298N Motor Driver Implementation
 * @details Triển khai chi tiết hàm khởi tạo GPIO + PWM và điều khiển motor
 * @author STM32 L298N Driver Team
 * @date 2026-03-14
 */

/* Khai báo extern từ bsp_motor.h */
#include "bsp_motor.h"

/* ============================================================================
 * HÀM KHỞI TẠO GPIO - DIRECTION CONTROL (IN1-IN4)
 * ============================================================================ */

/**
 * @brief Khởi tạo GPIO Port D cho các pin direction (IN1, IN2, IN3, IN4)
 * 
 * @details Hàm này cấu hình:
 *  1. Bật clock cho GPIOD (RCC_AHB1Periph_GPIOD)
 *  2. Cấu hình PD0-PD3 làm GPIO Output
 *     - Mode: Output (không phải alternate function)
 *     - Type: Push-Pull (CMOS output)
 *     - Speed: 50MHz (đủ cho GPIO)
 *     - PullUp/PullDown: NOPULL
 *  3. Reset tất cả pin về LOW (0V) - motor dừng
 * 
 * @return void
 */
static void BSP_Motor_GPIO_Init(void)
{
    /* Khai báo cấu trúc khởi tạo GPIO */
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* ========================================================================
     * BƯỚC 1: Bật clock cho GPIOD trên AHB1 bus
     * ======================================================================== */
    
    /* Gọi hàm RCC để bật clock GPIOD
       - GPIOD là peripheral trên AHB1 (Advanced High performance Bus 1)
       - Tần số AHB1 = 168MHz
       - Mỗi GPIO port có một clock enable bit riêng
    */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    
    /* ========================================================================
     * BƯỚC 2: Cấu hình GPIO_InitStructure cho tất cả 4 pin IN
     * ======================================================================== */
    
    /* Chọn các pin: PD0, PD1, PD2, PD3 (tất cả direction pins) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    
    /* Đặt chế độ: GPIO_Mode_OUT = Output digital
       - Pinmô hoạt động như output push-pull bình thường
       - Không phải alternate function (như PWM, UART, ...)
       - Có thể set HIGH/LOW trực tiếp bằng GPIO_SetBits/ResetBits
    */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    
    /* Đặt kiểu output: GPIO_OType_PP = Push-Pull
       - Output có khả năng kéo HIGH (push) và kéo LOW (pull)
       - Tiêu chuẩn cmos logic
       - Lựa chọn khác: GPIO_OType_OD = Open-Drain (kéo LOW chỉ)
    */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    
    /* Đặt tốc độ: GPIO_Speed_50MHz
       - Tốc độ switching của output stage
       - 50MHz đủ cho L298N (không cần quá nhan)
       - Lựa chọn khác: 2MHz, 25MHz, 100MHz
    */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    /* Đặt pull-up/pull-down: GPIO_PuPd_NOPULL
       - Không bật pull-up hoặc pull-down nội bộ
       - GPIO output không cần pull (có thể drive đủ mạnh)
       - Lựa chọn khác: GPIO_PuPd_UP, GPIO_PuPd_DOWN
    */
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    
    /* ========================================================================
     * BƯỚC 3: Áp dụng cấu trúc lên GPIOD
     * ======================================================================== */
    
    /* Hàm GPIO_Init() sẽ ghi các giá trị vào GPIO register của GPIOD
       - Thiết lập mode, type, speed, pull cho tất cả pin đã chọn
       - Sau hàm này, GPIO sẵn sàng để dùng
    */
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    /* ========================================================================
     * BƯỚC 4: Reset tất cả pin về LOW (dừng motor)
     * ======================================================================== */
    
    /* Hàm GPIO_ResetBits() đặt tất cả pin về 0V (LOW)
       - Tương đương với: IN1=0, IN2=0, IN3=0, IN4=0
       - Kết quả: tất cả motor ở chế độ STOP (coasting)
       - Không có phase đảo chiều
    */
    GPIO_ResetBits(GPIOD, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);
    
    /* Tại điểm này, GPIOD sẵn sàng để điều khiển direction motor */
}

/* ============================================================================
 * HÀM KHỞI TẠO PWM - SPEED CONTROL (ENA, ENB)
 * ============================================================================ */

/**
 * @brief Khởi tạo Timer1 PWM để điều khiển tốc độ motor
 * 
 * @details Hàm này cấu hình:
 *  1. Bật clock cho TIM1 (APB2) và GPIOE (PE13, PE14 pins)
 *  2. Cấu hình PE13 và PE14 làm Alternate Function TIM1
 *     - PE13 = TIM1_CH3 (ENA)
 *     - PE14 = TIM1_CH4 (ENB)
 *  3. Cấu hình TIM1 PWM mode:
 *     - Prescaler = 168 → TIM clock = 1MHz (168MHz/168)
 *     - Period = 1000 → PWM frequency = 1kHz (1MHz/1000)
 *     - Channel 3 & 4 mode PWM1
 *  4. Bật TIM1 counter và PWM output
 * 
 * @return void
 */
static void BSP_Motor_PWM_Init(void)
{
    /* Khai báo cấu trúc GPIO */
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* Khai báo cấu trúc Timer base (timebase configuration) */
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    /* Khai báo cấu trúc Timer Output Compare (PWM channel) */
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    /* ========================================================================
     * BƯỚC 1: Bật clock cho TIM1 (APB2 bus)
     * ======================================================================== */
    
    /* Bật clock cho Timer 1 trên APB2
       - TIM1 là timer trên APB2 (tần số 84MHz theo config)
       - APB2 prescaler = 2, nên TIM_CLK = 168MHz
    */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    
    /* ========================================================================
     * BƯỚC 2: Bật clock cho GPIOE (PE13, PE14 PWM pins)
     * ======================================================================== */
    
    /* Bật clock cho GPIO Port E trên AHB1
       - PE13 (TIM1_CH3) - dùng cho motor A speed (ENA)
       - PE14 (TIM1_CH4) - dùng cho motor B speed (ENB)
    */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    
    /* ========================================================================
     * BƯỚC 3: Cấu hình GPIO PE13, PE14 làm Alternate Function TIM1
     * ======================================================================== */
    
    /* Lựa chọn pin PE13 và PE14 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
    
    /* Đặt mode = Alternate Function (không phải GPIO output thường)
       - Khi GPIO_Mode_AF, pin được điều khiển bởi peripheral (TIM1)
       - TIM1 sẽ generate PWM signal trực tiếp trên pin
    */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    
    /* Đặt type = Push-Pull
       - PWM output cần drive cả HIGH lẫn LOW
    */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    
    /* Đặt tốc độ cao = 100MHz
       - PWM 1kHz cần tốc độ fetch nhanh
       - 100MHz = maximum speed cho GPIO
    */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    
    /* Đặt pull-up = ON
       - PWM output có pull-up để tránh floating state khi không drive
    */
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    
    /* Áp dụng cấu hình lên GPIOE */
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    
    /* ========================================================================
     * BƯỚC 4: Chỉ định PE13, PE14 có liên kết với TIM1 (GPIO_PinAFConfig)
     * ======================================================================== */
    
    /* Gọi hàm GPIO_PinAFConfig() để liên kết PE13 với TIM1
       - Tham số 1: Port = GPIOE
       - Tham số 2: Pin source = GPIO_PinSource13 (bit 13)
       - Tham số 3: AF = GPIO_AF_TIM1 (Alternate Function TIM1)
       - Kết quả: PE13 được điều khiển bởi TIM1 (chứ không phải GPIO)
    */
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_TIM1);
    
    /* Tương tự cho PE14 - chỉ định nó cũng là TIM1 AF */
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_TIM1);
    
    /* ========================================================================
     * BƯỚC 5: Cấu hình Timer1 Base (timebase)
     * ======================================================================== */
    
    /* Đặt prescaler = 168 - 1 = 167
       - Timer clock input = APB2 clock = 168MHz
       - Sau prescaler: Timer frequency = 168MHz / 168 = 1MHz
       - Mỗi tick = 1 microsecond
       - Ghi chú: công thức là (prescaler + 1), nên ghi 168-1 để được div/168
    */
    TIM_TimeBaseStructure.TIM_Prescaler = 168 - 1;
    
    /* Đặt period (auto reload value) = 999
       - PWM frequency = Timer frequency / (Period + 1) = 1MHz / 1000 = 1kHz
       - Mỗi 1ms sẽ có một chu kỳ PWM hoàn chỉnh
       - Period có thể thay từ 0..65535 (16-bit)
    */
    TIM_TimeBaseStructure.TIM_Period = MOTOR_PWM_PERIOD;  /* 1000 - 1 = 999 */
    
    /* Đặt chế độ đếm = Up counter
       - Timer đếm từ 0 đến Period rồi reset về 0
       - Có lựa chọn khác: Down counter, Center-aligned
    */
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    
    /* Đặt clock division = 1 (không chia clock nữa)
       - Tham số này điều chỉnh tần số sampling DMA/IRQ
       - TIM_CKD_DIV1 = không chia
       - Lựa chọn khác: DIV2, DIV4 (hiếm dùng)
    */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    
    /* Đặt repetition counter = 0 (chỉ dùng cho TIM1/TIM8)
       - Chế độ nâng cao, mặc định = 0 là đủ
    */
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    
    /* Gọi TIM_TimeBaseInit() để áp dụng cấu hình timebase vào TIM1 */
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
    
    /* ========================================================================
     * BƯỚC 6: Cấu hình Output Compare (OC) Channel 3 - Motor A PWM
     * ======================================================================== */
    
    /* Đặt chế độ OC = PWM1
       - PWM1: output HIGH khi counter < pulse, LOW khi counter >= pulse
       - Lựa chọn khác: PWM2 (đảo logic)
    */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    
    /* Đặt output state = ENABLE
       - Cho phép TIM1_CH3 output PWM signal
       - Nếu = DISABLE, pin không có PWM (cố định HIGH)
    */
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    
    /* Đặt pulse = 0 (duty cycle 0%)
       - Pulse là compare value - khi counter < pulse thì output HIGH
       - pulse = 0 → output luôn LOW (0%)
       - pulse = 500 → output HIGH 50% (500 của 1000)
       - pulse = 999 → output HIGH 100%
       - Có thể thay đổi runtime bằng TIM_SetCompare3()
    */
    TIM_OCInitStructure.TIM_Pulse = 0;
    
    /* Đặt polarity = High
       - Output active HIGH (1 = high, 0 = low)
       - Lựa chọn khác: TIM_OCPolarity_Low (logic đảo)
    */
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    
    /* Áp dụng cấu hình OC Channel 3 vào TIM1 (PE13 - Motor A) */
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);
    
    /* Bật preload (shadow register) cho Channel 3
       - Khi preload = ENABLE, pulse value được load vào shadow register
       - Kỳ sau mới update (tránh PWM glitch)
    */
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
    
    /* ========================================================================
     * BƯỚC 7: Cấu hình Output Compare Channel 4 - Motor B PWM
     * ======================================================================== */
    
    /* Cấu hình tương tự Channel 3, nhưng áp dụng cho Channel 4 */
    
    /* Áp dụng cấu hình OC Channel 4 vào TIM1 (PE14 - Motor B) */
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);
    
    /* Bật preload cho Channel 4 */
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
    
    /* ========================================================================
     * BƯỚC 8: Bật Auto-Reload Preload và khởi động Timer
     * ======================================================================== */
    
    /* Bật ARR (Auto-Reload Register) preload
       - Giống như OC preload, nhưng cho Period value
       - Tránh PWM frequency bị thay đổi giữa cycle
    */
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    
    /* BẬT TIMER1 - bắt đầu counting */
    TIM_Cmd(TIM1, ENABLE);
    
    /* ========================================================================
     * BƯỚC 9: Bật PWM output cho TIM1 (BẮT BUỘC cho TIM1/TIM8)
     * ======================================================================== */
    
    /* QUAN TRỌNG: TIM1 và TIM8 là advanced timer
       - Chúng có Dead-Time Generator và Break feature
       - Phải gọi TIM_CtrlPWMOutputs() = ENABLE mới cho phép output
       - Timer khác (TIM2-TIM7) không cần gọi hàm này
    */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    
    /* Tại điểm này, PE13 và PE14 đang generate PWM 1kHz, duty = 0% */
}

/* ============================================================================
 * HÀM KHỞI TẠO TOÀN BỘ MOTOR SYSTEM
 * ============================================================================ */

/**
 * @brief Khởi tạo toàn bộ system motor (GPIO + PWM)
 * 
 * @return void
 */
void BSP_Motor_Init(void)
{
    /* Gọi hàm khởi tạo GPIO cho direction pins (IN1-IN4) */
    BSP_Motor_GPIO_Init();
    
    /* Gọi hàm khởi tạo PWM cho speed pins (ENA-ENB) */
    BSP_Motor_PWM_Init();
    
    /* Tại điểm này, motor system sẵn sàng để điều khiển */
}

/* ============================================================================
 * HÀM ĐIỀU KHIỂN HƯỚNG QUAY MOTOR A
 * ============================================================================ */

/**
 * @brief Thiết lập hướng quay motor A (IN1, IN2)
 * 
 * @param[in] dir  Chế độ motor (MOTOR_STOP, FORWARD, BACKWARD, BRAKE)
 * 
 * @return void
 */
void BSP_Motor_A_SetDirection(MotorDirection_t dir)
{
    /* Xét từng chế độ */
    switch (dir)
    {
        /* ====================================================================
         * CHẾ ĐỘ: MOTOR_FORWARD (Động cơ tiến)
         * ====================================================================
         * IN1 = 1, IN2 = 0 → động cơ quay chiều dương (tới)
         */
        case MOTOR_FORWARD:
            /* Set IN1 (PD0) = 1 (HIGH) */
            GPIO_SetBits(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
            
            /* Reset IN2 (PD1) = 0 (LOW) */
            GPIO_ResetBits(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
            break;
        
        /* ====================================================================
         * CHẾ ĐỘ: MOTOR_BACKWARD (Động cơ lùi)
         * ====================================================================
         * IN1 = 0, IN2 = 1 → động cơ quay chiều âm (lùi)
         */
        case MOTOR_BACKWARD:
            /* Reset IN1 (PD0) = 0 (LOW) */
            GPIO_ResetBits(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
            
            /* Set IN2 (PD1) = 1 (HIGH) */
            GPIO_SetBits(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
            break;
        
        /* ====================================================================
         * CHẾ ĐỘ: MOTOR_BRAKE (Động cơ hãm điện)
         * ====================================================================
         * IN1 = 1, IN2 = 1 → cả hai phase HIGH → hãm (short circuit)
         */
        case MOTOR_BRAKE:
            /* Set IN1 (PD0) = 1 (HIGH) */
            GPIO_SetBits(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
            
            /* Set IN2 (PD1) = 1 (HIGH) */
            GPIO_SetBits(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
            break;
        
        /* ====================================================================
         * CHẾ ĐỘ MẶC ĐỊNH: MOTOR_STOP (Động cơ dừng/coasting)
         * ====================================================================
         * IN1 = 0, IN2 = 0 → cả hai phase LOW → coasting (không sức hút)
         */
        default:  /* case MOTOR_STOP: */
            /* Reset IN1 (PD0) = 0 (LOW) */
            GPIO_ResetBits(MOTOR_A_IN1_PORT, MOTOR_A_IN1_PIN);
            
            /* Reset IN2 (PD1) = 0 (LOW) */
            GPIO_ResetBits(MOTOR_A_IN2_PORT, MOTOR_A_IN2_PIN);
            break;
    }
}

/* ============================================================================
 * HÀM ĐIỀU KHIỂN HƯỚNG QUAY MOTOR B
 * ============================================================================ */

/**
 * @brief Thiết lập hướng quay motor B (IN3, IN4)
 * 
 * @param[in] dir  Chế độ motor (MOTOR_STOP, FORWARD, BACKWARD, BRAKE)
 * 
 * @return void
 */
void BSP_Motor_B_SetDirection(MotorDirection_t dir)
{
    /* Xét từng chế độ - tương tự Motor A nhưng dùng IN3 (PD2), IN4 (PD3) */
    switch (dir)
    {
        case MOTOR_FORWARD:
            GPIO_SetBits(MOTOR_B_IN3_PORT, MOTOR_B_IN3_PIN);
            GPIO_ResetBits(MOTOR_B_IN4_PORT, MOTOR_B_IN4_PIN);
            break;
        
        case MOTOR_BACKWARD:
            GPIO_ResetBits(MOTOR_B_IN3_PORT, MOTOR_B_IN3_PIN);
            GPIO_SetBits(MOTOR_B_IN4_PORT, MOTOR_B_IN4_PIN);
            break;
        
        case MOTOR_BRAKE:
            GPIO_SetBits(MOTOR_B_IN3_PORT, MOTOR_B_IN3_PIN);
            GPIO_SetBits(MOTOR_B_IN4_PORT, MOTOR_B_IN4_PIN);
            break;
        
        default:  /* MOTOR_STOP */
            GPIO_ResetBits(MOTOR_B_IN3_PORT, MOTOR_B_IN3_PIN);
            GPIO_ResetBits(MOTOR_B_IN4_PORT, MOTOR_B_IN4_PIN);
            break;
    }
}

/* ============================================================================
 * HÀM ĐIỀU KHIỂN TỐC ĐỘ (PWM) MOTOR A
 * ============================================================================ */

/**
 * @brief Thiết lập tốc độ motor A (ENA pin - TIM1_CH3)
 * 
 * @param[in] duty  Giá trị duty cycle (0..MOTOR_PWM_PERIOD = 0..999)
 * 
 * @return void
 */
void BSP_Motor_A_SetSpeed(uint16_t duty)
{
    /* Kiểm tra và giới hạn duty value
       - Nếu duty > MOTOR_PWM_PERIOD, thì đặt = MOTOR_PWM_PERIOD (100%)
       - Tránh overflow hoặc behavior không mong muốn
    */
    if (duty > MOTOR_PWM_PERIOD)
    {
        /* Duty vượt quá giới hạn → cắt xuống 999 */
        duty = MOTOR_PWM_PERIOD;
    }
    
    /* Gọi hàm TIM_SetCompare3() để update pulse width cho TIM1_CH3
       - Tham số 1: TIM1 = timer 1
       - Tham số 2: TIM_Channel_3 = channel 3
       - Tham số 3: duty = giá trị pulse mới (0..999)
       - Kết quả: PE13 sẽ generate PWM với duty = (duty/1000)*100%
    */
    TIM_SetCompare3(MOTOR_PWM_TIM, duty);
}

/* ============================================================================
 * HÀM ĐIỀU KHIỂN TỐC ĐỘ (PWM) MOTOR B
 * ============================================================================ */

/**
 * @brief Thiết lập tốc độ motor B (ENB pin - TIM1_CH4)
 * 
 * @param[in] duty  Giá trị duty cycle (0..MOTOR_PWM_PERIOD = 0..999)
 * 
 * @return void
 */
void BSP_Motor_B_SetSpeed(uint16_t duty)
{
    /* Kiểm tra và giới hạn duty value */
    if (duty > MOTOR_PWM_PERIOD)
    {
        duty = MOTOR_PWM_PERIOD;
    }
    
    /* Update pulse width cho TIM1_CH4 (PE14 - Motor B) */
    TIM_SetCompare4(MOTOR_PWM_TIM, duty);
}
