#include "iwdg.h"


void IWDG_Config(void)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	
  /* Timeout = (Reload + 1) * Prescaler / LSI_freq */
  /* IWDG counter clock: LSI / 32 */
  IWDG_SetPrescaler(IWDG_Prescaler_64); // Set prescaler to 32
	/* Set reload register (max 0xFFF = 4095) */
  IWDG_SetReload(3125-1); // 5s
  IWDG_ReloadCounter(); // Reload the counter to prevent reset
  IWDG_Enable();
}
