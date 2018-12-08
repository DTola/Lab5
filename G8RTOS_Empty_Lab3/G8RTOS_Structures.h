/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include "G8RTOS.h"
#include <stdbool.h>
#include "G8RTOS_Scheduler.h"

/*********************************************** Data Structure Definitions ***********************************************************/

#define MAX_NAME_LENGTH 16


/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

/* Create tcb struct here */
typedef struct tcb_t{
    //Moved stack pointer to the top so we dont need an increment at the assembly level
    int32_t* Stack_Pointer;
    struct tcb_t* preTCB;
    struct tcb_t* nextTCB;
    //int32_t* Stack_Pointer;

    /*
     * If the blocked flag was set, the blocked thread will yield
     * the CPU control to the next thread during the SysTick Handler
     */
    semaphore_t *blocked;   //blocking semaphore
    //These are not used yet
    //Nvm these are used now
    //Indicates whether thread is alive or is killed and no longer part of the linked list
    bool isAlive;
    uint8_t priority;
    bool Asleep;
    uint32_t Sleep_Count;

    //Each thread has unique ID so user can request ID of thread to kill
    threadId_t threadID;

    //Thread name for super convenience in variable explorer
    char threadName[MAX_NAME_LENGTH];

} tcb_t;

//extern tcb_t threadControlBlocks[MAX_THREADS];


/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

/* Create periodic thread struct here */
typedef struct ptcb_t{
    void (*Handler)(void);
    uint32_t Period;
    uint32_t Execute_Time;
    uint32_t Current_Time;
    struct ptcb_t *Previous_P_Event;
    struct ptcb_t *Next_P_Event;

} ptcb_t;

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
