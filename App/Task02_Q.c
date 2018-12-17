/**
  ******************************************************************************
  * @file    Task02.c 
  * @author  Windy Albert
  * @date    08-April-2014
  * @brief   Task to ...
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "minos.h"																  /* Header file for MinOS. */


/* Public variables ----------------------------------------------------------*/

// extern OS_EVENT *Event_Q;

// u16 msg=16;



/**
  * @brief  		Task00 program.
  * @function  	None
  * @RunPeriod 	None
	*/
void Task02(void *p_arg)
{	
	
	
/* Private variables ---------------------------------------------------------*/
	for(;;) {
		//TODO::
		// OSQPost(Event_Q,&msg);
		
		OSTimeDly(1000);
	}
}

/******************* (C) COPYRIGHT 2014 Windy Albert ***********END OF FILE****/
