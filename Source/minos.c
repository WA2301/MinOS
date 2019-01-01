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


/*
*********************************************************************************************************
*                                          TASK CONTROL BLOCK
*********************************************************************************************************
*/

typedef struct os_tcb {
    OS_STK          *OSTCBStkPtr;           /* Pointer to current top of stack                         */
    struct os_tcb   *OSTCBNext;             /* Pointer to next     TCB in the TCB list                 */

#if OS_Q_EN > 0
    OS_EVENT        *OSTCBEventPtr;         /* Pointer to          event control block                 */
    void            *OSTCBMsg;              /* Message received from OSMboxPost() or OSQPost()         */
    
    INT8U            OSTCBStat;             /* Task      status                                        */
    INT8U            OSTCBStatPend;         /* Task PEND status                                        */    
#endif

    INT16U           OSTCBDly;              /* Nbr ticks to delay task or, timeout waiting for event   */
    INT8U            OSTCBPrio;             /* Task priority (0 == highest)                            */
} OS_TCB;

OS_EXT  INT32U     OSRdyTbl;                        /* Table of tasks which are ready to run    */
OS_EXT  OS_STK     OSTaskIdleStk[OS_TASK_IDLE_STK_SIZE];      /* Idle task stack                */
OS_EXT  OS_TCB    *OSTCBCur;                        /* Pointer to currently running TCB         */
OS_EXT  OS_TCB    *OSTCBHighRdy;                    /* Pointer to highest priority TCB R-to-R   */
OS_EXT  OS_TCB    *OSTCBList;                       /* Pointer to doubly linked list of TCBs    */
OS_EXT  OS_TCB     OSTCBTbl[OS_TASK_IDLE_PRIO + 1]; /* Table of TCBs                            */


/*
*********************************************************************************************************
*                                            MinOS MANAGEMENT
*
*  Description              : The follow macro functions is implemented by your CPU architecture.  
*********************************************************************************************************
*/

#define  Trigger_PendSV()             (SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk)
#define  OS_ENTER_CRITICAL()          {cpu_sr = __get_PRIMASK();__disable_irq();}//不管当前中断使能如何，我要关中断了
#define  OS_EXIT_CRITICAL()           {__set_PRIMASK(cpu_sr);}                   //将中断状态恢复到我关之前
#define  CPU_CntTrailZeros(data)       __CLZ(__RBIT(data))


/*
*********************************************************************************************************
*                                              SCHEDULER
*
* Description: This function is called by other MinOS services to determine whether a new, high
*              priority task has been made ready to run.  This function is invoked by TASK level code
*              and is also used to reschedule tasks from ISRs (see OSIntExit()).
*
* Arguments  : none
*
* Returns    : none
*
*********************************************************************************************************
*/

void OS_Sched (void)
{
    OS_CPU_SR  cpu_sr = 0;
    
    OS_ENTER_CRITICAL();
		
    if (OSIntNesting == 0) {                            /* Schedule only if all ISRs done and ...       */
        /** OS_TCBGetHighest **/
        OSTCBHighRdy  = &OSTCBTbl[ (INT8U) CPU_CntTrailZeros( OSRdyTbl  )];
        if (OSTCBHighRdy != OSTCBCur) {         	 	 /* No Ctx Sw if current task is highest rdy     */
            Trigger_PendSV();                            /* Perform a context switch, see os_cpu_a.asm   */
        }        
    }
		
    OS_EXIT_CRITICAL();
}


/*
*********************************************************************************************************
*                                         PROCESS SYSTEM TICK
*
* Description: This function is used to signal to MinOS the occurrence of a 'system tick' (also known
*              as a 'clock tick').  This function should be called by the ticker ISR .
*
* Arguments  : none
*
* Returns    : none
*********************************************************************************************************
*/
void OS_SysTick_Handler (void)
{    
    OS_TCB    *ptcb;
    OS_CPU_SR  cpu_sr = 0;
    
    OS_ENTER_CRITICAL();
    
    OSIntEnter();       /** Tell MinOS that we are starting an ISR                **/

    ptcb = OSTCBList;                                  /* Point at first TCB in TCB list               */
    while (ptcb->OSTCBPrio != OS_TASK_IDLE_PRIO) {     /* Go through all TCBs in TCB list              */

        if (ptcb->OSTCBDly != 0) {                     /* No, Delayed or waiting for event with TO     */
            
            if (--ptcb->OSTCBDly == 0) {               /* Decrement nbr of ticks to end of delay       */
                                                       /* Check for timeout                            */
#if OS_Q_EN > 0                
                if((ptcb->OSTCBStat & OS_STAT_PEND_Q) != OS_STAT_RDY) {
                    ptcb->OSTCBStat  &= ~(INT8U)OS_STAT_PEND_Q;//清空“等待Q中”标志，若该任务只是在等待Q，则该
                                                               //语句相当于将任务设为“就绪”          	 /* Yes, Clear status flag   */
                    ptcb->OSTCBStatPend = OS_STAT_PEND_TO;     //等待状态：已超时 （已就绪，凭此标志判定是如何就绪的）               /* Indicate PEND timeout    */
                } 
                else 
                {
                    ptcb->OSTCBStatPend = OS_STAT_PEND_OK;
                }

                //任务已就绪：超时时间到、Q已收到？
                if (ptcb->OSTCBStat == OS_STAT_RDY)
#endif        
                {  /* Is task suspended?       */
                    OSRdyTbl |= ( 1 << ptcb->OSTCBPrio );                  /* No,  Make ready          */
                }
            }
        }
        ptcb = ptcb->OSTCBNext;                        /* Point at next TCB in TCB list                */
    }
    
    OS_EXIT_CRITICAL();
    
    OSIntExit();        /** Tell MinOS that we are leaving the ISR and reschedule **/
}

/*
;********************************************************************************************************
;                                         HANDLE PendSV EXCEPTION
;                                     void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
;              context switches with Cortex-M4.  This is because the Cortex-M4 auto-saves half of the
;              processor context on any exception, and restores same on return from exception.  So only
;              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
;              this way means that context saving and restoring is identical whether it is initiated from
;              a thread or occurs due to an interrupt or exception.
;
;           2) Pseudo-code is:
;              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
;              b) Save remaining regs r4-r11 on process stack;
;              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
;              d) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
;              e) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
;              f) Restore R4-R11 from new process stack;
;              g) Perform exception return which will restore remaining context.
;
;           3) On entry into PendSV handler:
;              a) The following have been saved on the process stack (by processor):
;                 xPSR, PC, LR, R12, R3, R2, R1, R0
;              b) Processor mode is switched to Handler mode (from Thread mode)
;              c) Stack is Main stack (switched from Process stack)
;              d) OSTCBCur      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdy  points to the OS_TCB of the task to resume
;
;           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
;              know that it will only be run when no other exception or interrupt is active, and
;              therefore safe to assume that context being switched out was using the process stack (PSP).
;********************************************************************************************************
*/
__asm void OS_PendSV_Handler (void)
{
    extern  OSTCBCur
    extern  OSTCBHighRdy
    
    PRESERVE8
    
    CPSID   I                   /* Prevent interruption during context switch              */
    MRS     R0, PSP             /* PSP is process stack pointer                            */
    CBZ     R0, _nosave		    /* Skip register save the first time  See:OSStartHighRdy   */
                                /*                                                         */
    SUBS    R0, R0, #0x20       /* Save remaining regs r4-11 on process stack              */
    STM     R0, {R4-R11}        /*                                                         */
                                /*                                                         */
    LDR     R1, =OSTCBCur       /* OSTCBCur->OSTCBStkPtr = SP;                             */
    LDR     R1, [R1]            /*                                                         */
    STR     R0, [R1]            /* R0 is SP of process being switched out                  */
                                /*                                                         */
                                /* At this point, entire context of process has been saved */
_nosave                         /*                                                         */
	LDR     R0, =OSTCBCur       /* OSTCBCur  = OSTCBHighRdy;                               */
    LDR     R1, =OSTCBHighRdy   /*                                                         */
    LDR     R2, [R1]            /*                                                         */
    STR     R2, [R0]            /*                                                         */
                                /*                                                         */
    LDR     R0, [R2]            /* R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;   */
    LDM     R0, {R4-R11}        /* Restore r4-11 from new process stack                    */
	                            /*                                                         */
	                            /*                                                         */
	ADDS    R0, R0, #0x20       /*                                                         */
    MSR     PSP, R0             /* Load PSP with new process SP                            */
    ORR     LR, LR, #0x04       /* Ensure exception return uses process stack              */
    CPSIE   I                   /*                                                         */
	                            /*                                                         */
    BX      LR                  /* Exception return will restore remaining context         */
                                
	ALIGN
}


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

    if (OSIntNesting > 0)                       /* See if trying to call from an ISR                  */
    {
        return;
    }
    
    if (ticks > 0)                              /* 0 means no delay!                                  */
    {
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
* Description: This task is internal to MinOS and executes whenever no other higher priority tasks
*              executes because they are ALL waiting for event(s) to occur.
*
* Arguments  : none
*
* Returns    : none
*
*********************************************************************************************************
*/

void __attribute__((weak)) OS_TaskIdle (void) 
{
    
    for (;;) 
    {
        //Do nothing.
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             INITIALIZATION
*
* Description: This function is used to initialize the internals of MinOS and MUST be called prior to
*              creating any MinOS object and, prior to calling OSStart().
*
* Arguments  : none
*
* Returns    : none
*********************************************************************************************************
*/

void  OSInit (void)
{
    INT8U    i;
		
#if OS_Q_EN > 0
    OS_EVENT  *pevent1,*pevent2;    
    
    pevent1 = &OSEventTbl[0];           /* Initialize the free list of OS_EVENTs    */
    pevent2 = &OSEventTbl[1];
    for (i = 0; i < (OS_MAX_QS - 1); i++) 
    {
        pevent1->OSEventPtr = pevent2;
        pevent1++;
        pevent2++;
    }
    pevent1->OSEventPtr = (OS_EVENT *)0;
    OSEventFreeList     = &OSEventTbl[0];
#endif	
		
    OSIntNesting  = 0;
    OSRdyTbl      = 0;            /* Clear the ready list                     */
		
    OSTCBHighRdy  = (OS_TCB *)&OSTCBTbl[OS_TASK_IDLE_PRIO];
    OSTCBCur      = (OS_TCB *)0;		
	
    for (i = 0; i < (OS_TASK_IDLE_PRIO + 1); i++) 
    {                                                       /* Init. list of free TCBs            */        
        OSTCBTbl[i].OSTCBNext = (OS_TCB *)0;
    }
    
    OSTCBList        = (OS_TCB *)0;//The First task MUST BE Idle_Task and BE the last of list                      /* TCB lists initializations          */

    OSTaskCreate(OS_TaskIdle,
                &OSTaskIdleStk[OS_TASK_IDLE_STK_SIZE - 1],
                 OS_TASK_IDLE_PRIO);                       /* Create the Idle Task                     */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                            CREATE A TASK
*
* Description: This function is used to have MinOS manage the execution of a task.  Tasks can either
*              be created prior to the start of multitasking or by a running task.  A task cannot be
*              created by an ISR.
*
* Arguments  : task     is a pointer to the task's code
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
* Returns    : The function CANNOT return normally if the task priority already exist
*
*********************************************************************************************************
*/

void  OSTaskCreate (void (*task)(void), OS_STK *ptos, INT8U prio)
{
    OS_STK    *stk;
    OS_TCB    *ptcb;
    
    stk       = ptos;                    /* Load stack pointer                                 */
    ptcb      = &OSTCBTbl[prio];
		
    if ( ptcb->OSTCBNext == (OS_TCB *)0 )  /* Make sure task doesn't already exist at this priority  */
    {
                                             /* Registers stacked as if [Auto-Saved on exception]  */
        *(  stk)  = (OS_STK)0x01000000L;     /* xPSR                                               */
        *(--stk)  = (OS_STK)task;            /* Entry Point                                        */
        *(--stk)  = (OS_STK)0xFFFFFFFEL;     /* R14 (LR) (init value will cause fault if ever used)  0xFFFFFFFE:返回ARM状态、线程模式、使用PSP,见EXC_RETURN */                                       
        *(--stk)  = (OS_STK)0x12121212L;     /* R12                                                */
        *(--stk)  = (OS_STK)0x03030303L;     /* R3                                                 */
        *(--stk)  = (OS_STK)0x02020202L;     /* R2                                                 */
        *(--stk)  = (OS_STK)0x01010101L;     /* R1                                                 */
        *(--stk)  = (OS_STK)0x00000000L;     /* R0                                                 */
        
                                             /* Remaining registers saved on process stack[PSP]    */
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

		
        
        if( prio < OSTCBHighRdy->OSTCBPrio )
        {
            OSTCBHighRdy = ptcb;
        }

        ptcb->OSTCBStkPtr     = stk;                    /* Load Stack pointer in TCB                */
        ptcb->OSTCBPrio       = prio;                   /* Load task priority into TCB              */
        ptcb->OSTCBDly        = 0;                      /* Task is not delayed                      */

    #if ( OS_Q_EN > 0 )
        ptcb->OSTCBStat       = OS_STAT_RDY;            /* Task is ready to run                     */
        ptcb->OSTCBStatPend   = OS_STAT_PEND_OK;        /* Clear pend status                        */
        ptcb->OSTCBEventPtr   = (OS_EVENT  *)0;         /* Task is not pending on an  event         */
    #endif		
        
        ptcb->OSTCBNext       = OSTCBList;              /* Link into TCB chain                      */
        OSTCBList             = ptcb;
        OSRdyTbl             |= (1 << ptcb->OSTCBPrio );/* Make task ready to run                   */
    }
    else
    {
        while(1);                                       /* Error: Minos Panic OS_ERR_PRIO_EXIST     */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                          START MULTITASKING
*
* Description: This function is used to start the multitasking process which lets MinOS manages the
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
    OSTCBCur = OSTCBHighRdy;
	
    /** OSStartHighRdy **/
    NVIC_SetPriority( PendSV_IRQn, 0xFF );
    __set_PSP(0);
    
    Trigger_PendSV();         /* Trigger the PendSV exception (causes context switch) */
    
    __enable_irq();
}



#if OS_Q_EN > 0
/*
*********************************************************************************************************
*                                        CREATE A MESSAGE QUEUE
*
* Description: This function creates a message queue if free event control blocks are available.
*
* Arguments  : start         is a pointer to the base address of the message queue storage area.  The
*                            storage area MUST be declared as an array of pointers to 'void' as follows
*
*                            void *MessageStorage[size]
*
*              size          is the number of elements in the storage area
*
* Returns    : != (OS_EVENT *)0  is a pointer to the event control clock (OS_EVENT) associated with the
*                                created queue
*              == (OS_EVENT *)0  if no event control blocks were available or an error was detected
*********************************************************************************************************
*/

OS_EVENT  *OSQCreate (void **start, INT16U size)
{
    OS_EVENT  *pevent;
    OS_CPU_SR  cpu_sr = 0;

    OS_ENTER_CRITICAL();
    if (OSEventFreeList != (OS_EVENT *)0) {      /* See if pool of free ECB pool was empty             */
        pevent = OSEventFreeList;                /* Get next free event control block                  */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OSEventPtr;
    }
    else
    {
        while(1);//No enough free ECB
    }
    
    pevent->OSQStart           = start;               /*      Initialize the queue                 */
    pevent->OSQEnd             = &start[size];
    pevent->OSQIn              = start;
    pevent->OSQOut             = start;
    pevent->OSQSize            = size;
    pevent->OSNMsgs            = 0;

    pevent->OSEventWaitTbl     = 0; /* No task waiting on event                           */

    OS_EXIT_CRITICAL();

    return (pevent);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                     PEND ON A QUEUE FOR A MESSAGE
*
* Description: This function waits for a message to be sent to a queue
*
* Arguments  : pevent        is a pointer to the event control block associated with the desired queue
*
*              timeout       is an optional timeout period (in clock ticks).  If non-zero, your task will
*                            wait for a message to arrive at the queue up to the amount of time
*                            specified by this argument.  If you specify 0, however, your task will wait
*                            forever at the specified queue or, until a message arrives.
*
*              perr          is a pointer to where an error message will be deposited.  Possible error
*                            messages are:
*
*                            OS_ERR_NONE         The call was successful and your task received a
*                                                message.
*                            OS_ERR_TIMEOUT      A message was not received within the specified 'timeout'.
*
*********************************************************************************************************
*/

void  *OSQPend (OS_EVENT *pevent, INT16U timeout, INT8U *perr)
{
    void      *pmsg;

    OS_CPU_SR  cpu_sr = 0;

    if (OSIntNesting > 0) {                      /* See if called from ISR ...                         */
        while(1);
    }
		
    OS_ENTER_CRITICAL();
    
    //消息队列中有现成的，直接返回结果，即若队列中有多个消息，该任务会一口气执行完所有消息再挂起
    if (pevent->OSNMsgs > 0) {                    /* See if any messages in the queue                   */
        
        //正常出列
        //取出队列中地址数值，并将指针下移一个单位，将已有数量减一
        pmsg = *pevent->OSQOut++;                    /* Yes, extract oldest message from the queue         */
        pevent->OSNMsgs--;                        /* Update the number of entries in the queue          */
        if (pevent->OSQOut == pevent->OSQEnd) {          /* Wrap OUT pointer if we are at the end of the queue */
            pevent->OSQOut = pevent->OSQStart;
        }
        OS_EXIT_CRITICAL();
        *perr = OS_ERR_NONE;
        return (pmsg);                           /* Return message received                            */
    }
    OSTCBCur->OSTCBStat     |= OS_STAT_PEND_Q;  //任务状态：正在等Q /* Task will have to pend for a message to be posted  */
    OSTCBCur->OSTCBStatPend  = OS_STAT_PEND_OK; //等待状态：正常等待
    OSTCBCur->OSTCBDly       = timeout;          /* Load timeout into TCB                              */
    
    //内部设置等待该事件的任务有哪些？有本任务！
    // OS_EventTaskWait(pevent);                    /* Suspend task until event or timeout occurs         */
    
    OSTCBCur->OSTCBEventPtr      = pevent;                 /* Store ptr to ECB in TCB         */
    pevent->OSEventWaitTbl          |=  ( 1<< OSTCBCur->OSTCBPrio );/* Put task in waiting list        */
    OSRdyTbl                    &= ~( 1<< OSTCBCur->OSTCBPrio );/* Task no longer ready                              */
    
    
    OS_EXIT_CRITICAL();
    OS_Sched(); 
    
    /////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////
    //已跳出该任务。。。直至超时时间到或收到Q将返回继续执行
                                     /* Find next highest priority task ready to run       */
    OS_ENTER_CRITICAL();

    //查看到底是超时还是确实收到Q了？
    switch (OSTCBCur->OSTCBStatPend) {                /* See if we timed-out or aborted                */
        //确实收到Q 了！
        case OS_STAT_PEND_OK:                         /* Extract message from TCB (Put there by QPost) */
             pmsg =  OSTCBCur->OSTCBMsg;
            *perr =  OS_ERR_NONE;
             break;

        //未收到Q，Pend超时了！
        case OS_STAT_PEND_TO:
        default:
            //  OS_EventTaskRemove(OSTCBCur, pevent);
             pevent->OSEventWaitTbl &= ~( 1 << OSTCBCur->OSTCBPrio ); /* Remove task from wait list                  */
             pmsg = (void *)0;
            *perr =  OS_ERR_TIMEOUT;                  /* Indicate that we didn't get event within TO   */
             break;
    }

    //任务已就绪
    OSTCBCur->OSTCBStat          =  OS_STAT_RDY;      /* Set   task  status to ready                   */
    
    //该次pend标志清除，下次pend重新设置
    OSTCBCur->OSTCBStatPend      =  OS_STAT_PEND_OK;  /* Clear pend  status                            */
    OSTCBCur->OSTCBEventPtr      = (OS_EVENT  *)0;    /* Clear event pointers                          */

    //清除已收到的Msg，即已将消息地址扔出去（赋值给内部变量pmsg），该Event表中已无记录	
    OSTCBCur->OSTCBMsg           = (void      *)0;    /* Clear  received message                       */
    OS_EXIT_CRITICAL();
    return (pmsg);                                    /* Return received message                       */
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                        POST MESSAGE TO A QUEUE
*
* Description: This function sends a message to a queue
*
* Arguments  : pevent        is a pointer to the event control block associated with the desired queue
*
*              pmsg          is a pointer to the message to send.
*
* Returns    : OS_ERR_NONE           The call was successful and the message was sent
*              OS_ERR_Q_FULL         If the queue cannot accept any more messages because it is full.
*
*********************************************************************************************************
*/

INT8U  OSQPost (OS_EVENT *pevent, void *pmsg)
{
    OS_TCB  *ptcb;
    INT8U    prio;
    OS_CPU_SR  cpu_sr = 0;

    OS_ENTER_CRITICAL();

    //有任务正在等待该Q！
    //若中断中连续Post会出现覆盖？不会入列？YES!
    if (pevent->OSEventWaitTbl != 0) {                     /* See if any task pending on queue             */
                                                       /* Ready highest priority task waiting on event */
        //该次Post只会喂饱一个任务（即等待该事件的任务中优先级最高的那个）
        // OS_EventTaskRdy(pevent, pmsg);

                                                        /* Find HPT waiting for message                */
        prio                  = (INT8U) CPU_CntTrailZeros( pevent-> OSEventWaitTbl );

        ptcb                  =  &OSTCBTbl[prio];        /* Point to this task's OS_TCB                 */
        ptcb->OSTCBDly        =  0;                         /* Prevent OSTimeTick() from readying task     */
            
        ptcb->OSTCBMsg        =  pmsg;                      /* Send message directly to waiting task       */
            
        ptcb->OSTCBStat      &= ~OS_STAT_PEND_Q;//若该任务只是在等待Q，则此语句相当于将任务就绪了                       /* Clear bit associated with event type        */
        ptcb->OSTCBStatPend   =  OS_STAT_PEND_OK;                 /* Set pend status of post or abort            */
                                                            /* See if task is ready (could be susp'd)      */
        if (ptcb->OSTCBStat == OS_STAT_RDY) {
            OSRdyTbl |= ( 1 << ptcb->OSTCBPrio );           /* Put task in the ready to run list           */
        }

        // OS_EventTaskRemove(ptcb, pevent);                   /* Remove this task from event   wait list     */
        pevent->OSEventWaitTbl &= ~( 1 << ptcb->OSTCBPrio ); /* Remove task from wait list                  */


        OS_EXIT_CRITICAL();
        OS_Sched();//若在中断中Post该调度将无效，将在中断退出时（OSIntExit）执行有效调度，前提是无中断嵌套                                    /* Find highest priority task ready to run      */
        return (OS_ERR_NONE);
    }
    
    if (pevent->OSNMsgs >= pevent->OSQSize) {               /* Make sure queue is not full                  */
        OS_EXIT_CRITICAL();
        while(1);//  OS_ERR_Q_FULL 消息队列已满，需增大数组
    }

    //正常入列
    *pevent->OSQIn++ = pmsg;                               /* Insert message into queue                    */
    pevent->OSNMsgs++;                                  /* Update the nbr of entries in the queue       */
    if (pevent->OSQIn == pevent->OSQEnd) {                     /* Wrap IN ptr if we are at end of queue        */
        pevent->OSQIn = pevent->OSQStart;
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}

#endif

/********************* (C) COPYRIGHT 2015 Windy Albert **************************** END OF FILE ********/
