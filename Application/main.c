/* Functions are for MINI RTOS tests */

#include <stdint.h>
#include <stdio.h>
#include "os.h"
#include "bsp.h"
#include "qassert.h"

Q_DEFINE_THIS_FILE

#define MSG_QUEUE1_SIZE 8
void *MsgQueue1[MSG_QUEUE1_SIZE];

#define MSG_QUEUE_TRACE_SIZE 16
void *MsgQueueTrace[MSG_QUEUE_TRACE_SIZE];

uint32_t stack_blinky1[128];
OS_TCB blinky1;
void main_blinky1() {
    OS_Trace("Task1 is running");

    while (1) {
        uint32_t volatile i;
        OS_Trace("Task1: Turn GREEN LED on/off");
        for (i = 3000U; i != 0U; --i) {
            BSP_ledGreenOn();
            BSP_ledGreenOff();
        }
        OS_Delay(50U); /* block for  tick */
    }
}

uint32_t stack_blinky2[128];
OS_TCB blinky2;

void main_blinky2() {
    uint8_t err;
    char *msg;
    
    while (1) {
        uint32_t volatile i;
#ifdef MQ_TEST
        OS_Trace("Task2, Waiting on MsgQ");
        msg = OS_MsgQ_Wait(SW1_MQ, NO_TIMEOUT, &err);
        Q_ASSERT(err == OS_ERR_NONE);
        OS_Trace("Task2, turn Blue LED on/off after SW pushed");
#endif
        OS_Trace("Task2: Turn BLUE LED on/off");
        for (i = 3000U; i != 0U; --i) {
            BSP_ledBlueOn();
            OS_Delay(3);
            BSP_ledBlueOff();
            OS_Delay(3);
        }        
#ifndef MQ_TEST 
#ifndef SEM_TEST        
        OS_Delay(100U); /* block for 50 ticks */
#endif 
#endif
    }
}

#if 1
uint32_t stack_blinky3[128];
OS_TCB blinky3;
void main_blinky3() {
    uint8_t err;
    
    while (1) {
#ifdef SEM_TEST
        OS_Trace("Task3, Waiting on SEMA");
        OS_Sem_Wait(SW1_sema, NO_TIMEOUT, &err);
        Q_ASSERT(err == OS_ERR_NONE);
#endif
        OS_Trace("Task3: Turn RED LED on/off");
        for (int i = 3000U; i != 0U; --i) {
            BSP_ledRedOn();
            BSP_ledRedOff();
        }
        OS_Delay(200U); /* block for 1 tick */
    }
}
#endif

#if 0
uint32_t stack_blinky4[128];
OS_TCB blinky4;
void main_blinky4() {
    uint8_t err;
    uint32_t counter;
    
    while (1) {
#ifdef SEM_TEST
        OS_Trace("Task3, Waiting on SEMA");
        OS_Sem_Wait(SW1_sema, NO_TIMEOUT, &err);
        Q_ASSERT(err == OS_ERR_NONE);
#endif
        OS_Trace("Task4: Turn RED LED on/off");
        counter=0;
        for (int i = 3000U; i != 0U; --i) {
            counter++;
        }
//        OS_Delay(200U); /* block for 1 tick */
    }
}
#endif

uint32_t stack_trace_task[128];
OS_TCB trace_task_tcb;
void task_trace() {
    uint8_t err;
    char *msg;
    
    while (1) {
        msg = OS_MsgQ_Wait(TRACE_MQ, NO_TIMEOUT, &err);
        Q_ASSERT(err == OS_ERR_NONE);
        MY_PRINTF("%s\n", msg);
    }
}

uint32_t stack_idleThread[128];

int main() {
    BSP_init();
    OS_Init(stack_idleThread, sizeof(stack_idleThread));

    TRACE_MQ = OS_MsgQ_Create(&MsgQueueTrace[0],MSG_QUEUE_TRACE_SIZE);
    Q_ASSERT(TRACE_MQ != (OS_EVENT *)0);

    /* start blinky1 task */
    OS_Task_Create(&blinky1,
                   5U, /* priority */
                   &main_blinky1,
                   stack_blinky1, sizeof(stack_blinky1));

    /* start blinky2 task */
    OS_Task_Create(&blinky2,
                   5U, /* priority */
                   &main_blinky2,
                   stack_blinky2, sizeof(stack_blinky2));

#if 1
    /* start blinky3 task */
    OS_Task_Create(&blinky3,
                   5U, /* priority */
                   &main_blinky3,

                   stack_blinky3, sizeof(stack_blinky3));
#endif     
#if 0
    /* start blinky4 task */
    OS_Task_Create(&blinky4,
                   5U, /* priority */
                   &main_blinky4,

                   stack_blinky4, sizeof(stack_blinky4));
#endif     

#if 1
    /* start trace task */
    OS_Task_Create(&trace_task_tcb,
                  2U, /* priority */
                  &task_trace,
                  stack_trace_task, sizeof(stack_trace_task));

#endif
#ifdef MQ_TEST
    SW1_MQ = OS_MsgQ_Create(&MsgQueue1[0],MSG_QUEUE1_SIZE);
    Q_ASSERT(SW1_MQ != (OS_EVENT *)0);
#endif
#ifdef SEM_TEST
/* initialize the SW1_sema semaphore as binary, signaling semaphore */
    SW1_sema = OS_Sem_Create(0,"SW1_sema");
    Q_ASSERT(SW1_sema != (OS_EVENT *)0);
#endif
    /* transfer control to the RTOS to run the task */
    OS_Run();
    Q_ERROR(); /* Should never reach here */
    return 0;
}
