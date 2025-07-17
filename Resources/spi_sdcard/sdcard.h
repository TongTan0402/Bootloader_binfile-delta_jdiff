#ifndef _LIB_SDCARD_H_
#define _LIB_SDCARD_H_

#include "spi.h"
#include "../spi_sdcard/fatfs/ff.h"        
#include "../spi_sdcard/fatfs/diskio.h"    

// SD card type definitions
#define SD_TYPE_ERR         	0x00
#define SD_TYPE_MMC         	0x01
#define SD_TYPE_V1          	0x02
#define SD_TYPE_V2          	0x04
#define SD_TYPE_V2HC        	0x06

// SD card command definitions
#define CMD0    	0   // Card reset
#define CMD1    	1
#define CMD8    	8   // SEND_IF_COND
#define CMD9    	9   // Read CSD
#define CMD10   10  // Read CID
#define CMD12   12  // Stop transmission
#define CMD16   16  // Set sector size
#define CMD17   17  // Read single sector
#define CMD18   18  // Read multi sector
#define CMD23   23  // Pre-erase blocks
#define CMD24   24  // Write single sector
#define CMD25   25  // Write multi sector
#define CMD41   41  // SEND_OP_COND
#define CMD55   55  // APP_CMD
#define CMD58   58  // Read OCR
#define CMD59   59  // Enable/disable CRC

// SD card response definitions
#define MSD_RESPONSE_NO_ERROR       	0x00
#define MSD_IN_IDLE_STATE           						0x01
#define MSD_RESPONSE_FAILURE        		0xFF

// Data write response definitions
#define MSD_DATA_OK                 							0x05
#define MSD_DATA_CRC_ERROR          			0x0B
#define MSD_DATA_WRITE_ERROR        		0x0D
#define MSD_DATA_OTHER_ERROR        		0xFF

// Block size
#define SD_BLOCK_SIZE               							512

// SD Card structure
typedef struct {
    uint8_t type;       		// SD card type (MMC, V1, V2, V2HC)
    uint32_t capacity;  // Card capacity in sectors
} SD_CardInfo;

// Function prototypes
char SD_Init();                            						// Initialize SD card
uint32_t SD_GetSectorCount();             	// Get total sector count
uint8_t SD_ReadSector(uint32_t sector, uint8_t* buffer, uint8_t cnt);  // Read sectors
uint8_t SD_WriteSector(uint32_t sector, uint8_t* buffer, uint8_t cnt); 	// Write sectors
DWORD SD_getFileSize(const char* filename);

// File system functions (FATFS integration)
char SD_WriteFile(const char* filename, const char* data, const DWORD length, DWORD offset);
char SD_ReadFile(const char* filename, unsigned char* data, const DWORD length, DWORD offset); // Read data from file 

// Disk I/O functions for FATFS
DSTATUS SD_disk_initialize(BYTE pdrv);
DSTATUS SD_disk_status(BYTE pdrv);
DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

#endif /* _LIB_SDCARD_H_ */
