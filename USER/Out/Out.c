
#include <stdio.h>

#include "lcd.h"
#include "gui.h"
#include "com.h"

void Out_Init()
{
	Com_USART1_Init();
	Com_USART2_Init();
	
	//printf("System Init ok\n\r");
	LCD_Init();
	//printf("LCD 	Init OK\n\r");
	
	Out_Config_FontType(Ascii_6x12);
}
