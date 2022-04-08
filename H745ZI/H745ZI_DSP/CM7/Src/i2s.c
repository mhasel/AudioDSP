// i2s.c, Michael Haselberger
// Description: Initialization code for the I2S peripheral

#define DMA

#include "i2s.h"

I2S_HandleTypeDef hi2s2;

#if defined(DMA)
DMA_HandleTypeDef hdma_i2s2_rx;
DMA_HandleTypeDef hdma_i2s2_tx;
#endif

// I2S2 init function
void MX_I2S2_Init(void)
{
	// configure SPI2 peripheral as I2S half duplex master receive
	hi2s2.Instance = SPI2;
	hi2s2.Init.Mode = I2S_MODE_MASTER_FULLDUPLEX;
	hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
	// 24 bit data on 32 bit frame
	hi2s2.Init.DataFormat = I2S_DATAFORMAT_24B;
	// enable master clock output
	hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
	// sampling frequency of 48 KHz
	hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_48K;
	// clock polarity = low
	hi2s2.Init.CPOL = I2S_CPOL_LOW;
	// big endian
	hi2s2.Init.FirstBit = I2S_FIRSTBIT_MSB;
	hi2s2.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
	// right aligned data
	hi2s2.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
	// keep IO state enabled (recommended by STM for H7 devices) **spi: set MasterKeepIOState to avoid glitches**
	hi2s2.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_ENABLE;
	
	if (HAL_I2S_Init(&hi2s2) != HAL_OK)
	{
		Error_Handler();
	}
}

// microcontroller support package - specific I2S init function
void HAL_I2S_MspInit(I2S_HandleTypeDef *i2sHandle)
{
	
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	
	// set up I2S instance SPI2
	if (i2sHandle->Instance == SPI2)
	{
		// enable peripheral clock for SPI2
		__HAL_RCC_SPI2_CLK_ENABLE();
		// enable gpio clock for pin-regions B and C
		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();

		/*  SPI2S1 GPIO Configuration
			PC3     ------> I2S2_SDO
			PC2_C   ------> I2S2_SDI
			PB10    ------> I2S2_CK
			PB12    ------> I2S2_WS
			PC6     ------> I2S2_MCK
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_6;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_15 ;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#if defined(DMA)
		/* I2S2 DMA Init */
		/* SPI2_RX Init */
		hdma_i2s2_rx.Instance = DMA1_Stream0;
		hdma_i2s2_rx.Init.Request = DMA_REQUEST_SPI2_RX;
		hdma_i2s2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_i2s2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_i2s2_rx.Init.MemInc = DMA_MINC_ENABLE;
		hdma_i2s2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
		hdma_i2s2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
		hdma_i2s2_rx.Init.Mode = DMA_CIRCULAR;
		hdma_i2s2_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
		hdma_i2s2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		if (HAL_DMA_Init(&hdma_i2s2_rx) != HAL_OK)
		{
			Error_Handler();
		}

		__HAL_LINKDMA(i2sHandle, hdmarx, hdma_i2s2_rx);

		/* SPI2_TX Init */
		hdma_i2s2_tx.Instance = DMA1_Stream1;
		hdma_i2s2_tx.Init.Request = DMA_REQUEST_SPI2_TX;
		hdma_i2s2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma_i2s2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_i2s2_tx.Init.MemInc = DMA_MINC_ENABLE;
		hdma_i2s2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
		hdma_i2s2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
		hdma_i2s2_tx.Init.Mode = DMA_CIRCULAR;
		hdma_i2s2_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
		hdma_i2s2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		if (HAL_DMA_Init(&hdma_i2s2_tx) != HAL_OK)
		{
			Error_Handler();
		}

		__HAL_LINKDMA(i2sHandle, hdmatx, hdma_i2s2_tx);
#endif
	}
}

#if defined(DMA)
void MX_DMA_Init(void)
{
	/* DMA interrupt init */
	/* DMA1_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
	/* DMA1_Stream1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

}
#endif

void HAL_I2S_MspDeInit(I2S_HandleTypeDef *i2sHandle)
{
	if (i2sHandle->Instance == SPI2)
	{
		__HAL_RCC_SPI2_CLK_DISABLE();
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2 | GPIO_PIN_6);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_15);
#if defined(DMA)
		HAL_DMA_DeInit(i2sHandle->hdmarx);
		HAL_DMA_DeInit(i2sHandle->hdmatx);
#endif
	}
}
