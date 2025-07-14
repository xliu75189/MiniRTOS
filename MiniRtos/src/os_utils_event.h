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

#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdint.h>
#include "os.h"

#define OS_EVENT_TYPE_UNUSED  0
#define OS_EVENT_TYPE_SEM     1
#define OS_EVENT_TYPE_MQ      2

#define OS_STAT_PEND_OK       0
#define OS_STATE_SEM          1
#define OS_STAT_MQ            2

void OS_InitEventList(void);
void OS_EventWaitListInit(OS_EVENT *pEvent);
void OS_EventTaskWait(OS_TCB *tcb_curr);
uint8_t OS_EventTaskReady(OS_EVENT  *pEvent,
                          void      *pMsg,
                          uint8_t   msk,
                          uint8_t   pend_state);
void OS_MemClr(uint8_t *pDest, uint16_t size);
void OS_MemCopy(uint8_t *pDest, uint8_t *pSrc, uint16_t size);

#endif /* EVENT_H */
