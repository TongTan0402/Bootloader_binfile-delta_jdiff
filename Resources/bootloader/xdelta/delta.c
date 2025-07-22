#include "delta.h"
#include "janpatch.h"
#include "../flash.h"
#include "../fsm.h"

// Buffer sizes for fread/fwrite operations
static unsigned char source_buf[FLASH_PAGE_SIZE];
static unsigned char target_buf[FLASH_PAGE_SIZE];
static unsigned char patch_buf [FLASH_PAGE_SIZE];

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
  {source_buf, FLASH_PAGE_SIZE}, 
  {target_buf, FLASH_PAGE_SIZE}, 
  {patch_buf , FLASH_PAGE_SIZE}, 

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
uint8_t Delta_Run()
{
  sfio_stream_t sources;
  sfio_stream_t patchs;
  sfio_stream_t targets;

  sources.address   = fsm_app_indication.app_address;
  sources.offset    = 0;
  sources.size      = fsm_app_indication.app_length;

  patchs.address    = fsm_app_indication.patch_address;
  patchs.offset     = 0;
  patchs.size       = fsm_app_indication.patch_length;

  targets.address   = fsm_app_indication.firmware_address;
  targets.offset    = 0;
  targets.size      = fsm_app_indication.firmware_length;

  return janpatch(ctx, (FIL*)&sources,  (FIL*)&patchs,  (FIL*)&targets);
	
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

	Flash_Read(stream->address + stream->offset, (uint8_t *)data, bytesToRead);
  return bytesToRead;
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
	
	Flash_Write(stream->address + stream->offset, (uint8_t *)data, bytesToRead);
	
  return bytesToRead;
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
  switch(origin)
  {
    case SEEK_SET:
      stream->offset = offset;
      break;
    case SEEK_CUR:
      stream->offset += offset;
      break;
    default:
      return -1;
  }
  if (stream->offset > stream->size) return -1;
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
