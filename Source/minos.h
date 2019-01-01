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

#include "stm32f4xx.h"

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
*  OS_SysTick_Handler       : The SysTick handler function for MinOS
*  OS_PendSV_Handler        : The PendSV handler function for MinOS
*********************************************************************************************************
*/

#define OS_TASK_IDLE_PRIO                         7
#define OS_TASK_IDLE_STK_SIZE                   128  
#define OS_Q_EN                                   1
#define OS_MAX_QS                                 4

#define OS_SysTick_Handler          SysTick_Handler
#define OS_PendSV_Handler            PendSV_Handler

/*
*********************************************************************************************************
*                                              DATA TYPES
*                                         (Compiler Specific)
*********************************************************************************************************
*/

typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */

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

#define  OS_ERR_NONE                  0u
#define  OS_ERR_TIMEOUT              10u

/*$PAGE*/
/*
*********************************************************************************************************
*                                          EVENT CONTROL BLOCK
*********************************************************************************************************
*/

#if OS_Q_EN > 0
typedef struct os_event {
    void    *OSEventPtr;                /* Pointer to message or queue structure                   */
    INT32U   OSEventWaitTbl;            /* List of tasks waiting for event to occur                */
    
    void         **OSQStart;            /* Pointer to start of queue data                              */
    void         **OSQEnd;              /* Pointer to end   of queue data                              */
    void         **OSQIn;               /* Pointer to where next message will be inserted  in   the Q  */
    void         **OSQOut;              /* Pointer to where next message will be extracted from the Q  */
    INT16U         OSQSize;             /* Size of queue (maximum number of entries)                   */
    INT16U         OSNMsgs;             /* Current number of of messages in message queue                      */
    
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
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/
OS_EXT  INT8U      OSIntNesting;                    /* Interrupt nesting level                  */

/*
*********************************************************************************************************
*                                            TASK MANAGEMENT
*********************************************************************************************************
*/

void OSTimeDly          (INT16U ticks);

void OSInit             (void);
void OSTaskCreate       (void (*task)(void), OS_STK *ptos, INT8U prio);
void OSStart            (void);

#define OSTask_Create(task)      OSTaskCreate (task, \
                                              &task##_Stk[ \
                                               task##_STK_SIZE - 1 ], \
                                               task##_PRIO)

#define  OSIntEnter()                   {if(OSIntNesting < 255u) OSIntNesting++;}
#define  OSIntExit()                    {if(OSIntNesting >   0 ) OSIntNesting--;OS_Sched();}


#endif
/********************* (C) COPYRIGHT 2018 Windy Albert **************************** END OF FILE ********/
