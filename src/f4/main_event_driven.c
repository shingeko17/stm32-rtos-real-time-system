/**
 * @file main_event_driven.c
 * @brief Main Application with Event-Driven Architecture
 * @details Chuyển từ bare-metal polling → Active Objects & Events
 * 
 * So sánh:
 * ========================================================================
 * BEFORE (Bare-metal polling):
 *   while(1) {
 *       if (time_elapsed) {
 *           speed = ADC_Read();
 *           Motor_Run(speed);
 *       }
 *   }
 * 
 * AFTER (Event-driven):
 *   - Tương tác qua events (MOTOR_SPEED_UPDATE, SENSOR_READ, etc)
 *   - Active Objects xử lý events asynchronously
 *   - Scheduler quản lý RTC (Run-To-Completion)
 *   - Không block, responsive
 * ========================================================================
 * 
 * @date 2026-04-01
 */

#include <stdint.h>
#include <stdbool.h>

#include "adc_driver.h"
#include "encoder_driver.h"
#include "motor_driver.h"
#include "system_stm32f4xx.h"
#include "uart_driver.h"

/* Event-driven framework */
#include "event.h"
#include "active_object.h"
#include "motor_controller_ao.h"

/* ============================================================================
 * GLOBAL SCHEDULER & ACTIVE OBJECTS
 * ============================================================================
 */

static Scheduler_t g_scheduler;
static ActiveObject_t *g_motor_a_ao = NULL;
static ActiveObject_t *g_motor_b_ao = NULL;

/* Sensors sampling AO (tương tự một active object nhỏ) */
typedef struct {
    uint32_t last_sample_tick;
    uint16_t sample_interval_ms;
} SensorsSamplerData_t;

static SensorsSamplerData_t g_sensors_data = {
    .last_sample_tick = 0,
    .sample_interval_ms = 100  /* 100ms */
};

/* ============================================================================
 * TICK/TIME MANAGEMENT
 * ============================================================================
 */

static volatile uint32_t g_ms_ticks = 0U;

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

static uint32_t app_millis(void)
{
    return g_ms_ticks;
}

/* ============================================================================
 * IDLE CALLBACK - Gọi khi tất cả AOs không có events
 * ============================================================================
 *
 * Đây là nơi CPU có thể vào sleep mode để tiết kiệm điện
 */

static void idle_callback(void)
{
    /* TODO: Set CPU vào low-power sleep mode */
    /* Ví dụ:
       SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
       __WFI();  // Wait For Interrupt
    */
    
    /* Để debug, chúng ta chỉ spin */
    __asm("NOP");
}

/* ============================================================================
 * SENSORS SAMPLING THREAD (Tạo events từ sensors)
 * ============================================================================
 *
 * Trong event-driven architecture, sensors không trực tiếp lấy dữ liệu từ ISR
 * Thay vào đó:
 * 1. SysTick ISR → post TICK event
 * 2. Sensors sampler AO xử lý TICK → đọc sensors → post SENSOR_UPDATE
 * 3. Motor AO nhận SENSOR_UPDATE → kiểm tra safety limits
 */

static void UpdateSensorsAO(void)
{
    uint32_t now = app_millis();
    
    if ((now - g_sensors_data.last_sample_tick) >= g_sensors_data.sample_interval_ms) {
        /* Đọc sensors */
        uint16_t speed_request = ADC_ReadSpeed();
        uint16_t measured_rpm = Encoder_GetRPM();
        uint16_t current_ma = ADC_ReadCurrent();
        uint16_t temp_c = ADC_ReadTemp();
        
        /* (không dùng measured_rpm trong ví dụ này, nhưng có sẵn) */
        (void)measured_rpm;
        
        /* Gửi MOTOR_SPEED_UPDATE tới Motor AOs */
        int16_t speed_cmd = (int16_t)(speed_request / 5U);  /* Normalize */
        
        if (g_motor_a_ao != NULL) {
            MotorController_UpdateSpeed(g_motor_a_ao, speed_cmd);
        }
        if (g_motor_b_ao != NULL) {
            MotorController_UpdateSpeed(g_motor_b_ao, speed_cmd);
        }
        
        /* Gửi SENSOR_UPDATE tới Motor AOs (để kiểm tra safety) */
        if (g_motor_a_ao != NULL) {
            MotorController_UpdateSensors(g_motor_a_ao, current_ma, temp_c);
        }
        if (g_motor_b_ao != NULL) {
            MotorController_UpdateSensors(g_motor_b_ao, current_ma, temp_c);
        }
        
        /* Send UART heartbeat */
        UART_SendHeartbeat();
        
        g_sensors_data.last_sample_tick = now;
    }
}

/* ============================================================================
 * MAIN - Khởi tạo & Chạy Event-Driven System
 * ============================================================================
 */

int main(void)
{
    /* ========== Hardware Initialization ========== */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);  /* 1ms ticks */

    BSP_Motor_Init();
    ADC_Init();
    Encoder_Init();
    UART_Init();

    Motor_All_Stop();

    /* ========== Event-Driven Framework Initialization ========== */
    
    /* 1. Khởi tạo Scheduler */
    Scheduler_Init(&g_scheduler, idle_callback);
    
    /* 2. Tạo Motor Controller Active Objects */
    g_motor_a_ao = MotorController_Create(0, 1);  /* Channel A, priority 1 */
    g_motor_b_ao = MotorController_Create(1, 1);  /* Channel B, priority 1 */
    
    if (g_motor_a_ao == NULL || g_motor_b_ao == NULL) {
        /* Initialization error - halt */
        while (1) {
            __asm("NOP");
        }
    }
    
    /* 3. Đăng ký AOs vào Scheduler */
    if (!Scheduler_Register(&g_scheduler, g_motor_a_ao)) {
        while (1) { __asm("NOP"); }
    }
    if (!Scheduler_Register(&g_scheduler, g_motor_b_ao)) {
        while (1) { __asm("NOP"); }
    }
    
    /* 4. Start AOs (initial transition - ENTRY_SIG) */
    ActiveObject_Start(g_motor_a_ao);
    ActiveObject_Start(g_motor_b_ao);
    
    /* 5. Send initial START event */
    MotorController_Start(g_motor_a_ao, 0);
    MotorController_Start(g_motor_b_ao, 0);

    /* ========== Main Event Loop ========== */
    
    while (1) {
        /* Sensors sampling (trigger thêm events vào AOs) */
        UpdateSensorsAO();
        
        /* Chạy Scheduler - RTC processing */
        /* Scheduler sẽ:
         * 1. Tìm AO có priority cao nhất với events
         * 2. Dispatch 1 RTC step (xử lý 1 event)
         * 3. Nếu không có AO nào có events → gọi idle_callback
         * 4. Quay lại bước 1
         */
        if (!Scheduler_Step(&g_scheduler)) {
            /* Không có event được xử lý - idle */
            idle_callback();
        }
    }

    return 0;
}

/* ============================================================================
 * DEBUGGING / MONITORING FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Lấy thông tin trạng thái Motor A (cho UART debug)
 */
void DebugPrintMotorA(void)
{
    if (g_motor_a_ao == NULL) return;
    
    MotorControllerData_t *data = MotorController_GetData(g_motor_a_ao);
    if (data == NULL) return;
    
    /* TODO: Print data to UART
       UART_Printf("Motor A: state=%d, speed=%d, current=%dmA, temp=%dC\r\n",
                   data->state, data->speed_cmd, data->current_ma, data->temp_c);
    */
}

/**
 * @brief Lấy thông tin trạng thái Motor B
 */
void DebugPrintMotorB(void)
{
    if (g_motor_b_ao == NULL) return;
    
    MotorControllerData_t *data = MotorController_GetData(g_motor_b_ao);
    if (data == NULL) return;
    
    /* TODO: Print data to UART */
}
