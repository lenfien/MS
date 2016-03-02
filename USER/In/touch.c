#include <math.h>
#include "stm32f10x.h"
#include "com.h"
#include "gui.h"
#include "touch.h"

#define TOUCH_PORT	GPIOB      		//ADS7483所使用的端口
#define SCK_PORT   	TOUCH_PORT    //时钟端口
#define SCK_PIN    	GPIO_Pin_13     //时钟引脚
#define OUT_PORT  	TOUCH_PORT    //输出端口
#define OUT_PIN   	GPIO_Pin_15     //输出引脚
#define IN_PORT  		TOUCH_PORT    //输入端口
#define IN_PIN   		GPIO_Pin_14     //输入引脚
#define CS_PORT   	TOUCH_PORT    //片选端口
#define CS_PIN    	GPIO_Pin_12     //片选引脚
#define INT_PORT		GPIOG						//中断端口
#define INT_PIN			GPIO_Pin_7      //中断引脚
#define INT_LINE		GPIO_Pin_7      //中断线

//时钟高/低
#define SCK_HIGH   	GPIO_WriteBit(SCK_PORT, SCK_PIN, Bit_SET)
#define SCK_LOW    	GPIO_WriteBit(SCK_PORT, SCK_PIN, Bit_RESET)

//输出高/低
#define OUT_HIGH	GPIO_WriteBit(OUT_PORT, OUT_PIN, Bit_SET)
#define OUT_LOW		GPIO_WriteBit(OUT_PORT, OUT_PIN, Bit_RESET)

//读引脚数据
#define GET_IN		GPIO_ReadInputDataBit(IN_PORT, IN_PIN)

//片选定义
#define CS_HIGH		GPIO_WriteBit(CS_PORT, CS_PIN, Bit_SET)
#define CS_LOW		GPIO_WriteBit(CS_PORT, CS_PIN, Bit_RESET)

//ADS7843 忙标志
#define TOUCH_BUSY_PORT			GPIOG
#define TOUCH_BUSY_PIN			GPIO_Pin_8
#define TOUCH_IS_BUSY()			(bool)GPIO_ReadInputDataBit(TOUCH_BUSY_PORT, TOUCH_BUSY_PIN)
	
#define CMD_X				0x90
#define CMD_Y				0xD0
#define ADS7483_IS_PRESSED()	(bool)(!GPIO_ReadInputDataBit(INT_PORT, INT_PIN))	//检测屏幕按下状态


static bool Touch_Conversion(PointType* point);
static int Touch_PointFilter(u16 *start, u8 num);

/*
	此结构体很潇洒的给出了当前触摸屏的状态
	isTouched;		//触屏是否被暂时按下
	isHold;     	//触屏是否被一直按下
	point;			//当前被触摸的点
	可以在初始化了触摸屏之后,在任何
	地方访问这个结构体来获知触摸屏的状态
	此结构体在ADS7483_update_state被更新
	ADS7483_update_state在触摸屏被按下或释放时在外部中断7线事上被调用
*/
static 	TouchStateType 	TouchState;

NVIC_InitTypeDef NVIC_InitStructure;


extern const TouchStateType* Touch_GetState()
{
	return &TouchState;
}




//外部INT配置
void Touch_PortInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOG | RCC_APB2Periph_AFIO, ENABLE ); 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 	
	GPIO_Init(GPIOG,&GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource7);

	EXTI_InitStructure.EXTI_Line = EXTI_Line7;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	   				// 设置中断组 为2 
	
	/* Enable the EXTI15_10_IRQn Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;				// 配置中断线4
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 设置占先优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  	// 设置副优先级为0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  		// 使能中断线4
	NVIC_Init(&NVIC_InitStructure);	
}


void Touch_Sleep()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	   				// 设置中断组 为2 
	
	/* Enable the EXTI15_10_IRQn Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;				// 配置中断线4
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 设置占先优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  	// 设置副优先级为0
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;			  		// 使能中断线4
	NVIC_Init(&NVIC_InitStructure);	
}

extern void Touch_Awake()
{
	/* Enable and set EXTI0 Interrupt to the lowest priority */
  /* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	   				// 设置中断组 为2 
	
	/* Enable the EXTI15_10_IRQn Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;				// 配置中断线4
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 设置占先优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  	// 设置副优先级为0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  		// 使能中断线4
	NVIC_Init(&NVIC_InitStructure);	
}

static void delay(u32 s)
{
	while(s--);
}

void Touch_Init()
{
	Touch_PortInit(); 			//端口初始化
	CS_HIGH;					//片选拉高
	delay(20);
	SCK_LOW;					//时钟为低
	delay(100);
	CS_LOW;           //片选拉低
	delay(100);
}

//给ADS7483发送命令
static void Touch_SendCMD(u8 cmd)
{
	u8 index = 0;
	
	for(index = 0; index < 8; index ++)
	{
		(cmd & 0x80)?OUT_HIGH:OUT_LOW;	//MSB first, 根据数据位拉高或拉低OUT
		//delay(1);
		SCK_LOW;                        //拉低
		//delay(1);
		SCK_HIGH;						//拉高  产生一个上升沿
		cmd <<= 1;
	}
	SCK_LOW;							//发送完后再拉低时钟
}


//从ADS7483获得数据
static u16	Touch_ReceiveData()
{
	u16 data = 0;
	u8 index;
	
	for(index = 0; index < 12; index ++)
	{
		data <<= 1;
		//delay(1);
		SCK_LOW;						//先低再高，模拟一个上升沿
		//delay(1);
		SCK_HIGH;
		data |= GET_IN;   	//在上升沿接收数据
	}
	SCK_LOW;
	
	return data;
}

/*
	获得坐标:
	此函数获得的坐标不对杂店进行处理
*/	

//static u8 Touch_get_exact_coordinate(PointType* point)
//{	
//	if(!ADS7483_IS_PRESSED())   //如果屏幕根本没按下则返回0
//		return 0;

//	//取4次,并取中间的2个的平均值 (这样做可以减少噪点)
//	//读取X,Y
//	Touch_SendCMD(CMD_X); 		 				//发送X命令
//	SCK_HIGH;												//经过一个高地脉冲数据在下一个上升沿被送出
//	SCK_LOW;
//	point->x = Touch_ReceiveData();  		//接收X
//	
//	Touch_SendCMD(CMD_Y);        		//发送Y命令
//	SCK_HIGH;
//	SCK_LOW;
//	point->y = Touch_ReceiveData();     	//接收Y
//	
//	return Touch_Conversion(point);
//}

/*
获取坐标:
	获得成功:返回N次采样中间两个的平均值(由SAMPLIE_TIME设置)
	获得失败:返回0
获得失败的原因有:
	触屏没有被按下
	SAMPLE_TIMES次连续采样距离过大(有噪点)
	采样点不在显示屏幕上

	只有在获得该函数的返回值为1时，才表明带进去的参数point获得了正确的坐标值
*/
#define SAMPLE_TIMES	4
static bool Touch_GetCoordinate(PointType* point)
{
	static u16 xs[SAMPLE_TIMES + 1];
	static u16 ys[SAMPLE_TIMES + 1];
	
	u8 index;
	u8 _index;
	
	if(!ADS7483_IS_PRESSED())   //如果屏幕根本没按下则返回0
	{
		return FALSE;
	}
	
	//取4次,并取中间的2个的平均值 (这样做可以减少噪点)
	//读取X,Y
	for(index = 1; index <= SAMPLE_TIMES; index ++)
	{
		Touch_SendCMD(CMD_X); 		 					//发送X命令
		SCK_HIGH;
		SCK_LOW;
		ys[index] = Touch_ReceiveData();  	//接收X
		
		Touch_SendCMD(CMD_Y);        				//发送Y命令
		SCK_HIGH;
		SCK_LOW;
		xs[index] = Touch_ReceiveData();     //接收Y
	}
	
	//将获得的4个值从小到大重新排序
	for(index = 1; index <= SAMPLE_TIMES; index ++)
	{
		for(_index = index + 1; index <= SAMPLE_TIMES; index ++)
		{
			u16 temp;
			if(ys[index] > ys[_index])
			{
				temp = ys[index];
				ys[index] = ys[_index];
				ys[_index] = temp;
			}
			
			if(xs[index] > xs[_index])
			{
				temp = xs[index];
				xs[index] = xs[_index];
				xs[_index] = temp;
			}
		}
	}
	
	//去除杂点(x,y都满足过滤条件才可以继续进行)
	if(!Touch_PointFilter(xs + 1, SAMPLE_TIMES) || !Touch_PointFilter(ys + 1, SAMPLE_TIMES))
	{
		return FALSE;
	}

	//计算出最后的结果(求中间2个数的平均值)
	point->x = (xs[2] + xs[3])>>1;	//除以2
	point->y = (ys[2] + ys[3])>>1;	//除以2
	
	return Touch_Conversion(point);
}


/*
	将ADS7483得到的AD值转换为以320*240为坐标的(x,y)值.
	如果AD值超出了屏幕的范围，point的内容将不变,并返回0
	否则将变为(x, y)，并返回1

	如果LCD的方向为240 * 320，此函数还会把 320 * 240的坐标变为240 * 320
*/
#define Y_START		260
#define Y_END			3700

#define X_START		180
#define X_END			3900

static bool Touch_Conversion(PointType* point)
{
	//此操作是从触屏和LCD之间的偏移测试出来，340和240是偏移量
	float x = point->x - point->x%10;
	float y = point->y - point->y%10;
	
	float x_temp;
	float y_temp;
	
	if(x <0 || y < 0)
		return FALSE;
	
	//Out_printf(0, 0, "%5d\t%5d", point->x, point->y);
	
	x_temp = (x - X_START)/(X_END - X_START);
	y_temp = (y - Y_START)/(Y_END - Y_START);
	
	point->x = SCREEN_WIDTH * x_temp;
	point->y = SCREEN_HEIGHT * y_temp;
	
	//Out_printf(0, 50, "%5d\t%5d", point->x, point->y);
	
	//Out_printf(0, 20, "%4.0f\t%4.0f\n%4d\t%4d\n%4.2f\t%4.2f",x, y, point->x, point->y, x_temp, y_temp);
	//	
	//Out_Draw_Point(point->x, point->y, 0xFFFF);
	
	return TRUE;
}


//过滤噪点函数，带入连续采样的点.
//该函数会在这些点不满足过滤条件时返回1否则返回0 
#define DELICACY 2					//灵敏度 越大越灵敏  越小越精确
static int Touch_PointFilter(u16 *start, u8 num)
{
	int maxSub = 0;
	int temp;
	
	while(num--)
	{
		if((temp = ABS(*(start+1) - (*start))) > maxSub)
			maxSub = temp;
	}
	return maxSub < DELICACY;
}

//*********************************更新屏幕按下状态的函数*********************************//
/*
	此函数在触摸和释放屏幕时调用来更新当前的触摸状态
	更新的内容有：
		屏幕是否被按下
		按下是否连续
			是否连续的检测方式为:
				如果在N下按下之间没有抬起，则连续按下状态被激活
				N最好取大于10(经过测试，N如果太小，连续状态会混乱)
					导致原因:因为有噪点过滤功能，有时候的触摸行为(在噪点的容纳范围外)并没有被识别为触摸，
					而在N次内如果都是噪点，则此函数会错误的将这N此触摸行为判断为连续的触摸操作，导致连续
					状态被激活。
					这可能会影响到LCD_ADS7483_interrupt_draw_line的工作。
*/
#define N 10				//连续采样到n次按下，才激活连续模式
extern s8 Touch_Update()
{
	static u8  preTouchState = 0;
	static int preTouchStateNum = 0;
	
	TouchState.isTouch = ADS7483_IS_PRESSED();   	//获取本次的按下状态
	
	//如果本次和上次都没有按下,直接返回
	if(preTouchState == 0 && TouchState.isTouch == 0)
	{
		TouchState.isFree = TRUE;
		return -1;
	}
	else 
		TouchState.isFree = FALSE;
	
	//本次没有按下
	if(TouchState.isTouch == 0)
	{
		preTouchStateNum = 0;
		TouchState.isHold = FALSE;
	}
	else
	{
		bool b;
		do
		{
			b = Touch_GetCoordinate(&TouchState.touchPoint);
		}
		while(b == FALSE && ADS7483_IS_PRESSED());
	}
	
	if(TouchState.isTouch && preTouchState)
		preTouchStateNum ++;
	
	if(preTouchStateNum >= N)
		TouchState.isHold = TRUE;
	
	preTouchState = TouchState.isTouch;
	
	return  TouchState.isTouch;              							//返回当前的按下状态
}


//**************************中断服务区**************************//
void interrupt_process()
{
	//如果想在触摸屏幕的时候发生一点事情，你可以把程序写在这里
	//Out_printf(2, 3, "%3d %3d", TouchState.touchPoint.x, TouchState.touchPoint.y);
}

