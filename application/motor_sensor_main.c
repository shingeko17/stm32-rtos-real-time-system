/**
 * @file motor_sensor_main.c
 * @brief Main application for motor + sensor + heartbeat runtime
 * @date 2026-03-24
 */

#include <stdint.h>

#include "adc_driver.h"
#include "encoder_driver.h"
#include "motor_driver.h"
#include "system_stm32f4xx.h"
#include "uart_driver.h"

#define CONTROL_PERIOD_MS      100U
#define MAX_SAFE_CURRENT_MA    3500U
#define MAX_SAFE_TEMP_C        85U

static volatile uint32_t g_ms_ticks = 0U;

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

static uint32_t app_millis(void)
{
    return g_ms_ticks;
}

int main(void)
{
    uint32_t last_control_tick = 0U;

    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);

    BSP_Motor_Init();
    ADC_Init();
    Encoder_Init();
    UART_Init();

    Motor_All_Stop();

    while (1) {
        uint32_t now = app_millis();

        if ((now - last_control_tick) >= CONTROL_PERIOD_MS) {
            uint16_t speed_request_rpm = ADC_ReadSpeed();
            uint16_t measured_rpm = Encoder_GetRPM();
            uint16_t current_ma = ADC_ReadCurrent();
            uint16_t temp_c = ADC_ReadTemp();

            int16_t speed_cmd = (int16_t)(speed_request_rpm / 5U); /* 0..500 RPM => 0..100% */

            if ((current_ma > MAX_SAFE_CURRENT_MA) || (temp_c > MAX_SAFE_TEMP_C)) {
                Motor_All_Brake();
            } else {
                (void)measured_rpm; /* Keep variable for next closed-loop step. */
                Motor_A_Run(speed_cmd);
                Motor_B_Run(speed_cmd);
            }

            UART_SendHeartbeat();
            last_control_tick = now;
        }
    }
}
