#include "fsm.h"
#include "uart.h"
#include "flash.h"

#define DIS_FROM_START_TO_DATA_STATE		(uint8_t)((uint8_t *)&fsm_frame.data - (uint8_t *)&fsm_frame)

#define CHUNK_SIZE		0x400	//1KB

typedef enum
{
	FSM_PACKET_STATE_START,
	FSM_PACKET_STATE_TYPE_MESSAGE,
	FSM_PACKET_STATE_LENGTH,
	FSM_PACKET_STATE_DATA,
	FSM_PACKET_STATE_CHECK_SUM
	
} PacketState_e;

typedef enum
{
	FSM_TYPE_MESSAGE_DATA,
	FSM_TYPE_MESSAGE_END,
	FSM_TYPE_MESSAGE_BASE_ADDRESS,
	
	FSM_TYPE_MESSAGE_COUNT
	
} TypeMessage_e;

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
uint8_t CheckSum(uint8_t *data, uint8_t length)
{
  uint8_t sum = 0;
  while(length--) sum += *data++;
  return ~sum + 1;
}

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

void FSM_ResetFrame(void)
{
	fsm_index = 0;
	packet_state = FSM_PACKET_STATE_START;
}


/* Public Functions ---------------------------------------------------------------*/

/**
	* @brief Nhận 2 ký tự một, sau đó chuyển 2 ký tự thành mã hex
	* @param **str_2_byte: con trỏ -> tró tới con trỏ -> trỏ tới giá trị. Mỗi lần gọi, địa chỉ con trỏ tăng lên 2
	* @retval FSM_ACK
*/
FSM_Ack_e	 FSM_GetMessage(uint8_t **str_2_byte)
{
	FSM_Ack_e 	ack = 0;
	uint8_t data = Convert_StrToByte(str_2_byte);
	switch(packet_state)
	{
		case FSM_PACKET_STATE_START:
			fsm_frame_ptr[fsm_index] = data;
			if(fsm_index == 1)
			{
				if(fsm_frame_ptr[0] != 0xAA || fsm_frame_ptr[1] != 0x55) ack = FSM_ACK_START_ERROR;
				else packet_state = FSM_PACKET_STATE_TYPE_MESSAGE;
			}
			break;
		
			
		case FSM_PACKET_STATE_TYPE_MESSAGE:
			if(data >= FSM_TYPE_MESSAGE_COUNT) ack = FSM_ACK_TYPE_MESSAGE_ERROR;
			else
			{
				fsm_frame_ptr[fsm_index] = data;
				packet_state = FSM_PACKET_STATE_LENGTH;
			}
			break;
		
		
		case FSM_PACKET_STATE_LENGTH:
			fsm_frame_ptr[fsm_index] = data;
			if(data) 	packet_state = FSM_PACKET_STATE_DATA;
			else 			packet_state = FSM_PACKET_STATE_CHECK_SUM;
			
			break;
		
		
		case FSM_PACKET_STATE_DATA:
			fsm_frame_ptr[fsm_index] = data;
			fsm_data[fsm_num_of_message_in_a_chunk++] = data;
		
			if(fsm_index >= fsm_frame.length + DIS_FROM_START_TO_DATA_STATE - 1)
			{
				packet_state = FSM_PACKET_STATE_CHECK_SUM;
			}
			break;
			
			
		case FSM_PACKET_STATE_CHECK_SUM:
			fsm_frame.check_sum = data;
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
	fsm_index++;
	
	if(ack)
	{
		FSM_ResetFrame();
	}
	return ack;
}

/**
	* @brief Phản hồi lại khi nhận xong một bản tin
	* @param ack:	1: Start Error - 2: Type Message Error - 3: Check Sum Error - 4: Sent Successfully
	* @retval None
*/
void FSM_Response(FSM_Ack_e	 ack)
{
	UART1.Print("%d\n", ack);
}

/**
	* @brief In bản tin
	* @retval None
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