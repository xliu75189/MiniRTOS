#ifndef __OS_MSG_Q_H__
#define __OS_MSG_Q_H__
#include "os.h"

void  OS_MsgQ_Init (void);
void *OS_MsgQ_Wait(OS_EVENT *pEvent,
                 uint32_t timeout,
                 uint8_t *pErr);
OS_EVENT *OS_MsgQ_Create (void **start,
                        uint16_t size);
uint8_t OS_MsgQ_Send(OS_EVENT *pEvent,
                   void     *pMsg);

#endif /* __OS_MSG_Q_H__ */