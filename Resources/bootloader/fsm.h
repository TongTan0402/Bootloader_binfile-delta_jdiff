
#ifndef __FSM__
#define __FSM__

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"                  // Device header

typedef enum
{
	FSM_ACK_START_ERROR = 0x01,
	FSM_ACK_TYPE_MESSAGE_ERROR,
	FSM_ACK_CHECK_SUM_ERROR,
	FSM_ACK_SENT_SUCCESSFULLY
	
}FSM_Ack_e;


extern uint32_t FSM_base_address;

uint8_t FSM_GetMessage(uint8_t **str_2_byte);
uint8_t FSM_LoadDataIntoFlash(void);
void FSM_Response(uint8_t ack);
void FSM_PrintMessage(void);


#ifdef __cplusplus
}
#endif

#endif
