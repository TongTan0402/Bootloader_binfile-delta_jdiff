#include "sys.h"
#include "iwdg.h"

int main(void)
{
	Sys_Init();
//	IWDG_Config();
	
	while(1)
	{
//		IWDG_ReloadCounter();
		Sys_Run();
	}
}
