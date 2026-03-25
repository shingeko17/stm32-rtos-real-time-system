/**
 * @file adc_driver.c
 * @brief ADC Driver Implementation - Analog Sensor Reading
 * @date 2026-03-22
 */

#include "adc_driver.h"
#include "misc_drivers.h"

/* Missing RCC bit in minimal header */
#define RCC_AHB1Periph_DMA2 0x00400000UL

/* ADC register bits */
#define ADC_CR2_ADON       (1UL << 0)
#define ADC_CR2_CONT       (1UL << 1)
#define ADC_CR2_DMA        (1UL << 8)
#define ADC_CR2_DDS        (1UL << 9)
#define ADC_CR2_SWSTART    (1UL << 30)

/* DMA register bits */
#define DMA_SxCR_EN        (1UL << 0)
#define DMA_SxCR_MINC      (1UL << 10)
#define DMA_SxCR_PSIZE_0   (1UL << 11)
#define DMA_SxCR_MSIZE_0   (1UL << 13)
#define DMA_SxCR_CIRC      (1UL << 8)
#define DMA_SxCR_PL_1      (1UL << 17)

/* Peripheral base addresses */
#define ADC1_BASE_ADDR       0x40012000UL
#define ADC_COMMON_BASE_ADDR 0x40012300UL
#define DMA2_BASE_ADDR       0x40026400UL
#define DMA2_STREAM0_ADDR    (DMA2_BASE_ADDR + 0x10UL)

/* Minimal ADC register layout */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR1;
    volatile uint32_t JOFR2;
    volatile uint32_t JOFR3;
    volatile uint32_t JOFR4;
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;
    volatile uint32_t JDR1;
    volatile uint32_t JDR2;
    volatile uint32_t JDR3;
    volatile uint32_t JDR4;
    volatile uint32_t DR;
} ADC_RegDef_t;

typedef struct {
    volatile uint32_t CSR;
    volatile uint32_t CCR;
    volatile uint32_t CDR;
} ADC_CommonRegDef_t;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t M0AR;
    volatile uint32_t M1AR;
    volatile uint32_t FCR;
} DMA_StreamRegDef_t;

#define ADC1_REGS         ((ADC_RegDef_t *)ADC1_BASE_ADDR)
#define ADC_COMMON_REGS   ((ADC_CommonRegDef_t *)ADC_COMMON_BASE_ADDR)
#define DMA2_STREAM0_REGS ((DMA_StreamRegDef_t *)DMA2_STREAM0_ADDR)

/* DMA Buffer d? luu ADC result (3 channel) */
static uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE * ADC_NUM_CHANNELS];

/* ADC calibration factor (n?u c?n) */
static float current_calibration = 1.0f;
static float speed_calibration = 1.0f;

void ADC_Init(void)
{
    GPIO_InitTypeDef gpio_cfg;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    gpio_cfg.GPIO_Pin = CURRENT_SENSOR_PIN | SPEED_SENSOR_PIN | TEMP_SENSOR_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AN;
    gpio_cfg.GPIO_OType = GPIO_OType_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_cfg.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio_cfg);

    DMA2_STREAM0_REGS->CR &= ~DMA_SxCR_EN;
    while ((DMA2_STREAM0_REGS->CR & DMA_SxCR_EN) != 0U) {
    }

    DMA2_STREAM0_REGS->PAR = (uint32_t)&ADC1_REGS->DR;
    DMA2_STREAM0_REGS->M0AR = (uint32_t)&adc_dma_buffer[0];
    DMA2_STREAM0_REGS->NDTR = ADC_DMA_BUFFER_SIZE * ADC_NUM_CHANNELS;
    DMA2_STREAM0_REGS->CR = DMA_SxCR_MINC |
                            DMA_SxCR_PSIZE_0 |
                            DMA_SxCR_MSIZE_0 |
                            DMA_SxCR_CIRC |
                            DMA_SxCR_PL_1;
    DMA2_STREAM0_REGS->FCR = 0U;

    /* ADC prescaler /8 */
    ADC_COMMON_REGS->CCR &= ~(3UL << 16);
    ADC_COMMON_REGS->CCR |= (3UL << 16);

    ADC1_REGS->CR1 = 0U;
    ADC1_REGS->CR2 = 0U;

    /* Sample time 480 cycles for channels 0,1,2 (bits = 111 each). */
    ADC1_REGS->SMPR2 = (7UL << (3U * 0U)) |
                       (7UL << (3U * 1U)) |
                       (7UL << (3U * 2U));

    ADC1_REGS->SQR1 = (2UL << 20);   /* 3 conversions => L = 2 */
    ADC1_REGS->SQR2 = 0U;
    ADC1_REGS->SQR3 = (0UL << 0) | (1UL << 5) | (2UL << 10);

    ADC1_REGS->CR2 = ADC_CR2_CONT | ADC_CR2_DMA | ADC_CR2_DDS | ADC_CR2_ADON;

    DMA2_STREAM0_REGS->CR |= DMA_SxCR_EN;
    ADC1_REGS->CR2 |= ADC_CR2_SWSTART;
}

uint16_t ADC_ReadCurrent(void)
{
    uint16_t raw = ADC_ReadRaw(0);
    uint16_t mA = (uint16_t)(((uint32_t)raw * 5000U) / 4095U);
    return (uint16_t)(mA * current_calibration);
}

uint16_t ADC_ReadSpeed(void)
{
    uint16_t raw = ADC_ReadRaw(1);
    uint16_t rpm = (uint16_t)(((uint32_t)raw * 500U) / 4095U);
    return (uint16_t)(rpm * speed_calibration);
}

uint16_t ADC_ReadTemp(void)
{
    uint16_t raw = ADC_ReadRaw(2);
    return (uint16_t)(((uint32_t)raw * 100U) / 4095U);
}

uint16_t ADC_ReadRaw(uint8_t channel)
{
    uint32_t sum = 0U;
    uint32_t i;

    if (channel >= ADC_NUM_CHANNELS) {
        return 0U;
    }

    /* Average same channel across DMA circular slots for stable reading. */
    for (i = 0U; i < ADC_DMA_BUFFER_SIZE; i++) {
        sum += adc_dma_buffer[(i * ADC_NUM_CHANNELS) + channel];
    }

    return (uint16_t)(sum / ADC_DMA_BUFFER_SIZE);
}

uint16_t* ADC_GetDMABuffer(void)
{
    return &adc_dma_buffer[0];
}
