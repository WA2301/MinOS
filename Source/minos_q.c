/*
*********************************************************************************************************
*                                                MinOS
*                                          The Real-Time Kernel
*                                             CORE FUNCTIONS
*
*                              (c) Copyright 2015-2020, ZH, Windy Albert
*                                           All Rights Reserved
*
* File    : MINOS_Q.C
* By      : Windy Albert & Jean J. Labrosse
* Version : V1.00 [From.V2.86]
*
*********************************************************************************************************
*/

#include <minos.h>

#if OS_Q_EN > 0
void  OS_Sched (void);

/*$PAGE*/
/*
*********************************************************************************************************
*                                   REMOVE TASK FROM EVENT WAIT LIST
*
* Description: Remove a task from an event's wait list.
*
* Arguments  : ptcb     is a pointer to the task to remove.
*
*              pevent   is a pointer to the event control block.
*
* Returns    : none
*
*********************************************************************************************************
*/
// static void  OS_EventTaskRemove (OS_TCB   *ptcb, OS_EVENT *pevent)
// {
//     pevent->OSEventTbl &= ~( 1 << ptcb->OSTCBPrio ); /* Remove task from wait list                  */
// }

/*
*********************************************************************************************************
*                             MAKE TASK READY TO RUN BASED ON EVENT OCCURING
*
* Description: This function is called by other uC/OS-II services and is used to ready a task that was
*              waiting for an event to occur.
*
* Arguments  : pevent      is a pointer to the event control block corresponding to the event.
*
*              pmsg        is a pointer to a message.  This pointer is used by message oriented services
*                          such as MAILBOXEs and QUEUEs.  The pointer is not used when called by other
*                          service functions.
*
*              msk         is a mask that is used to clear the status byte of the TCB.  For example,
*                          OSSemPost() will pass OS_STAT_SEM, OSMboxPost() will pass OS_STAT_MBOX etc.
*
*              pend_stat   is used to indicate the readied task's pending status:
*
*                          OS_STAT_PEND_OK      Task ready due to a post (or delete), not a timeout or
*                                               an abort.
*                          OS_STAT_PEND_ABORT   Task ready due to an abort.
*
* Returns    : none
*
* Note       : This function is INTERNAL to uC/OS-II and your application should not call it.
*********************************************************************************************************
*/

// static INT8U  OS_EventTaskRdy (OS_EVENT *pevent, void *pmsg)
// {
//     OS_TCB  *ptcb;
//     INT8U    prio;
//                                                         /* Find HPT waiting for message                */
//     prio                  = (INT8U) CPU_CntTrailZeros( pevent-> OSEventTbl );

//     ptcb                  =  OSTCBPrioTbl[prio];        /* Point to this task's OS_TCB                 */
//     ptcb->OSTCBDly        =  0;                         /* Prevent OSTimeTick() from readying task     */
		
//     ptcb->OSTCBMsg        =  pmsg;                      /* Send message directly to waiting task       */
		
//     ptcb->OSTCBStat      &= ~OS_STAT_PEND_Q;//若该任务只是在等待Q，则此语句相当于将任务就绪了                       /* Clear bit associated with event type        */
//     ptcb->OSTCBStatPend   =  OS_STAT_PEND_OK;                 /* Set pend status of post or abort            */
//                                                         /* See if task is ready (could be susp'd)      */
//     if (ptcb->OSTCBStat == OS_STAT_RDY) {
//         OSRdyTbl |= ( 1 << ptcb->OSTCBPrio );           /* Put task in the ready to run list           */
//     }

//     // OS_EventTaskRemove(ptcb, pevent);                   /* Remove this task from event   wait list     */
//     pevent->OSEventTbl &= ~( 1 << ptcb->OSTCBPrio ); /* Remove task from wait list                  */
//     return (prio);
// }

/*$PAGE*/
/*
*********************************************************************************************************
*                                   MAKE TASK WAIT FOR EVENT TO OCCUR
*
* Description: This function is called by other uC/OS-II services to suspend a task because an event has
*              not occurred.
*
* Arguments  : pevent   is a pointer to the event control block for which the task will be waiting for.
*
* Returns    : none
*
*********************************************************************************************************
*/
// static void  OS_EventTaskWait (OS_EVENT *pevent)
// {
//     OSTCBCur->OSTCBEventPtr               = pevent;                 /* Store ptr to ECB in TCB         */

//     pevent->OSEventTbl |= ( 1 << OSTCBCur->OSTCBPrio );/* Put task in waiting list        */
//     OSRdyTbl           &= ~( 1<< OSTCBCur->OSTCBPrio );/* Task no longer ready                              */
// }

/*
*********************************************************************************************************
*                                 INITIALIZE EVENT CONTROL BLOCK'S WAIT LIST
*
* Description: This function is called by other uC/OS-II services to initialize the event wait list.
*
* Arguments  : pevent    is a pointer to the event control block allocated to the event.
*
* Returns    : none
*
* Note       : This function is INTERNAL to uC/OS-II and your application should not call it.
*********************************************************************************************************
*/
// static void  OS_EventWaitListInit (OS_EVENT *pevent)
// {
//     pevent->OSEventTbl = 0; /* No task waiting on event                           */
// }

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
    OS_Q      *pq;
    OS_CPU_SR  cpu_sr = 0;

    OS_ENTER_CRITICAL();
    pevent = OSEventFreeList;                    /* Get next free event control block                  */
    if (OSEventFreeList != (OS_EVENT *)0) {      /* See if pool of free ECB pool was empty             */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OSEventPtr;
    }
    OS_EXIT_CRITICAL();
    if (pevent != (OS_EVENT *)0) {               /* See if we have an event control block              */
        OS_ENTER_CRITICAL();
        pq = OSQFreeList;                        /* Get a free queue control block                     */
        if (pq != (OS_Q *)0) {                   /* Were we able to get a queue control block ?        */
            OSQFreeList            = OSQFreeList->OSQPtr; /* Yes, Adjust free list pointer to next free*/
            OS_EXIT_CRITICAL();
            pq->OSQStart           = start;               /*      Initialize the queue                 */
            pq->OSQEnd             = &start[size];
            pq->OSQIn              = start;
            pq->OSQOut             = start;
            pq->OSQSize            = size;
            pq->OSQEntries         = 0;
						
            pevent->OSEventPtr     = pq;
						
            // OS_EventWaitListInit(pevent);                 /*      Initalize the wait list              */

            pevent->OSEventTbl     = 0; /* No task waiting on event                           */


        } else {
            pevent->OSEventPtr = (void *)OSEventFreeList; /* No,  Return event control block on error  */
            OSEventFreeList    = pevent;
            OS_EXIT_CRITICAL();
            pevent = (OS_EVENT *)0;
        }
    }
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
*                            OS_ERR_PEND_ABORT   The wait on the queue was aborted.
*                            OS_ERR_EVENT_TYPE   You didn't pass a pointer to a queue
*                            OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer
*                            OS_ERR_PEND_ISR     If you called this function from an ISR and the result
*                                                would lead to a suspension.
*                            OS_ERR_PEND_LOCKED  If you called this function with the scheduler is locked
*
* Returns    : != (void *)0  is a pointer to the message received
*              == (void *)0  if you received a NULL pointer message or,
*                            if no message was received or,
*                            if 'pevent' is a NULL pointer or,
*                            if you didn't pass a pointer to a queue.
*
*********************************************************************************************************
*/

void  *OSQPend (OS_EVENT *pevent, INT16U timeout, INT8U *perr)
{
    void      *pmsg;
    OS_Q      *pq;

    OS_CPU_SR  cpu_sr = 0;

    if (OSIntNesting > 0) {                      /* See if called from ISR ...                         */
        *perr = OS_ERR_PEND_ISR;                 /* ... can't PEND from an ISR                         */
        return ((void *)0);
    }
		
    OS_ENTER_CRITICAL();
    pq = (OS_Q *)pevent->OSEventPtr;             /* Point at queue control block                       */
    
    //消息队列中有现成的，直接返回结果，即若队列中有多个消息，该任务会一口气执行完所有消息再挂起
    if (pq->OSQEntries > 0) {                    /* See if any messages in the queue                   */
        
        //正常出列
        //取出队列中地址数值，并将指针下移一个单位，将已有数量减一
        pmsg = *pq->OSQOut++;                    /* Yes, extract oldest message from the queue         */
        pq->OSQEntries--;                        /* Update the number of entries in the queue          */
        if (pq->OSQOut == pq->OSQEnd) {          /* Wrap OUT pointer if we are at the end of the queue */
            pq->OSQOut = pq->OSQStart;
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
    pevent->OSEventTbl          |=  ( 1<< OSTCBCur->OSTCBPrio );/* Put task in waiting list        */
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
             pevent->OSEventTbl &= ~( 1 << OSTCBCur->OSTCBPrio ); /* Remove task from wait list                  */
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
*              OS_ERR_EVENT_TYPE     If you didn't pass a pointer to a queue.
*              OS_ERR_PEVENT_NULL    If 'pevent' is a NULL pointer
*
*********************************************************************************************************
*/

INT8U  OSQPost (OS_EVENT *pevent, void *pmsg)
{
    OS_Q      *pq;
    OS_TCB  *ptcb;
    INT8U    prio;
    OS_CPU_SR  cpu_sr = 0;

    OS_ENTER_CRITICAL();

    //有任务正在等待该Q！
    //若中断中连续Post会出现覆盖？不会入列？YES!
    if (pevent->OSEventTbl != 0) {                     /* See if any task pending on queue             */
                                                       /* Ready highest priority task waiting on event */
        //该次Post只会喂饱一个任务（即等待该事件的任务中优先级最高的那个）
        // OS_EventTaskRdy(pevent, pmsg);

                                                        /* Find HPT waiting for message                */
        prio                  = (INT8U) CPU_CntTrailZeros( pevent-> OSEventTbl );

        ptcb                  =  OSTCBPrioTbl[prio];        /* Point to this task's OS_TCB                 */
        ptcb->OSTCBDly        =  0;                         /* Prevent OSTimeTick() from readying task     */
            
        ptcb->OSTCBMsg        =  pmsg;                      /* Send message directly to waiting task       */
            
        ptcb->OSTCBStat      &= ~OS_STAT_PEND_Q;//若该任务只是在等待Q，则此语句相当于将任务就绪了                       /* Clear bit associated with event type        */
        ptcb->OSTCBStatPend   =  OS_STAT_PEND_OK;                 /* Set pend status of post or abort            */
                                                            /* See if task is ready (could be susp'd)      */
        if (ptcb->OSTCBStat == OS_STAT_RDY) {
            OSRdyTbl |= ( 1 << ptcb->OSTCBPrio );           /* Put task in the ready to run list           */
        }

        // OS_EventTaskRemove(ptcb, pevent);                   /* Remove this task from event   wait list     */
        pevent->OSEventTbl &= ~( 1 << ptcb->OSTCBPrio ); /* Remove task from wait list                  */









        OS_EXIT_CRITICAL();
        OS_Sched();//若在中断中Post该调度将无效，将在中断退出时（OSIntExit）执行有效调度，前提是无中断嵌套                                    /* Find highest priority task ready to run      */
        return (OS_ERR_NONE);
    }
    

    pq = (OS_Q *)pevent->OSEventPtr;                   /* Point to queue control block                 */
    if (pq->OSQEntries >= pq->OSQSize) {               /* Make sure queue is not full                  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_Q_FULL);//消息队列已满，需增大数组
    }

    //正常入列
    *pq->OSQIn++ = pmsg;                               /* Insert message into queue                    */
    pq->OSQEntries++;                                  /* Update the nbr of entries in the queue       */
    if (pq->OSQIn == pq->OSQEnd) {                     /* Wrap IN ptr if we are at end of queue        */
        pq->OSQIn = pq->OSQStart;
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}

#endif


/********************* (C) COPYRIGHT 2015 Windy Albert **************************** END OF FILE ********/

