#include "fsm.h"
#include "uart.h"
#include "flash.h"
#include "xdelta/delta.h"

#define FSM_GET_PATCH_ADDRESS()		\
	if(fsm_app_indication.app_address == (uint32_t)(~0)) while(1); \
  (fsm_app_indication.patch_address = ((uint32_t)((fsm_app_indication.app_address + fsm_app_indication.app_length) / FLASH_PAGE_SIZE) +1) * FLASH_PAGE_SIZE)

#define FSM_GET_FIRMWARE_ADDRESS()		\
	if(fsm_app_indication.patch_address == (uint32_t)(~0)) while(1); \
  (fsm_app_indication.firmware_address = ((uint32_t)((fsm_app_indication.patch_address + fsm_app_indication.patch_length) / FLASH_PAGE_SIZE) +1) * FLASH_PAGE_SIZE)


#define FSM_APP_INDICATION_ADDRESS		0x0800FC00 // Address to store app indication data
#define FSM_DEFAULT_APP_ADDRESS		    0x08004000 // Default application address
#define FSM_DEFAULT_FIRMWARE_ADDRESS	0x0800A000 // Default patch address

// Khoảng cách từ đầu frame đến data frame
#define DIS_FROM_START_TO_DATA_STATE		(uint8_t)((uint8_t *)&fsm_frame.data - (uint8_t *)&fsm_frame)

/**
 * Trạng thái của gói tin
 * Enum này định nghĩa các trạng thái khác nhau của gói tin trong quá trình truyền nhận dữ liệu.
 * Mỗi trạng thái tương ứng với một bước trong quá trình xử lý gói tin, từ việc bắt đầu đến việc kiểm tra tổng.
 * Các trạng thái này được sử dụng để xác định cách xử lý dữ liệu trong FSM (Finite State Machine).
 */
typedef enum
{
	FSM_PACKET_STATE_START,
	FSM_PACKET_STATE_TYPE_MESSAGE,
	FSM_PACKET_STATE_LENGTH,
	FSM_PACKET_STATE_DATA,
	FSM_PACKET_STATE_CHECK_SUM
	
} PacketState_e;

/**
 * Loại bản tin
 * Enum này định nghĩa các loại bản tin khác nhau mà FSM có thể xử lý.
 * Mỗi loại bản tin tương ứng với một hành động hoặc dữ liệu cụ thể trong quá trình truyền nhận.
 * Các loại bản tin này được sử dụng để xác định cách xử lý dữ liệu trong FSM.
 */
typedef enum
{
	FSM_TYPE_MESSAGE_START, // PATCH OR FIRMWARE (8 BIT) | FIRMWARE_LENGTH (32 BIT)
	FSM_TYPE_MESSAGE_DATA,
	FSM_TYPE_MESSAGE_END,
	
	FSM_TYPE_MESSAGE_COUNT
	
} TypeMessage_e;


typedef enum
{
	FSM_START_TYPE_FIRMWARE,
	FSM_START_TYPE_PATCH
	
} StartType_e;

/**
 * FSM Frame structure
 * Struct này định nghĩa cấu trúc của một khung dữ liệu FSM.
 * Nó bao gồm các trường để lưu trữ thông tin về địa chỉ bắt đầu, loại tin nhắn, chiều dài dữ liệu,
 * dữ liệu thực tế và tổng kiểm tra.
 */
typedef struct __PACKED 
{
	uint16_t 	start;
	uint8_t 	type_message;
	uint16_t 	length;					
	uint8_t 	data[FLASH_PAGE_SIZE + 1]; 
	uint16_t 	check_sum;
} FSM_Frame_s;


/* Private Variables -------------------------------------------------------------*/
FSM_Frame_s 	fsm_frame;
uint8_t 			*fsm_frame_ptr = &fsm_frame;

PacketState_e		fsm_packet_state = FSM_PACKET_STATE_START;

uint16_t fsm_num_of_message_in_a_chunk = 0;
uint8_t fsm_data[FLASH_PAGE_SIZE + 1];
uint8_t fsm_start_type = FSM_START_TYPE_FIRMWARE;
static volatile uint16_t check_sum = 0;
uint16_t	fsm_index = 0;

/* Public Variables -------------------------------------------------------------*/
App_Indication_t 		fsm_app_indication;


/* Private Functions --------------------------------------------------------------*/
/**
 * @brief Tính tổng kiểm tra cho dữ liệu
 * @param data Con trỏ đến dữ liệu cần tính tổng kiểm tra
 * @param length Số lượng byte trong dữ liệu
 */
uint16_t CheckSum(uint8_t *data, uint16_t length)
{
  uint16_t sum = 0;
  while(length--) sum += *data++;
  return ~sum + 1;
}

/**
 * @brief Reset the FSM frame
 * Hàm này đặt lại trạng thái của khung dữ liệu FSM.
 */
void FSM_ResetFrame(void)
{
	fsm_index = 0;
	fsm_packet_state = FSM_PACKET_STATE_START;
}


void FSM_GetAppIndication(void)
{
	uint32_t length = sizeof(fsm_app_indication);
  Flash_Read(FSM_APP_INDICATION_ADDRESS, (uint32_t *)&fsm_app_indication, length);
	if(fsm_app_indication.app_address != FSM_DEFAULT_APP_ADDRESS) fsm_app_indication.app_address = FSM_DEFAULT_APP_ADDRESS;
	if(fsm_app_indication.firmware_address != FSM_DEFAULT_FIRMWARE_ADDRESS) fsm_app_indication.firmware_address = FSM_DEFAULT_FIRMWARE_ADDRESS;
	fsm_app_indication.patch_length = 0;
}

void FSM_LoadAppIndicationIntoFlash(void)
{
  Flash_Write(FSM_APP_INDICATION_ADDRESS, (uint32_t *)&fsm_app_indication, sizeof(fsm_app_indication));
}


/* Public Functions ---------------------------------------------------------------*/
/**
 * @brief Nhận thông điệp từ FSM
 * @param str_1_byte Con trỏ đến chuỗi dữ liệu 1 byte
 * @return Mã xác nhận của quá trình nhận thông điệp
 */
FSM_Ack_e	 FSM_GetMessage(uint8_t **str_1_byte)
{
	FSM_Ack_e 	ack = 0;
	uint8_t data = *(*str_1_byte)++;
	switch(fsm_packet_state)
	{
		case FSM_PACKET_STATE_START:
			fsm_frame_ptr[fsm_index] = data;
			if(fsm_index == 1)
			{
				if(fsm_frame_ptr[0] != 0xAA || fsm_frame_ptr[1] != 0x55) ack = FSM_ACK_START_ERROR;
				// Nếu hai byte đầu tiên là đúng, chuyển sang trạng thái loại tin nhắn
				else fsm_packet_state = FSM_PACKET_STATE_TYPE_MESSAGE;
			}
			break;
		

		case FSM_PACKET_STATE_TYPE_MESSAGE:
			if(data >= FSM_TYPE_MESSAGE_COUNT) ack = FSM_ACK_TYPE_MESSAGE_ERROR;
			// Nếu type message hợp lệ, lưu nó và chuyển sang trạng thái chiều dài
			else
			{
				fsm_frame_ptr[fsm_index] = data;
				fsm_packet_state = FSM_PACKET_STATE_LENGTH;
			}
			break;
		
			
		case FSM_PACKET_STATE_LENGTH:
		{
			static uint8_t done = 0;
			fsm_frame_ptr[fsm_index] = data;
			if(done++) 
			{
				if(fsm_frame.length) fsm_packet_state = FSM_PACKET_STATE_DATA;
				else fsm_packet_state = FSM_PACKET_STATE_CHECK_SUM;
				done = 0;
			}
		}
		break;
		
			
		case FSM_PACKET_STATE_DATA:
			fsm_frame_ptr[fsm_index] = data;
			if(fsm_frame.type_message != FSM_TYPE_MESSAGE_START) fsm_data[fsm_num_of_message_in_a_chunk++] = data;

			if(fsm_index >= fsm_frame.length + DIS_FROM_START_TO_DATA_STATE - 1)
			{
				fsm_packet_state = FSM_PACKET_STATE_CHECK_SUM;
			}
			break;
			
			
		case FSM_PACKET_STATE_CHECK_SUM:
		{
			static uint8_t done = 0;
			if(done == 0) fsm_frame.check_sum = 0;
			fsm_frame.check_sum |= data << (done * 8);
			
			if(done++)
			{
				check_sum = CheckSum(fsm_frame_ptr, fsm_frame.length + DIS_FROM_START_TO_DATA_STATE);
				done = 0;
				// Nếu kiểm tra checksum không khớp, trả về lỗi
				if(fsm_frame.check_sum != CheckSum(fsm_frame_ptr, fsm_frame.length + DIS_FROM_START_TO_DATA_STATE)) 
				{
					fsm_num_of_message_in_a_chunk -= fsm_frame.length;
					ack = FSM_ACK_CHECK_SUM_ERROR;
				}
				else ack = FSM_ACK_SENT_SUCCESSFULLY;
			}
		}
		break;
			
		default:
			break;
	}
	
	fsm_index++; // Tăng chỉ số để trỏ đến byte tiếp theo trong khung dữ liệu
	
	// Nếu ack khác 0, reset khung dữ liệu
	if(ack)
	{
		FSM_ResetFrame();
	}
	return ack;
}

/**
 * @brief Gửi phản hồi từ FSM
 * @param ack Mã xác nhận để gửi
 * Hàm này gửi mã xác nhận về trạng thái của quá trình xử lý khung dữ liệu.
 * @return None
 */
void FSM_Response(FSM_Ack_e	 ack)
{
	UART1.Print("%d\n", ack);
}
/**
 * @brief In bản tin nhắn hiện tại
 * @param None
 * @return None
 */
void FSM_PrintMessage(void)
{
	UART1.Print("Packet: ");
	uint8_t *data_ptr = &fsm_frame;
	for(int i=0; i<(fsm_frame.length + DIS_FROM_START_TO_DATA_STATE); i++)
	{
		UART1.Print("%h", *data_ptr++);
	}
	
	UART1.Print("%h\n", fsm_frame.check_sum);
}


void Swap_AppAndFirmware(void)
{
	uint32_t source_address = fsm_app_indication.firmware_address;
	uint32_t des_address = fsm_app_indication.app_address;
	int size = fsm_app_indication.firmware_length; 
	while(size > 0)
	{
		Flash_Write(des_address, (uint32_t *)source_address, FLASH_PAGE_SIZE);
		source_address 	+= FLASH_PAGE_SIZE;
		des_address 		+= FLASH_PAGE_SIZE;
		size 						-= FLASH_PAGE_SIZE;
	}
}

/**
 * @brief Nạp dữ liệu vào bộ nhớ flash
 * @param None
 * @return 1 nếu nạp thành công, 0 nếu không thành công
 * Hàm này nạp dữ liệu vào bộ nhớ flash từ các khung dữ liệu đã nhận.
 */
FSM_Ack_e FSM_LoadDataIntoFlash(void)
{
	static TypeMessage_e  fsm_type_message = FSM_TYPE_MESSAGE_START;
	static uint32_t 			address;
	FSM_Ack_e 						ack = FSM_ACK_SENT_SUCCESSFULLY;
	
	switch(fsm_frame.type_message)
	{
		case FSM_TYPE_MESSAGE_START: // PATCH OR FIRMWARE
		{
			fsm_start_type = fsm_frame.data[0];
			fsm_app_indication.firmware_length = *((uint32_t *)(&fsm_frame.data[1]));
			address = fsm_app_indication.firmware_address;

			if(fsm_start_type == FSM_START_TYPE_PATCH) 
			{
				FSM_GET_PATCH_ADDRESS();
				address = fsm_app_indication.patch_address;
			}

			fsm_type_message = FSM_TYPE_MESSAGE_DATA;
			break;
		}

		case FSM_TYPE_MESSAGE_DATA:
		{
			if(fsm_type_message == FSM_TYPE_MESSAGE_DATA)
			{
				if(fsm_start_type == FSM_START_TYPE_FIRMWARE)
				{
					if(fsm_num_of_message_in_a_chunk >= FLASH_PAGE_SIZE) 
					{
						Flash_WriteByte(address, fsm_data, FLASH_PAGE_SIZE);
						address += FLASH_PAGE_SIZE;
						fsm_num_of_message_in_a_chunk %= FLASH_PAGE_SIZE;
					}
				}
				else if(fsm_start_type == FSM_START_TYPE_PATCH)
				{
					if(fsm_num_of_message_in_a_chunk >= FLASH_PAGE_SIZE)
					{
						Flash_WriteByte(address, fsm_data, FLASH_PAGE_SIZE);
						address += FLASH_PAGE_SIZE;
						fsm_app_indication.patch_length += FLASH_PAGE_SIZE;
						fsm_num_of_message_in_a_chunk %= FLASH_PAGE_SIZE;
					}
				}
			}
			else ack = FSM_ACK_START_ERROR;
			break;
		}
		
		case FSM_TYPE_MESSAGE_END:
		{
			fsm_type_message = FSM_TYPE_MESSAGE_START;
			
			if(fsm_start_type == FSM_START_TYPE_FIRMWARE)
			{
				Flash_WriteByte(address, fsm_data, fsm_num_of_message_in_a_chunk);
				fsm_app_indication.app_length = fsm_app_indication.firmware_length;
			}
			else if(fsm_start_type == FSM_START_TYPE_PATCH)
			{
				Flash_WriteByte(address, fsm_data, fsm_num_of_message_in_a_chunk);
				fsm_app_indication.patch_length += fsm_num_of_message_in_a_chunk;
				
				if(fsm_app_indication.patch_length > 5)
				{
					FSM_GET_FIRMWARE_ADDRESS();
					Delta_Run();
				}
			}
			
			if(fsm_app_indication.patch_length > 5 || fsm_start_type == FSM_START_TYPE_FIRMWARE)
			{
				Swap_AppAndFirmware();
				FSM_LoadAppIndicationIntoFlash();
			}
			ack = FSM_ACK_UPDATE_FIRMWARE_SUCESSFULLY;
			break;
		}
		default:
			break;
	}
	return ack;
}