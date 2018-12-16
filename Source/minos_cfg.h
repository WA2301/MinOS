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
                                       /* ... MUST NEVER be higher than 63!                            */

#define OS_MAX_TASKS             16    /* Max. number of tasks in your application, MUST be >= 2       */


#define OS_TASK_IDLE_STK_SIZE    80    /* Idle       task stack size (# of OS_STK wide entries)        */

#define OS_Q_EN                   1    /* Enable (1) or Disable (0) code generation for QUEUES         */
#define OS_MAX_QS                 4    /* Max. number of queue control blocks in your application      */


#endif
