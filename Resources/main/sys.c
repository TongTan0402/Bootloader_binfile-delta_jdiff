#include "sys.h"

uint8_t str_data[4100];
uint8_t flag = 0;
uint16_t str_length;

void Sys_Init(void)
{
	UART1.Init(115200, NO_REMAP);
	JumpButton_Init();
	FSM_GetAppIndication();
}

void Sys_Run(void)
{
	str_length = UART1.Scan(str_data);
	if(str_length)
  {
    uint8_t *ptr = str_data;
		FSM_Ack_e	 ack;
		
		SET_FLAG(FLAG_UPDATING);
		
    while(str_length--)
    {
			ack = FSM_GetMessage(&ptr);
      if(ack) 
			{
				if(ack == FSM_ACK_SENT_SUCCESSFULLY) 
				{
					uint8_t successful_loading = FSM_LoadDataIntoFlash();
					if(successful_loading)
					{
						SET_FLAG(FLAG_UPDATE_SUCCESSFULLY);
					}
				}
				FSM_Response(ack);
				break;
			}
    }
  }
	
	if(!IS_FLAG(FLAG_UPDATING) || IS_FLAG(FLAG_UPDATE_SUCCESSFULLY))
	{
		if(((GPIOB->IDR & GPIO_Pin_9) && (fsm_app_indication.app_address != ~0)) || IS_FLAG(FLAG_UPDATE_SUCCESSFULLY))
		{
			for(volatile int i=0; i<0xffff; i++) __NOP(); 
			JumpToApp(fsm_app_indication.app_address);
		}
	}
}
