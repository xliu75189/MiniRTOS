#ifndef __BSP_H__
#define __BSP_H__
#include "os.h"
/* system clock tick [Hz] */
#define BSP_TICKS_PER_SEC 1000U

#define MQ_TEST
#define SEM_TEST
#define MY_PRINTF_ENABLE

void BSP_init(void);

void BSP_ledRedOn(void);
void BSP_ledRedOff(void);

void BSP_ledBlueOn(void);
void BSP_ledBlueOff(void);

void BSP_ledGreenOn(void);
void BSP_ledGreenOff(void);

extern OS_EVENT *SW1_sema;
extern OS_EVENT *SW1_MQ;
extern OS_EVENT *TRACE_MQ;

#define MY_PRINTF(format_, ...) printf(format_, ##__VA_ARGS__)
#define MY_PRINTF_INIT()        printf_init()

void printf_init();
void OS_Trace(char * traceMsg);


#endif // __BSP_H__
