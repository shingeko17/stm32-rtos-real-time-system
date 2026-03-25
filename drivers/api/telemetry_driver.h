/**
 * @file telemetry_driver.h
 * @brief Telemetry Driver - Data Collection & Formatting
 * @details API để collect, format, và quản lý telemetry data từ tất cả sensor
 *          - Aggregate data từ: Motor, Sensor, Encoder, Temperature
 *          - Format và storing cho transmission
 * @author STM32 Telemetry Team
 * @date 2026-03-22
 * 
 * ============================================================================
 * MỤC ĐÍCH: Cung cấp unified interface để collect + format system telemetry
 * 
 * Data được collect:
 *   - Motor speed (RPM)
 *   - Motor current (mA)
 *   - Motor temperature (°C)
 *   - System load (%)
 *   - Timestamp (ms)
 * 
 * Ví dụ sử dụng:
 *   Telemetry_Init();                  // Khởi tạo buffer
 *   Telemetry_Update();                // Collect mới từ sensor (1kHz)
 *   Telemetry_GetSnapshot(&snapshot);  // Lấy latest data
 *   Telemetry_GetHistory(N);           // Lấy N samples trước đó
 * 
 * ============================================================================
 */

#ifndef TELEMETRY_DRIVER_H
#define TELEMETRY_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TELEMETRY DATA STRUCTURES
 * ============================================================================ */

/**
 * @struct Telemetry_Data_t
 * @brief Single telemetry snapshot
 */
typedef struct {
    uint32_t timestamp_ms;      // Timestamp (milliseconds since startup)
    
    uint16_t motor_speed_rpm;   // Motor speed feedback (0-5000 RPM)
    int16_t  motor_current_ma;  // Motor current draw (±5000 mA)
    uint16_t motor_temp_c;      // Motor temperature (0-100°C)
    
    uint8_t  system_load_pct;   // System CPU load (0-100%)
    uint8_t  error_flags;       // Error status (bit-packed)
    
    uint16_t adc_current_raw;   // Raw ADC current reading
    uint16_t adc_speed_raw;     // Raw ADC speed reading
    
} Telemetry_Data_t;

/**
 * @struct Telemetry_Stats_t
 * @brief Statistical summary of telemetry
 */
typedef struct {
    uint16_t avg_speed_rpm;     // Average speed
    uint16_t max_speed_rpm;     // Max speed recorded
    uint16_t min_speed_rpm;     // Min speed recorded
    
    int16_t  avg_current_ma;    // Average current
    int16_t  max_current_ma;    // Max current
    
    uint16_t avg_temp_c;        // Average temperature
    uint16_t max_temp_c;        // Max temperature
    
    uint32_t total_runtime_ms;  // Total runtime
    uint16_t error_count;       // Total errors encountered
    
} Telemetry_Stats_t;

/* ============================================================================
 * TELEMETRY CONFIGURATION
 * ============================================================================ */

/** Telemetry sampling rate (Hz) - should match ADC rate */
#define TELEMETRY_SAMPLE_RATE  1000   // 1000 samples/second

/** Telemetry circular buffer size (number of snapshots) */
#define TELEMETRY_BUFFER_SIZE  100    // Store last 100 snapshots (100ms @ 1kHz)

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo Telemetry buffer
 * 
 * @details Hàm này:
 *  1. Cấp phát bộ nhớ cho circular buffer
 *  2. Khởi tạo con trỏ head/tail
 *  3. Xóa stats cũ
 * 
 * @return void
 * @note Phải gọi TRƯỚC khi gọi các hàm telemetry khác
 */
void Telemetry_Init(void);

/**
 * @brief Collect mới telemetry data từ tất cả sensor
 * 
 * @details Hàm này:
 *  1. Gọi từ Telemetry Task (Priority 2)
 *  2. Đọc dữ liệu từ các driver:
 *     - motor_speed: từ Encoder_GetRPM()
 *     - motor_current: từ ADC_ReadCurrent()
 *     - motor_temp: từ ADC_ReadTemp()
 *     - system_load: từ SysMon_GetCPULoad()
 *  3. Lưu snapshot vào circular buffer
 *  4. Update stats (min/max/avg)
 * 
 * @return void
 * @note Nên gọi từ periodic task (ví dụ: mỗi 1ms từ SysTick)
 */
void Telemetry_Update(void);

/**
 * @brief Lấy latest telemetry snapshot
 * 
 * @param[out] pData  Con trỏ tới Telemetry_Data_t để nhận dữ liệu
 * 
 * @return uint8_t  1 nếu thành công, 0 nếu lỗi
 * 
 * @details Hàm này:
 *  1. Lấy snapshot mới nhất từ buffer
 *  2. Copy vào structure được cung cấp
 *  3. Trả về 1 nếu có data, 0 nếu buffer rỗng
 * 
 * @example
 *  Telemetry_Data_t latest;
 *  if (Telemetry_GetSnapshot(&latest)) {
 *      printf("Speed: %d RPM\n", latest.motor_speed_rpm);
 *      printf("Current: %d mA\n", latest.motor_current_ma);
 *  }
 */
uint8_t Telemetry_GetSnapshot(Telemetry_Data_t *pData);

/**
 * @brief Lấy telemetry data từ N samples trước đó
 * 
 * @param[in] index   Index: 0 = latest, 1 = 1 sample ago, 2 = 2 samples ago, etc.
 * @param[out] pData  Con trỏ tới Telemetry_Data_t để nhận dữ liệu
 * 
 * @return uint8_t  1 nếu thành công, 0 nếu index out of bounds
 * 
 * @example
 *  Telemetry_Data_t prev;
 *  Telemetry_GetHistory(5, &prev);  // Get data from 5 samples ago
 */
uint8_t Telemetry_GetHistory(uint16_t index, Telemetry_Data_t *pData);

/**
 * @brief Lấy statistical summary
 * 
 * @param[out] pStats  Con trỏ tới Telemetry_Stats_t
 * 
 * @return void
 * 
 * @details Tính toán min/max/average từ circular buffer
 * 
 * @example
 *  Telemetry_Stats_t stats;
 *  Telemetry_GetStats(&stats);
 *  printf("Avg Speed: %d RPM\n", stats.avg_speed_rpm);
 *  printf("Max Speed: %d RPM\n", stats.max_speed_rpm);
 */
void Telemetry_GetStats(Telemetry_Stats_t *pStats);

/**
 * @brief Set error flag (bit-packed)
 * 
 * @param[in] error_bit  Bit position (0-7)
 * 
 * @return void
 * 
 * @details Error flags (example):
 *  - Bit 0: Over-current detected
 *  - Bit 1: Over-temperature detected
 *  - Bit 2: Motor stuck
 *  - Bit 3: ADC error
 *  - Bit 4-7: Reserved
 * 
 * @example
 *  Telemetry_SetError(0);  // Set bit 0 (over-current)
 */
void Telemetry_SetError(uint8_t error_bit);

/**
 * @brief Clear error flag
 * 
 * @param[in] error_bit  Bit position (0-7)
 * 
 * @return void
 */
void Telemetry_ClearError(uint8_t error_bit);

/**
 * @brief Get current error flags
 * 
 * @return uint8_t  Bit-packed error flags
 * 
 * @example
 *  uint8_t errors = Telemetry_GetErrors();
 *  if (errors & (1 << 0)) {
 *      printf("Over-current!\n");
 *  }
 */
uint8_t Telemetry_GetErrors(void);

/**
 * @brief Reset all telemetry data
 * 
 * @return void
 * 
 * @details Xóa buffer + stats, restart counting
 */
void Telemetry_Reset(void);

/**
 * @brief Get total sample count collected
 * 
 * @return uint32_t  Total number of Telemetry_Update() calls
 */
uint32_t Telemetry_GetSampleCount(void);

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_DRIVER_H */
