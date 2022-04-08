// main.c: Michael Haselberger
// Description: Cortex M7 main entry point
#include "main.h"
#include <stdio.h>

// ------------ CONSTANTS --------------------
const uint32_t MAX24BIT = (1 << 24);
const float32_t DOWNSCALE24BIT = 1.0f / MAX24BIT;
const float32_t UPSCALE24BIT = (float32_t) MAX24BIT;

// ------------ ENUMERATIONS ----------------- 
enum callback_state
{
	NONE = 0,
	HALFCPLT,
	CPLT
};

// ------------ FUNCTION PROTOTYPES -----------------
void Error_Handler(void);
static void MPU_conf(void);
void peripheral_init(void);
static void rx_samples(enum ping_pong);
static void tx_samples(enum ping_pong);
static void pass_through(void);
static void run_fx(uint8_t mode);
static void update_menu(uint16_t count, uint8_t menu_depth);
// ------------ STATIC VARIABLES -----------------


#if defined(POLLING_MODE)
static uint32_t rx_buffer[SAMPLE_BLOCK] = {0};
static uint32_t tx_buffer[SAMPLE_BLOCK] = {0};


#elif defined (DMA)
/*
 * DMA has no access to DTCM, so a different RAM region needs to be specified for the DMA buffers.
 * Excerpt from Reference Manual:
	D1 domain, AXI SRAM:
		– AXI SRAM is mapped at address 0x2400 0000 and accessible by all system
		masters except BDMA through D1 domain AXI bus matrix. AXI SRAM can be
		used for application data which are not allocated in DTCM RAM or reserved for
		graphic objects (such as frame buffers)

 * This can be done by modifying the linker script (the section specified by the gcc attribute has to be defined in the linker script)
 * and setting compiler attributes. A small region in this RAM region is protected by the MPU to avoid read/write access by the CPU unless explicitly specified.
 * Also, the buffer is 32-byte aligned. The DMA requires the buffers to be aligned in 4-bytes (or multiples thereof) and the DTCM requires 32-byte aligned data.
 * Even after taking all this into account, cache-coherency operations (clean and invalidate) are required before and every CPU buffer access.
*/

// DMA buffers in non-TCM RAM region. 32-byte aligned (cache width)
uint32_t rx_buffer[DMA_BUFFER_SIZE] __attribute__((aligned(32))) __attribute__((section(".dma_buffer")));
uint32_t tx_buffer[DMA_BUFFER_SIZE] __attribute__((aligned(32))) __attribute__((section(".dma_buffer")));

// in/out buffers
// codec samples in stereo, but pedal is going to be used in mono exclusively (audio jacks and instrument cables are mono)
// so every second sample isn't needed -> half the buffer size will suffice for calculations
// guitar signal is mono - right channel will be ignored for now. since I'm using double buffering,
// buffer size can effectively be a quarter of rx/tx buffers
float32_t left_in[PING_PONG_BUFFER_SIZE] __attribute__((aligned(32)));
float32_t left_out[PING_PONG_BUFFER_SIZE] __attribute__((aligned(32)));

float32_t volume = 0.5f;

#endif

// flags for DMA callback
static volatile uint8_t callback_state = NONE;
static volatile uint8_t mode = FXNONE;

// flag for GPIO menu button callback
static volatile uint8_t btn_pressed = 0;


// effect handles
delay_handle_t delay_handle;
tremolo_handle_t tremolo_handle;
overdrive_handle_t overdrive_handle;
fuzz_handle_t fuzz_handle;
ring_mod_handle_t ring_mod_handle;

// Sample filter taps located in fx_lib.c
extern float32_t filter_taps[NUM_TAPS];





int main(void)
{	
	// Enable the CPU Cache
	SCB_EnableICache();
	SCB_EnableDCache();

	/*  HAL_Init description from STM32 example code:
     
        STM32H7xx HAL library initialization:
        - Systick timer is configured by default as source of time base, but user
            can eventually implement his proper time base source (a general purpose
            timer for example or other time source), keeping in mind that Time base
            duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
            handled in milliseconds basis.
        - Set NVIC Group Priority to 4
        - Low Level Initialization
    */
	HAL_Init();

	// configure the system clock for 480 MHz
	SystemClock_Config();
	// configure SPI1 and SPI2 clocks for 48 KHz I2S
	// (real clock frequency is 0.01% below target 48KHz with this configuration)
	PeriphCommonClock_Config();

#if defined(CHECK_CLK)
	// Check clock frequencies
	uint32_t sys_clk= HAL_RCCEx_GetD1SysClockFreq();
	uint32_t d1_periph_clk = HAL_RCCEx_GetD1PCLK1Freq();
	uint32_t spi_clk = HAL_RCCEx_GetPLL2ClockFreq();
#endif
	
#if defined(BOOTCM4)
	// Enables Cortex M4 (disabled via option bytes). 
	HAL_RCCEx_EnableBootCore(RCC_BOOT_C2);
#endif	
	
	// Initialize peripherals
	peripheral_init();
	HAL_Delay(50);

	//volatile int fpu_check = __FPU_USED;
	
#if defined(DMA)
	
	HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t *)tx_buffer, (uint16_t *)rx_buffer, DMA_BUFFER_SIZE);
#endif
	/*
	 *https://community.st.com/s/question/0D50X0000C0yPO3/when-do-we-need-to-call-clean-and-invalidate-d-cache
		"As usual with DMA: when the external device writes to the memory, you INVALIDATE the cache on the area, this means you discard anything that could come to the cache from the local side and accept the actual data in the RAM.
		If external party reads the memory, you FLUSH the cache. This is called "Clean" in ARM documentation.
		Flush ensures that all data written by the CPU arrives all way down to the memory and is available to the other party.
		Even if the memory in question is not cached (so flush and invalidate operations do not apply) you may need to use memory barriers (DSB, DMB) after writing to memory mapped registers. Especially those related to interrupts."

		-- pa
	 **/
	delay_init(&delay_handle, left_in, left_out, 400, 0.4, 0.4);
	tremolo_init(&tremolo_handle, left_in, left_out, 0.7f, 0.8f);
	init_fir_filter(filter_taps);

	mode = FXNONE;


	
	while (1)
    {		
		switch (callback_state)
		{
			case NONE:
				display_menu(btn_pressed, &mode);
				break;
			case HALFCPLT:
				//SCB_InvalidateDCache_by_Addr((uint32_t *)rx_buffer, PING_PONG_BUFFER_SIZE);
				rx_samples(PING);
				run_fx(mode);
				tx_samples(PING);
				//SCB_CleanDCache_by_Addr((uint32_t *)tx_buffer, PING_PONG_BUFFER_SIZE);
				callback_state = NONE;
#if defined(CHECK_TIMELINESS)
				GPIOB->ODR &= ~GPIO_PIN_8;
#endif
				break;
			case CPLT:
				//SCB_CleanDCache_by_Addr((uint32_t *)(rx_buffer + PING_PONG_BUFFER_SIZE * sizeof(tx_buffer[0])), PING_PONG_BUFFER_SIZE);
				rx_samples(PONG);
				run_fx(mode);
				tx_samples(PONG);
				//SCB_CleanDCache_by_Addr((uint32_t *)(tx_buffer + PING_PONG_BUFFER_SIZE * sizeof(tx_buffer[0])), PING_PONG_BUFFER_SIZE);
				callback_state = NONE;
#if defined(CHECK_TIMELINESS)
				GPIOB->ODR &= ~GPIO_PIN_9;
#endif
				break;
			default:
				Error_Handler();
		}

#if defined(DMA_DEBUG)
 		
		uint32_t err_i2s = HAL_I2S_GetError(&hi2s2);
		uint32_t err_rx_dma = HAL_DMA_GetError(&hdma_i2s2_rx);
		uint32_t err_tx_dma = HAL_DMA_GetError(&hdma_i2s2_tx);

		if (err_i2s | err_tx_dma | err_rx_dma)
			__asm("bkpt 255");
		#endif // DMA_DEBUG
	#if defined(POLLING_MODE) 
		
			HAL_I2S_Receive(&hi2s2, (uint16_t*)rx_buffer, SAMPLE_BLOCK, 1000);
			HAL_I2S_Transmit(&hi2s2, (uint16_t *)rx_buffer, SAMPLE_BLOCK, 1000);
	#endif // POLLING_MODE
	}
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
	callback_state = HALFCPLT;
#if defined(CHECK_TIMELINESS)
	GPIOB->ODR |= GPIO_PIN_8;
#endif
}
void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	callback_state = CPLT;
#if defined(CHECK_TIMELINESS)
	GPIOB->ODR |= GPIO_PIN_9;
#endif
//	memcpy((tx_buffer + (SAMPLE_BLOCK >> 1) * sizeof(uint32_t)), (rx_buffer + (SAMPLE_BLOCK >> 1) * sizeof(uint32_t)), (SAMPLE_BLOCK >> 1));
}

// EXTI Line9 External Interrupt ISR Handler CallBack
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_9) // If The INT Source Is EXTI Line9 (A9 Pin)
	{
		btn_pressed = 1;
	}
}

// since the audio jacks are mono, but the codec samples for stereo, both left and right channels will have identical information.
// every second sample can be skipped to reduce computational load
static void rx_samples(enum ping_pong p)
{
	// offset is either 0 or half the buffer size (= amount of samples)
	uint16_t offset = p * SAMPLES;
	uint16_t end = SAMPLES + offset;

	for (int i = 0, j = offset; j < end; ++i, j=j+2)
	{
		left_in[i] = (float32_t)rx_buffer[j];
	}
	//__asm("nop");
}
static void tx_samples(enum ping_pong p)
{
	// offset is either 0 or half the buffer size (= amount of samples)
	uint16_t offset = p * SAMPLES;
	uint16_t end = SAMPLES + offset;

	for (int i = 0, j = offset; j < end; ++i, j = j + 2)
	{		
		tx_buffer[j] = (uint32_t)left_out[i];
//		// check if any bits above the 24th bit are set (= overflow) if so, throw breakpoint trap
//		if (tx_buffer[i] & 0xFF00000000)
//			__asm("bkpt 255");
		
//		__asm("nop");
	}
}

static void run_fx(uint8_t mode)
{
	switch (mode)
	{
		case FXDELAY:
			run_delay(&delay_handle);
			break;
		case FXFILTER:
			run_fir_filter(left_in, left_out);
			break;
		case FXTREMOLO:
			run_tremolo(&tremolo_handle);
		case FXFUZZ:
			run_fuzz(&fuzz_handle);
			break;
		case FXOVERDRIVE:
			run_overdrive(&overdrive_handle);
			break;
		case FXRINGMOD:
			run_ring_mod(&ring_mod_handle);
			break;
		default:
			pass_through();
	}
}

static void pass_through() 
{
	const uint16_t size = DMA_BUFFER_SIZE >> 2;
	for (int i = 0; i < size; ++i)
	{
		left_out[i] = left_in[i];
	}
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
	// breakpoint trap. if this callback interrupt function is called, debugger will break/ stop here.
	__asm("bkpt 255");
	__asm("nop");
}

void peripheral_init(void)
{
	// Configure MPU
	MPU_conf();

	// GPIO
	// Enable the GPIO perihperals in RCC AHB4-enable-register
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
	// DMA Controller clock enable
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
	
	// enable gpio PB8 and PB9 for logic analyzer debugging
	GPIO_InitTypeDef gpio_debug;
	gpio_debug.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	gpio_debug.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_debug.Pull = GPIO_PULLUP;
	gpio_debug.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &gpio_debug);
	
	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn); 


	MX_DMA_Init();
	MX_I2S2_Init();
	MX_TIM2_Init();
	MX_I2C1_Init();

	//volatile HAL_StatusTypeDef result = HAL_I2C_IsDeviceReady(&hi2c1, 0x4E, 200, 200);
	lcd_init();
	lcd_send_string("Initializing");
	HAL_Delay(100);
}

// Set up MPU for DMA buffer region. Make not cacheable.
// see AN4838
void MPU_conf()
{
	MPU_Region_InitTypeDef MPU_Init_DMA_buffer;

	// disable MPU while setting parameters
	HAL_MPU_Disable();

	// Enable the following settings.
	MPU_Init_DMA_buffer.Enable = MPU_REGION_ENABLE;

	// Target buffer size from the SRAM 1 (AXI) area (0x2400 0000) of D1 Domain
	MPU_Init_DMA_buffer.BaseAddress = 0x24000000;
	MPU_Init_DMA_buffer.Size = ARM_MPU_REGION_SIZE_16KB;

	// Set access restrictionss
	MPU_Init_DMA_buffer.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_Init_DMA_buffer.TypeExtField = MPU_TEX_LEVEL0;
	MPU_Init_DMA_buffer.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_Init_DMA_buffer.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_Init_DMA_buffer.IsShareable = MPU_ACCESS_SHAREABLE;


	// Set the region/subregion number. (Number that uniquely identifies the setting)
	MPU_Init_DMA_buffer.Number = MPU_REGION_NUMBER0;
	MPU_Init_DMA_buffer.SubRegionDisable = 0x00;
	// Allow code to be executed from the target area (not relevant)
	MPU_Init_DMA_buffer.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
	
	// apply
	HAL_MPU_ConfigRegion(&MPU_Init_DMA_buffer);

	// reenable MPU
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  BSP_LED_Off(LED1);
  /* Turn LED3 on */
  BSP_LED_On(LED3);
  __disable_irq();
  while(1)
  {
  }
}




#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


