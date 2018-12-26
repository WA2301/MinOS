/*
*********************************************************************************************************
*                                                MinOS
*                                          The Real-Time Kernel
*                                             CORE FUNCTIONS
*
*                              (c) Copyright 2015-2020, ZH, Windy Albert
*                                           All Rights Reserved
*
* File    : MINOS.h
* By      : Windy Albert & Jean J. Labrosse
* Version : V1.00 [From.V2.86]
*
*********************************************************************************************************
*/

#ifndef   _MINOS_H
#define   _MINOS_H

#include <minos_cfg.h>
 #include "stm32f4xx.h"

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


// #if OS_CRITICAL_METHOD == 1  //不保护中断开启状态
// #define OS_ENTER_CRITICAL() __asm__("cli")  
// #define OS_EXIT_CRITICAL() __asm__("sti")  
// #endif  
  
// #if OS_CRITICAL_METHOD == 2  //入栈保护中断开启状态
// #define OS_ENTER_CRITICAL() __asm__("pushf \n\t cli")  
// #define OS_EXIT_CRITICAL() __asm__("popf")  
// #endif  
  
// #if OS_CRITICAL_METHOD == 3  //用变量保护中断开启状态
// #define OS_ENTER_CRITICAL() (cpu_sr = OSCPUSaveSR())  
// #define OS_EXIT_CRITICAL() (OSCPURestoreSR(cpu_sr))  
// #endif  


#define  Trigger_PendSV()              (SCB->ICSR |= ( 1<< SCB_ICSR_PENDSVSET_Pos ) ) 
//
//官方解释：若进入中断前已将中断关闭则不希望执行完后将中断开启
#define  OS_ENTER_CRITICAL()           {cpu_sr = __get_PRIMASK();__disable_irq();}//不管当前中断使能如何，我要关中断了
#define  OS_EXIT_CRITICAL()            {__set_PRIMASK(cpu_sr);}                   //将中断状态恢复到我关之前
//
#define  CPU_CntTrailZeros(data)       __CLZ(__RBIT(data))
//#define  OS_TCBGetHighest()            {OSTCBHighRdy  = &OSTCBTbl[ (INT8U) CPU_CntTrailZeros( OSRdyTbl  )]; }

#define OSIntEnter()                    {if(OSIntNesting < 255u) OSIntNesting++;}
#define OSIntExit()                     {if(OSIntNesting >   0 ) OSIntNesting--;OS_Sched();}




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

#define  OS_FALSE                     0u
#define  OS_TRUE                      1u

#define  OS_N_SYS_TASKS               1u
#define  OS_TASK_IDLE_PRIO  (OS_LOWEST_PRIO)            /* IDLE      task priority                     */

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


/*
*********************************************************************************************************
*                                             ERROR CODES
*********************************************************************************************************
*/
#define OS_ERR_NONE                   0u
#define OS_ERR_PEND_ISR               2u
#define OS_ERR_TIMEOUT               10u
// #define OS_ERR_PEND_ABORT            14u
#define OS_ERR_Q_FULL                30u
#define OS_ERR_Q_EMPTY               31u
#define OS_ERR_PRIO_EXIST            40u
#define OS_ERR_TASK_NO_MORE_TCB      66u
#define OS_ERR_TASK_NOT_EXIST        67u

/*$PAGE*/
/*
*********************************************************************************************************
*                                          EVENT CONTROL BLOCK
*********************************************************************************************************
*/

//MinOS中，一个Q就表示一个EVENT
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


/*$PAGE*/
/*
*********************************************************************************************************
*                                          TASK CONTROL BLOCK
*********************************************************************************************************
*/
//若无Q配置版，任务只有两种状态，在OSTCBDly中记录
typedef struct os_tcb {
    OS_STK          *OSTCBStkPtr;           /* Pointer to current top of stack                         */
    struct os_tcb   *OSTCBNext;             /* Pointer to next     TCB in the TCB list                 */

#if OS_Q_EN > 0
    OS_EVENT        *OSTCBEventPtr;         /* Pointer to          event control block                 */
    void            *OSTCBMsg;              /* Message received from OSMboxPost() or OSQPost()         */
    
    INT8U            OSTCBStat;             /* Task      status                                        */
    INT8U            OSTCBStatPend; //相当于Pend结果OK / TimeOut         /* Task PEND status                                        */
    
#endif

    INT16U           OSTCBDly;              /* Nbr ticks to delay task or, timeout waiting for event   */
    INT8U            OSTCBPrio;             /* Task priority (0 == highest)                            */
} OS_TCB;



/*$PAGE*/
/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/
OS_EXT  INT8U             OSIntNesting;             /* Interrupt nesting level                         */

/** Priority: 31 30 29 ... 3  2  1  0 
 ** OSRdyTbl:  0  0  0 ... 0  0  0  0 (32bit)
 **/
OS_EXT  INT32U            OSRdyTbl;                        /* Table of tasks which are ready to run    */

OS_EXT  OS_STK            OSTaskIdleStk[OS_TASK_IDLE_STK_SIZE];      /* Idle task stack                */

OS_EXT  OS_TCB           *OSTCBCur;                        /* Pointer to currently running TCB         */
OS_EXT  OS_TCB           *OSTCBHighRdy;                    /* Pointer to highest priority TCB R-to-R   */

//连接顺序与创建顺序相反，即Idle位于尾部，最后创建的用户任务位于头部
OS_EXT  OS_TCB           *OSTCBList;                       /* Pointer to doubly linked list of TCBs    */
OS_EXT  OS_TCB            OSTCBTbl[OS_LOWEST_PRIO + 1];/* Table of TCBs                            */


/*$PAGE*/
/*
*********************************************************************************************************
*                                            TASK MANAGEMENT
*********************************************************************************************************
*/

void          OSTaskCreate            (void           (*task)(void),
                                       OS_STK          *ptos,
                                       INT8U            prio);

void          OSTimeDly               (INT16U           ticks);
void          OSTimeTick              (void);

void          OSInit                  (void);
//void          OSIntEnter              (void);
//void          OSIntExit               (void);
void          OSStart                 (void);

#endif
/********************* (C) COPYRIGHT 2015 Windy Albert **************************** END OF FILE ********/

