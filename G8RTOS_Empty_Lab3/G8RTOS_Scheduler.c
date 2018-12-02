/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "msp.h"
#include "G8RTOS_Scheduler.h"
#include "G8RTOS_Structures.h"
#include <driverlib.h>
#include "ClockSys.h"
#include "BSP.h"
#include "interrupt.h"
#include <stdbool.h>
#include "G8RTOS_CriticalSection.h"
/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as invdividual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAXPTHREADS];

/*********************************************** Data Structures Used *****************************************************************/



/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;


/*********************************************** Private Variables ********************************************************************/

//Used to compare with priority value
static uint16_t currentMaxPriority = 256;

//used to count number of threads initialized
static uint16_t IDCounter = 0;

/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
    /* Implement this */
    SysTick_enableModule();     //Enables STCSR Enable (Sets 1)
    SysTick_setPeriod(numCycles);

}

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 * 	- Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 * 	- Check for sleeping and blocked threads
 */
void G8RTOS_Scheduler()
{
	/* Implement This */
    //Set to next thread in linked list fot round robin scheduling
    tcb_t *tempNextThread = CurrentlyRunningThread->nextTCB;

    //CurrentlyRunningThread = CurrentlyRunningThread->nextTCB;

    //Used to compare blocked flag
    //semaphore_t test = 1;

    //Checks self here
    if((CurrentlyRunningThread->Asleep) || (CurrentlyRunningThread->blocked < 0) || !(CurrentlyRunningThread->isAlive)){
        currentMaxPriority = 256;
    }

    //Iterates through all the threads
    //If next thread is blocked or sleeping then go to next thread
    //Then if next thread has higher priority (lower priority number) then it becomes the current thread
    //Idle thread is used wth lowest priority when all threads are unable to run (blocked or sleeeping)
    int i = 0;
    //DOes not use -1 because needs to check self
    //Use -1 if no need to check self
    for(i = 0; i < NumberOfThreads-1; i++){
        if(!((tempNextThread->Asleep) || ((tempNextThread->blocked) < 0) || !(tempNextThread->isAlive)) ){
            if(tempNextThread->priority < currentMaxPriority){
                CurrentlyRunningThread = tempNextThread;
                currentMaxPriority = tempNextThread->priority;
            }
        }
        tempNextThread = tempNextThread->nextTCB;
    }

//    //If thread if asleep or blocked then we assign the next tcb as the current tcb
//    while((CurrentlyRunningThread->Asleep) || (*(CurrentlyRunningThread->blocked) < 0)){
//        CurrentlyRunningThread = CurrentlyRunningThread->nextTCB;
//    }
}


/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{
    /* Implement this */
        //SystemTime++;   //Increments the System time
        //Interrupt_pendInterrupt(FAULT_PENDSV);

        //wake up sleeping thread of time to wake up

    int i = 0;
    //Future goal is to offset periodic threads
    ptcb_t *Pptr = &(Pthread[0]);
    for(i = 0; i < NumberOfPthreads; i++){
        if(Pptr->Execute_Time == SystemTime){
            Pptr->Execute_Time = Pptr->Period + SystemTime;
            Pptr->Handler();
        }
        Pptr = Pptr->Next_P_Event;
    }

    //increment system time after periodic thread to avoid initial time 0 threads not running
    SystemTime++;

        tcb_t *ptr = CurrentlyRunningThread;
        for(i = 0; i < NumberOfThreads; i++){
            if(ptr->Asleep){
                if(ptr->Sleep_Count <= SystemTime){
                    ptr->Asleep = false;
                    //Yoloswag$
                    ptr->Sleep_Count = 0;
                }
            }

            ptr = ptr->nextTCB;
        }

//        SystemTime++;

        SCB -> ICSR |= SCB_ICSR_PENDSVSET_Msk;      //Pend the PENSV interrupt to the interrupt controller,
                                                    //causing it to execute on the next available time
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    /* Implement this */
        //uint32_t = 0.01 * ClockSys_GetSysFreq()

        //Relocate ISR interrupt vector table to SRAM so we can relocate an ISRs interrupt vector
        //We will relocate the table to 0x200000000
        uint32_t newVTORTable = 0x20000000;
        memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4);  // 57 interrupt vectors to copy
        SCB->VTOR=newVTORTable;

        SystemTime = 0;
        NumberOfThreads = 0;
        BSP_InitBoard();    //Inits the whole board
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
    /* Implement this */

    /*
     * Loop to make the first currentlyRunningThread the thread with the highest priority
     */
    CurrentlyRunningThread = &threadControlBlocks[0];   //Set the currently running thread to the first thread
    tcb_t *tempThread = CurrentlyRunningThread->nextTCB;
    int i = 0;
    for(i = 0; i < NumberOfThreads-1; i++){
        if(tempThread->priority < CurrentlyRunningThread->priority){
            CurrentlyRunningThread = tempThread;
        }
        tempThread = tempThread->nextTCB;
    }
    uint32_t numcycles = 48000;  //0.001 * 48*10^6
    InitSysTick(numcycles);  //Init the systick
    Interrupt_setPriority(FAULT_PENDSV, 7); //Lowest priority
    Interrupt_setPriority(FAULT_SYSTICK, 7);    //Lowest priority
    //Interrupt_setPriority(FAULT_PENDSV, 0xE0); //Lowest priority
    //Interrupt_setPriority(FAULT_SYSTICK, 0xC0);    //2nd Lowest priority
    //SysTick_enableInterrupt();    //Dont need, enabled in assembly
    SysTick_enableInterrupt();
    G8RTOS_Start();
    return -1; //Failure code :(
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char * name)
{
    /* Implement this */
    uint32_t PRIMASK = StartCriticalSection();
    if(NumberOfThreads == MAX_THREADS){
        return THREAD_LIMIT_REACHED;  //Error Code, reached max number of threads, can't add new one
    }
    else{
        NumberOfThreads++;  //Adding a thread...
    }

    tcb_t* newThread = &(threadControlBlocks[NumberOfThreads-1]);
    newThread->Stack_Pointer = &threadStacks[NumberOfThreads-1][STACKSIZE-16];  //Doesn't matter how many threads

    //Only thread so points to itself
    //This might not be necessary?                 GET BACK TO THIS :)
    if(NumberOfThreads == 1){
        newThread->nextTCB = newThread;
        newThread->preTCB = newThread;
    }
    else{   //Not the only thread, loop through the threads to update pointers
//        int i;
//        for(i = 0; i < NumberOfThreads; i++){
//            if(i == 0){ //Wrap around to end
//                threadControlBlocks[i]->preTCB = threadControlBlocks[NumberOfThreads-1];
//            }
//            else{
//                threadControlBlocks[i]->preTCB = threadControlBlocks[i-1];
//            }
//
//            if(i == NumberOfThreads){ //Wrap around to beginning
//                threadControlBlocks[i]->nextTCB = threadControlBlocks[0];
//            }
//            else{
//                threadControlBlocks[i]->nextTCB = threadControlBlocks[i+1];
//            }
//        }
        //Actually only need to update the first thread to point to the new thread and the previous newest
        //thread to point to the new thread
        tcb_t *tempNextThread = &(threadControlBlocks[0]);
        bool lowestPriority = true;
        int j = 0;
        for(j = 0; j < NumberOfThreads; j++){
            if(priority >= tempNextThread->priority){    //If higher priority, insert it
                newThread->nextTCB = tempNextThread;
                newThread->preTCB = tempNextThread->preTCB;
                lowestPriority = false;
                break;
            }
        }

        //Insert at end
        if(lowestPriority){
            (threadControlBlocks[0]).preTCB = &(threadControlBlocks[NumberOfThreads-1]);    //Point to newest
            (threadControlBlocks[NumberOfThreads-2]).nextTCB = &(threadControlBlocks[NumberOfThreads-1]); //point to newest
            newThread->preTCB = &(threadControlBlocks[NumberOfThreads-2]);
            newThread->nextTCB = &(threadControlBlocks[0]);
        }
    }

    //Give Fake News/Context
    /*
     * 1008 = SP (R13) (Moves up/down automatically as stack changes)
     * 1009-1016 = R4:R11 (1024 - 15 to 1024 - 8)
     * 1017-1020 = R0:3, R12 (1024 - 7 to 1024-4)
     * 1021 = LR (R14) (1024-3)
     * 1022 = PC (R15) (1024 - 2)
     * 1023 = PSR
     */
    int i = 0;
    for(i = 3; i < 16; i++){    //Give R0-R12 default values of 0b101 (for testing purposes)
        threadStacks[NumberOfThreads-1][STACKSIZE-i] = 5; //Look at table above
    }

    threadStacks[NumberOfThreads-1][STACKSIZE-2] = (int32_t)threadToAdd; //PC to threads function pointer. int32_t fixes warning about void void
    threadStacks[NumberOfThreads-1][STACKSIZE-1] = THUMBBIT;   //PSR to some value with thumb-bit set

    //IDCounter++;    //incrementys everytime a thread is initialized
    newThread->isAlive = true;
    //newThread->threadName = name;

    i = 0;
    while(*name != '\0'){
        *((newThread->threadName) + i) = *name;
        name++;
        i++;
    }
    *((newThread->threadName) + i) = '\0';

    newThread->priority = priority;
    uint32_t tcbToInitialize = 0;
    bool correct = true;
    for(i = 0; i < NumberOfThreads; i++){
        if(!(threadControlBlocks[i]).isAlive){
            correct = true;
            tcbToInitialize = i;
            i = NumberOfThreads;
            break;
            //ends the block
        }
//        else{
//            correct = false;
////            tcbToInitialize = i;
////            i = NumberOfThreads;
//            //ends the block
//        }
    }



    newThread->threadID = ((IDCounter++)<<16) | tcbToInitialize;

    EndCriticalSection(PRIMASK);

    if(correct){
        return NO_ERROR;
    }
    else{
        return THREADS_INCORRECTLY_ALIVE;
    }

}


/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
sched_ErrCode_t G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    int32_t PRIMASK;
    PRIMASK = StartCriticalSection();   //Make critical section is it can run at the start of the OS
    /* Implement this */
    /* Implement this */
    if(NumberOfPthreads == MAXPTHREADS){
        return THREAD_LIMIT_REACHED;  //Error Code, reached max number of threads, can't add new one
    }
    else{
        NumberOfPthreads++;  //Adding a thread...
    }

    ptcb_t *newThread = &(Pthread[NumberOfPthreads-1]);

    if(NumberOfPthreads == 1){
            newThread->Next_P_Event = newThread;
            newThread->Previous_P_Event = newThread;
        }
        else{
            (Pthread[0]).Previous_P_Event = &(Pthread[NumberOfPthreads-1]);    //Point to newest
            (Pthread[NumberOfPthreads-2]).Next_P_Event = &(Pthread[NumberOfPthreads-1]); //point to newest
            newThread->Previous_P_Event = &(Pthread[NumberOfPthreads-2]);
            newThread->Next_P_Event = &(Pthread[0]);
        }

    newThread->Period = period;
    newThread->Handler = PthreadToAdd;

    EndCriticalSection(PRIMASK);

    return NO_ERROR;
}


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    /* Implement this */
    CurrentlyRunningThread->Sleep_Count = durationMS + SystemTime;
    CurrentlyRunningThread->Asleep = true;
    SCB -> ICSR |= SCB_ICSR_PENDSVSET_Msk;      //Pend the PENSV interrupt to the interrupt controller,
                                                //causing it to execute on the next available time
}

threadId_t G8RTOS_GetThreadId(){
    return CurrentlyRunningThread->threadID;
}

sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId){
    int32_t PRIMASK;
    PRIMASK = StartCriticalSection();

    //Return error if only one thread running
    if(NumberOfThreads == 1){
        return CANNOT_KILL_LAST_THREAD;
    }

    tcb_t *searcher = CurrentlyRunningThread;

    //Search for thread with same threadId
    bool correct = false;   //If found right thread
    int i = 0;
    for(i = 0; i < NumberOfThreads; i++){
        if(searcher->threadID == threadId){
            correct = true;
            i = NumberOfThreads;    //Close da loop
            break;
        }
        searcher = searcher->nextTCB;
    }

    //Return error if thread was never found :(
    if(!correct){
        return THREAD_DOES_NOT_EXIST;
    }

    //rip
    searcher->isAlive = false;

    //Close the gap (update thread pointers)
    tcb_t *previousThread = searcher->preTCB;
    previousThread->nextTCB = searcher->nextTCB;
    tcb_t *nextThread = searcher->nextTCB;
    nextThread->preTCB = searcher->preTCB;

    //Decrement number of threads
    NumberOfThreads--;

    EndCriticalSection(PRIMASK);

    //If we killed the currentlyRunningThread then we need to do context switching
    if(searcher == CurrentlyRunningThread){
        SCB -> ICSR |= SCB_ICSR_PENDSVSET_Msk;      //Pend the PENSV interrupt to the interrupt controller,
                                                    //causing it to execute on the next available time
    }

    return NO_ERROR;
}

sched_ErrCode_t G8RTOS_KillSelf(){
    int32_t PRIMASK;
    PRIMASK = StartCriticalSection();

    //If only one thread running then it can't kill itself for personal reasons
    if(NumberOfThreads == 1){
        return CANNOT_KILL_LAST_THREAD;
    }

    //Cri errytim
    CurrentlyRunningThread->isAlive = false;

    //Close the gap (update thread pointers)
    tcb_t *previousThread = CurrentlyRunningThread->preTCB;
    previousThread->nextTCB = CurrentlyRunningThread->nextTCB;
    tcb_t *nextThread = CurrentlyRunningThread->nextTCB;
    nextThread->preTCB = CurrentlyRunningThread->preTCB;

//    //Context switch
//    SCB -> ICSR |= SCB_ICSR_PENDSVSET_Msk;      //Pend the PENSV interrupt to the interrupt controller,
//                                                //causing it to execute on the next available time

    //Decrement num of threads
    NumberOfThreads--;

    EndCriticalSection(PRIMASK);

//    //Context switch
    SCB -> ICSR |= SCB_ICSR_PENDSVSET_Msk;      //Pend the PENSV interrupt to the interrupt controller,
//                                                //causing it to execute on the next available time

    return NO_ERROR;
}

sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn){
    int32_t PRIMASK;
    PRIMASK = StartCriticalSection();   //Make critical section so they can be called once the OS is running


    //Errors if IRQn  is less than the last exception and greater than last acceptable user IRQn
    if(!(IRQn > PSS_IRQn)){
        return IRQn_INVALID;
    }
    else if(!(IRQn < PORT6_IRQn)){
        return IRQn_INVALID;
    }

    //Error if priority is greater than the greatest user priority number
    if(priority > 6){
        return HWI_PRIORITY_INVALID;
    }

    //Sets an interrupt vector in SRAM based interrupt vector table.
    //The interrupt number can be positive to specify a device specific interrupt,
    //or negative to specify a processor exception.
    //VTOR must been relocated to SRAM before.
    __NVIC_SetVector(IRQn, (uint32_t)AthreadToAdd);

    //Sets the priority of a device specific interrupt or a processor exception.
    //The interrupt number can be positive to specify a device specific interrupt,
    //or negative to specify a processor exception.
    __NVIC_SetPriority(IRQn, priority);
    __NVIC_EnableIRQ(IRQn); //Enables a device specific interrupt in the NVIC interrupt controller.


    EndCriticalSection(PRIMASK);
    return NO_ERROR;
}

sched_ErrCode_t G8RTOS_KillAllOtherThreads(){
    int32_t PRIMASK;
    PRIMASK = StartCriticalSection();

    //Like all other kills, we must make sure it is not the only thread
    //If only one thread running then it can't kill itself for personal reasons
    if(NumberOfThreads == 1){
        return CANNOT_KILL_LAST_THREAD;
    }

    //Have it pointing to the next thread so we dont kill the current one
    tcb_t *searcher = CurrentlyRunningThread->nextTCB;

    //We don't need to do any fancy next or prev pointer fixing because we are left with one
    int i = 0;  //Compiler pls
    for(i = 0;  i < NumberOfThreads; i++){
        searcher->isAlive = false;
        searcher = searcher->nextTCB;

        //Worked better than NumberOfThreads - 1
        if(searcher == CurrentlyRunningThread){
            break;
        }
    }

    //Decrement thread count
    NumberOfThreads = 1;

    //Fix currentlyrunningthreads pointers
    CurrentlyRunningThread->preTCB = CurrentlyRunningThread;
    CurrentlyRunningThread->nextTCB = CurrentlyRunningThread;

    EndCriticalSection(PRIMASK);
    return NO_ERROR;
}

/*********************************************** Public Functions *********************************************************************/
