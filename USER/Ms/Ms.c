
#include <stdio.h>
#include <math.h>

#include "stm32f10x.h"
#include "GUI.h"
#include "ms.h"
#include "fft.h"

/***********************************************************************
	This Source file contains all function the measure block use.
************************************************************************/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
Canvas WaveFormCanvas;
Canvas WaveFormCanvass[3];
Canvas PhaseDifCanvas;

Button BtnChannelA;
Button BtnChannelB;
Button BtnChannelC;
Button BtnChannelAll;
Button BtnShowPhaseDir;
Button BtnCommunicationWay;
Button BtnDebug;

ADC_InitTypeDef           ADC_InitStructure;
DMA_InitTypeDef           DMA_InitStructure;
TIM_TimeBaseInitTypeDef   TIM_TimeBaseStructure;
TIM_OCInitTypeDef         TIM_OCInitStructure;

__IO u32 DataVerifyFlag = 0; 			//0 wait 1:success 2 fail

bool MasterOnLine = FALSE;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static void MS_InitADC1(void);
static void MS_Send3Packages(void);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ADC1_DR_Address    ((uint32_t)0x4001244C)

/* Private macro -------------------------------------------------------------*/
/* Public variables ---------------------------------------------------------*/
u32  CacheCount = 0;
__IO uint16_t 							ADC_RegularConvertedValueTab[DMA_SAMPLE_AMOUNT];

float 											FFTCache[3][2048];
WaveInfoType 								WavesInfo[3];
ComplexNumberType						Components[3];
float 											PhaseDifference[3];	//0: AB  1. AC  3.BC

#define PHASE_DIF_AB				0
#define PHASE_DIF_AC				1
#define PHASE_DIF_BC				2

#define ZERO_SEQUENCE				2
#define NEGTIVE_SEQUENCE			0
#define POSITIVE_SEQUENCE			1


u32									PreCacheNow = 0;
u32									CacheNow = 0;							// = 0, 1, 2
u32									NextCacheNow = 0;					//the next channel updated by button

u32									PreCacheAllFlag = 0;
u32									CacheAllFlag = 1;
u32									NextCacheAllFlag = 1;

u32									PrePhaseDifFlag = 0;
u32									PhaseDifFlag = 0;
u32									NextPhaseDifFlag = 0;

u32									SamplePoint = 1000;

u32									DebugMode = 0;
u32									NextDebugMode = 0;

bool 								UseWireless = TRUE;
bool								NextUseWireless = TRUE;

#define PRINT_STATE(s)			Out_printft(Ascii_6x12, STATE_COLOR, Black, STATE_POS_X, STATE_POS_Y, "%6s", s);
#define WAVEFORM_TITLE_POS_Y 	(WaveFormCanvass[index]->rect.yStart - WAVEFORM_DISPLAY_VSPACE + 4)

/**
  *@brief Configure the sample duty 
	*@param	sampleTime = t * 10us
  */
void MS_Config_SapTime(u16 t)
{
	
	TIM_SetAutoreload(TIM1, t);
}


/************************************************************************/
/*---------------------------Event Handler------------------------------*/
/************************************************************************/
void MS_Go()
{
	DMA_Cmd(DMA1_Channel1, ENABLE);

  TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

void MS_Stop()
{

  TIM_CtrlPWMOutputs(TIM1, DISABLE);
		DMA_Cmd(DMA1_Channel1, DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel1, DMA_SAMPLE_AMOUNT);
	CacheCount = 0;
}


void WaveformCanvas_EnterHandler(Canvas canvas, PointType *point)
{
	static PointType Points[2];
	static PointType PointStart, PointEnd;
	
	MS_Stop();
	
	Canvas_Draw_Line(canvas, Points, Points + 1, canvas->backColor);
	
	PointStart.x = Points->x - 1;
	PointStart.y = WaveFormCanvas->rect.Height - (s32)((FFTCache[CacheNow][Points->x - 1] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
	PointEnd.x = Points->x;
	PointEnd.y = WaveFormCanvas->rect.Height - (s32)((FFTCache[CacheNow][Points->x] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
	
	Canvas_Draw_Line(WaveFormCanvas, &PointStart, &PointEnd,  MS_WAVE_COLOR);	
	
	PointStart.x = Points->x;
	PointStart.y = WaveFormCanvas->rect.Height - (s32)((FFTCache[CacheNow][Points->x] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
	PointEnd.x = Points->x + 1;
	PointEnd.y = WaveFormCanvas->rect.Height - (s32)((FFTCache[CacheNow][Points->x + 1] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
	
	Canvas_Draw_Line(WaveFormCanvas, &PointStart, &PointEnd,  MS_WAVE_COLOR);	
	
	Out_printft(Ascii_6x12, Black, Black, Points->x + canvas->rect.xStart + ((Points->x + canvas->rect.xStart) > (canvas->rect.xStart + canvas->rect.Width/2) ? - 6 * Out_Get_FontWidth() : 0 ), canvas->rect.yStart + canvas->rect.Height, "%4.2fv", FFTCache[CacheNow][Points->x + 1] + 1.65);
	
	PointStart.x = 0;
	PointStart.y = canvas->rect.Height/2;
	PointEnd.x = canvas->rect.Width;
	PointEnd.y = PointStart.y;
	
	Canvas_Draw_Line(canvas, &PointStart, &PointEnd, MS_MIDDLE_COLOR);
	
	Points[0].x = point->x;
	Points[1].x = point->x;
	Points[0].y = 0;
	Points[1].y = canvas->rect.Height;
	
	Canvas_Draw_Line(canvas, Points, Points + 1, SAMPLE_LINE_COLOR);
	
	Out_printft(Ascii_6x12, SAMPLE_LINE_COLOR, Black, Points->x + canvas->rect.xStart + ((Points->x + canvas->rect.xStart) > (canvas->rect.xStart + canvas->rect.Width/2) ? - 6 * Out_Get_FontWidth() : 0 ), canvas->rect.yStart + canvas->rect.Height, "%.2fv", FFTCache[CacheNow][Points->x + 1] + 1.65);
	
	SamplePoint = Points->x;
}

void WaveformCanvas_LeaveHandler(Canvas canvas, PointType *point)
{
	MS_Go();
}

void BtnChannelA_ClickHandler(Button self, PointType *p)
{
	NextCacheAllFlag = 0;
	NextCacheNow = 0;
	NextPhaseDifFlag = 0;
	PRINT_STATE("Wait");
}

void BtnChannelB_ClickHandler(Button self, PointType *p)
{
	NextCacheAllFlag = 0;
	NextPhaseDifFlag = 0;
	
	NextCacheNow = 1;
	PRINT_STATE("Wait");
}

void BtnChannelC_ClickHandler(Button self, PointType *p)
{
	NextCacheAllFlag = 0;
	NextPhaseDifFlag = 0;
	
	NextCacheNow = 2;
	PRINT_STATE("Wait");
}

void BtnChannelAll_ClickHandler(Button self, PointType *p)
{
	NextPhaseDifFlag = 0;
	NextCacheAllFlag = !CacheAllFlag;
	PRINT_STATE("Wait");
}

void BtnShowPhaseDif_ClickHandler(Button self, PointType *p)
{
	//NextCacheAllFlag = 0;
	NextPhaseDifFlag = !PhaseDifFlag;
	PRINT_STATE("Wait");
}

void BtnCommunicationWay_ClickHandler(Button self, PointType *p)
{
	NextUseWireless = UseWireless ? FALSE : TRUE;
	PRINT_STATE("Wait");
}

void BtnDebug_ClickHandler(Button self, PointType *p)
{
	NextDebugMode = !DebugMode;
}


static void MS_InitUI()
{
	CanvasDefType canvasDefStructure;
	RectType canvasRect = {WAVEFORM_DISPLAY_XSTART, WAVEFORM_DISPLAY_YSTART, WAVEFORM_DISPLAY_WIDTH, WAVEFORM_DISPLAY_HEIGHT};
	RectType canvasRectSmall = {SMALL_WAVEFORM_DISPLAY_XSTART, SMALL_WAVEFORM_DISPLAY_YSTART, SMALL_WAVEFORM_DISPLAY_WIDTH, SMALL_WAVEFORM_DISPLAY_HEIGHT};
	RectType btnRect = {WAVEFORM_DISPLAY_XSTART + WAVEFORM_DISPLAY_WIDTH + 15, BUTTON_Y_START + 5, 80, 15};
	ButtonDefType btnDef = Button_Get_ButtonDefTypeTemplate();
	
	canvasDefStructure.name = "WaveForm";
	canvasDefStructure.backColor = MS_BACK_COLOR;
	canvasDefStructure.rect = canvasRect;
	canvasDefStructure.EnterHandler = WaveformCanvas_EnterHandler;
	canvasDefStructure.LeaveHandler = WaveformCanvas_LeaveHandler;
	WaveFormCanvas = Canvas_Create(&canvasDefStructure);
	
	canvasDefStructure.name = "WaveFormA";
	canvasDefStructure.rect = canvasRectSmall;
	WaveFormCanvass[0] = Canvas_Create(&canvasDefStructure);
	
	canvasDefStructure.name = "WaveFormB";
	canvasDefStructure.rect.yStart += canvasDefStructure.rect.Height + SMALL_WAVEFORM_DISPLAY_VSPACE;
	WaveFormCanvass[1] = Canvas_Create(&canvasDefStructure);
	
	canvasDefStructure.name = "WaveFormC";
	canvasDefStructure.rect.yStart += canvasDefStructure.rect.Height + SMALL_WAVEFORM_DISPLAY_VSPACE;
	WaveFormCanvass[2] = Canvas_Create(&canvasDefStructure);
	
	canvasDefStructure.name = "PhaseDif";
	canvasDefStructure.rect.xStart = PHASE_DIF_CANVAS_XSTART;
	canvasDefStructure.rect.yStart = PHASE_DIF_CANVAS_YSTART;
	canvasDefStructure.rect.Width = PHASE_DIF_CANVAS_WIDTH;
	canvasDefStructure.rect.Height = PHASE_DIF_CANVAS_HEIGHT;
	canvasDefStructure.EnterHandler = 0;
	canvasDefStructure.LeaveHandler = 0;
	canvasDefStructure.backColor = PHASE_DIF_BACK_COLOR;
	PhaseDifCanvas = Canvas_Create(&canvasDefStructure);
	
	btnDef.text = "CH-A";
	btnDef.clickHandler = BtnChannelA_ClickHandler;
	btnDef.rect = btnRect;
	btnDef.upColor = BUTTON_BACK_COLOR;
	BtnChannelA = Button_Create(&btnDef);
	
	btnRect.yStart += btnRect.Height + 5;
	btnDef.text = "CH-B";
	btnDef.clickHandler = BtnChannelB_ClickHandler;
	btnDef.rect = btnRect;
	BtnChannelB = Button_Create(&btnDef);
	
	btnRect.yStart += btnRect.Height + 5;
	btnDef.text = "CH-C";
	btnDef.clickHandler = BtnChannelC_ClickHandler;
	btnDef.rect = btnRect;
	BtnChannelC = Button_Create(&btnDef);
	
	btnRect.yStart += btnRect.Height + 5;
	btnDef.text = "CH-All";
	btnDef.clickHandler = BtnChannelAll_ClickHandler;
	btnDef.rect = btnRect;
	BtnChannelAll = Button_Create(&btnDef);
	
	btnRect.yStart += btnRect.Height + 5;
	btnDef.text = "Phase";
	btnDef.clickHandler = BtnShowPhaseDif_ClickHandler;
	btnDef.rect = btnRect;
	BtnShowPhaseDir = Button_Create(&btnDef);
	
	btnRect.yStart += btnRect.Height + 5;
	btnDef.text = "Toggle";
	btnDef.clickHandler = BtnCommunicationWay_ClickHandler;
	btnDef.rect = btnRect;
	BtnCommunicationWay = Button_Create(&btnDef);
	
	btnRect.yStart += btnRect.Height + 10;
	btnDef.text = "Debug";
	btnDef.clickHandler = BtnDebug_ClickHandler;
	btnDef.rect = btnRect;
	BtnDebug = Button_Create(&btnDef);
}

static void MS_UpdateCommunicationWay()
{
	Out_printft(Ascii_5x8, COMMUNICAITON_WAY_COLOR, Black, COMMUNICATION_AREA_START_X + 10, SCREEN_HEIGHT - 15, "COM:%-9s", UseWireless ? "Wireless" : "Wire");
}

static void MS_DrawUI()
{
	PointType p1;
	PointType p2;
	
	Button_Draw(BtnChannelA);
	Button_Draw(BtnChannelB);
	Button_Draw(BtnChannelC);
	Button_Draw(BtnChannelAll);
	Button_Draw(BtnShowPhaseDir);
	Button_Draw(BtnCommunicationWay);
	Button_Draw(BtnDebug);
	
	Button_Register(BtnChannelA);
	Button_Register(BtnChannelB);
	Button_Register(BtnChannelC);
	Button_Register(BtnChannelAll);
	Button_Register(BtnShowPhaseDir);
	Button_Register(BtnCommunicationWay);
	Button_Register(BtnDebug);
	
	Out_Draw_Line(WAVEFORM_DISPLAY_XSTART + WAVEFORM_DISPLAY_WIDTH + 1, 10, WAVEFORM_DISPLAY_XSTART + WAVEFORM_DISPLAY_WIDTH + 1, SCREEN_HEIGHT - 10, Red);
	Out_Draw_Line(WAVEFORM_DISPLAY_XSTART + WAVEFORM_DISPLAY_WIDTH + 1, BUTTON_Y_START - 3, SCREEN_WIDTH - 5, BUTTON_Y_START - 3, Red);
	Out_Draw_Line(WAVEFORM_DISPLAY_XSTART + WAVEFORM_DISPLAY_WIDTH + 1, COMMUNICATION_AREA_START_Y - 3, SCREEN_WIDTH - 5, COMMUNICATION_AREA_START_Y - 3, Red);
	Out_Draw_Line(WAVEFORM_DISPLAY_XSTART + WAVEFORM_DISPLAY_WIDTH + 1, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 5, SCREEN_HEIGHT - 20, Red);
	
	Out_printft(Ascii_5x8, STATE_TITLE_COLOR, Black, STATE_POS_X + 2, STATE_POS_Y - 13, "Current:");
	
	MS_UpdateCommunicationWay();
	
	if(PhaseDifFlag)
	{
		Canvas_Register(PhaseDifCanvas);
		Canvas_Draw(PhaseDifCanvas);
	}
	else
	{
		if(CacheAllFlag)
		{
			u32 index;
			for(index = 0; index < 3; index ++)
			{
				p1.x = 0;
				p1.y = WaveFormCanvass[index]->rect.Height/2;
				p2.x = WaveFormCanvass[index]->rect.Width;
				p2.y = p1.y;
				
				Out_printft(Ascii_5x8, Yellow, Black, WaveFormCanvass[index]->rect.xStart - 20,WAVEFORM_TITLE_POS_Y, "CH%c:", 'A' + index);
				
				Out_printft(Ascii_5x8, MS_WAVE_INFO_COLOR, Black, 0, WaveFormCanvass[index]->rect.yStart, "3.3");
				Out_printft(Ascii_5x8, MS_WAVE_INFO_COLOR, Black, 0, WaveFormCanvass[index]->rect.yStart + WaveFormCanvass[index]->rect.Height/2 - 8, "CH%c:", 'A' + index);
				Out_printft(Ascii_5x8, MS_WAVE_INFO_COLOR, Black, 0, WaveFormCanvass[index]->rect.yStart + WaveFormCanvass[index]->rect.Height - 8, "0");
				
				Canvas_Draw(WaveFormCanvass[index]);
				Canvas_Draw_Line(WaveFormCanvass[index], &p1, &p2, MS_MIDDLE_COLOR);
			}
		}
		else
		{
			p1.x = 0;
			p1.y = WaveFormCanvas->rect.Height/2;
			
			p2.x = WaveFormCanvas->rect.Width;
			p2.y = p1.y;
			
			Out_printft(Ascii_5x8, MS_WAVE_INFO_COLOR, Black, 0,WaveFormCanvas->rect.yStart, "3.3");
			Out_printft(Ascii_5x8, MS_WAVE_INFO_COLOR, Black, 0, WaveFormCanvas->rect.yStart + WaveFormCanvas->rect.Height/2 - 8, "CH%c:", 'A' + CacheNow);
			Out_printft(Ascii_5x8, MS_WAVE_INFO_COLOR, Black, 0, WaveFormCanvas->rect.yStart + WaveFormCanvas->rect.Height - 8, "0");
				
			
			Canvas_Register(WaveFormCanvas);
			Canvas_Draw(WaveFormCanvas);
			
			Canvas_Draw_Line(WaveFormCanvas, &p1, &p2, MS_MIDDLE_COLOR);
		}
	}
}




static void MS_DedrawUI()
{
	RectType rect = {0, 0, WAVEFORM_DISPLAY_XSTART + WAVEFORM_DISPLAY_WIDTH + 1, SCREEN_HEIGHT};
	Canvas_Deregister(PhaseDifCanvas);
	
	Canvas_Deregister(WaveFormCanvas);

	Out_Draw_FRectangle(&rect, Black);
}

static void MS_DrawPhaseDif(float chaPha, float chbPha, float chcPha)
{
	static float chaPhat = 0;
	static float chbPhat = 0;
	static float chcPhat = 0;
	
	static PointType center;
	PointType endPoint;
	
	s32 length = MIN(PhaseDifCanvas->rect.Width/2, PhaseDifCanvas->rect.Height/2) - 10;
	
	center.x = PhaseDifCanvas->rect.Width/2 - 20;
	center.y = PhaseDifCanvas->rect.Height/2;
	
	endPoint.x = cos(-chaPhat/180*PI) * length + center.x + PhaseDifCanvas->rect.xStart;
	endPoint.y = sin(-chaPhat/180*PI) * length + center.y + PhaseDifCanvas->rect.yStart;
	
	Out_printft(Ascii_5x8, PhaseDifCanvas->backColor, PhaseDifCanvas->backColor, endPoint.x, endPoint.y, "CHA:%-3.1f", chaPhat);
	Canvas_Draw_DirLine(PhaseDifCanvas, chaPhat, center, length, PhaseDifCanvas->backColor);
	
	endPoint.x = cos(-chbPhat/180*PI) * length + center.x + PhaseDifCanvas->rect.xStart;
	endPoint.y = sin(-chbPhat/180*PI) * length + center.y + PhaseDifCanvas->rect.yStart;
	Out_printft(Ascii_5x8, PhaseDifCanvas->backColor, PhaseDifCanvas->backColor, endPoint.x, endPoint.y, "CHB:%-3.1f", chbPhat);
	Canvas_Draw_DirLine(PhaseDifCanvas, chbPhat, center, length, PhaseDifCanvas->backColor);
	
	endPoint.x = cos(-chcPhat/180*PI) * length + center.x + PhaseDifCanvas->rect.xStart;
	endPoint.y = sin(-chcPhat/180*PI) * length + center.y + PhaseDifCanvas->rect.yStart;
	Out_printft(Ascii_5x8, PhaseDifCanvas->backColor, PhaseDifCanvas->backColor, endPoint.x, endPoint.y, "CHC:%-3.1f", chcPhat);
	Canvas_Draw_DirLine(PhaseDifCanvas, chcPhat, center, length, PhaseDifCanvas->backColor);
	
	chaPhat = (s32)(chaPha)%360;
	chbPhat = (s32)(chbPha)%360;
	chcPhat = (s32)(chcPha)%360;
	
	endPoint.x = cos(-chaPhat/180*PI) * length + center.x + PhaseDifCanvas->rect.xStart;
	endPoint.y = sin(-chaPhat/180*PI) * length + center.y + PhaseDifCanvas->rect.yStart;
	Out_printft(Ascii_5x8, PHASE_DIF_LINE_COLOR, PhaseDifCanvas->backColor, endPoint.x, endPoint.y, "CHA:%-3.1f", chaPhat);
	Canvas_Draw_DirLine(PhaseDifCanvas, chaPhat, center, length, PHASE_DIF_LINE_COLOR);
	
	endPoint.x = cos(-chbPhat/180*PI) * length + center.x + PhaseDifCanvas->rect.xStart;
	endPoint.y = sin(-chbPhat/180*PI) * length + center.y + PhaseDifCanvas->rect.yStart;
	Out_printft(Ascii_5x8, PHASE_DIF_LINE_COLOR, PhaseDifCanvas->backColor, endPoint.x, endPoint.y, "CHB:%-3.1f", chbPhat);
	Canvas_Draw_DirLine(PhaseDifCanvas, chbPhat, center, length, PHASE_DIF_LINE_COLOR);
	
	endPoint.x = cos(-chcPhat/180*PI) * length + center.x + PhaseDifCanvas->rect.xStart;
	endPoint.y = sin(-chcPhat/180*PI) * length + center.y + PhaseDifCanvas->rect.yStart;
	Out_printft(Ascii_5x8, PHASE_DIF_LINE_COLOR, PhaseDifCanvas->backColor, endPoint.x, endPoint.y, "CHC:%-3.1f", chcPhat);
	Canvas_Draw_DirLine(PhaseDifCanvas, chcPhat, center, length, PHASE_DIF_LINE_COLOR);
	
	Canvas_Draw_Circle(PhaseDifCanvas, center, 3, PHASE_DIF_LINE_COLOR);
	Canvas_Draw_Circle(PhaseDifCanvas, center, 2, PHASE_DIF_LINE_COLOR);
	Canvas_Draw_Circle(PhaseDifCanvas, center, 1, PHASE_DIF_LINE_COLOR);
	
	Out_printft(Ascii_6x12, PHASE_DIF_TITLE_COLOR, Black, PHASE_DIF_INFORM_XSTART, 		PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + 5, 
	"Phase Difference:");
	
	Out_printft(Ascii_5x8, PHASE_DIF_INFORM_COLOR, Black, PHASE_DIF_INFORM_XSTART + 5, 	PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + 5 + Out_Get_FontHeight() * 1 + 3, 
	"PdAB:%-4.0f PdAC:%-4.0f PdBC:%-4.0f", PhaseDifference[PHASE_DIF_AB], PhaseDifference[PHASE_DIF_AC], PhaseDifference[PHASE_DIF_BC]);
	
	Out_printft(Ascii_6x12, PHASE_DIF_TITLE_COLOR, Black, PHASE_DIF_INFORM_XSTART, 		PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + Out_Get_FontHeight() * 2 + 10, 
	"Components:");
	
	Out_printft(Ascii_5x8, PHASE_DIF_SUBTITLE_COLOR, Black, PHASE_DIF_INFORM_XSTART + 5, PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + Out_Get_FontHeight() * 2 + 10 + Out_Get_FontHeight() * 1 + 3, 
	"Zero    -Seq:");
	Out_printft(Ascii_5x8, PHASE_DIF_INFORM_COLOR, Black, PHASE_DIF_INFORM_XSTART + 5, 	PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + Out_Get_FontHeight() * 2 + 10 + Out_Get_FontHeight() * 1 + 3, 
	"\t\t\t\t\t\t\t {%-10.6f, %6.2f}", Components[ZERO_SEQUENCE].Module, Components[ZERO_SEQUENCE].Phase);
	
	Out_printft(Ascii_5x8, PHASE_DIF_SUBTITLE_COLOR, Black, PHASE_DIF_INFORM_XSTART + 5, 	PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + Out_Get_FontHeight() * 2 + 10 + Out_Get_FontHeight() * 2 + 3,
	"Negtive -Seq:");
	Out_printft(Ascii_5x8, PHASE_DIF_INFORM_COLOR, Black, PHASE_DIF_INFORM_XSTART + 5, 		PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + Out_Get_FontHeight() * 2 + 10 + Out_Get_FontHeight() * 2 + 3,
	"\t\t\t\t\t\t\t {%-10.6f, %6.2f}", Components[NEGTIVE_SEQUENCE].Module, Components[NEGTIVE_SEQUENCE].Phase);
	
	Out_printft(Ascii_5x8, PHASE_DIF_SUBTITLE_COLOR, Black, PHASE_DIF_INFORM_XSTART + 5, 	PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + Out_Get_FontHeight() * 2 + 10 + Out_Get_FontHeight() * 3 + 3,
	"Positive-Seq:");
	Out_printft(Ascii_5x8, PHASE_DIF_INFORM_COLOR, Black, PHASE_DIF_INFORM_XSTART + 5, 		PhaseDifCanvas->rect.yStart + PhaseDifCanvas->rect.Height + Out_Get_FontHeight() * 2 + 10 + Out_Get_FontHeight() * 3 + 3,
	"\t\t\t\t\t\t\t {%-10.6f, %6.2f}", Components[POSITIVE_SEQUENCE].Module, Components[POSITIVE_SEQUENCE].Phase);
}


PointType PointStart;
PointType PointEnd;

void MS_UpdateWaveform(bool clear)
{
	if (PhaseDifFlag)
		return;
	
	if (clear && CacheCount < WaveFormCanvas->rect.Width - 1)
	{
		if(CacheAllFlag && PreCacheAllFlag)
		{
			u32 index = 0;
			Canvas pWaveCanvas;
			for (index = 0; index < 3; index ++)
			{
				pWaveCanvas = WaveFormCanvass[index];
				
				PointStart.x = CacheCount;
				PointStart.y =  pWaveCanvas->rect.Height - (s32)((FFTCache[index][CacheCount] + 1.65)/3.3 * pWaveCanvas->rect.Height);
				
				PointEnd.x = CacheCount + 1;
				PointEnd.y = pWaveCanvas->rect.Height - (s32)((FFTCache[index][CacheCount + 1] + 1.65)/3.3 * pWaveCanvas->rect.Height);
				
				Canvas_Draw_Line(pWaveCanvas, &PointStart, &PointEnd,  pWaveCanvas->backColor);
			}
		}
		else if (CacheAllFlag == 0)
		{
			PointStart.x = CacheCount;
			PointStart.y =  WaveFormCanvas->rect.Height - (s32)((FFTCache[PreCacheNow][CacheCount] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
			
			PointEnd.x = CacheCount + 1;
			PointEnd.y = WaveFormCanvas->rect.Height - (s32)((FFTCache[PreCacheNow][CacheCount + 1] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
			
			Canvas_Draw_Line(WaveFormCanvas, &PointStart, &PointEnd,  WaveFormCanvas->backColor);
		}
	}
	else if (!clear && CacheCount > 1)
	{
		if (CacheAllFlag)
		{
			u32 index = 0;
			for(index = 0; index < 3; index ++)
			{
				Canvas pWaveCanvas = WaveFormCanvass[index];
				PointStart.x = CacheCount - 1;
				PointStart.y =  pWaveCanvas->rect.Height - (s32)((FFTCache[index][CacheCount - 1] + 1.65)/3.3 * pWaveCanvas->rect.Height);
				
				PointEnd.x = CacheCount;
				PointEnd.y = pWaveCanvas->rect.Height - (s32)((FFTCache[index][CacheCount] + 1.65)/3.3 * pWaveCanvas->rect.Height);
				
				Canvas_Draw_Line(pWaveCanvas, &PointStart, &PointEnd,  MS_WAVE_COLOR);
			}
		}
		else
		{
			PointStart.x = CacheCount - 1;
			PointStart.y = WaveFormCanvas->rect.Height - (s32)((FFTCache[CacheNow][CacheCount - 1] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
			PointEnd.x = CacheCount;
			PointEnd.y = WaveFormCanvas->rect.Height - (s32)((FFTCache[CacheNow][CacheCount] + 1.65)/3.3 * WaveFormCanvas->rect.Height);
			
			Canvas_Draw_Line(WaveFormCanvas, &PointStart, &PointEnd,  MS_WAVE_COLOR);	
			
			if (SamplePoint == CacheCount)
			{
				PointType Points[2];
				Points[0].x = SamplePoint;
				Points[1].x = SamplePoint;
				Points[0].y = 0;
				Points[1].y = WaveFormCanvas->rect.Height;
				Out_printft(Ascii_6x12, SAMPLE_LINE_COLOR, Black, 
							Points->x + WaveFormCanvas->rect.xStart + ((Points->x + WaveFormCanvas->rect.xStart) > (WaveFormCanvas->rect.xStart + WaveFormCanvas->rect.Width/2) ? - 6 * Out_Get_FontWidth() : 0 ), 
							WaveFormCanvas->rect.yStart + WaveFormCanvas->rect.Height, "%.2fv", FFTCache[CacheNow][Points->x + 1] + 1.65);
				Canvas_Draw_Line(WaveFormCanvas, Points, Points + 1, SAMPLE_LINE_COLOR);
			}
		}
	}
}


void MS_Init()
{
	GPIO_InitTypeDef GPIO_InitDefStructure;
	GPIO_InitDefStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitDefStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitDefStructure.GPIO_Speed = GPIO_Speed_50MHz;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_Init(GPIOC, &GPIO_InitDefStructure);
	
	MS_InitUI();
	MS_DrawUI();
	MS_InitADC1();
}


void MS_FrameEndHandler()
{
	PointType p1, p2;
	u32 index;
	bool dedrawUIFlag = FALSE;
	
	for(index = 0; index < 3; index ++)
	{
		FFT_Calculate(FFTCache[index], FFT_N, WavesInfo + index);
	}
	
	for(index = 0; index < 3; index ++)
	{
		WavesInfo[index].PhasePosition -= WavesInfo[0].PhasePosition;
		
		if(WavesInfo[index].PhasePosition < 0)
			WavesInfo[index].PhasePosition += 360.0;
		
	}
	
	PhaseDifference[PHASE_DIF_AB] = WavesInfo[0].PhasePosition - WavesInfo[1].PhasePosition;
	PhaseDifference[PHASE_DIF_AC] = WavesInfo[0].PhasePosition - WavesInfo[2].PhasePosition;
	PhaseDifference[PHASE_DIF_BC] = WavesInfo[1].PhasePosition - WavesInfo[2].PhasePosition;
	
	FFT_Calculate_NegComponents(WavesInfo, Components);
	
	if(DebugMode != NextDebugMode)
	{
		DebugMode = NextDebugMode;
		Out_printft(Ascii_6x12, Red, Black, STATE_POS_X, STATE_POS_Y + 12, "%6s", DebugMode ? "Debug" : "Normal");
	}
	
	if(NextPhaseDifFlag != PhaseDifFlag)
	{
		dedrawUIFlag = TRUE;
		PhaseDifFlag = NextPhaseDifFlag;
	}
	
	PreCacheAllFlag = CacheAllFlag;
	if(CacheAllFlag  !=  NextCacheAllFlag)
	{
		dedrawUIFlag = TRUE;
		CacheAllFlag = NextCacheAllFlag;
	}
	
	PreCacheNow = CacheNow;
	if(NextCacheNow != CacheNow)
	{
		dedrawUIFlag = TRUE;
		CacheNow = NextCacheNow;
	}
	
	if(dedrawUIFlag)
	{
		MS_DedrawUI();
		MS_DrawUI();
	}
	
	if(NextUseWireless != UseWireless)
	{
		UseWireless = NextUseWireless;
		MS_UpdateCommunicationWay();
	}
	
	PrePhaseDifFlag = PhaseDifFlag;
	PRINT_STATE(PhaseDifFlag ? "PHAS" : CacheAllFlag ? "AllCh" : (CacheNow == 0 ? "CHA" : (CacheNow == 1) ? "CHB" : "CHC"));
	
	
	
	
	
//	for(index = 0; index < 3; index ++)
//	{
//		printf("CH%c:DC:%.2f\tFRQ:0%.2fHz\tVirV:%f\tPhasePos:%.2f\tValue:%f\n\r", 'A' + index, 
//				WavesInfo[index].Dc, WavesInfo[index].BaseFrequency, WavesInfo[index].Rms, WavesInfo[index].PhasePosition, WavesInfo[index].Rms);
//	}
	

	
	if(PhaseDifFlag)
	{
		MS_DrawPhaseDif(WavesInfo[0].PhasePosition, WavesInfo[1].PhasePosition, WavesInfo[2].PhasePosition);
	}
	else
	{
		if(CacheAllFlag)
		{
			u32 index;
			for(index = 0; index < 3; index ++)
			{
				Out_printft(Ascii_5x8, Yellow, Black, WaveFormCanvass[index]->rect.xStart - 20, WAVEFORM_TITLE_POS_Y, 
				"CH%c:\tDC:%-4.1f\tFRQ:%-4.1fHz\tRMS:%-3.1f\n\t\t\tAMP:%3.1f\tPhas:%-7.1f",
				'A' + index, WavesInfo[index].Dc, WavesInfo[index].BaseFrequency,  WavesInfo[index].Rms, WavesInfo[index].Amplitude, WavesInfo[index].PhasePosition);
				
				p1.x = 0;
				p1.y = WaveFormCanvass[index]->rect.Height/2;
				p2.x = WaveFormCanvass[index]->rect.Width;
				p2.y = p1.y;
				
				Canvas_Draw_Line(WaveFormCanvass[index], &p1, &p2, MS_MIDDLE_COLOR);
			}
		}
		else
		{
			Out_printft(Ascii_6x12 , Yellow, Black, 0,  WaveFormCanvas->rect.yStart + WaveFormCanvas->rect.Height + 20, 
			"\tPhas:\t%-7.1f\t\t\tFRQ:\t%-7.3fHz\n\tDC:\t\t%f\t\tAMP:\t%f\n\tRMS:\t%f",
			WavesInfo[CacheNow].PhasePosition, WavesInfo[CacheNow].BaseFrequency, WavesInfo[CacheNow].Dc,  WavesInfo[CacheNow].Amplitude, WavesInfo[CacheNow].Rms);
			
			p1.x = 0;
			p1.y = WaveFormCanvas->rect.Height/2;
			p2.x = WaveFormCanvas->rect.Width;
			p2.y = p1.y;
			
			Canvas_Draw_Line(WaveFormCanvas, &p1, &p2, MS_MIDDLE_COLOR);
			FFT_Calculate(FFTCache[CacheNow], FFT_N, WavesInfo + CacheNow);
		}
	}
	
	MS_Send3Packages();
}

#define CMD_MASTER_IS_ONLINE 								0x80
#define CMD_SLAVE_ONLINE_WITH_DATA_READY					0x81
#define CMD_SLAVE_ONLINE_WITHOUT_DATA_READY					0x82
#define CMD_SLAVE_PACKAGE_BEGIN								0x83
#define CMD_SLAVE_PACKAGE_END								0x84
#define CMD_MASTER_DATA_VERIFY_SUCCESS						0x85
#define CMD_MASTER_DATA_VERIFY_FAIL							0x86
#define CMD_MASTER_RECEIVE_ACCEPT							0x87
#define CDM_MASTER_RECEIVE_REFUSE							0x88

#define TYPE_BASE_FREQUENCY									0xE3
#define TYPE_VIRTUAL_VALUE									0xE4
#define TYPE_PHASE_POSITION									0xE5
#define TYPE_NEGTIVE_SEQUENCE_COMPONENT						0xE6
#define TYPE_ADC_BEGIN										0xE7
#define TYPE_ADC_END										0xE8

#define TYPE_PACKAGE_CHA									0xF1									
#define TYPE_PACKAGE_CHB									0xF2
#define TYPE_PACKAGE_CHC									0xF3

#define PACKAGE_SIZE	272
			
__inline void MS_SendByte(u8 byte)
{
	if(UseWireless)
		Com_USART2_Putc(byte); 
	else 
		Com_USART1_Putc(byte);
}

/*
 * 发送一个放大了10000倍的float，ARM默认是小端模式
 */
void MS_SendFloat(float f)
{
	volatile int i = (int)(f * 100000);
	volatile unsigned char* pi = (unsigned char *)&i;
	int index;
	
	for(index = 0; index < 4; index ++)
	{
		MS_SendByte(*pi++);
	}
}

bool MS_SendPackage(u32 ch)
{
	u32 time = 10000000;
	u32 index;

	MS_SendByte(CMD_SLAVE_PACKAGE_BEGIN);			//Package Start
	MS_SendByte(PACKAGE_SIZE&0xFF);						//Package Size Lower 
	MS_SendByte((PACKAGE_SIZE >> 8)& 0xFF);		//Package Size Upper
	
	//Send Package Type
	switch(ch)
	{
		case 0:
			MS_SendByte(TYPE_PACKAGE_CHA);
			break;
		case 1:
			MS_SendByte(TYPE_PACKAGE_CHB);
			break;
		case 2:
			MS_SendByte(TYPE_PACKAGE_CHC);
			break;
		default:
			return FALSE;
	}
	
	MS_SendByte(TYPE_BASE_FREQUENCY);
	MS_SendFloat(WavesInfo[ch].BaseFrequency);
	MS_SendByte(TYPE_VIRTUAL_VALUE);
	MS_SendFloat(WavesInfo[ch].Rms);
	MS_SendByte(TYPE_PHASE_POSITION);
	MS_SendFloat(PhaseDifference[ch]);
	MS_SendByte(TYPE_NEGTIVE_SEQUENCE_COMPONENT);

	if(ch == 0)
	{
		MS_SendFloat(Components[ZERO_SEQUENCE].Module);
	}
	else if(ch == 1)
	{
		MS_SendFloat(Components[NEGTIVE_SEQUENCE].Module);
	}
	else if(ch == 2)
	{
		MS_SendFloat(Components[POSITIVE_SEQUENCE].Module);
	}
	
	MS_SendByte(TYPE_ADC_BEGIN);
	
	for(index = 0; index < 1024; index ++)
	{
		MS_SendFloat(FFTCache[ch][index]);
	}
	
	MS_SendByte(TYPE_ADC_END);	
	DataVerifyFlag = 0;
	MS_SendByte(CMD_SLAVE_PACKAGE_END);
	while(DataVerifyFlag == 0 && time --);
	
	return (bool)(DataVerifyFlag == 1);
}

void MS_ShowComState(const char *msg, u16 color)
{
	Out_printft(Ascii_5x8, color, Black, COMMUNICATION_AREA_START_X, COMMUNICATION_AREA_START_Y, "%6s", msg);
}

void MS_ReceivedFromMasterEvent(unsigned char rData)
{
	MS_Stop();
	
	if(MasterOnLine)
	{
		if(DataVerifyFlag == 0)
		{
			if(rData == CMD_MASTER_DATA_VERIFY_SUCCESS)
			{
				DataVerifyFlag = 1;
			}
			
			if(rData == CMD_MASTER_DATA_VERIFY_FAIL)
			{
				DataVerifyFlag = 2;
			}
		}
		else
		{
			//undefine
		}
	}
	
	if(rData == CMD_MASTER_IS_ONLINE)
	{
		MasterOnLine = TRUE;
		MS_ShowComState("Mst:Online ", Green);
		MS_SendByte(CMD_SLAVE_ONLINE_WITH_DATA_READY);
	}
	
	MS_Go();
}


void MS_Send3Packages()
{
	int index = 3;
	int tryTimes;
	
	if(MasterOnLine == FALSE)
	{
		MS_ShowComState("Mst:Offline", Red);
		return;
	}
	
	for(index = 0, tryTimes = 1; index < 3 && tryTimes <= 3; tryTimes ++)
	{
		Out_printft(Ascii_5x8, Yellow, Black, COMMUNICATION_AREA_START_X, 
		COMMUNICATION_AREA_START_Y + 10, "Try:CH%d,%d   ", index + 1, tryTimes);
		Out_printft(Ascii_5x8, Green, Black, COMMUNICATION_AREA_START_X , COMMUNICATION_AREA_START_Y + 20 , "Wait... ", index, tryTimes);
		
		if(MS_SendPackage(index))
		{
			index ++;
			tryTimes = 0;
		}
	}
	
	if(index == 3)
	{
		Out_printft(Ascii_5x8, Green, Black, COMMUNICATION_AREA_START_X , COMMUNICATION_AREA_START_Y + 20 , "Success! ", index, tryTimes);
	}
	else
	{
		Out_printft(Ascii_5x8, Red, Black, COMMUNICATION_AREA_START_X , COMMUNICATION_AREA_START_Y + 20 , "Fail!    ", index, tryTimes);
		
		MasterOnLine = FALSE;
		MS_ShowComState("Mst:Offline", Red);
	}
}


































static void MS_InitADC1()
{
	 /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */     
   GPIO_InitTypeDef GPIO_InitStructure;  
	 NVIC_InitTypeDef NVIC_InitStructure;
	 
  /* System clocks configuration ---------------------------------------------*/
	
  /* ADCCLK = PCLK2/4 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	
  /* Enable peripheral clocks ------------------------------------------------*/
  /* Enable DMA1 clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
  /* Enable GPIOA, GPIOC, ADC1 and TIM1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC |
                         RCC_APB2Periph_ADC1 | RCC_APB2Periph_TIM1, ENABLE);
	
  /* NVIC configuration ------------------------------------------------------*/
	/* Configure and enable ADC interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn; //ADC1_2_IRQn | 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
  /* Configure TIM1_CH1 (PA8) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
	
  /* Configure PC.06 as output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
  /* Configure PC.0, PC.01, PC.02 (ADC Channel10 Channel11 and Channel11) as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* TIM1 configuration ------------------------------------------------------*/
  /* Time Base configuration */
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure); 
  TIM_TimeBaseStructure.TIM_Period = 10;
  TIM_TimeBaseStructure.TIM_Prescaler = 72;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	
  /* TIM1 channel1 configuration in PWM mode */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;                
  TIM_OCInitStructure.TIM_Pulse = 0x1;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;         
  TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	
  /* DMA1 Channel1 Configuration ----------------------------------------------*/
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADC_RegularConvertedValueTab;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = DMA_SAMPLE_AMOUNT;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  //DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);	
  /* Enable DMA1 channel1 */
	DMA_Cmd(DMA1_Channel1, ENABLE);
	
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 3;
  ADC_Init(ADC1, &ADC_InitStructure);
	
  /* ADC1 regular channel14 configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_1Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_1Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_1Cycles5);
	
  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
	
  /* Enable ADC1 external trigger */ 
  ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	
	/* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
	
  /* Enable ADC1 reset calibration register */   
  ADC_ResetCalibration(ADC1);
	
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));
	
  /* Start ADC1 calibration */
  ADC_StartCalibration(ADC1);
	
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
	
  /* TIM1 counter enable */
  TIM_Cmd(TIM1, ENABLE);
	
	MS_Config_SapTime(488);
}
















