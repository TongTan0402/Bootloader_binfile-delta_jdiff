#include "spi.h" 

void SPI1_Setup(uint16_t spi_baurate)
{
    SPI_InitTypeDef SPI_InitStruct;    
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // Enable clock for Alternate Function I/O (AFIO)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    

		// Enable clock for GPIOA and SPI1 peripherals
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
		
		// Configure PA5 as Alternate Function Push-Pull (SCK for SPI1)
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;           					// Select PA5 pin
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;     // Set mode to Alternate Function Push-Pull
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;   // Set GPIO speed to 50MHz
		GPIO_Init(GPIOA, &GPIO_InitStruct);             								// Apply configuration to GPIOA

		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStruct);
	
		GPIOA->BSRR = GPIO_Pin_4;
		
    // Configure SPI peripheral settings
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 	// Set SPI to full-duplex, 2-line mode
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;                      			// Set SPI to Master mode
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;                 	 			// Set data size to 8 bits
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;                         			// Clock polarity: idle low
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;                       		// Clock phase: data captured on first edge
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;                          					// NSS (Slave Select) managed by software
    SPI_InitStruct.SPI_BaudRatePrescaler = spi_baurate;             			// Set baud rate prescaler from input parameter
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;                 					// Data transmission starts with Most Significant Bit
    SPI_InitStruct.SPI_CRCPolynomial = 7;                           						// CRC polynomial value (default is 7)
    SPI_Init(SPI1, &SPI_InitStruct);                                									// Initialize SPI peripheral with settings
    SPI_Cmd(SPI1, ENABLE);                                          								// Enable the SPI peripheral
}

/**
 * @brief Set the SPI speed by adjusting the baud rate prescaler
 * @param SPIx SPI peripheral (SPI1 or SPI2)
 * @param SPI_BaudRate Baud rate prescaler value
 */
void SPI1_SetSpeed(uint16_t spi_baurate) 
{
	SPI1->CR1 &= 0xFFC7;            		// Clear baud rate bits
	SPI1->CR1 |= spi_baurate;      			// Set new baud rate
	SPI_Cmd(SPI1, ENABLE);          	// Re-enable SPI1
}

// Function to transfer a byte via SPI and return received data
uint8_t SPI1_Transfer(uint8_t data)
{
	// Wait until the transmit buffer is empty
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	
	// Send the data byte
	SPI_I2S_SendData(SPI1, data);

	// Wait until data is received in the receive buffer
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	
	// Return the received data byte
	return SPI_I2S_ReceiveData(SPI1);
}
