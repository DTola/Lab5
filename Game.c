/*
 * Game.c
 *
 *  Created on: Nov 27, 2018
 *      Author: Daniel & Luis
 */
#include "msp.h"
#include <driverlib.h>
#include "G8RTOS.h"
#include "G8RTOS_Semaphores.h"
#include <BSP.h>
#include "RGBLeds.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "G8RTOS_CriticalSection.h"

/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 *
 * Yeet
 */
void IdleThread(){
    while(1){

    }
}

/*
 * Thread to draw all the objects in the game
 *
 * Description/Requirement:
 * -Should hold arrays of previous players and ball positions
 * -Draw and/or update balls (use alive attribute to tell if draw new or update position)
 * -Update players
 * -Sleep for 20ms (reasonable refresh rate)
 */
void DrawObjects(){

}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs();


/*********************************************** Data Structures ********************************************************************/

