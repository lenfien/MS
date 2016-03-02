#ifndef __FFT
#define __FFT

#define PI 3.1415926535897932384626433832795028841971               //����Բ����ֵ 
#define FFT_N 2048                                                  //���帣��Ҷ�任�ĵ���  

bool FFT_Calculate(float* reals, u32 amount, WaveInfoType* target);
void FFT_Calculate_NegComponents(WaveInfoType *waves, ComplexNumberType *target);	//���㸺�����
#endif
