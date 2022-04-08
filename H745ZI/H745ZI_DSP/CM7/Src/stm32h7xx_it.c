#include "main.h"
#include "stm32h7xx_it.h"

extern DMA_HandleTypeDef hdma_i2s2_rx;
extern DMA_HandleTypeDef hdma_i2s2_tx;


/* Private functions ---------------------------------------------------------*/
void DMA1_Stream0_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_i2s2_rx);
}

/**
  * @brief This function handles DMA1 stream1 global interrupt.
  */
void DMA1_Stream1_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_i2s2_tx);
}

/******************************************************************************/
/*            Cortex-M7 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
}

/******************************************************************************/
/*                 STM32H7xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32h7xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles SPI1 interrupt request.
  * @param  None
  * @retval None
  */
//void SPI1_IRQHandler(void)
//{
//  HAL_SPI_IRQHandler(&SpiHandle1);
//}

/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
//void SPI1_DMA_RX_IRQHandler(void)
//{
//  HAL_DMA_IRQHandler(SpiHandle1.hdmarx);
//}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
//void SPI1_DMA_TX_IRQHandler(void)
//{
//  HAL_DMA_IRQHandler(SpiHandle1.hdmatx);
//}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
