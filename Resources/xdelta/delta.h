

#ifndef _DELTA_
#define _DELTA_


#ifdef __cplusplus
extern "C"{
#endif
#include <stdio.h>
#include "../bootloader/uart.h"
#include "../spi_sdcard/sdcard.h"


#define SIZE_BUFFER 512


typedef struct
{
  char* name_file;
  size_t offset;
  size_t size;
} sfio_stream_t;


void Delta_Run(char *name_old_file, char *name_patch_file, char *name_new_file);

#ifdef __cplusplus
}
#endif

#endif


