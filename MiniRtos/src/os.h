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

#ifndef __OS_H__
#define __OS_H__
#include <stdint.h>
#include <os_cpu.h>

#define NO_TIMEOUT 0xffffffff

#define LOG2(x) (32U - __builtin_clz(x))

#define OS_EVENT_TBL_SIZE     8
#define HIGHEST_PRIOTITY_USED 8
#define OS_MAX_EVENTS         8

#define OS_ERR_OTHER          128
#define OS_ERR_NONE           0
#define OS_ERR_EVENT_TYPE     1
#define OS_ERR_Q_FULL         2
#define OS_ERR_SEM_OVF        100

#define PRIORITY_TO_BIT(index) (1U << (index - 1U))
#define OS_MAX_MQ 8

struct os_event;
struct os_tcb;

typedef struct os_tcb {
    void             *OS_TcbSp;           /* stack pointer */
    uint32_t         OS_TcbTimeout;       /* timeout delay down-counter */
    uint8_t          OS_TcbPrio;          /* thread priority */
    struct os_event  *OS_TcbEcbPtr;       /* Pointer to event control block */
    uint8_t          OS_TcbState;         /* Task status */
    uint8_t          OS_TcbStatePend;     /* Task PEND status */
    void             *OS_TcbMQMsg;        /* Message received from OSMboxPost() or OSQPost() */
    char             *OS_TcbName;          /* TCB name */
    /* ... other attributes associated with a thread */
} OS_TCB;
typedef struct os_event {
    uint8_t    OS_EventType;           /* Type of event control block                   */
    void       *OS_EventPtr;           /* Pointer to message or queue structure         */
    uint16_t   OS_EventCnt;            /* Semaphore Count (not used if other EVENT type)*/    
    char       *OS_EventName;
} OS_EVENT;

typedef struct os_mq {            /* MESSAGE QUEUE CONTROL BLOCK */
    struct os_mq *OS_MQPtr;       /* Link to next queue control block in list of free blocks */
    void         **OS_MQStart;    /* Ptr to start of queue data */
    void         **OS_MQEnd;      /* Ptr to end   of queue data */
    void         **OS_MQIn;       /* Ptr to where next message will be inserted in the Q    */
    void         **OS_MQOut;      /* Ptr to where next message will be extracted from the Q */
    uint16_t     OS_MQSize;       /* Size of queue (maximum number of entries) */
    uint16_t     OS_MQEntries;    /* Current number of entries in the queue */
} OS_MQ;

typedef void (*OS_TCBHandler)();

extern OS_MQ *OS_MQcb_FreeList;       /* Pointer to list of free MESSAGE QUEUE control blocks */
extern OS_MQ OS_MQcb_Tbl[OS_MAX_MQ];  /* Table of MESSAGE QUEUE control blocks */

void OS_Init(void *stkSto, uint32_t stkSize);

/* callback to handle the idle condition */
void OS_OnIdle(void);

/* transfer control to the RTOS to run the tasks */
void OS_Run(void);

/* blocking delay */
void OS_Delay(uint32_t ticks);

/* callback to configure and start interrupts */
void OS_OnStartup(void);

void OS_Task_Create(OS_TCB *me, uint8_t prio, OS_TCBHandler threadHandler,
                   void *stkSto, uint32_t stkSize);
OS_EVENT *OS_Sem_Create (uint16_t cnt, char *name);
uint8_t OS_Sem_Post(OS_EVENT *pEvent);
void OS_Sem_Wait(OS_EVENT *pEvent, uint32_t timeout, uint8_t *pErr);

/*********************************************************************
* MESSAGE QUEUE prototype
**********************************************************************/
void  OS_MsgQ_Init (void);
OS_EVENT *OS_MsgQ_Create (void **start, uint16_t size);
void *OS_MsgQ_Wait(OS_EVENT *pEvent, uint32_t timeout, uint8_t *pErr);
uint8_t OS_MsgQ_Send(OS_EVENT *pEvent, void *pMsg);

#endif /* __OS_H__ */
