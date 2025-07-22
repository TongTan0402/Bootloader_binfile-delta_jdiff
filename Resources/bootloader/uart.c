#include "uart.h"
#include <stdio.h>
#include <stdarg.h>

#define UART_RECEIVE_DELAY_FACTOR 1.1

#define MAX_LENGTH 0x420

typedef struct
{
	int16_t length;
	int16_t push_index;
	int16_t pop_index;
	uint8_t data[MAX_LENGTH+1];

}Queue;

// UART structure definition
UART_TypedefStruct 	UART1;
volatile Queue 			queue;


char number[50];
uint16_t delay_receiving_data = 0;
static volatile uint16_t data_timeout = 0;

void UARTx_Init(uint32_t baudrate, uint8_t remap);
void UARTx_SendData(const char *str, va_list args);
uint16_t UARTx_ReceiveData(uint8_t *data);

// Function prototypes for UART initialization and operations
void UART1_Init(uint32_t baudrate, uint8_t remap);
void UART1_Print(const char *str, ...);
uint16_t UART1_Scan(uint8_t *data);

static volatile __inline void Queue_Push(uint8_t data)
{
	if(queue.length < MAX_LENGTH)
	{
		queue.data[queue.push_index] = data;
		queue.push_index = (queue.push_index + 1) % MAX_LENGTH;
		queue.length = queue.length + 1;
	}
}

static volatile uint8_t Queue_Pop(uint8_t *data)
{
	if(queue.length > 0)
	{
		*data = queue.data[queue.pop_index];
		queue.pop_index = (queue.pop_index + 1) % MAX_LENGTH;
		queue.length = queue.length - 1;
		return 1;
	}
	return 0;
}


/**
	*************************************************************************
	* @brief 	Initialize Callback Functions
	* @param 	None
	* @retval None
	*************************************************************************
*/

void UART_FirstInit(void) __attribute__ ((constructor));			// Ham nay se duoc chay truoc khi vao ham main

void UART_FirstInit(void)
{
	UART1.Init  = UART1_Init;
	UART1.Scan  = UART1_Scan;
	UART1.Print = UART1_Print;
}

/**
	*************************************************************************
	* @brief 	UARTx Initialization
	* @param 	USART_TypeDef
	* @param  baudrate
	* @param  remap: remap or not
	* @retval None
	*************************************************************************
*/

void UARTx_Init(uint32_t baudrate, uint8_t remap)
{
	GPIO_InitTypeDef 		GPIO_InitStruct;
	USART_InitTypeDef 	UART_InitStruct;
	
	delay_receiving_data = (uint16_t)(72e7 * UART_RECEIVE_DELAY_FACTOR / 5 / baudrate);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	if(remap)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		
		GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
		
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
		GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6;
		GPIO_Init(GPIOB, &GPIO_InitStruct);
		
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
		GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_7;
		GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
	else
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
		GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
		GPIO_Init(GPIOA, &GPIO_InitStruct);
		
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
		GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_10;
		GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
	
	NVIC_SetPriority(USART1_IRQn, 0);
	NVIC_EnableIRQ(USART1_IRQn);
	
	UART_InitStruct.USART_BaudRate = baudrate;											/* toc do truyen: baud_rates */
	UART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	UART_InitStruct.USART_Parity = USART_Parity_No;
	UART_InitStruct.USART_StopBits = USART_StopBits_1;
	UART_InitStruct.USART_WordLength = USART_WordLength_8b;
	UART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &UART_InitStruct);
	
	USART_Cmd(USART1, ENABLE);																/* cho phep USART hoat dong */
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);						/* ngat nhan USART					*/
}

/**
	*************************************************************************
	* @brief 	UARTx Initialization
	* @param  baudrate
	* @param  remap: remap or not
	* @retval None
	*************************************************************************
*/

void UART1_Init(uint32_t baudrate, uint8_t remap)
{
	UARTx_Init(baudrate, remap);
}

/**
	*************************************************************************
	* @brief 	UART Print
	* @param  str: format string
	* @param  ...: arguments
	* @retval None
	*************************************************************************
*/

void UART1_Print(const char *str, ...)
{
	va_list args;
	va_start(args, str);
	UARTx_SendData(str, args);
	va_end(args);
}

/**
	*************************************************************************
	* @brief 	UART Scan
	* @param  data: received string
	* @retval true if there is a new string
	*************************************************************************
*/

uint16_t UART1_Scan(uint8_t *data)
{
	return UARTx_ReceiveData(data);
}

static __inline void UART_SendChar(const char str)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);
	USART_SendData(USART1, str);
}

static __inline void UART_SendString(const char *str)
{
	do
	{
		UART_SendChar(*str);
		
	}while(*(++str));
}

void UARTx_SendData(const char *str, va_list args)
{
	do{
		if(*str == '%')
		{
			switch(*(++str))
			{
				case 's':
				{
					char *temp_str = va_arg(args, char*);
					UART_SendString(temp_str);
					continue;
					break;
				}
				
				case 'c':
				{
					char temp_str = va_arg(args, char);
					UART_SendChar(temp_str);
					continue;
					break;
				}
				
				case 'd':
				{
					int temp_num = va_arg(args, int);
					sprintf(number, "%d", temp_num);
					UART_SendString(number);
					continue;
					break;
				}
				
				case 'h':
				{
					int temp_num = va_arg(args, int);
					sprintf(number, "%02X", temp_num);
					UART_SendString(number);
					continue;
					
					break;
				}
				
				case 'f':
				{
					float temp_num = va_arg(args, double);
					sprintf(number, "%f", temp_num);
					UART_SendString(number);
					continue;
					break;
				}
			}
		}
		
		UART_SendChar(*str);
	}while(*(++str));
}



uint16_t UARTx_ReceiveData(uint8_t *data)
{
	uint16_t length = 0;
	if(queue.length) 
	{	
		if(data_timeout <= delay_receiving_data) data_timeout = data_timeout + 1;
		else 
		{
			while(Queue_Pop(data++)) length++;
		}
	}
	*(data - 1) = '\0';
	return length;
}

/**
	*************************************************************************
	* @brief 	USARTxIRQn
	* @param 	None
	* @retval None
	*************************************************************************
*/

void USART1_IRQHandler(void)
{
	Queue_Push(USART1->DR);
	data_timeout = 0;
}