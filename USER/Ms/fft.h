#ifndef __FFT
#define __FFT

#define PI 3.1415926535897932384626433832795028841971               //定义圆周率值 
#define FFT_N 2048                                                  //定义福利叶变换的点数  

bool FFT_Calculate(float* reals, u32 amount, WaveInfoType* target);
void FFT_Calculate_NegComponents(WaveInfoType *waves, ComplexNumberType *target);	//计算负序分量
#endif
