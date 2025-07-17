#include "flash.h"

/**
 * @brief 	Flash Write
 * @param 	address:Địa chỉ để ghi dữ liệu
 * @param 	data: Dữ liệu để ghi
 * @param 	length: Số lượng dữ liệu cần ghi
 * @retval 	None
 * Hàm này ghi dữ liệu vào bộ nhớ flash tại địa chỉ đã cho.
 * Nó mở khóa flash, xóa trang tại địa chỉ đó, và sau đó ghi dữ liệu vào.
 * Sau khi hoàn thành, nó khóa lại bộ nhớ flash để bảo vệ.
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
 * @brief 	Flash Read
 * @param 	address: Địa chỉ để đọc dữ liệu
 * @param 	data: Con trỏ để lưu dữ liệu đã đọc
 * @param 	length: Số lượng dữ liệu cần đọc
 * Hàm này đọc dữ liệu từ bộ nhớ flash tại địa chỉ đã cho.
 * Nó lặp qua từng từ và lưu giá trị vào con trỏ data.
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

/**
 * @brief 	Flash Write Byte
 * @param 	address: Địa chỉ để ghi dữ liệu
 * @param 	data: Dữ liệu để ghi
 * @param 	length: Số lượng dữ liệu cần ghi
 */
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

/**
 * @brief 	Flash Read Byte
 * @param 	address: Địa chỉ để đọc dữ liệu
 * @param 	data: Con trỏ để lưu dữ liệu đã đọc
 * @param 	length: Số lượng dữ liệu cần đọc
 * Hàm này đọc dữ liệu từ bộ nhớ flash tại địa chỉ đã cho.
 * Nó lặp qua từng byte và lưu giá trị vào con trỏ data.
 */
void Flash_ReadByte(uint32_t address, uint8_t *data, uint32_t length)
{
	while(length--)
	{
		*data++ = *((uint8_t *)address);
		
		address += 1;
	}	
}
