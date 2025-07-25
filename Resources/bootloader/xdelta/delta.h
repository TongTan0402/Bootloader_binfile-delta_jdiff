

#ifndef _DELTA_
#define _DELTA_


#ifdef __cplusplus
extern "C"{
#endif
#include <stdio.h>
#include "../../bootloader/uart.h"
#include "../../spi_sdcard/sdcard.h"


typedef struct
{
  uint32_t address;
  uint32_t offset;
  uint32_t size;
} sfio_stream_t;


uint8_t Delta_Run();

#ifdef __cplusplus
}
#endif

#endif


