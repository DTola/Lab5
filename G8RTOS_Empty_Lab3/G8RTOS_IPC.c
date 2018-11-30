/*
 * G8RTOS_IPC.c
 *
 *  Created on: Jan 10, 2017
 *      Author: Daniel Gonzalez
 */
#include <stdint.h>
#include "msp.h"
#include "G8RTOS_IPC.h"
#include "G8RTOS_Semaphores.h"

/*********************************************** Defines ******************************************************************************/

#define FIFOSIZE 16
#define MAX_NUMBER_OF_FIFOS 4

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

/* Create FIFO struct here */

typedef struct FIFO_t{
    int32_t Buffer[FIFOSIZE];
    int32_t *Head;
    int32_t *Tail;
    uint32_t LostData;
    semaphore_t CurrentSize;
    semaphore_t Mutex;
} FIFO_t;

/* Array of FIFOS */
static FIFO_t FIFOs[4];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    /* Implement this */
    if(FIFOIndex > 3){
        return -1;
    }

    FIFO_t *Fptr = &(FIFOs[FIFOIndex]);

    int i = 0;
    for(i = 0; i < FIFOSIZE; i++){
        Fptr->Buffer[i] = 0;
    }
    Fptr->Head = (Fptr->Buffer);
    Fptr->Tail = (Fptr->Buffer);
    Fptr->CurrentSize = 0;
    Fptr->Mutex = 1;

    return 1;
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
int32_t readFIFO(uint32_t FIFOChoice)
{
    /* Implement this */
    //Just in case fifo was in the middle of being read from another thread
    G8RTOS_WaitSemaphore(&(FIFOs[FIFOChoice].Mutex));
    G8RTOS_WaitSemaphore(&(FIFOs[FIFOChoice].CurrentSize));
//    while(FIFOs[FIFOChoice].Mutex < 1);
//    //Just in case fifo is empty
//    while(FIFOs[FIFOChoice].CurrentSize < 1);

    int32_t returnData = *(FIFOs[FIFOChoice].Head);    //Get first out

    //CHeck to see if at the end of the buffer

    if(FIFOs[FIFOChoice].Head == &(FIFOs[FIFOChoice].Buffer[15])){
        FIFOs[FIFOChoice].Head = FIFOs[FIFOChoice].Buffer;  //Put it at beginning
    }
    else{
        FIFOs[FIFOChoice].Head++;    //Push it further down the array
    }

    G8RTOS_SignalSemaphore(&(FIFOs[FIFOChoice].Mutex));
    //G8RTOS_SignalSemaphore(&(FIFOs[FIFOChoice].CurrentSize));
    return returnData;

}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if ncessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, int32_t Data)
{
    /* Implement this */
    //Check if an interrupt has happened between rading the fifo and inc the head pointer
    //G8RTOS_WaitSemaphore(&(FIFOs[FIFOChoice].Mutex));

    if(FIFOs[FIFOChoice].CurrentSize > (FIFOSIZE - 1)){
        FIFOs[FIFOChoice].LostData++;
        return -1;
    }
    else{
        *(FIFOs[FIFOChoice].Tail) = Data;
        if(FIFOs[FIFOChoice].Tail == &(FIFOs[FIFOChoice].Buffer[15])){
                FIFOs[FIFOChoice].Tail = FIFOs[FIFOChoice].Buffer;  //Put it at beginning
            }
            else{
                FIFOs[FIFOChoice].Tail++;    //Push it further down the array
            }
    }

    G8RTOS_SignalSemaphore(&(FIFOs[FIFOChoice].CurrentSize));
    return 1;

}

