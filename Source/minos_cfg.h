/*
*********************************************************************************************************
*                                                MinOS
*                                          The Real-Time Kernel
*                                             CORE FUNCTIONS
*
*                              (c) Copyright 2015-2020, ZH, Windy Albert
*                                           All Rights Reserved
*
* File    : MINOS_CFG.h
* By      : Windy Albert & Jean J. Labrosse
* Version : V1.00 [From.V2.86]
*
*********************************************************************************************************
*/

#ifndef OS_CFG_H
#define OS_CFG_H      

#define OS_LOWEST_PRIO           31    /* Defines the lowest priority that can be assigned ...         */
                                       /* ... MUST NEVER be higher than 31!                            */
                                       /* In othe words,that is the priority of IDLE TASK, that also...*/
                                       /* is the (number of tasks -1)                                  */
                                        //尽量根据需求设定连续的优先级，已节省内存空间

#define OS_TASK_IDLE_STK_SIZE   128    /* Idle       task stack size (# of OS_STK wide entries)        */

#define OS_Q_EN                   1    /* Enable (1) or Disable (0) code generation for QUEUES         */
#define OS_MAX_QS                 4    /* Max. number of queue control blocks in your application      */
                                       /* 即事件的个数 一个完整的事件包括发送接受等... 目前大于等于2 */

#endif
