void I2S_DMA_init(void)
{
	// Disable peripherals
	SPI2->CR1 &= ~(SPI2->CR1);
	DMAMUX1_Channel0->CCR &= ~(DMAMUX1_Channel0->CCR);
	DMA1_Stream0->CR &= ~(DMA1_Stream0->CR);
	DMA1_Stream1->CR &= ~(DMA1_Stream1->CR);
	MDMA_Channel2->CCR &= ~(MDMA_Channel2->CCR);

	// DMA configuration:
	// Stream0 -> RX, Stream1 -> TX
	// Reset FIFO control register
	DMA1_Stream0->FCR &= ~(DMA1_Stream0->FCR);
	DMA1_Stream1->FCR &= ~(DMA1_Stream1->FCR);
	// Configure DMA control register
	// Ciruclar mode |  priority 0 | memory increment mode | transfer/half-transfer complete interrupt enabled | periph data alignment | memory data alignment | transfer-direction
	DMA1_Stream0->CR |= DMA_SxCR_CIRC | DMA_SxCR_PL_0 | DMA_SxCR_MINC | DMA_SxCR_HTIE | DMA_SxCR_TCIE | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0;
	DMA1_Stream1->CR |= DMA_SxCR_CIRC | DMA_SxCR_PL_0 | DMA_SxCR_MINC | DMA_SxCR_HTIE | DMA_SxCR_TCIE | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_DIR_0;
	// Set peripheral address to SPI rx/tx data registers
	DMA1_Stream0->PAR = (uint32_t) &(SPI2->RXDR);
	DMA1_Stream1->PAR = (uint32_t) &(SPI2->TXDR);
	// Set rx buffer sram address in dma stream 0 memory 0 address register.
	// Buffer address is defined in linker script (section .dma_buffer) and set via compiler attribute
	DMA1_Stream0->M0AR = (uint32_t)0x30000000;
	// Number of bytes to transfer
	DMA1_Stream0->NDTR = (SAMPLE_BLOCK * sizeof(*rx_buffer));
	DMA1_Stream1->NDTR = (SAMPLE_BLOCK * sizeof(*tx_buffer));
	// Clear interrupt registers
	DMA1->LISR &= ~(DMA1->LISR);
	DMA1->HISR &= ~(DMA1->HISR);
	DMA1->LIFCR &= ~(DMA1->LIFCR);
	DMA1->HIFCR &= ~(DMA1->HIFCR);
	
	// DMAMUX configuration:
	// Enable event generation and set DMAREQ_ID in configuration register
	// Value for SPI2 rx ID from DMAMUX1 mapping table
	DMAMUX1_Channel2->CCR |= DMAMUX_CxCR_EGE | 39U;
	// Request generator configuration register
	// Generator enable | rising edge trigger | SIG ID
	DMAMUX1_RequestGenerator2->RGCR = DMAMUX_RGxCR_GE | DMAMUX_RGxCR_GPOL_0 | 2U;
	// Clear interrupt registers
	DMAMUX1_ChannelStatus->CFR &= ~(DMAMUX1_ChannelStatus->CFR);
	DMAMUX1_RequestGenStatus->RGSR &= ~(DMAMUX1_RequestGenStatus->RGSR);
	DMAMUX1_RequestGenStatus->RGCFR &= ~(DMAMUX1_RequestGenStatus->RGCFR);
	
	// MDMA channel 2
	MDMA_Channel2->CCR |= MDMA_CCR_PL_0 | MDMA_CCR_TCIE | MDMA_CCR_CTCIE;
	//MDMA_Channel2->CTCR |= 
}