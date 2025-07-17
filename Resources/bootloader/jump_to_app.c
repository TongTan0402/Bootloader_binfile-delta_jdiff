#include "jump_to_app.h"

void JumpToApp(uint32_t app_address)
{
	USART_DeInit(USART1);   
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
	NVIC_EnableIRQ(USART1_IRQn);
	USART_DeInit(USART2);   
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, DISABLE);
	NVIC_EnableIRQ(USART2_IRQn);
	USART_DeInit(USART3);   
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, DISABLE);
	NVIC_EnableIRQ(USART3_IRQn);
	// Tat ngat
	__disable_irq();
	
	// Lay dia chi MSP v? Reset_Handler
	uint32_t app_stack = *(__IO uint32_t*) app_address;
	uint32_t jump_address = *(__IO uint32_t*) (app_address + 4);
	void (*app_reset_handler)(void) = (void (*)(void)) jump_address;
	
	// Set vector table
	SCB->VTOR = app_address;
	
	// Set MSP
	__set_MSP(app_stack);
	
	// Enable ngat
	__enable_irq();
	
	// Jump
	app_reset_handler();
}


void JumpButton_Init(void)
{
	GPIO_InitTypeDef		GPIO_InitStruct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
	
	GPIO_Init(GPIOB, &GPIO_InitStruct);
}