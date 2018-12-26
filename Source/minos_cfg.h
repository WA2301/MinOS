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

#define OS_LOWEST_PRIO           7    /* Defines the lowest priority that can be assigned ...         */
                                       /* ... MUST NEVER be higher than 31!                            */
                                       /* In othe words,that is the priority of IDLE TASK, that also...*/
                                       /* is the (number of tasks -1)                                  */
                                        //尽量根据需求设定连续的优先级，以节省内存空间
                                        //不可动态创建任务，必须在OSStart前创建完成

#define OS_TASK_IDLE_STK_SIZE   128    /* Idle       task stack size (# of OS_STK wide entries)        */

#define OS_Q_EN                   0    /* Enable (1) or Disable (0) code generation for QUEUES         */
#define OS_MAX_QS                 4    /* Max. number of queue control blocks in your application      */
                                       /* 即一套发送接收算一个事件... 目前大于等于2 */

#endif
