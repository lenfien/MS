#include <stdio.h>
#include "stm32f10x.h"

static vu32 TimingDelay;
vu32 PRIMASK_STATE = 0;
/********************************************************************************************
*函数名称：void Delay(u32 nTime)
*
*入口参数：u32 nTime:延时时间
*
*出口参数：无
*
*功能说明：SYSTICK滴答延时配置
*******************************************************************************************/
void Delay(u32 nTime)
{
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//时钟源选择
  SysTick_Config(72000);																//调置产生中断的计数个数

  TimingDelay = nTime;
  while(TimingDelay != 0);
}

void Systick_IRQ(void)
{
  if (TimingDelay != 0x00)
  {
    TimingDelay--;
  }
}


/**
	*Initial USART 1 and redefine the printf upon USART1
  */
void Com_USART1_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitTypeStructure;
	
	NVIC_InitTypeStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitTypeStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitTypeStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitTypeStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitTypeStructure);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	
	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);
	USART_SendData(USART1, 0);	//dummpy send
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

void Com_USART2_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitTypeStructure;
	
	NVIC_InitTypeStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitTypeStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitTypeStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitTypeStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitTypeStructure);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	
	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	//USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	USART_Cmd(USART2, ENABLE);
	
	USART_SendData(USART2, 0);	//dummpy send
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}



#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART1, (uint8_t) ch);
	
  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	
  return ch;
}



void Com_USART1_Putc(char c)
{
	USART_SendData(USART1, c);	//dummpy send
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

void Com_USART2_Putc(char c)
{
	USART_SendData(USART2, c);	//dummpy send
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}
