#include "fsm.h"
#include "uart.h"
#include "flash.h"

// Distance from the start of the FSM frame to the data field
// This is used to calculate the offset for accessing the data field in the FSM frame structure.
#define DIS_FROM_START_TO_DATA_STATE		(uint8_t)((uint8_t *)&fsm_frame.data - (uint8_t *)&fsm_frame)

// Maximum number of messages in a chunk
// This defines how many messages can be processed in a single chunk of data.
// It is used to limit the number of messages that can be handled at once.
#define CHUNK_SIZE		0x400	//1KB

/**
 * packet state enumeration
 * This enumeration defines the different states of the packet processing in the FSM.
 * It is used to track the current state of the packet as it is being processed.
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
 * Type of message enumeration
 * This enumeration defines the different types of messages that can be received by the FSM.
 * Each type corresponds to a specific operation or data structure that the FSM can handle.
 */
typedef enum
{
	FSM_TYPE_MESSAGE_DATA,
	FSM_TYPE_MESSAGE_END,
	FSM_TYPE_MESSAGE_BASE_ADDRESS,
	
	FSM_TYPE_MESSAGE_COUNT
	
} TypeMessage_e;

/**
 * FSM Frame structure
 * This structure defines the format of a message frame in the FSM.
 * It includes fields for the start bytes, type of message, length of data, actual data, and a checksum.
 * The structure is used to parse incoming messages and prepare responses.
 */
typedef struct
{
	uint16_t 	start;
	uint8_t 	type_message;
	uint8_t 	length;					
	uint8_t 	data[100]; 
	uint8_t 	check_sum;
}FSM_Frame_s;


/* Private Variables -------------------------------------------------------------*/
FSM_Frame_s 				fsm_frame;
uint8_t 						*fsm_frame_ptr = &fsm_frame;

PacketState_e				packet_state = FSM_PACKET_STATE_START;

uint16_t fsm_num_of_message_in_a_chunk = 0;
uint8_t fsm_data[CHUNK_SIZE];

uint8_t	fsm_index = 0;

/* Public Variables -------------------------------------------------------------*/
uint32_t FSM_base_address = 0;


/* Private Functions --------------------------------------------------------------*/
/**
 * @brief Tính tổng kiểm tra cho dữ liệu
 * @param data Con trỏ đến dữ liệu cần tính tổng kiểm tra
 * @param length Số lượng byte trong dữ liệu
 */
uint8_t CheckSum(uint8_t *data, uint8_t length)
{
  uint8_t sum = 0;
  while(length--) sum += *data++;
  return ~sum + 1;
}

/**
 * @brief Chuyển đổi chuỗi sang byte
 * @param s Con trỏ đến chuỗi cần chuyển đổi
 * @return Giá trị byte tương ứng với chuỗi
 * Hàm này chuyển đổi một chuỗi gồm hai ký tự hex thành một giá trị byte.
 * Nó lặp qua từng ký tự, chuyển đổi nó thành giá trị số và tính toán giá trị byte cuối cùng.
 */
uint8_t Convert_StrToByte(char **s) // Con trỏ trỏ đến con trỏ
{
  uint8_t num = 0;
  for(uint8_t i = 0; i<2; i++)
  {
    uint8_t temp = *(*s)++; // Lấy giá trị tại địa chỉ *s: *(*s) và tăng địa chỉ (*s)++
    if(temp >= 'A' && temp <= 'F')        temp = temp - 'A' + 10;
    else if(temp >= 'a' && temp <= 'f')   temp = temp - 'a' + 10;
    else temp -= '0';

    num =  num * 16 + temp;
  }
 
  return num;
}

/**
 * @brief Reset the FSM frame
 * Hàm này đặt lại trạng thái của khung dữ liệu FSM.
 */
void FSM_ResetFrame(void)
{
	fsm_index = 0;
	packet_state = FSM_PACKET_STATE_START;
}


/* Public Functions ---------------------------------------------------------------*/

/**
 * @brief Nhận thông điệp từ FSM
 * @param str_2_byte Con trỏ đến chuỗi dữ liệu 2 byte
 * @return Mã xác nhận của quá trình nhận thông điệp
 * Hàm này xử lý các thông điệp từ FSM bằng cách phân tích cú pháp và xác định trạng thái của gói tin.
 * Nó trả về mã xác nhận tương ứng với kết quả của quá trình nhận thông điệp.
 */
FSM_Ack_e	 FSM_GetMessage(uint8_t **str_2_byte)
{
	FSM_Ack_e 	ack = 0;
	// Convert 2 characters to a byte value
	uint8_t data = Convert_StrToByte(str_2_byte);
	switch(packet_state)
	{
			// Trạng thái bắt đầu: Nhận 2 byte đầu tiên
			// Nếu hai byte đầu tiên không phải là 0xAA và 0x55, trả về lỗi bắt đầu
			// Nếu đúng, chuyển sang trạng thái loại tin nhắn
		case FSM_PACKET_STATE_START:
			fsm_frame_ptr[fsm_index] = data;
			if(fsm_index == 1)
			{
				if(fsm_frame_ptr[0] != 0xAA || fsm_frame_ptr[1] != 0x55) ack = FSM_ACK_START_ERROR;
				// Nếu hai byte đầu tiên là đúng, chuyển sang trạng thái loại tin nhắn
				else packet_state = FSM_PACKET_STATE_TYPE_MESSAGE;
			}
			break;
		
		// Trạng thái loại tin nhắn: Nhận loại tin nhắn
		// Nếu loại tin nhắn không hợp lệ, trả về lỗi loại tin nhắn
		case FSM_PACKET_STATE_TYPE_MESSAGE:
			if(data >= FSM_TYPE_MESSAGE_COUNT) ack = FSM_ACK_TYPE_MESSAGE_ERROR;
			// Nếu type message hợp lệ, lưu nó và chuyển sang trạng thái chiều dài
			else
			{
				fsm_frame_ptr[fsm_index] = data;
				packet_state = FSM_PACKET_STATE_LENGTH;
			}
			break;
		
		// Trạng thái chiều dài: Nhận chiều dài của dữ liệu
		// Nếu chiều dài khác 0, chuyển sang trạng thái dữ liệu
		// Nếu chiều dài là 0, chuyển sang trạng thái kiểm tra tổng
		// Lưu chiều dài vào khung dữ liệu
		case FSM_PACKET_STATE_LENGTH:
			fsm_frame_ptr[fsm_index] = data;
			// Nếu length khác 0, chuyển sang trạng thái dữ liệu
			if(data) packet_state = FSM_PACKET_STATE_DATA;
			// Nếu length là 0, chuyển sang trạng thái kiểm tra tổng
			else packet_state = FSM_PACKET_STATE_CHECK_SUM;
			
			break;
		
		// Trạng thái dữ liệu: Nhận dữ liệu theo chiều dài đã chỉ định
		// Lưu dữ liệu vào mảng dữ liệu và tăng chỉ số
		case FSM_PACKET_STATE_DATA:
			fsm_frame_ptr[fsm_index] = data;
			fsm_data[fsm_num_of_message_in_a_chunk++] = data;
		
			if(fsm_index >= fsm_frame.length + DIS_FROM_START_TO_DATA_STATE - 1)
			{
				packet_state = FSM_PACKET_STATE_CHECK_SUM;
			}
			break;
			
		// Trạng thái kiểm tra tổng: Kiểm tra tổng của dữ liệu đã nhận
		// So sánh tổng kiểm tra với giá trị đã nhận
		// Nếu không khớp, trả về lỗi kiểm tra tổng
		// Nếu khớp, trả về thành công
		// Reset khung dữ liệu sau khi xử lý xong
		case FSM_PACKET_STATE_CHECK_SUM:
			fsm_frame.check_sum = data;
			// Nếu kiểm tra checksum không khớp, trả về lỗi
			if(fsm_frame.check_sum != CheckSum(fsm_frame_ptr, fsm_frame.length + DIS_FROM_START_TO_DATA_STATE)) 
			{
				fsm_num_of_message_in_a_chunk -= fsm_frame.length;
				ack = FSM_ACK_CHECK_SUM_ERROR;
			}
			else ack = FSM_ACK_SENT_SUCCESSFULLY;
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
 * Hàm này in ra nội dung của khung dữ liệu hiện tại.
 * Nó hiển thị các byte của khung dữ liệu, bao gồm cả dữ liệu và kiểm tra tổng.
 * Hàm này hữu ích để kiểm tra và gỡ lỗi quá trình truyền nhận dữ liệu.
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

/**
 * @brief Nạp dữ liệu vào bộ nhớ flash
 * @param None
 * @return 1 nếu nạp thành công, 0 nếu không thành công
 * Hàm này nạp dữ liệu vào bộ nhớ flash từ các khung dữ liệu đã nhận.
 * Nó xử lý các loại tin nhắn khác nhau và ghi dữ liệu vào flash theo địa chỉ đã chỉ định.
 * Nếu nạp thành công, nó cập nhật địa chỉ cơ sở và trả về 1, ngược lại trả về 0.
 */
uint8_t FSM_LoadDataIntoFlash(void)
{
	static uint32_t address = 0;
	uint8_t done = 0;
	
	switch(fsm_frame.type_message)
	{
		case FSM_TYPE_MESSAGE_BASE_ADDRESS:
		{
			fsm_num_of_message_in_a_chunk = 0;
			FSM_base_address = fsm_frame.data[0] << 24 | fsm_frame.data[1] << 16;
			address = FSM_base_address;
			break;
		}
		case FSM_TYPE_MESSAGE_DATA:
		{
			if(fsm_num_of_message_in_a_chunk >= CHUNK_SIZE) 
			{
				Flash_WriteByte(address, fsm_data, fsm_num_of_message_in_a_chunk);
				address += fsm_num_of_message_in_a_chunk;
				fsm_num_of_message_in_a_chunk = 0;
			}
			break;
		}
		case FSM_TYPE_MESSAGE_END:
		{
			Flash_WriteByte(address, fsm_data, fsm_num_of_message_in_a_chunk);
			done = 1;
			break;
		}
		default:
			break;
	}
	return done;
}