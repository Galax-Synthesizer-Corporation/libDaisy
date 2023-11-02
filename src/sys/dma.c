#include "stm32h7xx_hal.h"
#include "sys/dma.h"

// This is set to one level lower priority (+1 higher value)
// than the highest (zero) by default so exti interrupts
// for analog clocking can take priority
#define DSY_DMA_DEFAULT_PRIORITY 1
#define DSY_DMA_AUDIO_PRIORITY   DSY_DMA_DEFAULT_PRIORITY
#define DSY_DMA_ADC_PRIORITY     DSY_DMA_DEFAULT_PRIORITY

#ifdef __cplusplus
extern "C"
{
#endif

    void dsy_dma_init(void)
    {
        // DMA controller clock enable
        __HAL_RCC_DMA1_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();

        // DMA interrupt init
        // The next 4 DMA streams are used for audio codec I/O
        // DMA1_Stream0_IRQn interrupt configuration
        HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, DSY_DMA_AUDIO_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
        // DMA1_Stream1_IRQn interrupt configuration
        HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, DSY_DMA_AUDIO_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
        // DMA1_Stream3_IRQn interrupt configuration
        HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, DSY_DMA_AUDIO_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
        // DMA1_Stream4_IRQn interrupt configuration
        HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, DSY_DMA_AUDIO_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
        // DMA1_Stream2_IRQn interrupt configuration
        // This is used for the ADC Handle DMA
        HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, DSY_DMA_ADC_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
        // DMA1_Stream5_IRQn and DMA2_Stream4_IRQn interrupt configuration for uart rx and tx
        HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
        HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
        // DMA1_Stream6_IRQn interrupt configuration for I2C
        HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
        // DMA2_Stream0_IRQn, interrupt configuration for DAC Ch1
        HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
        // DMA2_Stream1_IRQn, interrupt configuration for DAC Ch2
        HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

        // DMA2_Stream2_IRQn and DMA2_Stream3_IRQn interrupt configuration for SPI
        HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
        HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

        // DMA2_Stream5 used for TIM Channel operation
        HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, DSY_DMA_DEFAULT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    }

    void dsy_dma_deinit(void)
    {
        // DMA controller clock enable
        __HAL_RCC_DMA1_CLK_DISABLE();
        __HAL_RCC_DMA2_CLK_DISABLE();

        // DMA interrupt init
        // DMA1_Stream0_IRQn interrupt configuration
        HAL_NVIC_DisableIRQ(DMA1_Stream0_IRQn);
        // DMA1_Stream1_IRQn interrupt configuration
        HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
        // DMA1_Stream2_IRQn interrupt configuration
        HAL_NVIC_DisableIRQ(DMA1_Stream2_IRQn);
        // DMA1_Stream3_IRQn interrupt configuration
        HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn);
        // DMA1_Stream4_IRQn interrupt configuration
        HAL_NVIC_DisableIRQ(DMA1_Stream4_IRQn);
        // DMA1_Stream5_IRQn and DMA2_Stream4_IRQn interrupt configuration for uart rx and tx
        HAL_NVIC_DisableIRQ(DMA1_Stream5_IRQn);
        HAL_NVIC_DisableIRQ(DMA2_Stream4_IRQn);
        // DMA1_Stream6_IRQn interrupt configuration for I2C
        HAL_NVIC_DisableIRQ(DMA1_Stream6_IRQn);
        // DMA2_Stream0_IRQn, interrupt configuration for DAC Ch1
        HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);
        // DMA2_Stream1_IRQn, interrupt configuration for DAC Ch2
        HAL_NVIC_DisableIRQ(DMA2_Stream1_IRQn);

        // DMA2_Stream2_IRQn and DMA2_Stream3_IRQn interrupt configuration for SPI
        HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn);
        HAL_NVIC_DisableIRQ(DMA2_Stream3_IRQn);
    }

    void dsy_dma_clear_cache_for_buffer(uint8_t* buffer, size_t size)
    {
        // clear all cache lines (32bytes each) that span the memory section
        // of our transmit buffer. This makes sure that the SRAM contains the
        // most recent version of the buffer.
        SCB_CleanDCache_by_Addr(
            (uint32_t*)((uint32_t)(buffer) & ~(uint32_t)0x1F), size + 32);
    }

    void dsy_dma_invalidate_cache_for_buffer(uint8_t* buffer, size_t size)
    {
        // invalidate all cache lines (32bytes each) that span the memory section
        // of our transmit buffer. This makes sure that the cache contains the
        // most recent version of the buffer.
        SCB_InvalidateDCache_by_Addr(
            (uint32_t*)((uint32_t)(buffer) & ~(uint32_t)0x1F), size + 32);
    }

#ifdef __cplusplus
}
#endif
