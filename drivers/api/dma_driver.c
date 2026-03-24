/**
 * @file dma_driver.c
 * @brief DMA Driver Implementation
 * @date 2026-03-22
 */

#include "dma_driver.h"

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================ */

/** DMA status flag */
static volatile uint8_t dma_running = 0;

/* ============================================================================
 * PUBLIC FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

/**
 * @brief DMA_Init_ADC - Khởi tạo DMA cho ADC
 */
void DMA_Init_ADC(uint16_t *pBuffer, uint32_t size)
{
    /*
     * TODO: Implement DMA initialization for ADC
     * 
     * Steps:
     * 1. Enable RCC clock:
     *    - RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE)
     * 
     * 2. Configure DMA2 Stream 0:
     *    - DMA_InitStructure.DMA_Channel = DMA_Channel_0
     *    - DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR
     *    - DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)pBuffer
     *    - DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory
     *    - DMA_InitStructure.DMA_BufferSize = size
     *    - DMA_InitStructure.DMA_Mode = DMA_Mode_Circular (auto-restart)
     *    - DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable
     *    - DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable
     *    - DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord
     *    - DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord
     *    - DMA_InitStructure.DMA_Priority = DMA_Priority_High
     *    - DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable
     *    - DMA_Init(DMA2_Stream0, &DMA_InitStructure)
     * 
     * 3. Enable DMA stream:
     *    - DMA_Cmd(DMA2_Stream0, ENABLE)
     *    - dma_running = 1
     * 
     * Note: ADC_DMACmd() should enable ADC-side DMA, done in ADC_Init()
     */
}

/**
 * @brief DMA_Start - Bắt đầu transfer
 */
void DMA_Start(void)
{
    /*
     * TODO: Enable DMA
     * 
     * DMA_Cmd(DMA2_Stream0, ENABLE)
     * dma_running = 1
     */
}

/**
 * @brief DMA_Stop - Dừng transfer
 */
void DMA_Stop(void)
{
    /*
     * TODO: Disable DMA
     * 
     * DMA_Cmd(DMA2_Stream0, DISABLE)
     * dma_running = 0
     */
}

/**
 * @brief DMA_IsRunning - Check status
 */
uint8_t DMA_IsRunning(void)
{
    /*
     * TODO: Return DMA running status
     */
    
    return dma_running;
}

/**
 * @brief DMA_GetTransferCount - Get transfer count
 */
uint16_t DMA_GetTransferCount(void)
{
    /*
     * TODO: Get remaining transfer count
     * 
     * return DMA_GetCurrDataCounter(DMA2_Stream0)
     */
    
    return DMA_GetCurrDataCounter(DMA2_Stream0);
}
