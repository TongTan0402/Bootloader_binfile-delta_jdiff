#include "flash.h"

/**
	*************************************************************************
	* @brief 	Flash Write
	* @param 	address
	* @param  data: Data array/struct
	* @param  length: length of array/struct
	* @retval None
	*************************************************************************
*/

void Flash_Write(uint32_t address, uint32_t *data, uint32_t length)
{
	FLASH_Unlock();
	
	FLASH_ErasePage(address);
	
	length /= sizeof(*data);
	
	while(length--)
	{
		FLASH_ProgramWord(address, *data++);
		address += sizeof(*data);
	}
	FLASH_Lock();
}

/**
	*************************************************************************
	* @brief 	Flash Read
	* @param 	address
	* @param  data: Data array/struct
	* @param  length: length of array/struct
	* @retval None
	*************************************************************************
*/

void Flash_Read(uint32_t address, uint32_t *data, uint32_t length)
{
	length /= sizeof(*data);
	while(length--)
	{
		*data++ = *((uint32_t *)address);
		
		address += sizeof(*data);
	}
}

void Flash_WriteByte(uint32_t address, uint8_t *data, uint32_t length)
{
	FLASH_Unlock();
	
	FLASH_ErasePage(address);
	
	while(length > 0)
	{
		uint16_t temp = length == 1 ? (*data++) : ((*data++) | ((*data++) << 8));
		FLASH_ProgramHalfWord(address, temp);
		address += 2;
		length  -= 2;
	}
	FLASH_Lock();
}
void Flash_ReadByte(uint32_t address, uint8_t *data, uint32_t length)
{
	while(length--)
	{
		*data++ = *((uint8_t *)address);
		
		address += 1;
	}	
}
