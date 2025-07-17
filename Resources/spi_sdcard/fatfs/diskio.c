/* Includes ------------------------------------------------------------------*/
#include "diskio.h"
#include "ffconf.h"
#include "../sdcard.h" 
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/
extern SD_CardInfo sd_card_info;  

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
    BYTE drv                /* Physical drive number (0..) */
)
{
    int Status;
    switch (drv) 
    {
        case 0:
            Status = SD_Init(); 
            if (Status == 0) {
                return RES_OK;
            } else {
                return STA_NOINIT;
            }
        case 1:
        case 2:
        case 3:
            return STA_NOINIT; 
        default:
            return STA_NOINIT;
    }
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
    BYTE drv        /* Physical drive number (0..) */
)
{
    switch (drv)
    {
        case 0:
            return sd_card_info.type == SD_TYPE_ERR ? STA_NOINIT : RES_OK;
        case 1:
        case 2:
        case 3:
            return STA_NOINIT;
        default:
            return STA_NOINIT;
    }
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
    BYTE drv,       /* Physical drive number (0..) */
    BYTE *buff,     /* Data buffer to store read data */
    DWORD sector,   /* Sector address (LBA) */
    BYTE count      /* Number of sectors to read (1..255) */
)
{
    int Status;
    if (!count) {
        return RES_PARERR;
    }
    switch (drv)
    {
        case 0:
            Status = SD_ReadSector(sector, buff, count);
            if (Status == 0) {
                return RES_OK;
            } else {
                return RES_ERROR;
            }
        case 1:
        case 2:
        case 3:
            return RES_NOTRDY;
        default:
            return RES_ERROR;
    }
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
#if _READONLY == 0
DRESULT disk_write (
    BYTE drv,           /* Physical drive number (0..) */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Sector address (LBA) */
    BYTE count          /* Number of sectors to write (1..255) */
)
{
    int Status;
    if (!count) {
        return RES_PARERR;
    }
    switch (drv)
    {
        case 0:
            Status = SD_WriteSector(sector, (uint8_t*)buff, count);
            if (Status == 0) {
                return RES_OK;
            } else {
                return RES_ERROR;
            }
        case 1:
        case 2:
        case 3:
            return RES_NOTRDY;
        default:
            return RES_ERROR;
    }
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
    BYTE drv,       /* Physical drive number (0..) */
    BYTE ctrl,      /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    switch (drv)
    {
        case 0:
            switch (ctrl) 
            {
                case CTRL_SYNC:
                    return RES_OK;
                case GET_SECTOR_COUNT:
                    *(DWORD*)buff = SD_GetSectorCount();
                    if (*(DWORD*)buff > 0) {
                        return RES_OK;
                    } else {
                        return RES_ERROR;
                    }
                case GET_SECTOR_SIZE:
                    *(WORD*)buff = SD_BLOCK_SIZE;
                    return RES_OK;
                case GET_BLOCK_SIZE:
                    *(DWORD*)buff = 1; 
                    return RES_OK;
                default:
                    return RES_PARERR;
            }
        case 1:
        case 2:
        case 3:
            return RES_NOTRDY;
        default:
            return RES_PARERR;
    }
}

/*-----------------------------------------------------------------------*/
/* User defined function to give a current time to fatfs module          */
/* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31)    */
/* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2)          */
/*-----------------------------------------------------------------------*/
DWORD get_fattime (void)
{
    return 0;
}
