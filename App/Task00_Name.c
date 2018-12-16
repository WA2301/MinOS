/**
  ******************************************************************************
  * @file    Task00.c 
  * @author  Windy Albert
  * @date    08-April-2014
  * @brief   Task to ...
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "minos.h"																  /* Header file for MinOS. */

/* Public variables ----------------------------------------------------------*/


/**
  * @brief  		Task00 program.
  * @function  	None
  * @RunPeriod 	None
	*/

void Task00(void *p_arg)
{	
/* Private variables ---------------------------------------------------------*/



	for(;;) {
		//TODO:		
		Rbit(GPIOF,6);
		OSTimeDly(100);
		Sbit(GPIOF,6);
		
		Rbit(GPIOF,7);
		OSTimeDly(100);
		Sbit(GPIOF,7);
		
		Rbit(GPIOF,8);
		OSTimeDly(100);
		Sbit(GPIOF,8);
		
//		Rbit(GPIOF,9);
//		OSTimeDly(100);
//		Sbit(GPIOF,9);
		
		
	}
}


/******************* (C) COPYRIGHT 2014 Windy Albert ***********END OF FILE****/
