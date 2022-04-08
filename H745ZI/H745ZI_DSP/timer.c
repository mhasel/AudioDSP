// timer.c, Michael Haselberger
// Description: Initialization code for the timer peripheral. Runs in encoder mode with internal debouncing.

#include "main.h"

TIM_HandleTypeDef htim2;

void MX_TIM2_Init(void)
{
	TIM_Encoder_InitTypeDef sConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 100;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
	sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC1Filter = 15;
	sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC2Filter = 0;
	if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *tim_encoderHandle)
{

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if (tim_encoderHandle->Instance == TIM2)
	{
		__HAL_RCC_TIM2_CLK_ENABLE();
		/* TIM2 GPIO Configuration
			PA0     ------> TIM2_CH1
			PB3 (JTDO/TRACESWO)     ------> TIM2_CH2
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_0;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
}

void HAL_TIM_Encoder_MspDeInit(TIM_HandleTypeDef *tim_encoderHandle)
{

	if (tim_encoderHandle->Instance == TIM2)
	{
		__HAL_RCC_TIM2_CLK_DISABLE();
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3);
	}
}