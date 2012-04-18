#include "clock.h"

#include "LPC17xx.h"
#include "lpc17xx_systick.h"

void clock_init() {
	SYSTICK_InternalInit(10);
	SYSTICK_Cmd(ENABLE);
	SYSTICK_IntCmd(ENABLE);
}
