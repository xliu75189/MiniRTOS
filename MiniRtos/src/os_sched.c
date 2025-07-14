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

#include <stdint.h>
#include <stdlib.h>
#include "os.h"
#include "qassert.h"
#include "os_utils_list.h"
#include "os_sched.h" 

Q_DEFINE_THIS_FILE


extern OS_TCB * volatile OS_Tcb_Curr; /* pointer to the current thread */
extern OS_TCB * volatile OS_Tcb_Next; /* pointer to the next thread to run */

extern Task_List WaitingTaskList;
static OS_TCB *os_schedGetNextTaskToRun();

/*
*********************************************************************************************************
*             Schedule the next task to execute
*
* Description: This function schedule the next task to execute. This function triggers the context switch 
*              interrupt ISR PENDSV. The PENDSV ISR actually does the context switch and to run other task.
*              This function calls os_schedGetNextTaskToRun() to decide next task to run. If there is only 
*              one task in the ReadyTaskList, and the ready task is current running task itself, then will 
*              not do the context switch and keep running the current task.
*
* Arguments  : None
**
* Returns    : None
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
               
*********************************************************************************************************
*/
void OS_sched(void) {
    /* choose the next task to execute... */
    OS_TCB *nextTcb;
    OS_CPU_SR  cpu_sr = 0u;
    
    OS_ENTER_CRITICAL();
    if (ReadyTaskList.TaskRriorityBitMap == 0U) { /* idle condition? */
        nextTcb = ReadyTaskList.TaskList[0]->pTcb; /* the idle thread */
    }
    else {
        nextTcb = os_schedGetNextTaskToRun();
        Q_ASSERT(nextTcb);
    }
    /* trigger PendSV, if needed */
    if (nextTcb != OS_Tcb_Curr) {
        OS_Tcb_Next = nextTcb;
        TRIGER_PENDSV_INT();
        //*(uint32_t volatile *)0xE000ED04 = (1U << 28);
    }
    OS_EXIT_CRITICAL();
}

/*
*********************************************************************************************************
*             OS tick
*
* Description: This function is called by SysTick_Handler() every system tick. It checks DelayedTaskList
*              to see if there is any suspend task is timeout to re-run. It moves the timeout task from 
*              DelayedTaskList to ReadyTaskAList. Wether the timeout suspend task is actaully to run is 
*              decided by the OS_sched().
*
* Arguments  : None
**
* Returns    : None
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
               
*********************************************************************************************************
*/
void OS_tick(void) {
    uint32_t workingSet;
    Task_List_Node *tempTask;
    Task_List_Node *pTask;    
    uint8_t index;
    uint32_t bit;
    OS_TCB *pTcp;

    workingSet = DelayedTaskList.TaskRriorityBitMap;
    while (workingSet != 0U) {
        index  = LOG2(workingSet);
        tempTask = DelayedTaskList.TaskList[index];
        pTcp = tempTask->pTcb;
        Q_ASSERT((pTcp!= (OS_TCB *)0) && (pTcp->OS_TcbTimeout != 0U));
        bit = (1U << (pTcp->OS_TcbPrio - 1U));
        if(tempTask->next == 0 ){ /* only one task in the priority group */
             pTcp->OS_TcbTimeout--;
            if (pTcp->OS_TcbTimeout == 0U){
                pTask = os_utilsRemoveFromListByTaskNode(tempTask, DELAYED_TASK_LIST);
                Q_ASSERT(pTask);
                os_utilsAddTaskToListByNode(pTask, READY_TASK_LIST);
            }
        }else{ /* this prority group has more than one tasks */
            while(tempTask!= 0){
                pTcp = tempTask->pTcb;
                pTcp->OS_TcbTimeout--;
                if (pTcp->OS_TcbTimeout == 0U) {
                    pTask = os_utilsRemoveFromListByTaskNode(tempTask, DELAYED_TASK_LIST);
                    Q_ASSERT(pTask);
                    os_utilsAddTaskToListByNode(pTask, READY_TASK_LIST);
                }
                tempTask = tempTask->next;
            }    
        }
        workingSet &= ~bit; /* remove from working set */
    }
}
/*
*********************************************************************************************************
*             Get Next Task To Run
*
* Description: This function pick up next highest task to run, but not remove the task from Reay List 
*              and bit map. It is round robin among same priority tasks. 
*              The macro LOG2(ReadyTaskList.TaskRriorityBitMap) guarantees to return the index for the 
*              highest priority task link list. If currnet running task in the link list, it will reutn 
*              next one, or next one doesn't exist, it will return the firet one in the link list. May be
*              the running one itself if there is only one task in the link list. We do not need to loop 
*              over whole ReadyTaskList set becuase LOG2(ReadyTaskList.TaskRriorityBitMap) guarantees 
*              to return one task in the priority list.
*
* Arguments  : None
**
* Returns    : None
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
*********************************************************************************************************
*/
OS_TCB *os_schedGetNextTaskToRun(){
    uint8_t index;
    OS_TCB *nextTcb;
    Task_List_Node *nextTask;
    OS_CPU_SR  cpu_sr = 0u;
    
    OS_ENTER_CRITICAL();
    index = LOG2(ReadyTaskList.TaskRriorityBitMap);
    nextTask = ReadyTaskList.TaskList[index];
    Q_ASSERT(nextTask);
    while (nextTask){
        nextTcb = nextTask->pTcb;
        if( nextTcb == OS_Tcb_Curr ){
            /* if current running task found, we either return next one, if exist. 
               Or if next one does not exist, we return the first one, it may be current one itself 
               if current one is the only one in the ready list */            
            if(nextTask->next != 0){
                nextTask = nextTask->next;
            }
            else {
                nextTask = ReadyTaskList.TaskList[index]; /* Loop back to first one */
            }
            nextTcb = nextTask->pTcb;
            OS_EXIT_CRITICAL();
            return nextTcb;
        }
        /* current running one not found, we look for next */
        nextTask = nextTask->next;
    }
    /* When reach here, no current running tcb found in the highest priority task link list,
       the curret running one may be in lower priority task link list, then we return first 
       one in the highest priority task link list. */
    nextTask = ReadyTaskList.TaskList[index];
    nextTcb = nextTask->pTcb;
    OS_EXIT_CRITICAL();
    return nextTcb;
}
/*
*********************************************************************************************************
*             Sys Tick Handler
*
* Description: This function is called every system tick. It calls OS_tick(). OS_tick() checks if there is
*              any suspend task timeouot. Regardless if there is any suspend task timeout, it will call
*              OS_sched() to schdule next task to run. The next task could be one of timeout suspend task.
*              same prioirty task round robin, etc.
*
* Arguments  : None
**
* Returns    : None
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
               
*********************************************************************************************************
*/
void SysTick_Handler(void) {
     OS_CPU_SR  cpu_sr = 0u;

    //GPIOF_AHB->DATA_Bits[TEST_PIN] = TEST_PIN;
    OS_tick();
    
    OS_ENTER_CRITICAL();
    OS_sched();
    OS_EXIT_CRITICAL();
    //GPIOF_AHB->DATA_Bits[TEST_PIN] = 0U;
}
