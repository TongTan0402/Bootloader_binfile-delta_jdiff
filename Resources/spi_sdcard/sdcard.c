#include "sdcard.h"
#include <string.h>
#define NULL (void *)0

SD_CardInfo sd_card_info;

static uint8_t SD_WaitReady();
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc);
static uint8_t SD_RecvData(uint8_t* buf, uint16_t len);
static uint8_t SD_SendBlock(uint8_t* buf, uint8_t cmd);

static __inline void SD_CS_Low()
{
	GPIOA->BRR = GPIO_Pin_4;
}

static __inline void SD_CS_High() 
{
	GPIOA->BSRR = GPIO_Pin_4;
}

static uint8_t SD_WaitReady() 
{
    uint32_t t = 0;
    do 
		{
        if (SPI1_Transfer(0xFF) == 0xFF) return 0;
        t++;
    } 
		while (t < 0xFFFF);
    return 1;
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc) 
{
    uint8_t r1, retry = 0x1F;
    SD_CS_High();
    SPI1_Transfer(0xFF);
    SD_CS_Low();
    if (SD_WaitReady()) 
		{
				return 0xFF;
		}

    SPI1_Transfer(cmd | 0x40);
    SPI1_Transfer(arg >> 24);
    SPI1_Transfer(arg >> 16);
    SPI1_Transfer(arg >> 8);
    SPI1_Transfer(arg);
    SPI1_Transfer(crc);

    do 
		{
        r1 = SPI1_Transfer(0xFF);
    } 
		while ((r1 & 0x80) && retry--);

    return r1;
}

static uint8_t SD_RecvData(uint8_t* buf, uint16_t len) 
{
    uint16_t t = 0xFFFF;
    while (SPI1_Transfer(0xFF) != 0xFE && t--);
    if (t == 0) 
		{
				return 1;
		}

    while (len--) 
		{
				*buf++ = SPI1_Transfer(0xFF);
		}
		
    SPI1_Transfer(0xFF); 
    SPI1_Transfer(0xFF);
    return 0;
}

static uint8_t SD_SendBlock(uint8_t* buf, uint8_t cmd) 
{
    if (SD_WaitReady()) 
		{
				return 1;
		}
		
    SPI1_Transfer(cmd);
    if (cmd != 0xFD) 
		{
        for (uint16_t t = 0; t < SD_BLOCK_SIZE; t++) 
				{
						SPI1_Transfer(buf[t]);
				}
				
        SPI1_Transfer(0xFF); 
        SPI1_Transfer(0xFF);
        uint8_t resp = SPI1_Transfer(0xFF);
        if ((resp & 0x1F) != 0x05) 
				{
						return 2;
				}
    }
    return 0;
}

static void SD_PowerOn(void) 
{
  uint8_t cmd_arg[6];
  uint32_t Count = 0x1FFF;
  
  SD_CS_High();
  
  for(int i = 0; i < 10; i++)
  {
    SPI1_Transfer(0xFF);
  }
  
  /* SPI Chips Select */
  SD_CS_Low();
  
  /* GO_IDLE_STATE */
  cmd_arg[0] = (CMD0 | 0x40);
  cmd_arg[1] = 0;
  cmd_arg[2] = 0;
  cmd_arg[3] = 0;
  cmd_arg[4] = 0;
  cmd_arg[5] = 0x95;
  
	/* Gửi lệnh Power On */
  for (int i = 0; i < 6; i++)
  {
    SPI1_Transfer(cmd_arg[i]);
  }
  
  /* Chờ phản hồi */
  while ((SPI1_Transfer(0xff) != 0x01) && Count)
  {
    Count--;
  }
  
  SD_CS_High();
  SPI1_Transfer(0XFF);
}

char SD_Init() 
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

		SD_PowerOn();

    SD_CS_Low();
    for (uint8_t i = 0; i < 10; i++) 
		{
				SPI1_Transfer(0xFF);
		}

    uint8_t retry = 0, r1 = 0;
    do 
		{
        r1 = SD_SendCmd(CMD0, 0, 0x95);
    } 
		while (r1 != MSD_IN_IDLE_STATE && retry++ < 200);

    if (retry >= 200) return 1;

    r1 = SD_SendCmd(CMD8, 0x1AA, 0x87);
    if (r1 == MSD_IN_IDLE_STATE) 
		{
        sd_card_info.type = SD_TYPE_V2;
        uint8_t ocr[4];
        for (uint8_t i = 0; i < 4; i++) 
				{
						ocr[i] = SPI1_Transfer(0xFF);
				}
				
        if (ocr[2] == 0x01 && ocr[3] == 0xAA) 
				{
            while(1) 
						{
                if (SD_SendCmd(CMD55, 0, 0x01) <= 1 && SD_SendCmd(CMD41, 0x40000000, 0x01) == 0)
                  break; /* ACMD41 with HCS bit */
            } 

            r1 = SD_SendCmd(CMD58, 0, 0x01);
            if (r1 == 0x00) 
						{
                for (uint8_t i = 0; i < 4; i++) ocr[i] = SPI1_Transfer(0xFF);
                if (ocr[0] & 0x40) sd_card_info.type = SD_TYPE_V2HC;
            }
        }
    } 
		else 
		{
        retry = 0;
        do 
				{
            r1 = SD_SendCmd(CMD1, 0, 0x01);
        } 
				while (r1 != 0x00 && retry++ < 200);
				
        if (retry >= 200) 
				{
						return 3;
				}
        sd_card_info.type = SD_TYPE_V1;
    }

    SD_CS_High();
    SPI1_Transfer(0xFF);
    return 0;
}

uint32_t SD_GetSectorCount() 
{
    uint8_t csd[16];
    if (SD_SendCmd(CMD9, 0, 0x01) != 0 || SD_RecvData(csd, 16) != 0) 
		{
				return 0;
		}

    uint32_t capacity;
    if ((csd[0] & 0xC0) == 0x40) 
		{ 
				// SDHC
        uint16_t csize = csd[9] + ((uint16_t)csd[8] << 8) + 1;
        capacity = (uint32_t)csize << 10;
    } 
		else 
		{ 
				// SDSC
        uint8_t n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
        uint16_t csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
        capacity = (uint32_t)csize << (n - 9);
    }
		
    sd_card_info.capacity = capacity;
    SD_CS_High();
    return capacity;
}

uint8_t SD_ReadSector(uint32_t sector, uint8_t* buffer, uint8_t cnt) 
{
    uint8_t r1;
    if (sd_card_info.type != SD_TYPE_V2HC) sector <<= 9;

    if (cnt == 1) 
		{
        r1 = SD_SendCmd(CMD17, sector, 0x01);
        if (r1 == 0) 
				{
						r1 = SD_RecvData(buffer, SD_BLOCK_SIZE);
				}
    } 
		else 
		{
        r1 = SD_SendCmd(CMD18, sector, 0x01);
        do 
				{
            r1 = SD_RecvData(buffer, SD_BLOCK_SIZE);
            buffer += SD_BLOCK_SIZE;
        } 
				while (--cnt && r1 == 0);
				
        SD_SendCmd(CMD12, 0, 0x01);
    }
		
    SD_CS_High();
    return r1;
}

uint8_t SD_WriteSector(uint32_t sector, uint8_t* buffer, uint8_t cnt) 
{
    uint8_t r1;
    if (sd_card_info.type != SD_TYPE_V2HC) 
		{
				sector <<= 9;
		}

    if (cnt == 1) 
		{
        r1 = SD_SendCmd(CMD24, sector, 0x01);
        if (r1 == 0) 
				{
						r1 = SD_SendBlock(buffer, 0xFE);
				}
    } 
		else 
		{
        if (sd_card_info.type != SD_TYPE_MMC) 
				{
            SD_SendCmd(CMD55, 0, 0x01);
            SD_SendCmd(CMD23, cnt, 0x01);
        }
				
        r1 = SD_SendCmd(CMD25, sector, 0x01);
        if (r1 == 0) 
				{
            do 
						{
                r1 = SD_SendBlock(buffer, 0xFC);
                buffer += SD_BLOCK_SIZE;
            } 
						while (--cnt && r1 == 0);
            r1 = SD_SendBlock(0, 0xFD);
        }
    }
    SD_CS_High();
    return r1;
}

char SD_WriteFile(const char* filename, const char* data, const DWORD length, DWORD offset) 
{
	FIL file;
	FATFS fs;
	UINT written;
	
	if(f_mount(0, &fs) != FR_OK) return 0;
	
	if(f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
	{
		if(f_lseek(&file, offset) != FR_OK) return 0;
		
		if(f_write(&file, data, length,  &written) != FR_OK) return 0;
		
		if(f_close(&file) != FR_OK) return 0;
	}
	f_mount(1, &fs);
	return 1;
}

char SD_ReadFile(const char* filename, unsigned char* data, const DWORD length, DWORD offset) 
{  
	FIL file;
	FATFS fs;
	UINT written;
	
	if(f_mount(0, &fs) != FR_OK) return 0;
	
	if(f_open(&file, filename, FA_OPEN_EXISTING | FA_READ) == FR_OK)
	{
		if(f_lseek(&file, offset) != FR_OK) return 0;
		
		if(f_read(&file, data, length,  &written) != FR_OK) return 0;
		
		if(f_close(&file) != FR_OK) return 0;
	}
	f_mount(1, &fs);

	return 1;
}

DWORD SD_getFileSize(const char* filename)
{
	DWORD fileSize = 0;
	FATFS fs;
  FIL file;
	if(f_mount(0, &fs) != FR_OK) return 0;
  if (f_open(&file, filename, FA_READ) == FR_OK)
  {
    // Get the size of the file
    fileSize = f_size(&file);
    f_close(&file); // Close the file when done

    // Now you have the fileSize in bytes, you can do whatever you want with it
  }
	f_mount(1, &fs);
  return fileSize;
}

DSTATUS SD_disk_initialize(BYTE pdrv) 
{
    return SD_Init() != 0 ? STA_NOINIT : 0; 
}

DSTATUS SD_disk_status(BYTE pdrv) 
{
    return sd_card_info.type == SD_TYPE_ERR ? STA_NOINIT : 0;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) 
{
    return SD_ReadSector(sector, buff, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) 
{
    return SD_WriteSector(sector, (uint8_t*)buff, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) 
{
    switch (cmd) 
		{
        case CTRL_SYNC:
            return RES_OK;
				
        case GET_SECTOR_COUNT:
            *(DWORD*)buff = SD_GetSectorCount();
            return RES_OK;
				
        case GET_SECTOR_SIZE:
            *(WORD*)buff = SD_BLOCK_SIZE;
            return RES_OK;
				
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;
				
        default:
            return RES_PARERR;
    }
}
