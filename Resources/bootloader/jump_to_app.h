#ifndef __JUMP_TO_APP__
#define __JUMP_TO_APP__
#ifdef __cplusplus
extern "C" {
#endif
#include "stm32f10x.h"                  // Device header
#include <stdint.h>

void JumpToApp(uint32_t app_address);
void JumpButton_Init(void);

#ifdef __cplusplus
}
#endif
#endif