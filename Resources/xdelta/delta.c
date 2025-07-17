#include "delta.h"
#include "janpatch.h"

//FATFS fs;

static unsigned char source_buf[SIZE_BUFFER];
static unsigned char target_buf[SIZE_BUFFER];
static unsigned char patch_buf [SIZE_BUFFER];

size_t sfio_fread(void* data, 				size_t size, size_t count, sfio_stream_t* stream);
size_t sfio_fwrite(const void *data, 	size_t size, size_t count, sfio_stream_t *stream);
int sfio_fseek(sfio_stream_t *stream, long int offset, int origin);
long sfio_tell(sfio_stream_t *stream);

janpatch_ctx ctx =
{
  {source_buf, SIZE_BUFFER}, 
  {target_buf, SIZE_BUFFER}, 
  {patch_buf , SIZE_BUFFER}, 

  &sfio_fread,
  &sfio_fwrite,
  &sfio_fseek,
  &sfio_tell,
};

//void Delta_Mount(void)
//{
//	res = f_mount(0, &fs);
//  if (res != FR_OK) 
//	{
//    printf("mount filesystem 0 failed : %d\n\r", res);
//  }
//}

void Delta_Run(char *name_old_file, char *name_patch_file, char *name_new_file)
{
  sfio_stream_t sources;
  sfio_stream_t patchs;
  sfio_stream_t targets;

  sources.name_file = name_old_file;
  sources.offset    = 0;
  sources.size      = SD_getFileSize(name_old_file);

  patchs.name_file  = name_patch_file;
  patchs.offset     = 0;
  patchs.size       = SD_getFileSize(name_patch_file);;

  targets.name_file = name_new_file;
  targets.offset    = 0;
  targets.size      = (sources.size + patchs.size);

  janpatch(ctx, (FIL*)&sources,  (FIL*)&patchs,  (FIL*)&targets);
	UART1.Print("Kich thuoc file %s la %d bytes\n", (char*)name_old_file, SD_getFileSize(name_old_file));
  UART1.Print("Kich thuoc file %s la %d bytes\n", (char*)name_patch_file , SD_getFileSize(name_patch_file));
	UART1.Print("Kich thuoc file %s la %d bytes\n", (char*)name_new_file, SD_getFileSize(name_new_file));
}

size_t sfio_fread(void* data, size_t size, size_t count, sfio_stream_t* stream)
{
  size_t bytesToRead;

  bytesToRead = size * count;

  if (stream->offset + count > stream->size)
  {
    bytesToRead = stream->size - stream->offset;
  }

	char res;
	res = SD_ReadFile(stream->name_file, data, bytesToRead, stream->offset);
  return res ? bytesToRead : res;
}

size_t sfio_fwrite(const void *data, size_t size, size_t count, sfio_stream_t *stream)
{
  size_t bytesToRead;

  bytesToRead = size * count;

  if (stream->offset + count > stream->size)
  {
    bytesToRead = stream->size - stream->offset;
  }

	char res;
	res = SD_WriteFile(stream->name_file, data, bytesToRead, stream->offset);
	
  return res ? bytesToRead : res;
}

int sfio_fseek(sfio_stream_t *stream, long int offset, int origin)
{
  if (offset > stream->size)
  {
    return -1;
  }
  else
  {
    stream->offset = offset;
  }
  return 0;
}

long sfio_tell(sfio_stream_t *stream)
{
  if (stream == NULL)
  {
    return -1;
  }
  return stream->offset;
}
