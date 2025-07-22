/*
 * @file       Flash library
 * @board      STM32F10x
 * @author     Tong Sy Tan
 * @date       Sun, 11/04/2025
*/



#ifndef __FLASH__
#define __FLASH__
#ifdef __cplusplus
extern "C" {
#endif
#include "stm32f10x.h"                  // Device header
#include <stdint.h>

#define FLASH_PAGE_SIZE 0x400 // 1KB

void Flash_Write(uint32_t address, uint32_t *data, uint32_t length);
void Flash_Read (uint32_t address, uint32_t *data, uint32_t length);

void Flash_WriteByte(uint32_t address, uint8_t *data, uint32_t length);
void Flash_ReadByte (uint32_t address, uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif
#endif
