#include "sys.h"

uint8_t str_data[FLASH_PAGE_SIZE];
uint8_t flag = 0;
uint16_t str_length;


void PC13_Config();
void PC13_Blink(void);

void Sys_Init(void)
{
	UART1.Init(115200, NO_REMAP);
	PC13_Config();
	JumpButton_Init();
	FSM_GetAppIndication();
	PC13_Blink();
}

void Sys_Run(void)
{
	str_length = UART1.Scan(str_data);
	GPIOC->BSRR = GPIO_Pin_13;
	if(str_length)
  {
		GPIOC->BRR = GPIO_Pin_13;
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
					ack = FSM_LoadDataIntoFlash();
					if(ack == FSM_ACK_UPDATE_FIRMWARE_SUCESSFULLY)
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

void PC13_Config()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitTypeDef 		GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void PC13_Blink(void)
{
	GPIOC->ODR |= GPIO_Pin_13;
	for(int i=0; i<6; i++)
	{
		GPIOC->ODR ^= GPIO_Pin_13;
		for(volatile int j=0; j<0x5ffff; j++);
	}
}
