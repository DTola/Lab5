/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "msp.h"
#include "G8RTOS_Semaphores.h"
#include "G8RTOS_CriticalSection.h"
#include "G8RTOS_Structures.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

void StartContextSwitch(){
    SCB -> ICSR |= SCB_ICSR_PENDSVSET_Msk;      //Pend the PENSV interrupt to the interrupt controller,
                                                //causing it to execute on the next available time
}


/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    /* Implement this */
    int32_t test;
    test = StartCriticalSection();  //Can be called once OS is running
    *s = value;
    EndCriticalSection(test);
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread is sempahore is unavalible
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
    /* Implement this */
    (*s)--;

    int32_t test;
    test = StartCriticalSection();

    /*
     * if s < 0, then the semaphore was already being used,
     * therefore this thread is blocked to the semaphore
     */
    if((*s) < 0){
        CurrentlyRunningThread -> blocked = s;
        //EndCriticalSection(test);
        StartContextSwitch();
        //test = StartCriticalSection();
    }
//    while(*s == 0){
//        //test = StartCriticalSection();  //Disable interrupts
//        EndCriticalSection(test);   //Enable interrupts
//        test = StartCriticalSection(); //Disable interrupts
//    }
    //(*s)--;
    EndCriticalSection(test); //Enable INterrupts
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    /* Implement this */
    int32_t test = StartCriticalSection(); //Enter critical section, disables interrupts
    (*s)++;  //Increment the semaphore

    /*
     * If semaphore is < 0 then a thread(s) is still blocked,
     * we must find the next thread that is blocked to that semaphore
     * and unblock it (set blocked to 0)
     */
    if((*s) < 0){
        tcb_t *pt = (tcb_t *) &CurrentlyRunningThread->nextTCB;
        while(pt -> blocked != s){
            pt = pt -> nextTCB;
        }

        pt -> blocked = 0;
    }
    EndCriticalSection(test);   //Enable interrupts, exit critical section
}

/*********************************************** Public Functions *********************************************************************/


