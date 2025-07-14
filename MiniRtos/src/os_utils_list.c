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

#include "os.h"
#include "os_utils_list.h"
#include <stdlib.h>
#include "qassert.h"
#include "os_utils_event.h"

Q_DEFINE_THIS_FILE

static Task_List* getTaskList( uint8_t whichList);
/*
*********************************************************************************************************
*              Initialized all task lists and bitmap
*
* Description: Initialized all task lists to 0 (NULL) and TaskRriorityBitMap to 0.
*
* Arguments  : 
**
* Returns    : 
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
*********************************************************************************************************
*/
void os_utilsTaskListInit(){
    uint8_t i;
    for(i=0;i<MAX_TASKS_IN_LIST;i++){
        ReadyTaskList.TaskList[i] = 0;
        DelayedTaskList.TaskList[i]= 0;
        WaitingTaskList.TaskList[i]= 0;
    }
    ReadyTaskList.TaskRriorityBitMap =0;
    DelayedTaskList.TaskRriorityBitMap =0;
    WaitingTaskList.TaskRriorityBitMap =0;    
}

/*
*********************************************************************************************************
*              Add a task to a specified task list
*
* Description: This function add a task to a specified task list. The added task is specified by Task List Node.
*
* Arguments  : *pTaskNode       Task List Node to be added
*               toWhichTaskList Specify which Task List to add
**
* Returns    : 
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
*********************************************************************************************************
*/
void os_utilsAddTaskToListByNode(Task_List_Node* pTaskNode, uint8_t toWhichTaskList ){
    uint8_t index;
    uint32_t bit;
    Task_List_Node *tempTask;
    Task_List_Node **taskList;
    Task_List *pTaskList;
    OS_TCB *pTcb;
    OS_CPU_SR  cpu_sr = 0u;

    index = pTaskNode->pTcb->OS_TcbPrio;
    Q_ASSERT((index>0) && (index <MAX_TASK_PRIORITY));

    pTaskList = getTaskList(toWhichTaskList);
    Q_ASSERT(pTaskList);
    taskList = pTaskList->TaskList;
    
    OS_ENTER_CRITICAL();
    if(taskList[index] == 0 ) { /* for this piority, it will be the first task */
        taskList[index] = pTaskNode;
    }
    else { /* this priority level already has task(s) */
        tempTask = taskList[index];
        while (tempTask->next != 0 ) {     /* To find last one in the list */ 
            tempTask= tempTask->next;
        }
        tempTask->next = pTaskNode;  /* tempTask is the last one, add new task to the end of the list */
        pTaskNode->prev = tempTask;
    }
    bit = PRIORITY_TO_BIT(index);
    pTaskList->TaskRriorityBitMap |= bit;
    
    OS_EXIT_CRITICAL();
}
/*
*********************************************************************************************************
*              Add a task to ReadyTaskList
*
* Description: This function add a task to ReadyTaskList. The added task is specified by task tcb.
*
* Arguments  : *task_tcb       task tcb to be added
**
* Returns    : uint8_t         the added result status
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
               The task list node is dynamically allocated. Once allocated, for deep embedded system,it will not
               be freed till the programe terminated. With limited tasks, it will not cause memeory leak.
*********************************************************************************************************
*/
uint8_t os_utilsAddTaskToReadyListByTcb(OS_TCB *task_tcb ){
    uint8_t index;
    uint32_t bit;
    Task_List_Node *pTempTask;
    Task_List_Node *pWalkTask;
    OS_CPU_SR  cpu_sr = 0u;

    index = task_tcb->OS_TcbPrio;

    Q_ASSERT(index <= MAX_TASK_PRIORITY );
    pTempTask = malloc(sizeof(Task_List_Node));
    if(!pTempTask)
        return OS_ERR_OTHER;
    pTempTask->prev = 0;
    pTempTask->next =0;
    pTempTask->pTcb = task_tcb;

    bit = PRIORITY_TO_BIT(index);
    OS_ENTER_CRITICAL();
    if(ReadyTaskList.TaskList[index] == 0 ) { /* for this piority, it is first task */
        ReadyTaskList.TaskList[index] = pTempTask;
        ReadyTaskList.TaskRriorityBitMap |= bit;
    }
    else {
        /* for this piority, it already has task(s), add to end */
        pWalkTask = ReadyTaskList.TaskList[index];
        while (pWalkTask->next != 0 ){
            pWalkTask= pWalkTask->next;
        }
        pWalkTask->next = pTempTask;
        pTempTask->prev = pWalkTask; 
    }        
    OS_EXIT_CRITICAL();
    return OS_ERR_NONE;
}
/*
*********************************************************************************************************
*              Add a task to DelayedTaskList
*
* Description: This function add a task to DelayedTaskList. The added task is specified by task list node.
*
* Arguments  : *pTaskNode       Task List Node to be added
**
* Returns    : 
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
*********************************************************************************************************
*/
void os_utilsAddTaskToDelayedListByNode(Task_List_Node *pTaskNode ){
    uint8_t index;
    uint32_t bit;
    Task_List_Node *pWalkTask;
    OS_CPU_SR  cpu_sr = 0u;

    index = pTaskNode->pTcb->OS_TcbPrio;
    Q_ASSERT((index>0) && (index<=MAX_TASK_PRIORITY));
    pTaskNode->prev = 0;
    pTaskNode->next =0;
    bit = PRIORITY_TO_BIT(index);
    
    OS_ENTER_CRITICAL();
    if(DelayedTaskList.TaskList[index] == 0 ) { /* for this piority, it is first task */
        DelayedTaskList.TaskList[index] = pTaskNode;
    }
    else { /* this priority level already has task(s) */
        pWalkTask = DelayedTaskList.TaskList[index];
        while (pWalkTask->next != 0 ){
            pWalkTask= pWalkTask->next;
        }
        pWalkTask->next = pTaskNode;
        pTaskNode->prev = pWalkTask;
        
    }
    DelayedTaskList.TaskRriorityBitMap |= bit;
    OS_EXIT_CRITICAL();
}
/*
*********************************************************************************************************
*              Remove the task from the specified task list
*
* Description: This function remove a task matching by TCB form the specified task list.
*
* Arguments  : *task_tcb        Matched task TCB to be removed
*              fromWhichList    Specified which task list
**
* Returns    : Task_List_Node*    The task list node matched the task TCB
* Note(s)    : This utility function is called by other functions in OS,and should not be used by applications.
*********************************************************************************************************
*/
Task_List_Node *os_utilsRemoveFromListByTaskTcb(OS_TCB *task_tcb, uint8_t fromWhichList ){
    uint8_t index;
    uint32_t bit;
    Task_List_Node *pWalkTask;
    Task_List_Node **taskList;
    Task_List *pTaskList;
    OS_CPU_SR  cpu_sr = 0u;

    index = task_tcb->OS_TcbPrio;
    Q_ASSERT((index>0) && (index<=MAX_TASK_PRIORITY));
    bit = PRIORITY_TO_BIT(index);
    
    pTaskList = getTaskList(fromWhichList);
    Q_ASSERT(pTaskList);
    taskList = pTaskList->TaskList;
        
    OS_ENTER_CRITICAL();
    pWalkTask = taskList[index];

    if( pWalkTask->next == 0 ) { /* this is the only task in list */
        if( pWalkTask->pTcb == task_tcb ){     
            taskList[index] =0;
            pTaskList->TaskRriorityBitMap &= ~bit;
            OS_EXIT_CRITICAL();
            return pWalkTask;
        } else {
            OS_EXIT_CRITICAL();
            return (Task_List_Node*)0; /* No match found */
        }
    }
    while ((pWalkTask->pTcb != task_tcb ) && (pWalkTask)){
        pWalkTask= pWalkTask->next;
    }

    if( pWalkTask == 0 ) {
        OS_EXIT_CRITICAL();
        return (Task_List_Node*)0; /* No match found */
    }
    else { /* remove pWalkTask */
        if( pWalkTask->next == 0){ /*pWalkTask is the last one */
            pWalkTask->prev->next = 0;
        } else if( pWalkTask->prev ==0 ){ /* This is the first one to be removed */
            pWalkTask->next->prev =0;
            taskList[index] = pWalkTask->next;
        }
        else{ /* This is middle one to be removed */
            pWalkTask->prev->next = pWalkTask->next;
            pWalkTask->next->prev =  pWalkTask->prev;
        }
    }
    OS_EXIT_CRITICAL();
    pWalkTask->prev = 0;
    pWalkTask->next = 0;
    return pWalkTask;
}
/*
*********************************************************************************************************
*              Remove the task from the Waiting task list
*
* Description: This function remove the highiest priority task which is waiting for 
*              the even from the Waiting task list.
*
* Arguments  : pEvent             The even the task is waiting for.
**
* Returns    : Task_List_Node*    The highiest priority task in the waiting task node which waiting for the spceified event
* Note(s)    : This utility function called by other functions in OS,and should not be used by applications.
               This function search from highiest priority task, once find a task waiting for the specifyied even
               it will return withour further searching.
*********************************************************************************************************
*/
Task_List_Node *os_utilsRemoveFromWaitingListHPT(OS_EVENT  *pEvent){
    uint8_t index;
    uint32_t bit;
    Task_List_Node **taskList;
    uint32_t workingSet;
    Task_List_Node *pTask;
    OS_TCB *pTcb; 
    OS_CPU_SR  cpu_sr = 0u;

    taskList = WaitingTaskList.TaskList;
    workingSet = WaitingTaskList.TaskRriorityBitMap;
    while (workingSet != 0U) {
        index  = LOG2(workingSet);
        pTask = taskList[index];
        pTcb = pTask->pTcb;
        Q_ASSERT(pTcb!= (OS_TCB *)0);
        bit = (1U << (pTcb->OS_TcbPrio - 1U));

        while(pTask!= 0)
        {
            if (pTask->pTcb->OS_TcbEcbPtr == pEvent)
            { /* Matched, will return */
                if((pTask->next == 0)&& (pTask->prev ==0) )
                { /* only one task in the priority group */
                    taskList[index] = 0;
                    bit = (1U << (pTcb->OS_TcbPrio - 1U));
                    WaitingTaskList.TaskRriorityBitMap &= ~bit;
                }
                else { /* this prority group has more than one tasks */
                    if( pTask->prev == 0 )
                    { /* first in the list */
                        taskList[index] = pTask->next;
                        pTask->next->prev=0;
                    } else if( pTask->next == 0)
                    { /*last one */
                        pTask->prev->next =0;
                    } else
                    { /*middle node */
                        pTask->prev->next = pTask->next;
                        pTask->next->prev = pTask->prev;
                    }
                }
                pTask->prev = 0;
                pTask->next = 0;
                return pTask;
            } /* no matched found */
            pTask = pTask->next;
        } /* while(pTask!= 0) */  
        workingSet &= ~bit; /* remove from working set */
    } /* while (workingSet != 0U) */
    return 0;
}
/*
*********************************************************************************************************
*              Remove the task from the task list
*
* Description: This function remove the task list node from the spcified task list.
*
* Arguments  : taskToBeRemove    The task list node to be removed.
*              fromWhichList     Specify which list from the task to remove  
**
* Returns    : Task_List_Node*    The tassk list node removed.
* Note(s)    : This function called by other functions in OS,and should not be used by applications.
               (1)taskToBeRemove must be in theTaskList
               (2)the function is called with taskToBeRemove's prev and next un-touched. 
                  Othersie, has to use while loop to search the whole list 
                  to check if taskToBeRemove is matched in the list
*********************************************************************************************************
*/
Task_List_Node *os_utilsRemoveFromListByTaskNode(Task_List_Node *taskToBeRemove, 
                                  uint8_t fromWhichList){
    uint8_t index;
    uint32_t bit;
    Task_List *taskList;
    Task_List_Node *pTaskNode;
    OS_CPU_SR  cpu_sr = 0u;
                                      
    Q_ASSERT(taskToBeRemove);
    index = taskToBeRemove->pTcb->OS_TcbPrio;
    Q_ASSERT((index>0) && (index<=MAX_TASK_PRIORITY));
    bit = PRIORITY_TO_BIT(index);

    taskList = getTaskList(fromWhichList);
    Q_ASSERT(taskList);                                      
    OS_ENTER_CRITICAL();
    pTaskNode = taskList->TaskList[index];
                                      
    if( pTaskNode->next == 0 ) { /* this is the only task in list */
        taskList->TaskList[index] =0;
        taskList->TaskRriorityBitMap &= ~bit;
    }
    else { /* The list has more than one tasks */
        if( taskToBeRemove->prev == 0 ){ /* first one */
            pTaskNode = taskToBeRemove;
            taskToBeRemove->next->prev=0;
            taskList->TaskList[index] = taskToBeRemove->next;
        }
        else if ( taskToBeRemove->next == 0 ) /* last one */
        {
            pTaskNode = taskToBeRemove;
            taskToBeRemove->prev->next = 0;
        }
        else { /* Middle one */
            pTaskNode = taskToBeRemove;
            taskToBeRemove->prev->next = taskToBeRemove->next;
            taskToBeRemove->next->prev = taskToBeRemove->prev;
        }
    }
    OS_EXIT_CRITICAL();
    pTaskNode->next=0;
    pTaskNode->prev=0;
    return pTaskNode;
}

/*
*********************************************************************************************************
*              Get address for one of the three task lists
*
* Description: This function gets the address for one of ReadyTaskList, DelayedTaskList, and WaitingTaskList
*
* Arguments  : whichList    Specify one of the three list.
**
* Returns    : Task_List*    Adress of the return task list.
* Note(s)    : This function called by functions in this file only.
*********************************************************************************************************
*/
static Task_List* getTaskList( uint8_t whichList){
    Task_List *pTaskList;
    
    if( whichList == READY_TASK_LIST) {
        pTaskList = &ReadyTaskList;
    }
    else if ( whichList == DELAYED_TASK_LIST) {
        pTaskList = &DelayedTaskList;
    }
    else if ( whichList == WAITING_TASK_LIST ) {
        pTaskList = &WaitingTaskList;
    }
    else {
       pTaskList = 0;
    }
    return pTaskList;
}
