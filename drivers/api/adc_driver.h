/**
 * @file adc_driver.h
 * @brief ADC Driver - Analog-to-Digital Conversion for Sensor Reading
 * @details API để đọc sensor qua ADC với DMA continuous mode
 *          - Current sensor (ACT712 - DC current)
 *          - Speed feedback (potentiometer hoặc encoder output voltage)
 *          - Temperature sensor (NTC thermistor - optional)
 * @author STM32 Sensor Team
 * @date 2026-03-22
 * 
 * ============================================================================
 * MỤC ĐÍCH: Cung cấp giao diện đơn giản để đọc analog sensor
 * 
 * Ví dụ sử dụng:
 *   ADC_Init();                    // Khởi tạo ADC + DMA
 *   uint16_t current = ADC_ReadCurrent();    // Đọc dòng điện (mA)
 *   uint16_t speed = ADC_ReadSpeed();        // Đọc tốc độ (rpm)
 *   uint16_t temp = ADC_ReadTemp();          // Đọc nhiệt độ (°C)
 * 
 * ============================================================================
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ADC CONFIGURATION
 * ============================================================================ */

/* ADC1 được dùng cho Sensor Reading */
#define SENSOR_ADC          ADC1

/* ============================================================================
 * SENSOR PIN MAPPING
 * ============================================================================ */

/**
 * Current Sensor (ACT712-5A):
 *   - Output: 2.5V @ 0A, tuyến tính ±2.5V ↔ ±5A
 *   - STM32 pin: PA0 (ADC_IN0) - Channel 0
 *   - Range: 0 mA đến 5000 mA (±5A)
 */
#define CURRENT_SENSOR_PIN       GPIO_Pin_0   // PA0
#define CURRENT_SENSOR_PORT      GPIOA
#define CURRENT_SENSOR_CHANNEL   ADC_Channel_0

/**
 * Speed Feedback (Potentiometer / Encoder Output):
 *   - Output: 0V - 3.3V tuyến tính với tốc độ
 *   - STM32 pin: PA1 (ADC_IN1) - Channel 1
 *   - Range: 0 RPM đến 500 RPM (tùy motor)
 */
#define SPEED_SENSOR_PIN         GPIO_Pin_1   // PA1
#define SPEED_SENSOR_PORT        GPIOA
#define SPEED_SENSOR_CHANNEL     ADC_Channel_1

/**
 * Temperature Sensor (NTC Thermistor - Optional):
 *   - Output: 0V - 3.3V tuyến tính với nhiệt độ
 *   - STM32 pin: PA2 (ADC_IN2) - Channel 2
 *   - Range: 0°C đến 100°C
 */
#define TEMP_SENSOR_PIN          GPIO_Pin_2   // PA2
#define TEMP_SENSOR_PORT         GPIOA
#define TEMP_SENSOR_CHANNEL      ADC_Channel_2

/* ============================================================================
 * ADC SAMPLING CONFIGURATION
 * ============================================================================ */

/** ADC Sample Rate - số lần ADC chạy trong 1 giây */
#define ADC_SAMPLE_RATE          1000   /* 1000 samples/second = 1kHz */

/** ADC DMA Buffer Size - số sample được lưu trữ */
#define ADC_DMA_BUFFER_SIZE      10     /* 10 sample buffer */

/** Số channel được scan trong ADC */
#define ADC_NUM_CHANNELS         3      /* Current, Speed, Temp */

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo ADC + GPIO + DMA cho Sensor Reading
 * 
 * @details Hàm này:
 *  1. Bật clock cho ADC1 và GPIO Port A
 *  2. Cấu hình GPIO PA0, PA1, PA2 (input analog)
 *  3. Cấu hình ADC1:
 *     - Prescaler = /8 (21MHz từ 168MHz)
 *     - Resolution = 12-bit
 *     - Sampling time = 480 cycles (đủ lâu cho ADC ổn định)
 *     - Scan mode = enabled (quét 3 channel)
 *     - Continuous mode = enabled (chạy liên tục)
 *  4. Cấu hình DMA2 Stream 0:
 *     - Lưu kết quả ADC vào RAM buffer
 *     - Circular mode (tự động restart)
 *  5. Khởi động ADC + DMA
 * 
 * @return void
 * @note Phải gọi hàm này TRONG SystemInit() hoặc main() trước khi đọc sensor
 */
void ADC_Init(void);

/**
 * @brief Đọc giá trị dòng điện từ Current Sensor
 * 
 * @return uint16_t Dòng điện (mA)
 *         Range: 0 - 5000 mA (±5A)
 *         0mA = 0V, 2500mA = 2.5V, 5000mA = 5V
 * 
 * @details Hàm này:
 *  1. Đọc giá trị từ DMA buffer (PA0 - ADC_Channel_0)
 *  2. Chuyển đổi ADC 12-bit (0-4095) sang mA (0-5000)
 *  3. Trả về giá trị trung bình (nếu có filter)
 * 
 * @example
 *  uint16_t current_mA = ADC_ReadCurrent();
 *  if (current_mA > 3000) {  // Quá dòng 3A
 *      Motor_All_Stop();
 *  }
 */
uint16_t ADC_ReadCurrent(void);

/**
 * @brief Đọc giá trị tốc độ từ Speed Feedback
 * 
 * @return uint16_t Tốc độ (RPM)
 *         Range: 0 - 500 RPM (tùy cấu hình motor)
 *         0 RPM = 0V, 500 RPM = 3.3V
 * 
 * @details Hàm này:
 *  1. Đọc giá trị từ DMA buffer (PA1 - ADC_Channel_1)
 *  2. Chuyển đổi ADC 12-bit (0-4095) sang RPM (0-500)
 *  3. Trả về giá trị trung bình
 * 
 * @example
 *  uint16_t speed_rpm = ADC_ReadSpeed();
 *  printf("Motor speed: %d RPM\n", speed_rpm);
 */
uint16_t ADC_ReadSpeed(void);

/**
 * @brief Đọc giá trị nhiệt độ từ Temperature Sensor (Optional)
 * 
 * @return uint16_t Nhiệt độ (°C)
 *         Range: 0 - 100 °C
 *         0°C = 0V, 100°C = 3.3V
 * 
 * @details Hàm này:
 *  1. Đọc giá trị từ DMA buffer (PA2 - ADC_Channel_2)
 *  2. Chuyển đổi ADC 12-bit (0-4095) sang °C (0-100)
 *  3. Trả về giá trị trung bình
 * 
 * @note Sensor này là optional - có thể không lắp
 * 
 * @example
 *  uint16_t temp_c = ADC_ReadTemp();
 *  if (temp_c > 80) {  // Quá nóng 80°C
 *      Motor_All_Stop();  // Dừng motor để tản nhiệt
 *  }
 */
uint16_t ADC_ReadTemp(void);

/**
 * @brief Đọc raw ADC value (12-bit) từ channel cụ thể
 * 
 * @param[in] channel  ADC channel (0, 1, hoặc 2)
 *                     - 0: Current sensor
 *                     - 1: Speed sensor
 *                     - 2: Temp sensor
 * 
 * @return uint16_t Raw ADC value (0 - 4095)
 * 
 * @details Hàm này:
 *  1. Lấy raw value từ DMA buffer
 *  2. Không chuyển đổi (trả về giá trị thô 12-bit)
 *  3. Dùng cho debug hoặc calibration
 * 
 * @note Hàm internal - dùng cho development/debug
 */
uint16_t ADC_ReadRaw(uint8_t channel);

/**
 * @brief Trả về con trỏ đến DMA buffer (dùng cho direct read)
 * 
 * @return uint16_t* Con trỏ tới DMA buffer chứa 3 sample ADC
 *                   buffer[0] = Current (PA0)
 *                   buffer[1] = Speed (PA1)
 *                   buffer[2] = Temp (PA2)
 * 
 * @details Dùng cho advanced application needs hoặc direct DMA access
 */
uint16_t* ADC_GetDMABuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_DRIVER_H */
