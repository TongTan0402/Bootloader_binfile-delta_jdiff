#ifndef __SYS__
#define __SYS__
#ifdef __cplusplus
extern "C" {
#endif
#include "stm32f10x.h"                  // Device header

#include "../bootloader/uart.h"
#include "../bootloader/fsm.h"
#include "../bootloader/flash.h"
#include "../spi_sdcard/sdcard.h"
#include "../bootloader/xdelta/delta.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define APP_INDICATION_ADDRESS  (uint32_t)(0x0800FC00)
#define FLAG_UPDATING							0x01
#define FLAG_UPDATE_SUCCESSFULLY 	0x02

#define IS_FLAG(FLAG)			(flag & FLAG)
#define SET_FLAG(FLAG)		(flag |= FLAG)
#define RESET_FLAG(FLAG)	(flag &= ~FLAG)

void Sys_Init(void);
void Sys_Run(void);

#ifdef __cplusplus
}
#endif
#endif