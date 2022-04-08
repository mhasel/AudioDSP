// i2c.c, Michael Haselberger
// Description: Peripheral initialization code for I2C communication.

#include "main.h"

I2C_HandleTypeDef hi2c1;
void MX_I2C1_Init(void)
{
	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x307075B1;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Analogue filter */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Digital filter */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
	{
		Error_Handler();
	}
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *i2cHandle)
{

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	if (i2cHandle->Instance == I2C1)
	{
		/** Initializes the peripherals clock */
		PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
		PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_D2PCLK1;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		/**I2C1 GPIO Configuration
			PB6     ------> I2C1_SCL
			PB7     ------> I2C1_SDA
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		__HAL_RCC_I2C1_CLK_ENABLE();
	}
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *i2cHandle)
{

	if (i2cHandle->Instance == I2C1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_I2C1_CLK_DISABLE();
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);
	}
}