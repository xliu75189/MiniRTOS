#ifndef  __OS_CPU_H__
#define  __OS_CPU_H__

/*
*********************************************************************************************************
*                                     EXTERNAL C LANGUAGE LINKAGE
*
*********************************************************************************************************
*/

#define  CPU_PRIO_BASEPRI       4u
#define  CPU_NVIC_PRIO_BITS     3u

typedef unsigned int   OS_CPU_SR;   /* Define size of CPU status register (PSR = 32 bits) */

/*********************************************************************************************************
*                                    FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/* See OS_CPU_A.ASM */
OS_CPU_SR OS_CPU_SR_Save(OS_CPU_SR new_basepri);
void OS_CPU_SR_Restore(OS_CPU_SR cpu_sr);

/*
*********************************************************************************************************
*                                              Cortex-M
*                                      Critical Section Management
*
* Method #1:  Disable/Enable interrupts using simple instructions.  After critical section, interrupts
*             will be enabled even if they were disabled before entering the critical section.
*             NOT IMPLEMENTED
*
* Method #2:  Disable/Enable interrupts by preserving the state of interrupts.  In other words, if
*             interrupts were disabled before entering the critical section, they will be disabled when
*             leaving the critical section.
*             NOT IMPLEMENTED
*
* Method #3:  Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
*             would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
*             disable interrupts.  'cpu_sr' is allocated in all of functions that need to
*             disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
*             into the CPU's status register.
*********************************************************************************************************
*/
#define  OS_CRITICAL_METHOD3
#ifdef   OS_CRITICAL_METHOD1
#define  OS_ENTER_CRITICAL() __asm volatile ("cpsid i")
#define  OS_EXIT_CRITICAL()  __asm volatile ("cpsie i")
#endif /* OS_CRITICAL_METHOD1 */
#ifdef   OS_CRITICAL_METHOD3
/* Save current BASEPRI priority lvl for exception... */
#define  OS_ENTER_CRITICAL()  do { cpu_sr = OS_CPU_SR_Save(CPU_PRIO_BASEPRI << (8u - CPU_NVIC_PRIO_BITS));} while (0)
/* Restore CPU BASEPRI priority level.                */
#define  OS_EXIT_CRITICAL()   do { OS_CPU_SR_Restore(cpu_sr);} while (0)
#endif /* OS_CRITICAL_METHOD3 */
#define  OS_CONTEXT_SWITCH() *(uint32_t volatile *)0xE000ED04 = (1U << 28)

#endif /* __OS_CPU_H__ */
