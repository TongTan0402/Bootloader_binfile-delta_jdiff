#include "delta.h"
#include "janpatch.h"

// Buffer sizes for fread/fwrite operations
static unsigned char source_buf[SIZE_BUFFER];
static unsigned char target_buf[SIZE_BUFFER];
static unsigned char patch_buf [SIZE_BUFFER];

size_t sfio_fread(void* data, 				size_t size, size_t count, sfio_stream_t* stream);
size_t sfio_fwrite(const void *data, 	size_t size, size_t count, sfio_stream_t *stream);
int sfio_fseek(sfio_stream_t *stream, long int offset, int origin);
long sfio_tell(sfio_stream_t *stream);

/**
 * @brief Context for janpatch operations
 * 
 * This context holds the buffers and function pointers for reading, writing,
 * seeking, and telling the position in the streams used by janpatch.
 */
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


/**
 * @brief Chạy quá trình delta patching
 * Hàm này thực hiện quá trình delta patching bằng cách sử dụng các tệp nguồn, patch và tệp đích.
 * Nó sử dụng cấu trúc sfio_stream_t để quản lý các tệp và kích thước của chúng.
 * Sau khi hoàn thành, nó in ra kích thước của các tệp đã xử lý.
 * @param name_old_file Tên tệp gốc cần cập nhật
 * @param name_patch_file Tên tệp patch chứa các thay đổi
 * @param name_new_file Tên tệp mới sẽ được tạo ra sau quá trình patching
 */
void Delta_Run(char *name_old_file, char *name_patch_file, char *name_new_file)
{
  sfio_stream_t sources;
  sfio_stream_t patchs;
  sfio_stream_t targets;

  sources.name_file = name_old_file;
  sources.offset    = 0;
  sources.size      = SD_GetFileSize(name_old_file);

  patchs.name_file  = name_patch_file;
  patchs.offset     = 0;
  patchs.size       = SD_GetFileSize(name_patch_file);;

  targets.name_file = name_new_file;
  targets.offset    = 0;
  targets.size      = (sources.size + patchs.size);

  janpatch(ctx, (FIL*)&sources,  (FIL*)&patchs,  (FIL*)&targets);
	UART1.Print("Kich thuoc file %s la %d bytes\n", (char*)name_old_file, SD_GetFileSize(name_old_file));
  UART1.Print("Kich thuoc file %s la %d bytes\n", (char*)name_patch_file , SD_GetFileSize(name_patch_file));
	UART1.Print("Kich thuoc file %s la %d bytes\n", (char*)name_new_file, SD_GetFileSize(name_new_file));
}

/**
 * @brief Đọc dữ liệu từ nguồn file stream 
 * @param data Con trỏ đến vùng nhớ để lưu dữ liệu đọc
 * @param size Kích thước của mỗi phần tử cần đọc
 * @param count Số lượng phần tử cần đọc
 * @param stream Con trỏ đến nguồn file stream
 * @return Số lượng byte đã đọc, hoặc 0 nếu không thành công
 */
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

/**
 * @brief Ghi dữ liệu vào file stream
 * @param data Con trỏ đến dữ liệu cần ghi
 * @param size Kích thước của mỗi phần tử cần ghi
 * @param count Số lượng phần tử cần ghi
 * @param stream Con trỏ đến file stream đích
 * @return Số lượng byte đã ghi, hoặc 0 nếu không thành công
 */
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

/**
 * @brief Cài đặt vị trí trong file stream
 * @param stream Con trỏ đến file stream
 * @param offset Vị trí mới cần đặt
 * @param origin Vị trí gốc để tính toán (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return 0 nếu thành công, -1 nếu không thành công
 */
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

/**
 * @brief Nhận vị trí hiện tại trong file stream
 * @param stream Con trỏ đến file stream
 * @return Vị trí hiện tại trong file stream, hoặc -1 nếu không thành công
 */
long sfio_tell(sfio_stream_t *stream)
{
  if (stream == NULL)
  {
    return -1;
  }
  return stream->offset;
}
