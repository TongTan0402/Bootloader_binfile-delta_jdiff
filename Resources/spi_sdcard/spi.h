#ifndef _LIB_SPI_H_
#define _LIB_SPI_H_
#include "stm32f10x.h"                  // Device header

void SPI1_Setup(uint16_t spi_baurate);
void SPI1_SetSpeed(uint16_t spi_baurate);
uint8_t SPI1_Transfer(uint8_t data);

#endif
