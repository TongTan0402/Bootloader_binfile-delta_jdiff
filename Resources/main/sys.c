#include "sys.h"

uint8_t data[4100];
uint8_t flag = 0;

void Sys_Init(void)
{
	UART1.Init(115200, NO_REMAP);
	JumpButton_Init();
	Flash_Read(APP_INDICATION_ADDRESS, &FSM_base_address, 1);
	SPI1_Setup(SPI_BaudRatePrescaler_8);  // SPI for SD card with prescaler 8
	
	
	UART1.Print("[MAIN]: SD Card Init...\n");
	if(SD_Init() == 0)
	{
		UART1.Print("[MAIN]: SD Card Init SUCCESS!\n");
		
		f_unlink("new.bin");
		Delta_Run("old.bin", "patch.patch", "new.bin");
		
		UART1.Print("Done!!!\n");
	}
	else
	{
			UART1.Print("[MAIN]: SD Card Init ERROR!\n");
	}
}

void Sys_Run(void)
{
	
//	if(UART1.Scan(data))
//  {
//    uint8_t *ptr = data;
//		FSM_Ack_e	 ack;
//		
//		SET_FLAG(FLAG_UPDATING);
//		
//    while(*ptr)
//    {
//			ack = FSM_GetMessage(&ptr);
//      if(ack) 
//			{
//				if(ack == FSM_ACK_SENT_SUCCESSFULLY) 
//				{
//					uint8_t successful_loading = FSM_LoadDataIntoFlash();
//					if(successful_loading)
//					{
//						SET_FLAG(FLAG_UPDATE_SUCCESSFULLY);
//						Flash_Write(APP_INDICATION_ADDRESS, &FSM_base_address, 1);
//					}
//				}
//				FSM_Response(ack);
//				break;
//			}
//    }
//  }
//	
//	if(!IS_FLAG(FLAG_UPDATING) || IS_FLAG(FLAG_UPDATE_SUCCESSFULLY))
//	{
//		if((GPIOB->IDR & GPIO_Pin_9) && (FSM_base_address != ~0))
//		{
//			JumpToApp(FSM_base_address);
//		}
//	}
}
