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
#include "os.h"
#include "qassert.h"
#include "os_utils_list.h"
#include "os_sched.h"
#include "os_utils_event.h"
#include "os_msg_q.h"
Q_DEFINE_THIS_FILE

OS_TCB * volatile OS_Tcb_Curr; /* pointer to the current task */
OS_TCB * volatile OS_Tcb_Next; /* pointer to the next task to run */

Task_List ReadyTaskList;
Task_List DelayedTaskList;
Task_List WaitingTaskList;

 
OS_TCB idleTask;
/*
*********************************************************************************************************
*              main Idle Task
*
* Description: This function is the idle task. When no other task is running, this task is running
*
* Arguments  : None
*
* Returns    : None
*
* Note       : The application callback function OS_OnIdle() is good place to add power saving funtionality.
*********************************************************************************************************
*/
void main_idleTask() { 
    while (1) {
        OS_OnIdle();
    }
}

/*
*********************************************************************************************************
*              OS Init
*
* Description: This function initailizes all OS related functionality
*
* Arguments  : None
*
* Returns    : None
*
* Note       : 
*********************************************************************************************************
*/
void OS_Init(void *stkSto, uint32_t stkSize) {
    /* set the PendSV interrupt priority to the lowest level 0xFF */
    SET_PENDSV_INT_PRIO_TO_LOWEST_LEVEL();

    OS_InitEventList();
    OS_MsgQ_Init();
    os_utilsTaskListInit();
    /* start idleTask */
    OS_Task_Create(&idleTask,
                   0U, /* idle task priority */
                   &main_idleTask,
                   stkSto, stkSize);
}

/*
*********************************************************************************************************
*              OS Run
*
* Description: This function starts running OS function, and should never returns to main()
*              It enables OS related interrupt like PENDSV, and starts OS schedule.
*
* Arguments  : None
*
* Returns    : None
*
* Note       : 
*********************************************************************************************************
*/
void OS_Run(void) {
    OS_CPU_SR  cpu_sr = 0u;
    /* callback to configure and start interrupts */
    OS_OnStartup();

    OS_ENTER_CRITICAL();
    OS_sched();
    OS_EXIT_CRITICAL();
    /* the following code should never execute */
    Q_ERROR();
}
/*
*********************************************************************************************************
*              OS Delay
*
* Description: This function delays/suspends current task for a period of time. It moves currrent task 
*              from Ready Task List to Delayed Task List, and then reschedule the next task to run.
*
* Arguments  : ticks  Ticks to delayed
*
* Returns    : None
*
* Note       : 
*********************************************************************************************************
*/
void OS_Delay(uint32_t ticks) {
    Task_List_Node *tempTask;
    /* never call OS_delay from the idleTask */
    Q_REQUIRE(OS_Tcb_Curr != ReadyTaskList.TaskList[0]->pTcb);

    OS_Tcb_Curr->OS_TcbTimeout = ticks;
    tempTask = os_utilsRemoveFromListByTaskTcb(OS_Tcb_Curr, READY_TASK_LIST);
    Q_ASSERT(tempTask);
    os_utilsAddTaskToDelayedListByNode(tempTask);
    OS_sched();
}

/*
*********************************************************************************************************
*              OS Task Create
*
* Description: This function creates a task and then put the task to Ready Task List. The Task wil be started
*              running by OS_Run(). Some detail description is below inline.
*
* Arguments:   myTcb          pre alloocated task tcb
*              prio           Task priority
*              threadHandler  task main functin pointer
*              stkSto         pre allocated task stack start addess
*              stkSize        pre allocated task stack size
*
* Returns    : None
*
* Note       : 
*********************************************************************************************************
*/
void OS_Task_Create(
    OS_TCB *myTcb,
    uint8_t prio, /* Task priority */
    OS_TCBHandler threadHandler,
    void *stkSto, uint32_t stkSize)
{
    uint32_t *sp;
    uint32_t *stk_limit;
    uint8_t addStatus;
    
    /* round down the stack top to the 8-byte boundary
    * NOTE: ARM Cortex-M stack grows down from hi -> low memory
    */
    sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);

    /* priority must be in range
    * and the priority level must be unused
    */
    Q_REQUIRE(prio < Q_DIM(ReadyTaskList.TaskList));
    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t)threadHandler; /* Initailly is threadHandler, later will be PC */
    /* pre-fill the stack space reserved for registers for easy debug 
     * the stack space will acutully pushed by register's context when context swithch happens */ 
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
    *(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */

    /* save the top of the stack in the task's attibute */
    myTcb->OS_TcbSp = sp;

    /* round up the bottom of the stack to the 8-byte boundary */
    stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) / 8) + 1U) * 8);

    /* pre-fill the unused part of the stack with 0xDEADBEEF for easy debug */
    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xDEADBEEFU;
    }

    /* register the task with the OS */
    myTcb->OS_TcbPrio = prio;
    /* make the task ready to run */
    addStatus = os_utilsAddTaskToReadyListByTcb(myTcb);
    Q_ASSERT(addStatus == OS_ERR_NONE);
}
