/**
 * @file dma_driver.h
 * @brief DMA Driver - Direct Memory Access Configuration
 * @details API để cấu hình DMA cho ADC continuous conversion
 *          - ADC → RAM transfer (ADC DMA2 Stream 0)
 *          - Circular mode (auto-restart)
 * @author STM32 DMA Team
 * @date 2026-03-22
 * 
 * ============================================================================
 * MỤC ĐÍCH: Cung cấp low-level DMA configuration cho ADC sensor reading
 * 
 * DMA được sử dụng:
 *   - DMA2 Stream 0 Channel 0: ADC1 → RAM buffer (circular mode)
 *   - Continuous conversion: ADC tự động update DMA buffer
 *   - Không cần interrupt mỗi sample = CPU savings
 * 
 * ============================================================================
 */

#ifndef DMA_DRIVER_H
#define DMA_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DMA CONFIGURATION
 * ============================================================================ */

/** DMA controller + stream cho ADC */
#define SENSOR_DMA              DMA2
#define SENSOR_DMA_STREAM       DMA2_Stream0
#define SENSOR_DMA_CHANNEL      DMA_Channel_0

/** DMA stream IRQ */
#define SENSOR_DMA_IRQ          DMA2_Stream0_IRQn

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo DMA2 Stream 0 cho ADC1
 * 
 * @param[in] pBuffer  Con trỏ tới RAM buffer để lưu ADC data
 * @param[in] size     Buffer size (number of elements)
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Bật RCC clock cho DMA2
 *  2. Cấu hình DMA2 Stream 0:
 *     - Channel 0 (ADC1)
 *     - Source: ADC1->DR (ADC data register)
 *     - Destination: pBuffer (RAM)
 *     - Transfer mode: Peripheral-to-Memory
 *     - Circular mode: enabled (auto-restart)
 *     - Data align: HalfWord (16-bit)
 *  3. Bật DMA stream
 * 
 * @note Được gọi từ ADC_Init()
 */
void DMA_Init_ADC(uint16_t *pBuffer, uint32_t size);

/**
 * @brief Enable DMA transfer
 * 
 * @return void
 */
void DMA_Start(void);

/**
 * @brief Disable DMA transfer
 * 
 * @return void
 */
void DMA_Stop(void);

/**
 * @brief Check DMA transfer status
 * 
 * @return uint8_t  1 if DMA is running, 0 if stopped
 */
uint8_t DMA_IsRunning(void);

/**
 * @brief Get DMA transfer count
 * 
 * @return uint16_t  Remaining items to transfer in current cycle
 */
uint16_t DMA_GetTransferCount(void);

#ifdef __cplusplus
}
#endif

#endif /* DMA_DRIVER_H */
