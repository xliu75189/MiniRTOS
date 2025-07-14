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
#include "os.h"
#include "os_utils_list.h"
#include "os_sched.h"

extern OS_TCB * volatile OS_Tcb_Curr;  /* pointer to the current thread */
extern OS_EVENT	*OSEventFreeList;      /* Pointer to list of free EVENT control blocks    */


/*
*********************************************************************************************************
*                   CREATE A SEMAPHORE
*
* Description: This function creates a semaphore.
*
* Arguments  : cnt  is the initial value for the semaphore.  If the value is 0, no resource is
*                   available (or no event has occurred).  You initialize the semaphore to a
*                   non-zero value to specify how many resources are available (e.g. if you have
*                   10 resources, you would initialize the semaphore to 10).
*
* Returns    : != (void *)0  is a pointer to the event control block (OS_EVENT) associated with the
*                            created semaphore
*              == (void *)0  if no event control blocks were available
*********************************************************************************************************
*/
OS_EVENT *OS_Sem_Create (uint16_t cnt,char *se_name)
{
    OS_EVENT  *pEvent;
    OS_CPU_SR  cpu_sr = 0u;

    OS_ENTER_CRITICAL();
    pEvent = OSEventFreeList;                      /* Get next free event control block */
    if (OSEventFreeList != (OS_EVENT *)0) {        /* See if pool of free ECB pool was empty   */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OS_EventPtr;
    }
    OS_EXIT_CRITICAL();
    if (pEvent != (OS_EVENT *)0) {                  /* Get an event control block */
        pEvent->OS_EventType    = OS_EVENT_TYPE_SEM;
        pEvent->OS_EventCnt     = cnt;              /* Set semaphore value        */
        pEvent->OS_EventPtr     = (void *)0;        /* Unlink from ECB free list  */
        pEvent->OS_EventName    = se_name;
        //OS_EventWaitListInit(pEvent);             /* Initialize to 'nobody waiting' on sem. */
    }
    return (pEvent);
}
/*
*********************************************************************************************************
*              WAIT/PEND ON SEMAPHORE
*
* Description: This function waits for a semaphore.
*
* Arguments  : pevent        is a pointer to the event control block associated with the desired
*                            semaphore.
*
*              timeout       is an optional timeout period (in clock ticks).  If non-zero, your task will
*                            wait for the resource up to the amount of time specified by this argument.
*                            If you specify 0, however, your task will wait forever at the specified
*                            semaphore or, until the resource becomes available (or the event occurs).
*
*              perr          is a pointer to where an error message will be deposited.  Possible error
*                            messages are:
*
*                            OS_ERR_NONE         The call was successful and your task owns the resource
*                                                or, the event you are waiting for occurred.
*                            OS_ERR_TIMEOUT      The semaphore was not received within the specified
*                                                'timeout'.
*                            OS_ERR_PEND_ABORT   The wait on the semaphore was aborted.
*                            OS_ERR_EVENT_TYPE   If you didn't pass a pointer to a semaphore.
*                            OS_ERR_PEND_ISR     If you called this function from an ISR and the result
*                                                would lead to a suspension.
*                            OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer.
*                            OS_ERR_PEND_LOCKED  If you called this function when the scheduler is locked
*
* Returns    : none
*********************************************************************************************************
*/
void OS_Sem_Wait(OS_EVENT  *pEvent,
                 uint32_t  timeout,
                 uint8_t   *pErr)
{
    OS_CPU_SR  cpu_sr = 0u;
    
    if (pEvent->OS_EventType != OS_EVENT_TYPE_SEM) {   /* Validate event block type */
        *pErr = OS_ERR_EVENT_TYPE;
        return;
    }
    OS_ENTER_CRITICAL();
    if (pEvent->OS_EventCnt > 0u) {                    /* If sem. is positive, resource available   */
        pEvent->OS_EventCnt--;                         /* decrement semaphore only if positive.     */
        OS_EXIT_CRITICAL();
        *pErr = OS_ERR_NONE;
        return;
    }
 
    /* Otherwise, must wait until event occurs */
    OS_Tcb_Curr->OS_TcbState     |= OS_STATE_SEM;     /* Resource not available, pend on semaphore   */
    OS_Tcb_Curr->OS_TcbStatePend  = OS_STAT_PEND_OK;
    OS_Tcb_Curr->OS_TcbTimeout          = timeout;    /* Store pend timeout in TCB                   */
    OS_Tcb_Curr->OS_TcbEcbPtr = pEvent;/* Store ptr to ECB in cutrrent TCB. A task can only wait for one event*/
    OS_EventTaskWait(OS_Tcb_Curr);             /* Suspend task until event or timeout occurs  */
    OS_sched();                                       /* Find next highest priority task ready       */
    OS_EXIT_CRITICAL();
    *pErr = OS_ERR_NONE;
}
/*
*********************************************************************************************************
*               POST TO A SEMAPHORE
*
* Description: This function signals a semaphore
*
* Arguments  : pevent        is a pointer to the event control block associated with the desired
*                            semaphore.
*
* Returns    : OS_ERR_NONE         The call was successful and the semaphore was signaled.
*              OS_ERR_SEM_OVF      If the semaphore count exceeded its limit. In other words, you have
*                                  signaled the semaphore more often than you waited on it with either
*                                  OSSemAccept() or OSSemPend().
*              OS_ERR_EVENT_TYPE   If you didn't pass a pointer to a semaphore
*              OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer.
*********************************************************************************************************
*/
uint8_t OS_Sem_Post (OS_EVENT *pEvent)
{
    OS_CPU_SR  cpu_sr = 0u;
    if (pEvent->OS_EventType != OS_EVENT_TYPE_SEM) {   /* Validate event block type */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    if (WaitingTaskList.TaskRriorityBitMap != 0u) { /* See if any task waiting for semaphore */
        /* Ready HPT waiting on event */
        (void)OS_EventTaskReady(pEvent, (void *)0, OS_STATE_SEM, OS_STAT_PEND_OK);
        OS_sched();       /* Find next highest priority task ready */ /* Find HPT ready to run */
        OS_EXIT_CRITICAL();
        return (OS_ERR_NONE);
    }
    if (pEvent->OS_EventCnt < 65535u) {             /* Make sure semaphore will not overflow       */
        pEvent->OS_EventCnt++;                      /* Increment semaphore count to register event */
        OS_EXIT_CRITICAL();
        return (OS_ERR_NONE);
    }
    OS_EXIT_CRITICAL();                             /* Semaphore value has reached its maximum     */
    return (OS_ERR_SEM_OVF);
}
