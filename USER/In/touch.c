#include <math.h>
#include "stm32f10x.h"
#include "com.h"
#include "gui.h"
#include "touch.h"

#define TOUCH_PORT	GPIOB      		//ADS7483��ʹ�õĶ˿�
#define SCK_PORT   	TOUCH_PORT    //ʱ�Ӷ˿�
#define SCK_PIN    	GPIO_Pin_13     //ʱ������
#define OUT_PORT  	TOUCH_PORT    //����˿�
#define OUT_PIN   	GPIO_Pin_15     //�������
#define IN_PORT  		TOUCH_PORT    //����˿�
#define IN_PIN   		GPIO_Pin_14     //��������
#define CS_PORT   	TOUCH_PORT    //Ƭѡ�˿�
#define CS_PIN    	GPIO_Pin_12     //Ƭѡ����
#define INT_PORT		GPIOG						//�ж϶˿�
#define INT_PIN			GPIO_Pin_7      //�ж�����
#define INT_LINE		GPIO_Pin_7      //�ж���

//ʱ�Ӹ�/��
#define SCK_HIGH   	GPIO_WriteBit(SCK_PORT, SCK_PIN, Bit_SET)
#define SCK_LOW    	GPIO_WriteBit(SCK_PORT, SCK_PIN, Bit_RESET)

//�����/��
#define OUT_HIGH	GPIO_WriteBit(OUT_PORT, OUT_PIN, Bit_SET)
#define OUT_LOW		GPIO_WriteBit(OUT_PORT, OUT_PIN, Bit_RESET)

//����������
#define GET_IN		GPIO_ReadInputDataBit(IN_PORT, IN_PIN)

//Ƭѡ����
#define CS_HIGH		GPIO_WriteBit(CS_PORT, CS_PIN, Bit_SET)
#define CS_LOW		GPIO_WriteBit(CS_PORT, CS_PIN, Bit_RESET)

//ADS7843 æ��־
#define TOUCH_BUSY_PORT			GPIOG
#define TOUCH_BUSY_PIN			GPIO_Pin_8
#define TOUCH_IS_BUSY()			(bool)GPIO_ReadInputDataBit(TOUCH_BUSY_PORT, TOUCH_BUSY_PIN)
	
#define CMD_X				0x90
#define CMD_Y				0xD0
#define ADS7483_IS_PRESSED()	(bool)(!GPIO_ReadInputDataBit(INT_PORT, INT_PIN))	//�����Ļ����״̬


static bool Touch_Conversion(PointType* point);
static int Touch_PointFilter(u16 *start, u8 num);

/*
	�˽ṹ��������ĸ����˵�ǰ��������״̬
	isTouched;		//�����Ƿ���ʱ����
	isHold;     	//�����Ƿ�һֱ����
	point;			//��ǰ�������ĵ�
	�����ڳ�ʼ���˴�����֮��,���κ�
	�ط���������ṹ������֪��������״̬
	�˽ṹ����ADS7483_update_state������
	ADS7483_update_state�ڴ����������»��ͷ�ʱ���ⲿ�ж�7�����ϱ�����
*/
static 	TouchStateType 	TouchState;

NVIC_InitTypeDef NVIC_InitStructure;


extern const TouchStateType* Touch_GetState()
{
	return &TouchState;
}




//�ⲿINT����
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
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	   				// �����ж��� Ϊ2 
	
	/* Enable the EXTI15_10_IRQn Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;				// �����ж���4
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // ����ռ�����ȼ�Ϊ2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  	// ���ø����ȼ�Ϊ0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  		// ʹ���ж���4
	NVIC_Init(&NVIC_InitStructure);	
}


void Touch_Sleep()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	   				// �����ж��� Ϊ2 
	
	/* Enable the EXTI15_10_IRQn Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;				// �����ж���4
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // ����ռ�����ȼ�Ϊ2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  	// ���ø����ȼ�Ϊ0
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;			  		// ʹ���ж���4
	NVIC_Init(&NVIC_InitStructure);	
}

extern void Touch_Awake()
{
	/* Enable and set EXTI0 Interrupt to the lowest priority */
  /* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	   				// �����ж��� Ϊ2 
	
	/* Enable the EXTI15_10_IRQn Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;				// �����ж���4
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // ����ռ�����ȼ�Ϊ2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  	// ���ø����ȼ�Ϊ0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  		// ʹ���ж���4
	NVIC_Init(&NVIC_InitStructure);	
}

static void delay(u32 s)
{
	while(s--);
}

void Touch_Init()
{
	Touch_PortInit(); 			//�˿ڳ�ʼ��
	CS_HIGH;					//Ƭѡ����
	delay(20);
	SCK_LOW;					//ʱ��Ϊ��
	delay(100);
	CS_LOW;           //Ƭѡ����
	delay(100);
}

//��ADS7483��������
static void Touch_SendCMD(u8 cmd)
{
	u8 index = 0;
	
	for(index = 0; index < 8; index ++)
	{
		(cmd & 0x80)?OUT_HIGH:OUT_LOW;	//MSB first, ��������λ���߻�����OUT
		//delay(1);
		SCK_LOW;                        //����
		//delay(1);
		SCK_HIGH;						//����  ����һ��������
		cmd <<= 1;
	}
	SCK_LOW;							//�������������ʱ��
}


//��ADS7483�������
static u16	Touch_ReceiveData()
{
	u16 data = 0;
	u8 index;
	
	for(index = 0; index < 12; index ++)
	{
		data <<= 1;
		//delay(1);
		SCK_LOW;						//�ȵ��ٸߣ�ģ��һ��������
		//delay(1);
		SCK_HIGH;
		data |= GET_IN;   	//�������ؽ�������
	}
	SCK_LOW;
	
	return data;
}

/*
	�������:
	�˺�����õ����겻���ӵ���д���
*/	

//static u8 Touch_get_exact_coordinate(PointType* point)
//{	
//	if(!ADS7483_IS_PRESSED())   //�����Ļ����û�����򷵻�0
//		return 0;

//	//ȡ4��,��ȡ�м��2����ƽ��ֵ (���������Լ������)
//	//��ȡX,Y
//	Touch_SendCMD(CMD_X); 		 				//����X����
//	SCK_HIGH;												//����һ���ߵ�������������һ�������ر��ͳ�
//	SCK_LOW;
//	point->x = Touch_ReceiveData();  		//����X
//	
//	Touch_SendCMD(CMD_Y);        		//����Y����
//	SCK_HIGH;
//	SCK_LOW;
//	point->y = Touch_ReceiveData();     	//����Y
//	
//	return Touch_Conversion(point);
//}

/*
��ȡ����:
	��óɹ�:����N�β����м�������ƽ��ֵ(��SAMPLIE_TIME����)
	���ʧ��:����0
���ʧ�ܵ�ԭ����:
	����û�б�����
	SAMPLE_TIMES�����������������(�����)
	�����㲻����ʾ��Ļ��

	ֻ���ڻ�øú����ķ���ֵΪ1ʱ���ű�������ȥ�Ĳ���point�������ȷ������ֵ
*/
#define SAMPLE_TIMES	4
static bool Touch_GetCoordinate(PointType* point)
{
	static u16 xs[SAMPLE_TIMES + 1];
	static u16 ys[SAMPLE_TIMES + 1];
	
	u8 index;
	u8 _index;
	
	if(!ADS7483_IS_PRESSED())   //�����Ļ����û�����򷵻�0
	{
		return FALSE;
	}
	
	//ȡ4��,��ȡ�м��2����ƽ��ֵ (���������Լ������)
	//��ȡX,Y
	for(index = 1; index <= SAMPLE_TIMES; index ++)
	{
		Touch_SendCMD(CMD_X); 		 					//����X����
		SCK_HIGH;
		SCK_LOW;
		ys[index] = Touch_ReceiveData();  	//����X
		
		Touch_SendCMD(CMD_Y);        				//����Y����
		SCK_HIGH;
		SCK_LOW;
		xs[index] = Touch_ReceiveData();     //����Y
	}
	
	//����õ�4��ֵ��С������������
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
	
	//ȥ���ӵ�(x,y��������������ſ��Լ�������)
	if(!Touch_PointFilter(xs + 1, SAMPLE_TIMES) || !Touch_PointFilter(ys + 1, SAMPLE_TIMES))
	{
		return FALSE;
	}

	//��������Ľ��(���м�2������ƽ��ֵ)
	point->x = (xs[2] + xs[3])>>1;	//����2
	point->y = (ys[2] + ys[3])>>1;	//����2
	
	return Touch_Conversion(point);
}


/*
	��ADS7483�õ���ADֵת��Ϊ��320*240Ϊ�����(x,y)ֵ.
	���ADֵ��������Ļ�ķ�Χ��point�����ݽ�����,������0
	���򽫱�Ϊ(x, y)��������1

	���LCD�ķ���Ϊ240 * 320���˺�������� 320 * 240�������Ϊ240 * 320
*/
#define Y_START		260
#define Y_END			3700

#define X_START		180
#define X_END			3900

static bool Touch_Conversion(PointType* point)
{
	//�˲����ǴӴ�����LCD֮���ƫ�Ʋ��Գ�����340��240��ƫ����
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


//������㺯�����������������ĵ�.
//�ú���������Щ�㲻�����������ʱ����1���򷵻�0 
#define DELICACY 2					//������ Խ��Խ����  ԽСԽ��ȷ
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

//*********************************������Ļ����״̬�ĺ���*********************************//
/*
	�˺����ڴ������ͷ���Ļʱ���������µ�ǰ�Ĵ���״̬
	���µ������У�
		��Ļ�Ƿ񱻰���
		�����Ƿ�����
			�Ƿ������ļ�ⷽʽΪ:
				�����N�°���֮��û��̧������������״̬������
				N���ȡ����10(�������ԣ�N���̫С������״̬�����)
					����ԭ��:��Ϊ�������˹��ܣ���ʱ��Ĵ�����Ϊ(���������ɷ�Χ��)��û�б�ʶ��Ϊ������
					����N�������������㣬��˺��������Ľ���N�˴�����Ϊ�ж�Ϊ�����Ĵ�����������������
					״̬�����
					����ܻ�Ӱ�쵽LCD_ADS7483_interrupt_draw_line�Ĺ�����
*/
#define N 10				//����������n�ΰ��£��ż�������ģʽ
extern s8 Touch_Update()
{
	static u8  preTouchState = 0;
	static int preTouchStateNum = 0;
	
	TouchState.isTouch = ADS7483_IS_PRESSED();   	//��ȡ���εİ���״̬
	
	//������κ��ϴζ�û�а���,ֱ�ӷ���
	if(preTouchState == 0 && TouchState.isTouch == 0)
	{
		TouchState.isFree = TRUE;
		return -1;
	}
	else 
		TouchState.isFree = FALSE;
	
	//����û�а���
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
	
	return  TouchState.isTouch;              							//���ص�ǰ�İ���״̬
}


//**************************�жϷ�����**************************//
void interrupt_process()
{
	//������ڴ�����Ļ��ʱ����һ�����飬����԰ѳ���д������
	//Out_printf(2, 3, "%3d %3d", TouchState.touchPoint.x, TouchState.touchPoint.y);
}

