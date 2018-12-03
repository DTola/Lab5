#include "msp.h"
#include "driverlib.h"
#include "G8RTOS.h"
#include <BSP.h>
#include <time.h>
//#include "Threads.h"
#include "Game.h"
#include "cc3100_usage.h"
#include "LCD_empty.h"


/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	G8RTOS_Init();  //Inits everything for use

//	initCC3100();

	//Already done in the init
//    G8RTOS_InitSemaphore(&I2C_Semaphore, 1);
//    G8RTOS_InitSemaphore(&LCD_Semaphore, 1);
//    G8RTOS_InitSemaphore(&GameState_Semaphore, 1);
//    G8RTOS_InitSemaphore(&CC3100_WIFI_Semaphore, 1);
//    G8RTOS_InitSemaphore(&Client_Player_Semaphore, 1);

    LCD_Clear(LCD_BLACK);
	LCD_Text(0, 120, "Press Button 1 for HOST or Button 2 for CLIENT", LCD_WHITE);

	playerType Type_Choice = GetPlayerRole();

	LCD_Clear(LCD_BLACK);

	if(Type_Choice == Host){
	    //Host creates the game
	    G8RTOS_AddThread(&CreateGame, 225, "CreateGame");   //Higher than other threads just in case
	}
	else{
	    //Clients gotta join the game
	    G8RTOS_AddThread(&JoinGame, 225, "JoinGame");
	}

	G8RTOS_Launch();

//	G8RTOS_Launch();    //Starts G8RTOS
	//Should never reach the while loop
//	while(1){
//
//	}
}
