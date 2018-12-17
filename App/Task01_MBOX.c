/**
  ******************************************************************************
  * @file    Task01.c 
  * @author  Windy Albert
  * @date    08-April-2014
  * @brief   Task to ...
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "minos.h"																  /* Header file for MinOS. */

/* Public variables ----------------------------------------------------------*/
// OS_EVENT *Event_Q;



void *MessageStorage[4];

INT8U *pmsg;

/**
  * @brief  		Task01 program.
  * @function  	None
  * @RunPeriod 	None
	*/
	
void Task01(void *p_arg)
{		
/* Private variables ---------------------------------------------------------*/


INT8U err;

	
	// Event_Q = OSQCreate(MessageStorage,2);
	// for(;;) {
	// 	//TODO::
	// 	pmsg = OSQPend(Event_Q,0,&err);
	// 	Zbit(GPIOF,9);
		
	// 	//OSTimeDly(10);
	// }
}

/******************* (C) COPYRIGHT 2014 Windy Albert ***********END OF FILE****/
