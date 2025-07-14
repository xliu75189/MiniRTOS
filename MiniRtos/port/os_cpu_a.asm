;********************************************************************************************************
;********************************************************************************************************

;********************************************************************************************************
;
;                                             ARMv7-M Port
;
; Filename  : os_cpu_a.asm
; Version   : V2.93.00
;********************************************************************************************************
; For       : ARMv7-M Cortex-M
; Mode      : Thumb-2 ISA
; Toolchain : ARM C Compiler
;********************************************************************************************************
; Note(s)   : This port supports the ARM Cortex-M3, Cortex-M4 and Cortex-M7 architectures.
;********************************************************************************************************

;********************************************************************************************************
;                                          PUBLIC FUNCTIONS
;********************************************************************************************************

    EXTERN  OS_CPU_ExceptStkBase
    EXTERN  OS_KA_BASEPRI_Boundary

    EXTERN  OS_Tcb_Curr
    EXTERN  OS_Tcb_Next

    EXPORT  OS_CPU_SR_Save                                      ; Functions declared in this file
    EXPORT  OS_CPU_SR_Restore
    EXPORT PendSV_Handler

;********************************************************************************************************
;                                               EQUATES
;********************************************************************************************************

NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
NVIC_SYSPRI14   EQU     0xE000ED22                              ; System priority register (priority 14).
NVIC_PENDSV_PRI EQU           0xFF                              ; PendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.


;********************************************************************************************************
;                                     CODE GENERATION DIRECTIVES
;********************************************************************************************************

    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8



;********************************************************************************************************
;                                   CRITICAL SECTION METHOD 3 FUNCTIONS
;
; Description : Disable/Enable Kernel aware interrupts by preserving the state of BASEPRI.  Generally speaking,
;               the state of the BASEPRI interrupt exception processing is stored in the local variable
;               'cpu_sr' & Kernel Aware interrupts are then disabled ('cpu_sr' is allocated in all functions
;               that need to disable Kernel aware interrupts). The previous BASEPRI interrupt state is restored
;               by copying 'cpu_sr' into the BASEPRI register.
;
; Prototypes  : OS_CPU_SR  OS_CPU_SR_Save   (OS_CPU_SR  new_basepri);
;               void       OS_CPU_SR_Restore(OS_CPU_SR  cpu_sr);
;
;
; Note(s)     : 1) These functions are used in general like this:
;
;                  void Task (void *p_arg)
;                  {
;                      OS_CPU_SR  cpu_sr;
;
;                          :
;                          :
;                      OS_ENTER_CRITICAL();             /* cpu_sr = OS_CPU_SR_Save(new_basepri);     */
;                          :
;                          :
;                      OS_EXIT_CRITICAL();              /* OS_CPU_RestoreSR(cpu_sr);                 */
;                          :
;                          :
;                  }
;
;               2) Increasing priority using a write to BASEPRI does not take effect immediately.
;                  (a) IMPLICATION  This erratum means that the instruction after an MSR to boost BASEPRI
;                      might incorrectly be preempted by an insufficient high priority exception.
;
;                  (b) WORKAROUND  The MSR to boost BASEPRI can be replaced by the following code sequence:
;
;                      CPSID i
;                      MSR to BASEPRI
;                      DSB
;                      ISB
;                      CPSIE i
;********************************************************************************************************

OS_CPU_SR_Save
    CPSID   I                                   ; Cortex-M7 errata notice. See Note #2
    PUSH   {R1}
    MRS     R1, BASEPRI
    MSR     BASEPRI, R0
    DSB
    ISB
    MOV     R0, R1
    POP    {R1}
    CPSIE   I
    BX      LR

OS_CPU_SR_Restore
    CPSID   I                                   ; Cortex-M7 errata notice. See Note #2
    MSR     BASEPRI, R0
    DSB
    ISB
    CPSIE   I
    BX      LR
;;;;;;;;;;;;;;;;;;
PendSV_Handler
    ;/* __disable_irq(); */
    CPSID         I
    ;/* if (OS_Tcb_Curr != (OS_TCB *)0) { */
    LDR           r1,=OS_Tcb_Curr
    LDR           r1,[r1,#0x00]    
    CMP           r1,#0            
    BEQ           PendSV_restore 

    ;/*     push registers r4-r11 on the stack */
    PUSH          {r4-r11}          

    ;/*     OS_Tcb_Curr->OS_TcbSp = sp; */
    LDR           r1,=OS_Tcb_Curr       
    LDR           r1,[r1,#0x00]     
    MOV           r0,sp             
    STR           r0,[r1,#0x00]    
    ;/* } */

PendSV_restore                   
    ;/* sp = OS_Tcb_Next->OS_TcbSp; */
    LDR           r1,=OS_Tcb_Next       
    LDR           r1,[r1,#0x00]     
    LDR           r0,[r1,#0x00]     
    MOV           sp,r0             

    ;/* OS_tcb_curr = OS_Tcb_Next; */
    LDR           r1,=OS_Tcb_Next       
    LDR           r1,[r1,#0x00]    
    LDR           r2,=OS_Tcb_Curr       
    STR           r1,[r2,#0x00]     

    ;/* pop registers r4-r11 */
    POP           {r4-r11}         

    ;/* __enable_irq(); */
    CPSIE         I                

    ;/* return to the next thread */
    BX            lr                
;;;;;;;;;;;;;;;;;;;;

    ALIGN                                                       ; Removes warning[A1581W]: added <no_padbytes> of padding at <address>

    END
