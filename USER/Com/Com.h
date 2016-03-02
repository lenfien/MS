

#ifndef __COM_FUNCS
#define __COM_FUNCS

#include "stm32f10x.h"

void Delay(u32 nTime);

typedef enum
{
	TRUE = 1, FALSE = !TRUE
}bool;

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) ((a > b) ? (b) : (a))
#define ABS(a)		(((a) >= 0) ? (a) : (-(a)))

/**
  * This macro provides ways to disable NVIC interrupt and enable NVIC interrupt
	* this macro definition have a return value representing the previous state of PRIMASK
  */
extern vu32 PRIMASK_STATE;
#define __MASK_IRQ(state)	 	(PRIMASK_STATE = __get_PRIMASK(), __set_PRIMASK(state), PRIMASK_STATE)


void Com_USART1_Init(void);
void Com_USART2_Init(void);

void Com_USART1_Putc(char c);
void Com_USART2_Putc(char c);
#endif
