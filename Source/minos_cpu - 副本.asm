;********************************************************************************************************
;                                               MinOS
;                                         The Real-Time Kernel
;
;                               (c) Copyright 2015-2020, ZH, Windy Albert
;                                          All Rights Reserved
;
;                                           Generic ARM Port
;
; File      : MINOS_CPU.ASM
; Version   : V1.00[From.V2.86]
; By        : Windy Albert & Jean J. Labrosse
;
; For       : ARMv7M Cortex-M3
; Mode      : Thumb2
; Toolchain : RealView Development Suite
;             RealView Microcontroller Development Kit (MDK)
;             ARM Developer Suite (ADS)
;             Keil uVision
;********************************************************************************************************

;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;********************************************************************************************************

    EXTERN  OSPrioCur					       					; External references
    EXTERN  OSPrioHighRdy
    EXTERN  OSTCBCur
    EXTERN  OSTCBHighRdy

    EXPORT  OS_CPU_SR_Save                                      ; Functions declared in this file
    EXPORT  OS_CPU_SR_Restore
    EXPORT  OSStartHighRdy
    EXPORT  OSCtxSw
    EXPORT  PendSV_Handler
	EXPORT  CPU_CntTrailZeros
	;EXPORT  OS_CPU_FP_Reg_Push
	;EXPORT  OS_CPU_FP_Reg_Pop

;********************************************************************************************************
;                                                EQUATES
;********************************************************************************************************

NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
NVIC_SYSPRI14   EQU     0xE000ED22                              ; System priority register (priority 14).
NVIC_PENDSV_PRI EQU           0xFF                              ; PendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.

;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8

;********************************************************************************************************
;                                   CRITICAL SECTION METHOD 3 FUNCTIONS
;
; Description: Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
;              would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
;              disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to
;              disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
;              into the CPU's status register.
;
; Prototypes :     OS_CPU_SR  OS_CPU_SR_Save(void);
;                  void       OS_CPU_SR_Restore(OS_CPU_SR cpu_sr);
;
;
; Note(s)    : 
;********************************************************************************************************

OS_CPU_SR_Save
    MRS     R0, PRIMASK                                         ; Set prio int mask to mask all (except faults)
    CPSID   I
    BX      LR

OS_CPU_SR_Restore
    MSR     PRIMASK, R0
    BX      LR
	

;********************************************************************************************************
;                                          START MULTITASKING
;                                       void OSStartHighRdy(void)
;
; Note(s) : 1) This function triggers a PendSV exception (essentially, causes a context switch) to cause
;              the first task to start.
;
;           2) OSStartHighRdy() MUST:
;              a) Setup PendSV exception priority to lowest;
;              b) Set initial PSP to 0, to tell context switcher this is first run;
;              c) Set OSRunning to TRUE;
;              d) Trigger PendSV exception;
;              e) Enable interrupts (tasks will run with interrupts enabled).
;********************************************************************************************************

OSStartHighRdy
    LDR     R0, =NVIC_SYSPRI14                                  ; Set the PendSV exception priority
    LDR     R1, =NVIC_PENDSV_PRI
    STRB    R1, [R0]

    MOVS    R0, #0                                              ; Set the PSP to 0 for initial context switch call
    MSR     PSP, R0

    LDR     R0, =NVIC_INT_CTRL                                  ; Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I                                                   ; Enable interrupts at processor level



;********************************************************************************************************
;                               PERFORM A CONTEXT SWITCH (From task level)
;                                           void OSCtxSw(void)
;
; Note(s) : 1) OSCtxSw() is called when OS wants to perform a task context switch.  This function
;              triggers the PendSV exception which is where the real work is done.
;********************************************************************************************************

OSCtxSw
	LDR     R0, =NVIC_INT_CTRL                                  ; Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR




;********************************************************************************************************
;                                         CPU_CntTrailZeros()
;                                        COUNT TRAILING ZEROS
;
; Description : Counts the number of contiguous, least-significant, trailing zero bits before the 
;                   first binary one bit in a data value.
;
; Prototype   : CPU_DATA  CPU_CntTrailZeros(CPU_DATA  val);
;
; Argument(s) : val         Data value to count trailing zero bits.
;
; Return(s)   : Number of contiguous, least-significant, trailing zero bits in 'val'.
;
; Caller(s)   : Application.
;
;               This function is an INTERNAL CPU module function but MAY be called by application 
;               function(s).
;
; Note(s)     : (1) (a) Supports 32-bit data value size as configured by 'CPU_DATA' (see 'cpu.h  
;                       CPU WORD CONFIGURATION  Note #1').
;
;                   (b) For 32-bit values :
;
;                             b31  b30  b29  b28  b27  ...  b02  b01  b00    # Trailing Zeros
;                             ---  ---  ---  ---  ---       ---  ---  ---    ----------------
;                              x    x    x    x    x         x    x    1            0
;                              x    x    x    x    x         x    1    0            1
;                              x    x    x    x    x         1    0    0            2
;                              :    :    :    :    :         :    :    :            :
;                              :    :    :    :    :         :    :    :            :
;                              x    x    x    x    1         0    0    0           27
;                              x    x    x    1    0         0    0    0           28
;                              x    x    1    0    0         0    0    0           29
;                              x    1    0    0    0         0    0    0           30
;                              1    0    0    0    0         0    0    0           31
;                              0    0    0    0    0         0    0    0           32
;
;
;               (2) MUST be defined in 'cpu_a.asm' (or 'cpu_c.c') if CPU_CFG_TRAIL_ZEROS_ASM_PRESENT is 
;                   #define'd in 'cpu_cfg.h' or 'cpu.h'.
;********************************************************************************************************

CPU_CntTrailZeros
        RBIT    R0, R0                          ; Reverse bits
        CLZ     R0, R0                          ; Count trailing zeros
        BX      LR

		ALIGN
;********************************************************************************************************
;                                   FLOATING POINT REGISTERS PUSH
;                             void  OS_CPU_FP_Reg_Push (CPU_STK  *stkPtr)
;                          
; Note(s) : 1) This function saves S0-S31, and FPSCR registers of the Floating Point Unit.
;
;           2) Pseudo-code is:
;              a) Get FPSCR register value;
;              b) Push value on process stack;
;              c) Push remaining regs S0-S31 on process stack;
;              d) Update OSTCBCurPtr->StkPtr;
;********************************************************************************************************

    ;IF {FPU} != "SoftVFP"

;OS_CPU_FP_Reg_Push
    ;MRS     R1, PSP                                             ; PSP is process stack pointer
    ;CBZ     R1, OS_CPU_FP_nosave                                ; Skip FP register save the first time

    ;VMRS    R1, FPSCR
    ;STR R1, [R0, #-4]!
    ;VSTMDB  R0!, {S0-S31}
    ;LDR     R1, =OSTCBCur
    ;LDR     R2, [R1]
    ;STR     R0, [R2]
;OS_CPU_FP_nosave
    ;BX      LR

    ;ENDIF


;********************************************************************************************************
;                                   FLOATING POINT REGISTERS POP
;                             void  OS_CPU_FP_Reg_Pop (CPU_STK  *stkPtr)
;
; Note(s) : 1) This function restores S0-S31, and FPSCR registers of the Floating Point Unit.
;
;           2) Pseudo-code is:
;              a) Restore regs S0-S31 of new process stack;
;              b) Restore FPSCR reg value
;              c) Update OSTCBHighRdyPtr->StkPtr pointer of new proces stack;
;********************************************************************************************************

    ;IF {FPU} != "SoftVFP"

;OS_CPU_FP_Reg_Pop
    ;VLDMIA  R0!, {S0-S31}
    ;LDMIA   R0!, {R1}
    ;VMSR    FPSCR, R1
    ;LDR     R1, =OSTCBHighRdy
    ;LDR     R2, [R1]
    ;STR     R0, [R2]
    ;BX      LR

    ;ENDIF
		
		
		
		

;********************************************************************************************************
;                                         HANDLE PendSV EXCEPTION
;                                     void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
;              context switches with Cortex-M3.  This is because the Cortex-M3 auto-saves half of the
;              processor context on any exception, and restores same on return from exception.  So only
;              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
;              this way means that context saving and restoring is identical whether it is initiated from
;              a thread or occurs due to an interrupt or exception.
;
;           2) Pseudo-code is:
;              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
;              b) Save remaining regs r4-r11 on process stack;
;              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
;              d) Call OSTaskSwHook();
;              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
;              f) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
;              g) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
;              h) Restore R4-R11 from new process stack;
;              i) Perform exception return which will restore remaining context.
;
;           3) On entry into PendSV handler:
;              a) The following have been saved on the process stack (by processor):
;                 xPSR, PC, LR, R12, R0-R3
;              b) Processor mode is switched to Handler mode (from Thread mode)
;              c) Stack is Main stack (switched from Process stack)
;              d) OSTCBCur      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdy  points to the OS_TCB of the task to resume
;
;           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
;              know that it will only be run when no other exception or interrupt is active, and
;              therefore safe to assume that context being switched out was using the process stack (PSP).
;********************************************************************************************************

PendSV_Handler
    CPSID   I                                                   ; Prevent interruption during context switch
    MRS     R0, PSP                                             ; PSP is process stack pointer
    CBZ     R0, OS_CPU_PendSVHandler_nosave                     ; Skip register save the first time  See:OSStartHighRdy

    SUBS    R0, R0, #0x20                                       ; Save remaining regs r4-11 on process stack
    STM     R0, {R4-R11}

    LDR     R1, =OSTCBCur                                       ; OSTCBCur->OSTCBStkPtr = SP;
    LDR     R1, [R1]
    STR     R0, [R1]                                            ; R0 is SP of process being switched out

                                                                ; At this point, entire context of process has been saved
OS_CPU_PendSVHandler_nosave
	    ;IF {FPU} != "SoftVFP"
    ;LDR     R0, =OSTCBCur
;OS_CPU_FP_Reg_Push
    ;MRS     R1, PSP                                             ; PSP is process stack pointer
    ;CBZ     R1, OS_CPU_FP_nosave                                ; Skip FP register save the first time

    ;VMRS    R1, FPSCR
    ;STR R1, [R0, #-4]!
    ;VSTMDB  R0!, {S0-S31}
    ;LDR     R1, =OSTCBCur
    ;LDR     R2, [R1]
    ;STR     R0, [R2]
;OS_CPU_FP_nosave
    ;;No save

    ;LDR     R0, =OSTCBHighRdy
;OS_CPU_FP_Reg_Pop
    ;VLDMIA  R0!, {S0-S31}
    ;LDMIA   R0!, {R1}
    ;VMSR    FPSCR, R1
    ;LDR     R1, =OSTCBHighRdy
    ;LDR     R2, [R1]
    ;STR     R0, [R2]

    ;ENDIF






    LDR     R0, =OSPrioCur                                      ; OSPrioCur = OSPrioHighRdy;
    LDR     R1, =OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, =OSTCBCur                                       ; OSTCBCur  = OSTCBHighRdy;
    LDR     R1, =OSTCBHighRdy
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            ; R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;
    LDM     R0, {R4-R11}                                        ; Restore r4-11 from new process stack
    
	
	
	;测试代码
	;以R0内容为地址递增0x20填充0xFF
	LDR	    R3, =0xFFFFFFFF
	STR		R3,[R0 , #0] 
	STR		R3,[R0 , #4]
	STR		R3,[R0 , #8]
	STR		R3,[R0 , #12]
	STR		R3,[R0 , #16]
	STR		R3,[R0 , #20]
	STR		R3,[R0 , #24]
	STR		R3,[R0 , #28]
	
	
	
	ADDS    R0, R0, #0x20
    MSR     PSP, R0                                             ; Load PSP with new process SP
    ORR     LR, LR, #0x04                                       ; Ensure exception return uses process stack，将bit2置1，返回后将使用PSP
    CPSIE   I
	
    BX      LR                                                  ; Exception return will restore remaining context

    END
