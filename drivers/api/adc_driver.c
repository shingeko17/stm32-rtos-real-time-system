/**
 * @file adc_driver.c
 * @brief ADC Driver Implementation - Analog Sensor Reading
 * @date 2026-03-22
 */

#include "adc_driver.h"
#include "misc_drivers.h"

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================ */

/** DMA Buffer để lưu ADC result (3 channel) */
static uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE * ADC_NUM_CHANNELS];

/** ADC calibration factor (nếu cần) */
static float current_calibration = 1.0f;
static float speed_calibration = 1.0f;

/* ============================================================================
 * PUBLIC FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

/**
 * @brief ADC_Init - Khởi tạo ADC + GPIO + DMA
 */
void ADC_Init(void)
{
    /*
     * TODO: Implement ADC initialization
     * 
     * Steps:
     * 1. Enable RCC clocks:
     *    - RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE)
     *    - RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE)
     *    - RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE)
     * 
     * 2. Configure GPIO (PA0, PA1, PA2) as analog input:
     *    - For each pin: GPIO_Mode = GPIO_Mode_AN
     *    - GPIO_Init(GPIOA, &GPIO_InitStructure)
     * 
     * 3. Configure ADC1:
     *    - ADC_InitStructure.ADC_Prescaler = ADC_Prescaler_Div8
     *    - ADC_InitStructure.ADC_ScanConvMode = ENABLE (multi-channel)
     *    - ADC_InitStructure.ADC_ContinuousConvMode = ENABLE (continuous)
     *    - ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None
     *    - ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right
     *    - ADC_InitStructure.ADC_NbrOfConversion = 3 (3 channels)
     *    - ADC_Init(ADC1, &ADC_InitStructure)
     * 
     * 4. Configure ADC regular channel sequence:
     *    - ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_480Cycles)
     *    - ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_480Cycles)
     *    - ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_480Cycles)
     * 
     * 5. Configure DMA2 Stream 0:
     *    - DMA_InitStructure.DMA_Channel = DMA_Channel_0
     *    - DMA_InitStructure.DMA_PeripheralBaseAddr = &ADC1->DR
     *    - DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)adc_dma_buffer
     *    - DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory
     *    - DMA_InitStructure.DMA_BufferSize = ADC_DMA_BUFFER_SIZE * ADC_NUM_CHANNELS
     *    - DMA_InitStructure.DMA_Mode = DMA_Mode_Circular (auto-restart)
     *    - DMA_InitStructure.DMA_DataSize = DMA_DataSize_HalfWord
     *    - DMA_Init(DMA2_Stream0, &DMA_InitStructure)
     * 
     * 6. Enable DMA + ADC:
     *    - ADC_DMACmd(ADC1, ENABLE)
     *    - DMA_Cmd(DMA2_Stream0, ENABLE)
     *    - ADC_Cmd(ADC1, ENABLE)
     *    - ADC_SoftwareStartConv(ADC1)
     */
}

/**
 * @brief ADC_ReadCurrent - Đọc dòng điện hiện tại
 */
uint16_t ADC_ReadCurrent(void)
{
    /*
     * TODO: Implement current reading
     * 
     * 1. Get raw ADC value from buffer[0]
     * 2. Convert 12-bit (0-4095) to mA (0-5000)
     *    Formula: mA = (raw_value * 5000) / 4095
     * 3. Apply calibration factor if needed
     * 4. Return as uint16_t
     * 
     * Current sensor spec:
     *   - ACT712-5A: 2.5V @ 0A, ±2.5V for ±5A
     *   - Output: 0V to 5V
     *   - Linear mapping:
     *     * 0V → 0mA
     *     * 2.5V → 2500mA (2.5A)
     *     * 5V → 5000mA (5A)
     */
    
    uint16_t raw = ADC_ReadRaw(0);
    uint16_t mA = (raw * 5000) / 4095;
    return (uint16_t)(mA * current_calibration);
}

/**
 * @brief ADC_ReadSpeed - Đọc tốc độ motor
 */
uint16_t ADC_ReadSpeed(void)
{
    /*
     * TODO: Implement speed reading
     * 
     * 1. Get raw ADC value from buffer[1]
     * 2. Convert 12-bit (0-4095) to RPM (0-500)
     *    Formula: RPM = (raw_value * 500) / 4095
     * 3. Apply calibration factor if needed
     * 4. Return as uint16_t
     * 
     * Speed sensor spec (potentiometer output):
     *   - Output: 0V to 3.3V
     *   - Linear mapping:
     *     * 0V → 0 RPM
     *     * 1.65V → 250 RPM
     *     * 3.3V → 500 RPM
     * 
     * Note: Adjust 500 RPM max based on actual motor specs
     */
    
    uint16_t raw = ADC_ReadRaw(1);
    uint16_t rpm = (raw * 500) / 4095;
    return (uint16_t)(rpm * speed_calibration);
}

/**
 * @brief ADC_ReadTemp - Đọc nhiệt độ (optional)
 */
uint16_t ADC_ReadTemp(void)
{
    /*
     * TODO: Implement temperature reading
     * 
     * 1. Get raw ADC value from buffer[2]
     * 2. Convert 12-bit (0-4095) to °C (0-100)
     *    Formula: °C = (raw_value * 100) / 4095
     * 3. Return as uint16_t
     * 
     * Temperature sensor spec (NTC thermistor):
     *   - Output: 0V to 3.3V
     *   - Linear mapping (simplified):
     *     * 0V → 0°C
     *     * 1.65V → 50°C
     *     * 3.3V → 100°C
     * 
     * Note: For accurate reading, use Steinhart-Hart equation
     */
    
    uint16_t raw = ADC_ReadRaw(2);
    uint16_t temp = (raw * 100) / 4095;
    return temp;
}

/**
 * @brief ADC_ReadRaw - Đọc raw ADC value
 */
uint16_t ADC_ReadRaw(uint8_t channel)
{
    /*
     * TODO: Implement raw reading
     * 
     * 1. Validate channel (0, 1, 2)
     * 2. Return adc_dma_buffer[channel]
     * 
     * Note: These are continuous updates from DMA interrupt
     */
    
    if (channel < ADC_NUM_CHANNELS) {
        return adc_dma_buffer[channel];
    }
    return 0;
}

/**
 * @brief ADC_GetDMABuffer - Trả về DMA buffer pointer
 */
uint16_t* ADC_GetDMABuffer(void)
{
    /*
     * TODO: Return DMA buffer for direct access
     * 
     * Buffer layout:
     *   buffer[0] = Current ADC value
     *   buffer[1] = Speed ADC value
     *   buffer[2] = Temp ADC value
     */
    
    return &adc_dma_buffer[0];
}
