/*
*********************************************************************************************************
*                                                MinOS
*                                          The Real-Time Kernel
*                                             CORE FUNCTIONS
*
*                              (c) Copyright 2018-2020, Windy Albert
*                                           All Rights Reserved
*
* File    : MINOS.h
* By      : Windy Albert & Jean J. Labrosse
* Version : V2.00 [From.V2.86]
*
*********************************************************************************************************
*/

#ifndef   _MINOS_H
#define   _MINOS_H

#include <minos_cfg.h>
//  #include "stm32f4xx.h"

/*
*********************************************************************************************************
*                                   MinOS CONFIGURATION
*  
*  Description              : You can configure the MinOS as needed.In order for MinOS to work properly, 
*                             you MUST:
*                             1.Complete task creation before OSStart()
*                             2.Invoke OSIntEnter() and OSIntExit() in pair when service an interrupt
*                               service routine (ISR).
*                               void XXX_ISR_Handler(void)
*                               {
*                                   OSIntEnter();
*
*                                   //Handler code ...
*
*                                   OSIntExit();
*                               }
*
*  OS_TASK_IDLE_PRIO        : Defines the lowest priority that can be assigned( 1 - 31 )
*                             And then the priority 0 ~ [OS_TASK_IDLE_PRIO-1] can be used for application
*  OS_TASK_IDLE_STK_SIZE    : Idle task stack size (# of OS_STK wide entries)    
*  OS_Q_EN                  : Enable (1) or Disable (0) code generation for QUEUES
*  OS_MAX_QS                : Max.number of queue control blocks in your application                   
*********************************************************************************************************
*/

#define OS_TASK_IDLE_PRIO                         7
#define OS_TASK_IDLE_STK_SIZE                   128  
#define OS_Q_EN                                   0
#define OS_MAX_QS                                 4

/*
*********************************************************************************************************
*                                              DATA TYPES
*                                         (Compiler Specific)
*********************************************************************************************************
*/

typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   int   INT32S;                   /* Signed   32 bit quantity                           */

typedef unsigned int   OS_STK;                   /* Each stack entry is 32-bit wide                    */
typedef unsigned int   OS_CPU_SR;                /* Define size of CPU status register (PSR = 32 bits) */


/*
*********************************************************************************************************
*                                             MISCELLANEOUS
*********************************************************************************************************
*/

#ifdef   OS_GLOBALS
#define  OS_EXT
#else
#define  OS_EXT  extern
#endif

// #define  OS_FALSE                     0u
// #define  OS_TRUE                      1u

// #define  OS_N_SYS_TASKS               1u
// #define  OS_TASK_IDLE_PRIO  (OS_LOWEST_PRIO)            /* IDLE      task priority                     */

/*$PAGE*/
/*
*********************************************************************************************************
*                              TASK STATUS (Bit definition for OSTCBStat)
*********************************************************************************************************
*/
#define  OS_STAT_RDY               0x00u    /* Ready to run                                            */
#define  OS_STAT_PEND_Q            0x04u    /* Pending on queue                                        */

#define  OS_STAT_PEND_OK              0u    /* Pending status OK, 1-not pending, or 2-pending complete     */
#define  OS_STAT_PEND_TO              1u    /* Pending timed out                                       */

#define  OS_ERR_NONE                   0u
// #define OS_ERR_PEND_ISR               2u
#define  OS_ERR_TIMEOUT               10u
// #define OS_ERR_PEND_ABORT            14u
// #define OS_ERR_Q_FULL                30u
// #define OS_ERR_Q_EMPTY               31u
// #define OS_ERR_PRIO_EXIST            40u
// #define OS_ERR_TASK_NO_MORE_TCB      66u
// #define OS_ERR_TASK_NOT_EXIST        67u

/*$PAGE*/
/*
*********************************************************************************************************
*                                          EVENT CONTROL BLOCK
*********************************************************************************************************
*/

#if OS_Q_EN > 0
typedef struct os_event {
    void    *OSEventPtr;                /* Pointer to message or queue structure                   */
    INT32U   OSEventWaitTbl;              /* List of tasks waiting for event to occur                */
    
    
    void         **OSQStart;            /* Pointer to start of queue data                              */
    void         **OSQEnd;              /* Pointer to end   of queue data                              */
    void         **OSQIn;               /* Pointer to where next message will be inserted  in   the Q  */
    void         **OSQOut;              /* Pointer to where next message will be extracted from the Q  */
    INT16U         OSQSize;             /* Size of queue (maximum number of entries)                   */
    INT16U         OSNMsgs;          /* Current number of of messages in message queue                      */
    
} OS_EVENT;

OS_EXT  OS_EVENT  *OSEventFreeList;          /* Pointer to list of free EVENT control blocks    */
OS_EXT  OS_EVENT   OSEventTbl[OS_MAX_QS];    /* Table of EVENT control blocks                   */

/*$PAGE*/
/*
*********************************************************************************************************
*                                         MESSAGE QUEUE MANAGEMENT
*********************************************************************************************************
*/

OS_EVENT   *OSQCreate (void **start,  INT16U size);
void 	   *OSQPend (OS_EVENT *pevent, INT16U timeout, INT8U *perr);
INT8U 	    OSQPost (OS_EVENT *pevent, void *pmsg);

#endif


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


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/
OS_EXT  INT8U      OSIntNesting;                    /* Interrupt nesting level                  */
OS_EXT  INT32U     OSRdyTbl;                        /* Table of tasks which are ready to run    */
OS_EXT  OS_STK     OSTaskIdleStk[OS_TASK_IDLE_STK_SIZE];      /* Idle task stack                */
OS_EXT  OS_TCB    *OSTCBCur;                        /* Pointer to currently running TCB         */
OS_EXT  OS_TCB    *OSTCBHighRdy;                    /* Pointer to highest priority TCB R-to-R   */
OS_EXT  OS_TCB    *OSTCBList;                       /* Pointer to doubly linked list of TCBs    */
OS_EXT  OS_TCB     OSTCBTbl[OS_TASK_IDLE_PRIO + 1]; /* Table of TCBs                            */

/*
*********************************************************************************************************
*                                            TASK MANAGEMENT
*********************************************************************************************************
*/

void OSTimeDly          (INT16U ticks);
void OSTimeTick         (void);

void OSInit             (void);
void OSTaskCreate       (void (*task)(void), OS_STK *ptos, INT8U prio);
void OSStart            (void);

#define  OSIntEnter()                 //   {if(OSIntNesting < 255u) OSIntNesting++;}
#define  OSIntExit()                  //   {if(OSIntNesting >   0 ) OSIntNesting--;OS_Sched();}

/*
*********************************************************************************************************
*                                            MinOS MANAGEMENT
*
*  Description              : The follow macro functions is implemented by your CPU architecture.  
*********************************************************************************************************
*/

#define  Trigger_PendSV()             // (SCB->ICSR |= ( 1<< SCB_ICSR_PENDSVSET_Pos ) )
#define  OS_ENTER_CRITICAL()         //  {cpu_sr = __get_PRIMASK();__disable_irq();}//不管当前中断使能如何，我要关中断了
#define  OS_EXIT_CRITICAL()          //  {__set_PRIMASK(cpu_sr);}                   //将中断状态恢复到我关之前
#define  CPU_CntTrailZeros(data)     //  __CLZ(__RBIT(data))

#endif
/********************* (C) COPYRIGHT 2018 Windy Albert **************************** END OF FILE ********/

