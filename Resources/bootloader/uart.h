/*
 * @file       UART library
 * @board      STM32F10x
 * @author     Tong Sy Tan
 * @date       Sun, 09/02/2025
*/

#ifndef __UART__
#define __UART__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "stm32f10x.h"                  // Device header

#define REMAP 		1
#define NO_REMAP 	0

typedef struct 
{
	void (*Init)(uint32_t baudrate, uint8_t remap);
	void (*Print)(const char *str, ...);
	uint16_t (*Scan)(uint8_t *data);
	
}UART_TypedefStruct;

extern UART_TypedefStruct UART1;

#ifdef __cplusplus
}
#endif

#endif
