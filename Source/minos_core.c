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

void  OS_Sched (void);

/**
  * @brief  This function handles SysTick exception.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    OSIntEnter();       /** Tell MinOS that we are starting an ISR                **/
    OSTimeTick();
    OSIntExit();        /** Tell MinOS that we are leaving the ISR and reschedule **/
}


/*$PAGE*/
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

void  OS_Sched (void)
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
    
    OS_ENTER_CRITICAL();

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
* Description: This task is internal to uC/OS-II and executes whenever no other higher priority tasks
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
* Description: This function is used to have uC/OS-II manage the execution of a task.  Tasks can either
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
    OSTCBCur = OSTCBHighRdy;
	
    /** OSStartHighRdy **/
    NVIC_SetPriority( PendSV_IRQn, 0xFF );
    __set_PSP(0);
    
    Trigger_PendSV();         /* Trigger the PendSV exception (causes context switch) */
    
    __enable_irq();
}

/********************* (C) COPYRIGHT 2015 Windy Albert **************************** END OF FILE ********/
