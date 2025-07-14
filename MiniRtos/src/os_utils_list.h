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

#ifndef __OS_UTILS_H__
#define __OS_UTILS_H__

#include <stdint.h>
#include "os.h"
#define OS_TASK_PENDING    0
#define OS_NO_TASK_PENDING 1

#define READY_TASK_LIST    1
#define DELAYED_TASK_LIST  2
#define WAITING_TASK_LIST  3

#define MAX_TASK_PRIORITY     8
#define MAX_TASKS_IN_LIST     MAX_TASK_PRIORITY+1

#define GET_CURRENT_TCB() OS_tcb_curr 

typedef struct task_list_node{
    struct  task_list_node *prev;
    struct  task_list_node *next;
    OS_TCB  *pTcb;
} Task_List_Node;

typedef struct task_list{
    Task_List_Node *TaskList[MAX_TASKS_IN_LIST];
    uint32_t   TaskRriorityBitMap;
} Task_List;

extern Task_List ReadyTaskList;
extern Task_List DelayedTaskList;
extern Task_List WaitingTaskList;

void os_utilsTaskListInit();
void os_utilsAddTaskToListByNode(Task_List_Node* pTaskNode, uint8_t toWhichTaskList );
uint8_t os_utilsAddTaskToReadyListByTcb(OS_TCB *task_tcb );
void os_utilsAddTaskToDelayedListByNode(Task_List_Node *pTaskNode );
Task_List_Node *os_utilsRemoveFromListByTaskTcb(OS_TCB *task_tcb, uint8_t fromWhichList );
Task_List_Node *os_utilsRemoveFromListByTaskNode(Task_List_Node *taskToBeRemove, uint8_t fromWhichList);
Task_List_Node *os_utilsRemoveFromWaitingListHPT(OS_EVENT *pEvent);

#endif /*__OS_UTILS_H__ */