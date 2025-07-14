/****************************************************************************
* Mini Real-time Operating System (MiniRTOS)
* version 1.0 2025
*
* This software is to illustrate the concepts of Real-Time Operating System (RTOS).
* This MiniRTOS program is designed to use Array and Bit Map to implement Task List 
* to speed up task search time. Therefore, the task priority is limited 0-31. It allows
* same priority has more than one tasks. The same priority tasks are arranged with link list. 
* For most applications, few tasks need at same priority. Therefore the same priority task 
* link list should be short, and its search time and variant should be acceptable.
*
* This program is under the terms of the GNU General Public License as published by
* the Free Software Foundation. This program does not have ANY WARRANTY; without even 
* the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
* See GNU General Public License <https://www.gnu.org/licenses/> for more details.
*
* Git repo:
*
****************************************************************************/

#include "os_utils_event.h"
#include "os_utils_list.h"
#include "os.h"
#include "os_sched.h"

extern OS_TCB * volatile OS_Tcb_Curr; /* pointer to the current thread */
extern OS_EVENT *OSEventFreeList;     /* Pointer to list of free EVENT control blocks */

OS_MQ *OS_MQcb_FreeList;              /* Pointer to list of free QUEUE control blocks */
OS_MQ OS_MQcb_Tbl[OS_MAX_MQ];         /* Table of QUEUE control blocks */

/*
*********************************************************************************************************
*               QUEUE MODULE INITIALIZATION
*
* Description : This function is called by OS to initialize the message queue module. 
*               The application MUST NOT call this function.
*
* Arguments   :  none
*
* Returns     : none
*
* Note(s)    : This function is INTERNAL to OS and your application should not call it.
*********************************************************************************************************
*/
void  OS_MsgQ_Init (void)
{
    uint16_t index;
    uint16_t index_next;
    OS_MQ *pMQ1;
    OS_MQ *pMQ2;

    OS_MemClr((uint8_t *)&OS_MQcb_Tbl[0], sizeof(OS_MQcb_Tbl));  /* Clear the queue table                   */
    for (index = 0u; index < (OS_MAX_MQ - 1u); index++) {        /* Init. list of free QUEUE control blocks */
        index_next = index + 1u;
        pMQ1 = &OS_MQcb_Tbl[index];
        pMQ2 = &OS_MQcb_Tbl[index_next];
        pMQ1->OS_MQPtr = pMQ2;
    }
    pMQ1         = &OS_MQcb_Tbl[index];
    pMQ1->OS_MQPtr = (OS_MQ *)0;
    OS_MQcb_FreeList = &OS_MQcb_Tbl[0];
}

/*
*********************************************************************************************************
*              CREATE A MESSAGE QUEUE
*
* Description: This function creates a message queue if free event control blocks are available.
*
* Arguments  : start         is a pointer to the base address of the message queue storage area. The
*                            storage area MUST be declared as an array of pointers to 'void' as follows
*                            void *MessageStorage[size]
*              size          is the number of elements in the storage area
*
* Returns    : != (OS_EVENT *)0  is a pointer to the event control clock (OS_EVENT) associated with the
*                                created queue
*              == (OS_EVENT *)0  if no event control blocks were available or an error was detected
*********************************************************************************************************
*/

OS_EVENT *OS_MsgQ_Create (void **start,
                          uint16_t size)
{
    OS_EVENT  *pEvent;
    OS_MQ      *pMsgQ;
    /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;

    OS_ENTER_CRITICAL();
    pEvent = OSEventFreeList;                /* Get next free event control block */
    if (OSEventFreeList != (OS_EVENT *)0) {  /* See if pool of free ECB pool was empty */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OS_EventPtr;
    }
    OS_EXIT_CRITICAL();
    
    if (pEvent != (OS_EVENT *)0) {           /* See if we have an event control block */
        OS_ENTER_CRITICAL();
        pMsgQ = OS_MQcb_FreeList;              /* Get a free queue control block */
        if (pMsgQ != (OS_MQ *)0) {              /* Were we able to get a queue control block ? */
            OS_MQcb_FreeList = OS_MQcb_FreeList->OS_MQPtr; /* Yes, Adjust free list pointer to next free*/
            OS_EXIT_CRITICAL();
            pMsgQ->OS_MQStart   =  start;      /*      Initialize the queue  */
            pMsgQ->OS_MQEnd     =  &start[size];
            pMsgQ->OS_MQIn      =  start;
            pMsgQ->OS_MQOut     =  start;
            pMsgQ->OS_MQSize    =  size;
            pMsgQ->OS_MQEntries = 0u;
            
            pEvent->OS_EventType    = OS_EVENT_TYPE_MQ;
            pEvent->OS_EventCnt     = 0u;
            pEvent->OS_EventPtr     = pMsgQ;
            pEvent->OS_EventName    = "MsgQ";
            //OS_EventWaitListInit(pEvent); /* Initialize the wait list */
        }
        else {
            /* revert pMsgQ */
        }               
    } else { /* No. we do not have free event control block */
        pEvent->OS_EventPtr = (void *)OSEventFreeList; /* Return event control block on error  */
        OSEventFreeList    = pEvent; /* Previously,get next free event control block  */
        OS_EXIT_CRITICAL();
        pEvent = (OS_EVENT *)0;
    }
    return (pEvent);
}

/*
*********************************************************************************************************
*              SEND/POST MESSAGE TO A QUEUE
*
* Description: This function sends a message to a queue
*
* Arguments  : pEvent    is a pointer to the event control block associated with the desired queue
*
*              pmsg      is a pointer to the message to send.
*
* Returns    : OS_ERR_NONE           The call was successful and the message was sent
*              OS_ERR_Q_FULL         If the queue cannot accept any more messages because it is full.
*              OS_ERR_EVENT_TYPE     If you didn't pass a pointer to a queue.
*              OS_ERR_PEVENT_NULL    If 'pevent' is a NULL pointer
* Note(s)    : 
*********************************************************************************************************
*/
uint8_t OS_MsgQ_Send(OS_EVENT *pEvent, void *pMsg)
{
    OS_MQ   *pMq;
    uint8_t status;
    /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;

    if (pEvent->OS_EventType != OS_EVENT_TYPE_MQ) {  /* Validate event block type */
        return (OS_ERR_EVENT_TYPE);
    }

    OS_ENTER_CRITICAL();
    pMq = (OS_MQ *)pEvent->OS_EventPtr;             /* Point to queue control block */
    if (pMq->OS_MQEntries >= pMq->OS_MQSize) {      /* Make sure queue is not full  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_Q_FULL);
    }
    *pMq->OS_MQIn++ = pMsg;                         /* Insert message into queue              */
    pMq->OS_MQEntries++;                            /* Update the nbr of entries in the queue */
    if (pMq->OS_MQIn == pMq->OS_MQEnd) {            /* Wrap IN ptr if we are at end of queue  */
        pMq->OS_MQIn = pMq->OS_MQStart;
    }

    if (WaitingTaskList.TaskRriorityBitMap != 0u)  /* See if any task pending */
    {
        /* There is task pending. If the pending ask is waiting for this message */
        /* Ready highest priority task waiting on the event */
        status = OS_EventTaskReady(pEvent, pMsg, OS_STAT_MQ, OS_STAT_PEND_OK);
        if( status == OS_TASK_PENDING ){
            /* To schdule highest priority task ready to run */
            /* If the sender's priority higher than receiver's, */
            /* after scehdule, sender still runs */
            OS_sched();
        }            
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}

/*
*********************************************************************************************************
*              WAIT/PEND ON A QUEUE FOR A MESSAGE
*
* Description: This function waits for a message to be sent to a queue
*
* Arguments  : pevent   is a pointer to the event control block associated with the desired queue
*
*              timeout  is an optional timeout period (in clock ticks).  If non-zero, your task will
*                       wait for a message to arrive at the queue up to the amount of time
*                       specified by this argument.  If you specify 0, however, your task will wait
*                       forever at the specified queue or, until a message arrives.
*
*              perr     is a pointer to where an error message will be deposited.  Possible error
*                       messages are:
*
*                       OS_ERR_NONE         The call was successful and your task received a
*                                           message.
*                       OS_ERR_TIMEOUT      A message was not received within the specified 'timeout'.
*                       OS_ERR_PEND_ABORT   The wait on the queue was aborted.
*                       OS_ERR_EVENT_TYPE   You didn't pass a pointer to a queue
*                       OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer
*                       OS_ERR_PEND_ISR     If you called this function from an ISR and the result
*                                           would lead to a suspension.
*                       OS_ERR_PEND_LOCKED  If you called this function with the scheduler is locked
*
* Returns    : != (void *)0  is a pointer to the message received
*              == (void *)0  if you received a NULL pointer message or,
*                            if no message was received or,
*                            if 'pevent' is a NULL pointer or,
*                            if you didn't pass a pointer to a queue.
*
* Note(s)    : 
*********************************************************************************************************
*/

void *OS_MsgQ_Wait(OS_EVENT *pEvent,
                   uint32_t timeout,
                   uint8_t *pErr)
{
    void       *pMessage;
    OS_MQ      *pMsgQ;
    OS_CPU_SR  cpu_sr = 0u; /* Allocate storage for CPU status register  */

    if (pEvent->OS_EventType != OS_EVENT_TYPE_MQ) { /* Validate event block type */
        *pErr = OS_ERR_EVENT_TYPE;
        return ((void *)0);
    }
    pMsgQ = (OS_MQ *)pEvent->OS_EventPtr;         /* Point at queue control block */
    while(1){
        OS_ENTER_CRITICAL();
        if (pMsgQ->OS_MQEntries > 0u) {           /* See if any messages in the queue */
            pMessage = *pMsgQ->OS_MQOut++;        /* Yes, extract oldest message from the queue */
            pMsgQ->OS_MQEntries--;                /* Update the number of entries in the queue */
            if (pMsgQ->OS_MQOut == pMsgQ->OS_MQEnd) { /* Wrap OUT pointer if we are at the end of the queue */
                pMsgQ->OS_MQOut = pMsgQ->OS_MQStart;
            }
            OS_EXIT_CRITICAL();
            *pErr = OS_ERR_NONE;
            return (pMessage); /* Return message received */
        }else { /* there is no message in the queue */
            OS_Tcb_Curr->OS_TcbTimeout = timeout;   /* Store pend timeout in TCB */
            OS_Tcb_Curr->OS_TcbEcbPtr = pEvent;/* Store ptr to ECB in cutrrent TCB. A task can only wait for one event*/
            OS_EventTaskWait(OS_Tcb_Curr);  /* Suspend task until event or timeout occurs */    
            OS_sched(); /* Schedule next highest priority task ready to run */
            OS_EXIT_CRITICAL();
            /* timeout or abort should be processed here also */
        }
    } /* end of while(1) */
    return ((void*)0);  /* shoud never come to here */
}
