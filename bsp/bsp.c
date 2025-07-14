/* Board Support Package (BSP) for the EK-TM4C123GXL board */
#include <stdint.h>  /* Standard integers. WG14/N843 C99 Standard */
#include <stdlib.h>
#include <stdio.h>
#include "bsp.h"
#include "os.h"
#include "qassert.h"
#include "TM4C123GH6PM.h" /* the TM4C MCU Peripheral Access Layer (TI) */

// BASEPRI threshold for "QF-aware" interrupts, see NOTE3
#define QF_BASEPRI          0x3F
// CMSIS threshold for "QF-aware" interrupts, see NOTE5
#define QF_AWARE_ISR_CMSIS_PRI (QF_BASEPRI >> (8 - __NVIC_PRIO_BITS))
#define TASK_AWARE_ISR_PRIO 4

/* on-board LEDs */
#define LED_RED   (1U << 1)
#define LED_BLUE  (1U << 2)
#define LED_GREEN (1U << 3)

#define PD1_PIN      (1U << 1)
/* on-board switch */
#define BTN_SW1   (1U << 4)

OS_EVENT *SW1_sema;
OS_EVENT *SW1_MQ;
OS_EVENT *TRACE_MQ;

#if 1
uint32_t cnt =0;
void GPIOPortF_IRQHandler(void) {
    uint8_t msgQueueStstus;
    if ((GPIOF_AHB->RIS & BTN_SW1) != 0U) { /* interrupt caused by SW1? */
        cnt++;
        if(cnt%2 ==0) {
            msgQueueStstus =OS_MsgQ_Send(SW1_MQ,"Msg Queue Test Message");
                if( msgQueueStstus == OS_ERR_NONE){
                }
            else {
                // light up RED and Blue LEDs
                GPIOF_AHB->DATA_Bits[LED_RED | LED_BLUE] = 0xFFU;
                }
       }
#ifdef SEM_TEST
       else
        OS_Sem_Post(SW1_sema);
#endif 
   }
    GPIOF_AHB->ICR = 0xFFU; /* clear interrupt sources */
}
#endif

void BSP_init(void) {
    SYSCTL->GPIOHBCTL |= (1U << 5); /* enable AHB for GPIOF */
    SYSCTL->RCGCGPIO  |= (1U << 5); /* enable Run Mode for GPIOF */

    GPIOF_AHB->DIR |= (LED_RED | LED_BLUE | LED_GREEN );
    GPIOF_AHB->DEN |= (LED_RED | LED_BLUE | LED_GREEN );
    /* configure switch SW1 */
    GPIOF_AHB->DIR &= ~BTN_SW1; /* input */
    GPIOF_AHB->DEN |= BTN_SW1; /* digital enable */
    GPIOF_AHB->PUR |= BTN_SW1; /* pull-up resistor enable */

    /* GPIO interrupt setup for SW1 */
    GPIOF_AHB->IS  &= ~BTN_SW1; /* edge detect for SW1 */
    GPIOF_AHB->IBE &= ~BTN_SW1; /* only one edge generate the interrupt */
    GPIOF_AHB->IEV &= ~BTN_SW1; /* a falling edge triggers the interrupt */
    GPIOF_AHB->IM  |= BTN_SW1;  /* enable GPIOF interrupt for SW1 */

    MY_PRINTF_INIT();
}

void BSP_ledRedOn(void) {
    GPIOF_AHB->DATA_Bits[LED_RED] = LED_RED;
}

void BSP_ledRedOff(void) {
    GPIOF_AHB->DATA_Bits[LED_RED] = 0U;
}

void BSP_ledBlueOn(void) {
    GPIOF_AHB->DATA_Bits[LED_BLUE] = LED_BLUE;
}

void BSP_ledBlueOff(void) {
    GPIOF_AHB->DATA_Bits[LED_BLUE] = 0U;
}

void BSP_ledGreenOn(void) {
    GPIOF_AHB->DATA_Bits[LED_GREEN] = LED_GREEN;
}

void BSP_ledGreenOff(void) {
    GPIOF_AHB->DATA_Bits[LED_GREEN] = 0U;
}

void OS_OnStartup(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);

    /* set the SysTick interrupt priority (highest) */
    //NVIC_SetPriority(SysTick_IRQn, 0U);
    /* set the interrupt priorities of "kernel aware" interrupts */
    NVIC_SetPriority(SysTick_IRQn, TASK_AWARE_ISR_PRIO);
    NVIC_SetPriority(GPIOF_IRQn,   TASK_AWARE_ISR_PRIO+1);

    /* enable IRQs in NVIC... */
    NVIC_EnableIRQ(GPIOF_IRQn);
}

void OS_OnIdle(void) {
    GPIOF_AHB->DATA_Bits[LED_RED] = LED_RED;
    GPIOF_AHB->DATA_Bits[LED_RED] = 0U;
    //OS_Trace("Idle task running");
    __WFI(); /* stop the CPU and Wait for Interrupt */
}

/* support for MY_PRINTF() ====================================================*/
#ifdef MY_PRINTF_ENABLE

#define UART_BAUD_RATE      115200U
#define UART_FR_TXFE        (1U << 7)
#define UART_FR_RXFE        (1U << 4)
#define UART_BUSY           (1U << 3)
#define UART_TXFF           (1U << 5)
#define UART_TXFIFO_DEPTH   16U


int fputc(int c, FILE *stream) {
    (void)stream; /* unused parameter */
//    GPIOD_AHB->DATA_Bits[PD1_PIN] = PD1_PIN;
    /* busy-wait as long as UART busy */
    while ((UART0->FR & UART_BUSY) != 0) {
    }
    UART0->DR = c; /* write the byte into Data Register */
//    GPIOD_AHB->DATA_Bits[PD1_PIN] = 0;
    return c;
}

void printf_init() {
    /* enable clock for UART0 and GPIOA (used by UART0 pins) */
    SYSCTL->RCGCUART   |= (1U << 0); /* enable Run mode for UART0 */
    SYSCTL->RCGCGPIO   |= (1U << 0); /* enable Run mode for GPIOA */

    /* configure UART0 pins for UART operation */
    uint32_t tmp = (1U << 0) | (1U << 1);
    GPIOA->DIR   &= ~tmp;
    GPIOA->SLR   &= ~tmp;
    GPIOA->ODR   &= ~tmp;
    GPIOA->PUR   &= ~tmp;
    GPIOA->PDR   &= ~tmp;
    GPIOA->AMSEL &= ~tmp;  /* disable analog function on the pins */
    GPIOA->AFSEL |= tmp;   /* enable ALT function on the pins */
    GPIOA->DEN   |= tmp;   /* enable digital I/O on the pins */
    GPIOA->PCTL  &= ~0x00U;
    GPIOA->PCTL  |= 0x11U;

    /* configure the UART for the desired baud rate, 8-N-1 operation */
    SystemCoreClockUpdate();
    tmp = (((SystemCoreClock * 8U) / UART_BAUD_RATE) + 1U) / 2U;
    UART0->IBRD  = tmp / 64U;
    UART0->FBRD  = tmp % 64U;
    UART0->LCRH  = (0x3U << 5); /* configure 8-N-1 operation */
    UART0->CTL   = (1U << 0)    /* UART enable */
                    | (1U << 8)  /* UART TX enable */
                    | (1U << 9); /* UART RX enable */
}
#endif

void OS_Trace(char * traceMsg){
    uint8_t msgQueueStstus;
    
    msgQueueStstus =OS_MsgQ_Send(TRACE_MQ,traceMsg);    
}

//............................................................................
_Noreturn void Q_onAssert(char const * const module, int const id) {
    (void)module; // unused parameter
    (void)id;     // unused parameter
#ifndef NDEBUG
    // light up all LEDs
    GPIOF_AHB->DATA_Bits[LED_GREEN | LED_RED | LED_BLUE] = 0xFFU;
    // for debugging, hang on in an endless loop...
    for (;;) {
    }
#endif
    NVIC_SystemReset();
}
//............................................................................
_Noreturn void assert_failed(char const * const module, int const id);
_Noreturn void assert_failed(char const * const module, int const id) {
    Q_onAssert(module, id);
}
