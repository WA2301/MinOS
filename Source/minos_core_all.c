/*
*********************************************************************************************************
*                                                MinOS
*                                          The Real-Time Kernel
*                                             CORE FUNCTIONS
*
*                              (c) Copyright 2015-2020, ZH, Windy Albert
*                                           All Rights Reserved
*
* File    : MINOS_CORE.C
* By      : Windy Albert & Jean J. Labrosse
* Version : V1.00 [From.V2.86]
*
*********************************************************************************************************
*/

#define  OS_GLOBALS  																															/* OS_EXT is BLANK.  */
#include <minos.h>
#include "stm32f4xx.h"

#if (OS_CPU_ARM_FP_EN == DEF_ENABLED)
void  OS_CPU_FP_Reg_Push   (INT32U    *stkPtr);
void  OS_CPU_FP_Reg_Pop    (INT32U    *stkPtr);
#endif


#define Trigger_PendSV()  (SCB->ICSR |= ( 1<< SCB_ICSR_PENDSVSET_Pos ) ) 


#define  OS_ENTER_CRITICAL()  {cpu_sr = __get_PRIMASK();__disable_irq();}
#define  OS_EXIT_CRITICAL()   {__set_PRIMASK(cpu_sr);}

#define  CPU_CntTrailZeros(data)    __CLZ(__RBIT(data))
#define  OS_PrioGetHighest()        (OSPrioHighRdy = (INT8U) CPU_CntTrailZeros( OSRdyTbl ))  


register volatile uint32_t* __R0  __ASM("r0");

register volatile uint32_t __R4  __ASM("r4");
register volatile uint32_t __R5  __ASM("r5");
register volatile uint32_t __R6  __ASM("r6");
register volatile uint32_t __R7  __ASM("r7");
register volatile uint32_t __R8  __ASM("r8");
register volatile uint32_t __R9  __ASM("r9");
register volatile uint32_t __R10 __ASM("r10");
register volatile uint32_t __R11 __ASM("r11");

register volatile uint32_t LR __ASM("lr");
register volatile uint32_t __PSP  __ASM("psp");



//void PendSV_Handler(void)
//{
//    
//    __disable_irq();
//    
//    
//    
//    __R0 = (uint32_t*)__PSP;
//    
//    if( __R0 != 0 )
//    {
//        *(uint32_t*)--__R0 = __R11;
//        *(uint32_t*)--__R0 = __R10;
//        *(uint32_t*)--__R0 = __R9;
//        *(uint32_t*)--__R0 = __R8;
//        *(uint32_t*)--__R0 = __R7;
//        *(uint32_t*)--__R0 = __R6;
//        *(uint32_t*)--__R0 = __R5;
//        *(uint32_t*)--__R0 = __R4;
//        

//       OSTCBCur->OSTCBStkPtr = (OS_STK*) __R0;
//    }
//    
//    
//    
//    OSPrioCur = OSPrioHighRdy;
//    OSTCBCur  = OSTCBHighRdy;
//   
//    
//    __R0 = OSTCBCur->OSTCBStkPtr;
//    
//    __R4  = *(uint32_t*)__R0++;
//    __R5  = *(uint32_t*)__R0++;
//    __R6  = *(uint32_t*)__R0++;
//    __R7  = *(uint32_t*)__R0++;
//    __R8  = *(uint32_t*)__R0++;
//    __R9  = *(uint32_t*)__R0++;
//    __R10 = *(uint32_t*)__R0++;
//    __R11 = *(uint32_t*)__R0++;
//    
//    __PSP = (uint32_t)__R0;

//   LR |= 0x04;
//    

//   __enable_irq();
//   
//}








/*$PAGE*/
/*
*********************************************************************************************************
*                              FIND HIGHEST PRIORITY TASK READY TO RUN
*
* Description: This function is called by other uC/OS-II services to determine the highest priority task
*              that is ready to run.  The global variable 'OSPrioHighRdy' is changed accordingly.
*
* Arguments  : none
*
* Returns    : none
*
*********************************************************************************************************
*/

//static  void  OS_PrioGetHighest (void)
//{
//    OSPrioHighRdy = (INT8U) CPU_CntTrailZeros( OSRdyTbl );
//}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              SCHEDULER
*
* Description: This function is called by other uC/OS-II services to determine whether a new, high
*              priority task has been made ready to run.  This function is invoked by TASK level code
*              and is not used to reschedule tasks from ISRs (see OSIntExit() for ISR rescheduling).
*
* Arguments  : none
*
* Returns    : none
*
*********************************************************************************************************
*/

void  OS_Sched (void)
{
    OS_CPU_SR  cpu_sr = 0;
    
    OS_ENTER_CRITICAL();
		
    if (OSIntNesting == 0) {                            /* Schedule only if all ISRs done and ...       */
        OS_PrioGetHighest();
        if (OSPrioHighRdy != OSPrioCur) {         	 	 /* No Ctx Sw if current task is highest rdy     */
            OSTCBHighRdy = OSTCBPrioTbl[OSPrioHighRdy];

//#if (OS_CPU_ARM_FP_EN == DEF_ENABLED)
//            OS_CPU_FP_Reg_Push(OSTCBCur->OSTCBStkPtr);   //OSTCBCur是什么时候赋值的？在pend中断里！
//            OS_CPU_FP_Reg_Pop(OSTCBHighRdy->OSTCBStkPtr);
//#endif
            
            Trigger_PendSV();                          		/* Perform a context switch, see os_cpu_a.asm   */
        }        
    }
		
    OS_EXIT_CRITICAL();
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           ENTER/EXIT ISR
*
* Description: This function is used to notify uC/OS-II that you are about to service an interrupt
*              service routine (ISR).  This allows uC/OS-II to keep track of interrupt nesting and thus
*              only perform rescheduling at the last nested ISR.
*
* Arguments  : none
*
* Returns    : none
*
* Notes      : You MUST invoke OSIntEnter() and OSIntExit() in pair.  In other words, for every call
*              to OSIntEnter() at the beginning of the ISR you MUST have a call to OSIntExit() at the
*              end of the ISR.
*              
*********************************************************************************************************
*/

void  OSIntEnter (void)//不可关中断，否则将无法实现硬嵌套
{
    if (OSIntNesting < 255u) {
        OSIntNesting++;                      							/* Increment ISR nesting level               */
    }
}

void  OSIntExit (void)
{
    OS_CPU_SR  cpu_sr = 0;
    
    OS_ENTER_CRITICAL(); //为什么需要保存？有其他关中断的可能？
    
    if (OSIntNesting > 0) {                            		 /* Prevent OSIntNesting from wrapping       */
        OSIntNesting--;
    }
    if (OSIntNesting == 0) {                           		 /* Reschedule only if all ISRs complete ... */
        OS_PrioGetHighest();
        if (OSPrioHighRdy != OSPrioCur) {          			 /* No Ctx Sw if current task is highest rdy */
            OSTCBHighRdy   = OSTCBPrioTbl[OSPrioHighRdy];
            Trigger_PendSV();                          	 		/* Perform interrupt level ctx switch       */
        }				
    }
    
    OS_EXIT_CRITICAL();
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                         PROCESS SYSTEM TICK
*
* Description: This function is used to signal to uC/OS-II the occurrence of a 'system tick' (also known
*              as a 'clock tick').  This function should be called by the ticker ISR but, can also be
*              called by a high priority task.
*
* Arguments  : none
*
* Returns    : none
*********************************************************************************************************
*/

void  OSTimeTick (void)
{
    OS_TCB    *ptcb;
    OS_CPU_SR  cpu_sr = 0;

    ptcb = OSTCBList;                                  /* Point at first TCB in TCB list               */
    while (ptcb->OSTCBPrio != OS_TASK_IDLE_PRIO) {     /* Go through all TCBs in TCB list              */
        OS_ENTER_CRITICAL();
        if (ptcb->OSTCBDly != 0) {                     /* No, Delayed or waiting for event with TO     */
            if (--ptcb->OSTCBDly == 0) {               /* Decrement nbr of ticks to end of delay       */
                                                       /* Check for timeout                            */
                if((ptcb->OSTCBStat & OS_STAT_PEND_Q) != OS_STAT_RDY) {
                    ptcb->OSTCBStat  &= ~(INT8U)OS_STAT_PEND_Q;          	 /* Yes, Clear status flag   */
                    ptcb->OSTCBStatPend = OS_STAT_PEND_TO;                 /* Indicate PEND timeout    */
                } 
                else 
                {
                    ptcb->OSTCBStatPend = OS_STAT_PEND_OK;
                }

                if ((ptcb->OSTCBStat & OS_STAT_SUSPEND) == OS_STAT_RDY) {  /* Is task suspended?       */
                    OSRdyTbl |= ( 1 << ptcb->OSTCBPrio );                  /* No,  Make ready          */
                }
            }
        }
        ptcb = ptcb->OSTCBNext;                        /* Point at next TCB in TCB list                */
        OS_EXIT_CRITICAL();
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                DELAY TASK 'n' TICKS   (n from 0 to 65535)
*
* Description: This function is called to delay execution of the currently running task until the
*              specified number of system ticks expires.  This, of course, directly equates to delaying
*              the current task for some time to expire.  No delay will result If the specified delay is
*              0.  If the specified delay is greater than 0 then, a context switch will result.
*
* Arguments  : ticks     is the time delay that the task will be suspended in number of clock 'ticks'.
*                        Note that by specifying 0, the task will not be delayed.
*
* Returns    : none
*********************************************************************************************************
*/
void  OSTimeDly (INT16U ticks)
{
    OS_CPU_SR  cpu_sr = 0;

    if (OSIntNesting > 0) {                      /* See if trying to call from an ISR                  */
        return;
    }
    
    if (ticks > 0) {                             /* 0 means no delay!                                  */
        OS_ENTER_CRITICAL();

        OSRdyTbl &= ~( 1<< OSTCBCur->OSTCBPrio );/* Delay current task                                 */ 
        OSTCBCur->OSTCBDly = ticks;              /* Load ticks in TCB                                  */
        OS_EXIT_CRITICAL();
        OS_Sched();                              /* Find next task to run!                             */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              IDLE TASK
*
* Description: This task is internal to uC/OS-II and executes whenever no other higher priority tasks
*              executes because they are ALL waiting for event(s) to occur.
*
* Arguments  : none
*
* Returns    : none
*
*********************************************************************************************************
*/

void  OS_TaskIdle (void)
{
    INT8U i = 0;
    for (;;) 
    {
        //Do nothing.
        i++;
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             INITIALIZATION
*
* Description: This function is used to initialize the internals of uC/OS-II and MUST be called prior to
*              creating any uC/OS-II object and, prior to calling OSStart().
*
* Arguments  : none
*
* Returns    : none
*********************************************************************************************************
*/

void  OSInit (void)
{
    INT8U    i;		
    OS_TCB  *ptcb1,*ptcb2;
		
#if OS_Q_EN > 0
    OS_EVENT  *pevent1,*pevent2;
    OS_Q   	  *pq1,	   *pq2;
#endif	
		
    OSIntNesting  = 0;

    OSRdyTbl      = 0;            /* Clear the ready list                     */
    OSPrioCur     = 0;
    OSPrioHighRdy = 0;
		
    OSTCBHighRdy  = (OS_TCB *)0;
    OSTCBCur      = (OS_TCB *)0;		
	
																													 /* Initialize the free list of OS_TCBs      */
    ptcb1 = &OSTCBTbl[0];
    ptcb2 = &OSTCBTbl[1];
    for (i = 0; i < (OS_LOWEST_PRIO + 1); i++) {                 /* Init. list of free TCBs            */
        ptcb1->OSTCBNext = ptcb2;
        ptcb1++;
        ptcb2++;
    }
    ptcb1->OSTCBNext = (OS_TCB *)0;                              /* Last OS_TCB                        */
    OSTCBList        = (OS_TCB *)0;                       /* TCB lists initializations          */
    OSTCBFreeList    = &OSTCBTbl[0];	

#if OS_Q_EN > 0		
    pevent1 = &OSEventTbl[0];															 /* Initialize the free list of OS_EVENTs    */
    pevent2 = &OSEventTbl[1];
    for (i = 0; i < (OS_MAX_QS - 1); i++) {
        pevent1->OSEventPtr     = pevent2;
        pevent1++;
        pevent2++;
    }
    pevent1->OSEventPtr             = (OS_EVENT *)0;
    OSEventFreeList                 = &OSEventTbl[0];		

    pq1 = &OSQTbl[0];																			 /* Initialize the message queue structures  */
    pq2 = &OSQTbl[1];
    for (i = 0; i < (OS_MAX_QS - 1); i++) {          			 /* Init. list of free QUEUE control blocks  */
        pq1->OSQPtr = pq2;
        pq1++;
        pq2++;
    }
    pq1->OSQPtr = (OS_Q *)0;
    OSQFreeList = &OSQTbl[0];
#endif

    OSTaskCreate(OS_TaskIdle,
                &OSTaskIdleStk[OS_TASK_IDLE_STK_SIZE - 1],
                 OS_TASK_IDLE_PRIO);                       /* Create the Idle Task                     */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                            INITIALIZE TCB
*
* Description: This function is internal to uC/OS-II and is used to initialize a Task Control Block when
*              a task is created (see OSTaskCreate() and OSTaskCreateExt()).
*
* Arguments  : prio          is the priority of the task being created
*
*              ptos          is a pointer to the task's top-of-stack assuming that the CPU registers
*                            have been placed on the stack.  Note that the top-of-stack corresponds to a
*                            'high' memory location is OS_STK_GROWTH is set to 1 and a 'low' memory
*                            location if OS_STK_GROWTH is set to 0.  Note that stack growth is CPU
*                            specific.
*
*              pbos          is a pointer to the bottom of stack.  A NULL pointer is passed if called by
*                            'OSTaskCreate()'.
*
*              id            is the task's ID (0..65535)
*
*              stk_size      is the size of the stack (in 'stack units').  If the stack units are INT8Us
*                            then, 'stk_size' contains the number of bytes for the stack.  If the stack
*                            units are INT32Us then, the stack contains '4 * stk_size' bytes.  The stack
*                            units are established by the #define constant OS_STK which is CPU
*                            specific.  'stk_size' is 0 if called by 'OSTaskCreate()'.
*
*              pext          is a pointer to a user supplied memory area that is used to extend the task
*                            control block.  This allows you to store the contents of floating-point
*                            registers, MMU registers or anything else you could find useful during a
*                            context switch.  You can even assign a name to each task and store this name
*                            in this TCB extension.  A NULL pointer is passed if called by OSTaskCreate().
*
* Returns    : OS_ERR_NONE         if the call was successful
*              OS_ERR_TASK_NO_MORE_TCB  if there are no more free TCBs to be allocated and thus, the task cannot
*                                  be created.
*
*********************************************************************************************************
*/

INT8U  OS_TCBInit ( INT8U prio, OS_STK *ptos )
{
    OS_TCB    *ptcb;
    OS_CPU_SR  cpu_sr = 0;

    OS_ENTER_CRITICAL();
		
    ptcb = OSTCBFreeList;                                  /* Get a free TCB from the free TCB list    */
    if (ptcb != (OS_TCB *)0) {
        OSTCBFreeList            = ptcb->OSTCBNext;        /* Update pointer to free TCB list          */
        OS_EXIT_CRITICAL();
        ptcb->OSTCBStkPtr        = ptos;                   /* Load Stack pointer in TCB                */
        ptcb->OSTCBPrio          = prio;                   /* Load task priority into TCB              */
        ptcb->OSTCBStat          = OS_STAT_RDY;            /* Task is ready to run                     */
        ptcb->OSTCBStatPend      = OS_STAT_PEND_OK;        /* Clear pend status                        */
        ptcb->OSTCBDly           = 0;                      /* Task is not delayed                      */

#if ( OS_Q_EN > 0 )
        ptcb->OSTCBEventPtr      = (OS_EVENT  *)0;         /* Task is not pending on an  event         */
#endif

        OS_ENTER_CRITICAL();				
				
        OSTCBPrioTbl[prio] = ptcb;
        ptcb->OSTCBNext    = OSTCBList;                    /* Link into TCB chain                      */
        ptcb->OSTCBPrev    = (OS_TCB *)0;
        if (OSTCBList != (OS_TCB *)0) {
            OSTCBList->OSTCBPrev = ptcb;
        }
        OSTCBList               = ptcb;

        OSRdyTbl |= ( 1 << ptcb->OSTCBPrio );               /* Make task ready to run                   */
				
        OS_EXIT_CRITICAL();
        return (OS_ERR_NONE);
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_TASK_NO_MORE_TCB);
}

/*
*********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*
* Description: This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
*              stack frame of the task being created.  This function is highly processor specific.
*
* Arguments  : task          is a pointer to the task code
*
*              p_arg         is a pointer to a user supplied data area that will be passed to the task
*                            when the task first executes.
*
*              ptos          is a pointer to the top of stack.  It is assumed that 'ptos' points to
*                            a 'free' entry on the task stack.  If OS_STK_GROWTH is set to 1 then
*                            'ptos' will contain the HIGHEST valid address of the stack.  Similarly, if
*                            OS_STK_GROWTH is set to 0, the 'ptos' will contains the LOWEST valid address
*                            of the stack.
*
* Returns    : Always returns the location of the new top-of-stack once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when your task starts executing.
*              2) All tasks run in Thread mode, using process stack.
*              
*              3) See "Cortex-M3 Programming manual.pdf"
*                 When the processor takes an exception( unless... ), 
*                 the processor pushes information onto the current stack. 
*                 This operation is referred as stacking and the structure of 
*                 eight data words is referred as stack frame. 
*                 The stack frame contains the following information:
*                      • R0-R3, R12
*                      • Return address
*                      • PSR
*                      • LR
                  取向量（指令总线）和入栈（数据总线）并行，同时执行
                  中断入栈取决于当时正在使用哪个栈，如果是正在执行中断（即被更高优先级打断），则
                  使用MSP（中断中只能使用MSP），如果是在执行任务，则使用PSP（uCOS规划在线程中
                  使用PSP）
*********************************************************************************************************
*/

OS_STK *OSTaskStkInit (void (*task)(void), OS_STK *ptos,INT8U prio)
{
    OS_STK *stk;
		
    stk       = ptos;                    /* Load stack pointer                                 */
                                         /* Registers stacked as if auto-saved on exception    */
    *(  stk)  = (OS_STK)0x01000000L;     /* xPSR                                               */
    *(--stk)  = (OS_STK)task;            /* Entry Point                                        */
    *(--stk)  = (OS_STK)0xFFFFFFFEL;     /* R14 (LR) (init value will cause fault if ever used)*/
                                         /* 0xFFFFFFFE:返回ARM状态、线程模式、使用PSP,见EXC_RETURN */
                                           
    *(--stk)  = (OS_STK)0x12121212L;     /* R12                                                */
    *(--stk)  = (OS_STK)0x03030303L;     /* R3                                                 */
    *(--stk)  = (OS_STK)0x02020202L;     /* R2                                                 */
    *(--stk)  = (OS_STK)0x01010101L;     /* R1                                                 */
    *(--stk)  = (OS_STK)0x000000000;     /* R0 : argument 原任务传入参数                                     */
                                         /* Remaining registers saved on process stack         */
    *(--stk)  = (OS_STK)0x11111111L;     /* R11                                                */
    *(--stk)  = (OS_STK)0x10101010L;     /* R10                                                */
    *(--stk)  = (OS_STK)0x09090909L;     /* R9                                                 */
    *(--stk)  = (OS_STK)0x08080808L;     /* R8                                                 */
    *(--stk)  = (OS_STK)0x07070707L;     /* R7                                                 */
    *(--stk)  = (OS_STK)0x06060606L;     /* R6                                                 */
    *(--stk)  = (OS_STK)0x05050505L;     /* R5                                                 */
    *(--stk)  = (OS_STK)0x04040404L;     /* R4                                                 */





    






















//    *(--stk)  = (OS_STK)0x02000000u;     /* FPSCR                                                  */
//                                         /* Initialize S0-S31 floating point registers             */
//    *(--stk)  = (OS_STK)0x41F80000u;     /* S31                                                    */
//    *(--stk)  = (OS_STK)0x41F00000u;     /* S30                                                    */
//    *(--stk)  = (OS_STK)0x41E80000u;     /* S29                                                    */
//    *(--stk)  = (OS_STK)0x41E00000u;     /* S28                                                    */
//    *(--stk)  = (OS_STK)0x41D80000u;     /* S27                                                    */
//    *(--stk)  = (OS_STK)0x41D00000u;     /* S26                                                    */
//    *(--stk)  = (OS_STK)0x41C80000u;     /* S25                                                    */
//    *(--stk)  = (OS_STK)0x41C00000u;     /* S24                                                    */
//    *(--stk)  = (OS_STK)0x41B80000u;     /* S23                                                    */
//    *(--stk)  = (OS_STK)0x41B00000u;     /* S22                                                    */
//    *(--stk)  = (OS_STK)0x41A80000u;     /* S21                                                    */
//    *(--stk)  = (OS_STK)0x41A00000u;     /* S20                                                    */
//    *(--stk)  = (OS_STK)0x41980000u;     /* S19                                                    */
//    *(--stk)  = (OS_STK)0x41900000u;     /* S18                                                    */
//    *(--stk)  = (OS_STK)0x41880000u;     /* S17                                                    */
//    *(--stk)  = (OS_STK)0x41800000u;     /* S16                                                    */
//    *(--stk)  = (OS_STK)0x41700000u;     /* S15                                                    */
//    *(--stk)  = (OS_STK)0x41600000u;     /* S14                                                    */
//    *(--stk)  = (OS_STK)0x41500000u;     /* S13                                                    */
//    *(--stk)  = (OS_STK)0x41400000u;     /* S12                                                    */
//    *(--stk)  = (OS_STK)0x41300000u;     /* S11                                                    */
//    *(--stk)  = (OS_STK)0x41200000u;     /* S10                                                    */
//    *(--stk)  = (OS_STK)0x41100000u;     /* S9                                                     */
//    *(--stk)  = (OS_STK)0x41000000u;     /* S8                                                     */
//    *(--stk)  = (OS_STK)0x40E00000u;     /* S7                                                     */
//    *(--stk)  = (OS_STK)0x40C00000u;     /* S6                                                     */
//    *(--stk)  = (OS_STK)0x40A00000u;     /* S5                                                     */
//    *(--stk)  = (OS_STK)0x40800000u;     /* S4                                                     */
//    *(--stk)  = (OS_STK)0x40400000u;     /* S3                                                     */
//    *(--stk)  = (OS_STK)0x40000000u;     /* S2                                                     */
//    *(--stk)  = (OS_STK)0x3F800000u;     /* S1                                                     */
//    *(--stk)  = (OS_STK)0x00000000u;     /* S0                                                     */






    return (stk);
}


//CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
//                         void          *p_arg,
//                         CPU_STK       *p_stk_base,
//                         CPU_STK       *p_stk_limit,
//                         CPU_STK_SIZE   stk_size,
//                         OS_OPT         opt)
//{
//    CPU_STK    *p_stk;


//    (void)opt;                                                  /* Prevent compiler warning                               */

//    p_stk = &p_stk_base[stk_size];                              /* Load stack pointer                                     */
//                                                                /* Align the stack to 8-bytes.                            */
//    p_stk = (CPU_STK *)((CPU_STK)(p_stk) & 0xFFFFFFF8);
//                                                                /* Registers stacked as if auto-saved on exception        */
//    *--p_stk = (CPU_STK)0x01000000u;                            /* xPSR                                                   */
//    *--p_stk = (CPU_STK)p_task;                                 /* Entry Point                                            */
//    *--p_stk = (CPU_STK)OS_TaskReturn;                          /* R14 (LR)                                               */
//    *--p_stk = (CPU_STK)0x12121212u;                            /* R12                                                    */
//    *--p_stk = (CPU_STK)0x03030303u;                            /* R3                                                     */
//    *--p_stk = (CPU_STK)0x02020202u;                            /* R2                                                     */
//    *--p_stk = (CPU_STK)p_stk_limit;                            /* R1                                                     */
//    *--p_stk = (CPU_STK)p_arg;                                  /* R0 : argument                                          */
//                                                                /* Remaining registers saved on process stack             */
//    *--p_stk = (CPU_STK)0x11111111u;                            /* R11                                                    */
//    *--p_stk = (CPU_STK)0x10101010u;                            /* R10                                                    */
//    *--p_stk = (CPU_STK)0x09090909u;                            /* R9                                                     */
//    *--p_stk = (CPU_STK)0x08080808u;                            /* R8                                                     */
//    *--p_stk = (CPU_STK)0x07070707u;                            /* R7                                                     */
//    *--p_stk = (CPU_STK)0x06060606u;                            /* R6                                                     */
//    *--p_stk = (CPU_STK)0x05050505u;                            /* R5                                                     */
//    *--p_stk = (CPU_STK)0x04040404u;                            /* R4                                                     */

//#if (OS_CPU_ARM_FP_EN == DEF_ENABLED)
//    if ((opt & OS_OPT_TASK_SAVE_FP) != (OS_OPT)0) {
//        *--p_stk = (CPU_STK)0x02000000u;                        /* FPSCR                                                  */
//                                                                /* Initialize S0-S31 floating point registers             */
//        *--p_stk = (CPU_STK)0x41F80000u;                        /* S31                                                    */
//        *--p_stk = (CPU_STK)0x41F00000u;                        /* S30                                                    */
//        *--p_stk = (CPU_STK)0x41E80000u;                        /* S29                                                    */
//        *--p_stk = (CPU_STK)0x41E00000u;                        /* S28                                                    */
//        *--p_stk = (CPU_STK)0x41D80000u;                        /* S27                                                    */
//        *--p_stk = (CPU_STK)0x41D00000u;                        /* S26                                                    */
//        *--p_stk = (CPU_STK)0x41C80000u;                        /* S25                                                    */
//        *--p_stk = (CPU_STK)0x41C00000u;                        /* S24                                                    */
//        *--p_stk = (CPU_STK)0x41B80000u;                        /* S23                                                    */
//        *--p_stk = (CPU_STK)0x41B00000u;                        /* S22                                                    */
//        *--p_stk = (CPU_STK)0x41A80000u;                        /* S21                                                    */
//        *--p_stk = (CPU_STK)0x41A00000u;                        /* S20                                                    */
//        *--p_stk = (CPU_STK)0x41980000u;                        /* S19                                                    */
//        *--p_stk = (CPU_STK)0x41900000u;                        /* S18                                                    */
//        *--p_stk = (CPU_STK)0x41880000u;                        /* S17                                                    */
//        *--p_stk = (CPU_STK)0x41800000u;                        /* S16                                                    */
//        *--p_stk = (CPU_STK)0x41700000u;                        /* S15                                                    */
//        *--p_stk = (CPU_STK)0x41600000u;                        /* S14                                                    */
//        *--p_stk = (CPU_STK)0x41500000u;                        /* S13                                                    */
//        *--p_stk = (CPU_STK)0x41400000u;                        /* S12                                                    */
//        *--p_stk = (CPU_STK)0x41300000u;                        /* S11                                                    */
//        *--p_stk = (CPU_STK)0x41200000u;                        /* S10                                                    */
//        *--p_stk = (CPU_STK)0x41100000u;                        /* S9                                                     */
//        *--p_stk = (CPU_STK)0x41000000u;                        /* S8                                                     */
//        *--p_stk = (CPU_STK)0x40E00000u;                        /* S7                                                     */
//        *--p_stk = (CPU_STK)0x40C00000u;                        /* S6                                                     */
//        *--p_stk = (CPU_STK)0x40A00000u;                        /* S5                                                     */
//        *--p_stk = (CPU_STK)0x40800000u;                        /* S4                                                     */
//        *--p_stk = (CPU_STK)0x40400000u;                        /* S3                                                     */
//        *--p_stk = (CPU_STK)0x40000000u;                        /* S2                                                     */
//        *--p_stk = (CPU_STK)0x3F800000u;                        /* S1                                                     */
//        *--p_stk = (CPU_STK)0x00000000u;                        /* S0                                                     */
//    }
//#endif

//    return (p_stk);
//}

/*$PAGE*/
/*
*********************************************************************************************************
*                                            CREATE A TASK
*
* Description: This function is used to have uC/OS-II manage the execution of a task.  Tasks can either
*              be created prior to the start of multitasking or by a running task.  A task cannot be
*              created by an ISR.
*
* Arguments  : task     is a pointer to the task's code
*
*              p_arg    is a pointer to an optional data area which can be used to pass parameters to
*                       the task when the task first executes.  Where the task is concerned it thinks
*                       it was invoked and passed the argument 'p_arg' as follows:
*
*                           void Task (void *p_arg)
*                           {
*                               for (;;) {
*                                   Task code;
*                               }
*                           }
*
*              ptos     is a pointer to the task's top of stack.  If the configuration constant
*                       OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                       memory to low memory).  'pstk' will thus point to the highest (valid) memory
*                       location of the stack.  If OS_STK_GROWTH is set to 0, 'pstk' will point to the
*                       lowest memory location of the stack and the stack will grow with increasing
*                       memory locations.
*
*              prio     is the task's priority.  A unique priority MUST be assigned to each task and the
*                       lower the number, the higher the priority.
*
* Returns    : OS_ERR_NONE             if the function was successful.
*              OS_PRIO_EXIT            if the task priority already exist
*                                      (each task MUST have a unique priority).
*              OS_ERR_PRIO_INVALID     if the priority you specify is higher that the maximum allowed
*                                      (i.e. >= OS_LOWEST_PRIO)
*              OS_ERR_TASK_CREATE_ISR  if you tried to create a task from an ISR.
*********************************************************************************************************
*/

INT8U  OSTaskCreate (void (*task)(void), OS_STK *ptos, INT8U prio)
{
    OS_STK    *psp;
    INT8U      err;
    OS_CPU_SR  cpu_sr = 0;

    OS_ENTER_CRITICAL();
		
    if (OSTCBPrioTbl[prio] == (OS_TCB *)0) { /* Make sure task doesn't already exist at this priority  */
		OSTCBPrioTbl[prio] =  (OS_TCB *)1;   /* Reserve the priority to prevent others from doing ...  */
                                             /* ... the same thing until task is created.              */
        OS_EXIT_CRITICAL();
        psp = OSTaskStkInit(task, ptos,prio);              		/* Initialize the task's stack         */				
        err = OS_TCBInit( prio, psp );				
        return (err);
    }

    OS_EXIT_CRITICAL();
    return (OS_ERR_PRIO_EXIST);
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                          START MULTITASKING
*
* Description: This function is used to start the multitasking process which lets uC/OS-II manages the
*              task that you have created.  Before you can call OSStart(), you MUST have called OSInit()
*              and you MUST have created at least one task.
*
* Arguments  : none
*
* Returns    : none
*
*********************************************************************************************************
*/

void  OSStart (void)
{
	OS_PrioGetHighest();                               	 /* Find highest priority's task priority number   */
	OSPrioCur     = OSPrioHighRdy;
	OSTCBHighRdy  = OSTCBPrioTbl[OSPrioHighRdy]; 		 /* Point to highest priority task ready to run    */
	OSTCBCur      = OSTCBHighRdy;
//	OSStartHighRdy();                            		 /* Execute target specific code to start task     */

  
   
    NVIC_SetPriority( PendSV_IRQn, 0xFF );
    __set_PSP(0);
    
    Trigger_PendSV();         /* Trigger the PendSV exception (causes context switch) */
    
    __enable_irq();

}

/********************* (C) COPYRIGHT 2015 Windy Albert **************************** END OF FILE ********/

