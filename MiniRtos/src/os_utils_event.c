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
#include <stdint.h>
#include "os_utils_list.h"
#include "qassert.h"

Q_DEFINE_THIS_FILE

OS_EVENT  *OSEventFreeList;          /* Pointer to list of free EVENT control blocks    */
OS_EVENT  OSEventTbl[OS_MAX_EVENTS]; /* Table of EVENT control blocks                   */

void OS_MemClr(uint8_t  *pDest, uint16_t  size);
void OS_MemCopy(uint8_t *pDest, uint8_t *pSrc, uint16_t size);

/*
*********************************************************************************************************
*                  INITIALIZATION
*                  INITIALIZE THE FREE LIST OF EVENT CONTROL BLOCKS
*
* Description: This function is called by OSInit() to initialize the free list of event control blocks.
*
* Arguments  : none
*
* Returns    : none
*********************************************************************************************************
*/

void OS_InitEventList(void)
{
    uint16_t index;
    uint16_t index_next;
    OS_EVENT *pEvent1;
    OS_EVENT *pEvent2;


    OS_MemClr((uint8_t*)&OSEventTbl[0], sizeof(OSEventTbl));   /* Clear the event table */
    for (index = 0u; index < (OS_MAX_EVENTS - 1u); index++) {  /* Init. list of free EVENT control blocks */
        index_next = index + 1u;
        pEvent1 = &OSEventTbl[index];
        pEvent2 = &OSEventTbl[index_next];
        pEvent1->OS_EventType = OS_EVENT_TYPE_UNUSED;
        pEvent1->OS_EventPtr  = pEvent2;
        pEvent1->OS_EventName = "?";     /* Unknown name */
    }
    pEvent1                 = &OSEventTbl[index];
    pEvent1->OS_EventType   = OS_EVENT_TYPE_UNUSED;
    pEvent1->OS_EventPtr    = (OS_EVENT *)0;
    pEvent1->OS_EventName   = "?"; /* Unknown name                            */
    OSEventFreeList         = &OSEventTbl[0];
}

void OS_EventWaitListInit(OS_EVENT *pEvent)
{
    int i;
    /* No task waiting on event */
    for(i=0;i<MAX_TASKS_IN_LIST;i++){
        WaitingTaskList.TaskList[i] = 0;
    }
}

/*
*********************************************************************************************************
*              MAKE TASK WAIT FOR EVENT TO OCCUR
*
* Description: This function is called by other services to suspend a task to wait for an event.
*
* Arguments  : tcb_curr   is a pointer to current task control block for which the task will be waiting for.
*
* Returns    : None
*
* Note       : This function is INTERNAL to OS and your application should not call it.
*********************************************************************************************************
*/
void OS_EventTaskWait(OS_TCB *tcb_curr)
{   
    Task_List_Node *taskListNode;
    
    taskListNode = os_utilsRemoveFromListByTaskTcb(tcb_curr,READY_TASK_LIST);
    Q_ASSERT(taskListNode);
    os_utilsAddTaskToListByNode(taskListNode, WAITING_TASK_LIST);
}

/*
*********************************************************************************************************
*              MAKE TASK READY TO RUN BASED ON EVENT OCCURING
*
* Description: This function is called by other services and is used to move a task that was
*              waiting for the event from waiting list to ready list. This function finds the highiest 
               priority task in the waiting list matched the waiting event and return it.
*
* Arguments  : pevent      is a pointer to the event control block corresponding to the event.
*
*              pmsg        is a pointer to a message.  This pointer is used by message oriented services
*                          such as MAILBOXEs and QUEUEs.  The pointer is not used when called by other
*                          service functions.
*
*              msk         is a mask that is used to clear the status byte of the TCB.  For example,
*                          OSSemPost() will pass OS_STAT_SEM, OSMboxPost() will pass OS_STAT_MBOX etc.
*
*              pend_stat   is used to indicate the readied task's pending status:
*
*                          OS_STAT_PEND_OK      Task ready due to a post (or delete), not a timeout or
*                                               an abort.
*                          OS_STAT_PEND_ABORT   Task ready due to an abort.
*
* Returns    : none
*
* Note       : This function is INTERNAL to OS and your application should not call it.
*********************************************************************************************************
*/
uint8_t OS_EventTaskReady(OS_EVENT  *pEvent,
                          void      *pMsg,
                          uint8_t   msk,
                          uint8_t   pend_state)
{
    Task_List_Node *pTaskListNode;

    msk = msk;
    pMsg = pMsg;
    
    pTaskListNode = os_utilsRemoveFromWaitingListHPT(pEvent);
    if(pTaskListNode){
        os_utilsAddTaskToListByNode(pTaskListNode, READY_TASK_LIST);
        return OS_TASK_PENDING;
    }
    else 
        return OS_NO_TASK_PENDING;
}
/*
*********************************************************************************************************
*                                       CLEAR A BLOCK OF MEMORY
*
* Description: This function is called by other OSservices to clear a block of memory.
* Arguments  : pdest    is a pointer to the memory block to be cleared
*
*              size     is the number of bytes to be cleared.
*
* Returns    : none
*
* Notes      : 1) This function is INTERNAL to OS and your application should not call it. 
*              2) The clear is done one byte at a time since this will work on any processor irrespective
*                 of the alignment of the source and destination.
*********************************************************************************************************
*/

void OS_MemClr(uint8_t  *pdest,
               uint16_t  size)
{
    while (size > 0u) {
        *pdest++ = (uint8_t)0;
        size--;
    }
}
/*
*********************************************************************************************************
*                                       COPY A BLOCK OF MEMORY
*
* Description: This function is called by other OSservices to copy a block of memory from one
*              location to another.
*
* Arguments  : pdest    is a pointer to the 'destination' memory block
*
*              psrc     is a pointer to the 'source'      memory block
*
*              size     is the number of bytes to copy.
*
* Returns    : none
*
* Notes      : 1) This function is INTERNAL to OS and your application should not call it.  There is
*                 no provision to handle overlapping memory copy.  However, that's not a problem since this
*                 is not a situation that will happen.
*              2) Note that we can only copy up to 64K bytes of RAM
*              3) The copy is done one byte at a time since this will work on any processor irrespective
*                 of the alignment of the source and destination.
*********************************************************************************************************
*/
void OS_MemCopy(uint8_t  *pdest,
                  uint8_t  *psrc,
                  uint16_t  size)
{
    while (size > 0u) {
        *pdest++ = *psrc++;
        size--;
    }
}
