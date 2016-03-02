#ifndef __MEASURE
#define __MEASURE

#include "com.h"
#include "gui.h"


typedef struct
{
	float Real;
	float Image;
	float Module;
	float Phase;
}ComplexNumberType;


/*
 This struct contains the information of signal.
**/
typedef struct 
{
	float Dc;						//Dc    Component
	float BaseFrequency;			//Base  Frequency
	float PhasePosition;			//Phase Position
	float Rms;						//Rms
	float Amplitude;				//
	ComplexNumberType Complex; 		//
}WaveInfoType;

extern Canvas 	WaveFormCanvas;
extern u32 		DebugMode;

void MS_Config_SapTime(u16 t);
void MS_UpdateWaveform(bool);
void MS_Init(void);
void MS_Go(void);
void MS_Stop(void);
void MS_FrameEndHandler(void);
void MS_ReceivedFromMasterEvent(unsigned char rData);

#define SYSTEM_STATE_COLOR			((Components[ZERO_SEQUENCE].Module < 0.0000001) ? Green : Red) 

#define WAVEFORM_DISPLAY_WIDTH		350
#define WAVEFORM_DISPLAY_HEIGHT		200
#define WAVEFORM_DISPLAY_XSTART		20
#define WAVEFORM_DISPLAY_YSTART		0
#define WAVEFORM_DISPLAY_VSPACE		25

#define SMALL_WAVEFORM_DISPLAY_WIDTH		350
#define SMALL_WAVEFORM_DISPLAY_HEIGHT		50
#define SMALL_WAVEFORM_DISPLAY_XSTART		20
#define SMALL_WAVEFORM_DISPLAY_YSTART		25
#define SMALL_WAVEFORM_DISPLAY_VSPACE		40

#define STATE_POS_X							(WAVEFORM_DISPLAY_WIDTH + WAVEFORM_DISPLAY_XSTART + 2)
#define STATE_POS_Y							25
#define STATE_COLOR							Yellow
#define STATE_TITLE_COLOR				Blue

#define BUTTON_BACK_COLOR	 			RGB565(0x1F, 10, 10)
#define BUTTON_Y_START		 			60

#define MS_WAVE_COLOR			 			SYSTEM_STATE_COLOR
#define MS_WAVE_INFO_COLOR			MS_MIDDLE_COLOR
#define MS_MIDDLE_COLOR		 			RGB565(10, 15, 0)
#define MS_BACK_COLOR			 			RGB565(5,5,0)

#define DMA_SAMPLE_AMOUNT  			(WAVEFORM_DISPLAY_WIDTH * 3)		// 256 * 3

#define PHASE_DIF_BACK_COLOR			RGB565(5,5,0)
#define PHASE_DIF_LINE_COLOR			SYSTEM_STATE_COLOR
#define PHASE_DIF_CANVAS_WIDTH		WAVEFORM_DISPLAY_WIDTH
#define PHASE_DIF_CANVAS_HEIGHT		160
#define PHASE_DIF_CANVAS_XSTART		WAVEFORM_DISPLAY_XSTART/2
#define PHASE_DIF_CANVAS_YSTART		10
#define PHASE_DIF_INFORM_XSTART		5

#define PHASE_DIF_TITLE_COLOR			Red
#define PHASE_DIF_SUBTITLE_COLOR	Yellow
#define PHASE_DIF_INFORM_COLOR		Yellow

#define SAMPLE_LINE_COLOR			Red

#define COMMUNICATION_AREA_START_X	(WAVEFORM_DISPLAY_WIDTH +  WAVEFORM_DISPLAY_XSTART + 5)
#define COMMUNICATION_AREA_START_Y	(SCREEN_HEIGHT - 50)
#define COMMUNICAITON_WAY_COLOR			RGB565(0xF, 9, 2)

/*
*uncommenting this macro definition enable the FFT detail showing to COM.
*/
//#ifdef __FFT_DEBUG 

#endif
