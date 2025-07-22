
#ifndef __FSM__
#define __FSM__

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"                  // Device header
#include <stdint.h>

typedef enum
{
	FSM_ACK_START_ERROR = 0x01,
	FSM_ACK_TYPE_MESSAGE_ERROR,
	FSM_ACK_CHECK_SUM_ERROR,
	FSM_ACK_SENT_SUCCESSFULLY
	
}FSM_Ack_e;

typedef struct
{
  uint32_t app_address;
  uint32_t app_length;
  uint32_t patch_address;
  uint32_t patch_length;
  uint32_t firmware_address;
  uint32_t firmware_length; 

}App_Indication_t;


extern App_Indication_t 		fsm_app_indication;


void FSM_GetAppIndication(void);
uint8_t FSM_GetMessage(uint8_t **str_2_byte);
uint8_t FSM_LoadDataIntoFlash(void);
void FSM_Response(uint8_t ack);
void FSM_PrintMessage(void);


#ifdef __cplusplus
}
#endif

#endif
