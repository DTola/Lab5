#include "msp.h"
#include "driverlib.h"
#include "G8RTOS.h"
#include <BSP.h>
#include <time.h>
#include "Threads.h"
#include "Game.h"
#include "cc3100_usage.h"
#include "LCD_empty.h"

//hi
/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	G8RTOS_Init();  //Inits everything for use

//	initCC3100();

//	G8RTOS_Launch();    //Starts G8RTOS

	while(1){

	}
}
