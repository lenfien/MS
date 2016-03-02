
#include <stdio.h>
#include <math.h>
#include "ms.h"
#include "fft.h"
#include "com.h"


struct compx {float real,imag;};                                    //����һ�������ṹ 
struct compx s[FFT_N];                                             	//FFT������������S[1]��ʼ��ţ����ݴ�С�Լ����� 

void FFT_show(void);

/******************************************************************* 
����ԭ�ͣ�struct compx EE(struct compx b1,struct compx b2)   
�������ܣ��������������г˷����� ��������������������嶨��ĸ���a,b 
���������a��b�ĳ˻��������������ʽ��� 
*******************************************************************/ 
struct compx EE(struct compx a,struct compx b)       
{ 
	struct compx c; 
	c.real=a.real*b.real-a.imag*b.imag;  
	c.imag=a.real*b.imag+a.imag*b.real;  
	return(c);
}

/*****************************************************************
 ����ԭ�ͣ�void FFT(struct compx *xin,int N) 
�������ܣ�������ĸ�������п��ٸ���Ҷ�任��FFT�� 
���������*xin�����ṹ������׵�ַָ�룬struct�� 
*****************************************************************/ 
void FFT(struct compx *xin) 
{
	int f,m,nv2,nm1,i,k,l,j=0;   
	struct compx u,w,t;     
	nv2=FFT_N/2;                  //��ַ���㣬������Ȼ˳���ɵ�λ�򣬲����׵��㷨 
	nm1=FFT_N-1;   
	for(i=0;i<nm1;i++)            
	{ 
		if(i<j)                    //���i<j,�����б�ַ      
		{ 
			t=xin[j];                  
			xin[j]=xin[i];       
			xin[i]=t;      
		} 
		k=nv2;                    //��j����һ����λ�� 
		while(k<=j)               //���k<=j,��ʾj�����λΪ1        
		{            
			j=j-k;                 //�����λ���0 
			k=k/2;                 //k/2���Ƚϴθ�λ���������ƣ�����Ƚϣ�ֱ��ĳ��λΪ0      
		} 
		j=j+k;                   //��0��Ϊ1   
	} 
                    
	{
		int le,lei,ip;                            		//FFT����ˣ�ʹ�õ����������FFT����     
		f=FFT_N; 
		for (l = 1; (f = f / 2) != 1; l++);       		//����l��ֵ����������μ���            

		for(m=1;m<=l;m++)                         		// ���Ƶ��νἶ�� 
		{                                        		//m��ʾ��m�����Σ�lΪ���μ�����l=log(2��N 
			le=2<<(m-1);                            	//le���ν���룬����m�����εĵ��ν����le�� 
			lei=le/2;                               	//ͬһ���ν��вμ����������ľ���     
			u.real=1.0;                             	//uΪ���ν�����ϵ������ʼֵΪ1     
			u.imag=0.0; 
			w.real=cos(PI/lei);                     	//wΪϵ���̣�����ǰϵ����ǰһ��ϵ������    
			w.imag=-sin(PI/lei); 
			for(j=0;j<=lei-1;j++)                   	//���Ƽ��㲻ͬ�ֵ��νᣬ������ϵ����ͬ�ĵ��ν� 
			{
				for(i=j;i<=FFT_N-1;i=i+le)            	//����ͬһ���ν����㣬������ϵ����ͬ���ν�
				{
					ip=i+lei;                           //i��ip�ֱ��ʾ�μӵ�������������ڵ�         
					t=EE(xin[ip],u);                    //�������㣬�����ʽ         
					xin[ip].real=xin[i].real-t.real;         
					xin[ip].imag=xin[i].imag-t.imag;         
					xin[i].real=xin[i].real+t.real;         
					xin[i].imag=xin[i].imag+t.imag;       
				} 
				u=EE(u,w);                           	//�ı�ϵ����������һ����������      
			}
		} 
	} 
}  



bool FFT_Calculate(float* reals, u32 amount, WaveInfoType* target)
{
	u32 index;
	
	for(index = 0; index < amount; index ++)
	{
		s[index].real = reals[index];
		s[index].imag = 0;
	}
#ifdef __FFT_DEBUG
	//FFT_show();
#endif
	
	FFT(s);
	
#ifdef __FFT_DEBUG
	//FFT_show();
#endif
	
	target->Dc = s[0].real / amount;
	
	for(index = 1; index < amount; index ++)
	{
		if((ABS(s[index].real) - (s[index - 1].real))  > 50 || ABS((s[index].imag) - (s[index - 1].imag)) > 50)
		{
			target->BaseFrequency = index;
			target->Amplitude = sqrt((s[index].real) * (s[index].real) + (s[index].imag) * (s[index].imag))/(amount/2.0);
			target->Rms = target->Amplitude/1.4142135623730;
			target->PhasePosition = atan2(s[index].imag, s[index].real) * (180 / PI);
			target->Complex.Real = target->Amplitude * cos(target->PhasePosition /180.0 * PI);
			target->Complex.Image = target->Amplitude * sin(target->PhasePosition / 180.0 * PI);
			return TRUE;
		}
	}
	
	return FALSE;
}

#include "ms.h"

ComplexNumberType* Complex_Addition(ComplexNumberType* c1, ComplexNumberType* c2, ComplexNumberType* target)
{
	target->Real = c1->Real + c2->Real;
	target->Image = c1->Image + c2->Image;
	return target;
}

ComplexNumberType* Complex_Multiplication(ComplexNumberType* c1, ComplexNumberType* c2, ComplexNumberType* target)
{
	target->Real = c1->Real * c2->Real - c1->Image * c2->Image;
	target->Image = c1->Real * c2->Image + c1->Image * c2->Real;
	return target;
}

ComplexNumberType* Complex_Multiplication_by_constant(double constant, ComplexNumberType* complex)
{
	complex->Real *= constant;
	complex->Image *= constant;
	
	return complex;
}

/*
	���㸺�����
*/
void FFT_Calculate_NegComponents(WaveInfoType *waves, ComplexNumberType *target)
{
	ComplexNumberType a = {-0.5, 0.86602540378443864676372317075294}; 
	ComplexNumberType IA = waves[0].Complex;
	ComplexNumberType IB = waves[1].Complex;
	ComplexNumberType IC = waves[2].Complex;
	
	ComplexNumberType tempIA, tempIB, tempIC, tempaa;
	ComplexNumberType I1, I2, I3;
	ComplexNumberType tempI1, tempI2, tempI3;
	
	Complex_Multiplication(&a, &a, &tempaa);			//����aƽ��
	
	//����I1 �������
	tempIA = IA;
	Complex_Multiplication(&a, &IB, &tempIB);			//���� a * IB		IB��ʱ����ת120��
	Complex_Multiplication(&tempaa, &IC, &tempIC);		//���� a ^ 2 * IC	IC˳ʱ����ת120��
	
	Complex_Addition(&tempIA, &tempIB, &tempI1);		//���� IA + a * IB	
	Complex_Addition(&tempI1, &tempIC, &I1);			//���� IA + a * IB + a ^ 2 * IC
	Complex_Multiplication_by_constant(1.0/3.0, &I1);	//���� 1/3 * (IA + a * IB + a ^ 2 * IC)
	
	//����I2 �������
	tempIA = IA;
	Complex_Multiplication(&tempaa, &IB, &tempIB);	  	//IB ˳ʱ����ת120��
	Complex_Multiplication(&a, &IC, &tempIC);			//IC ��ʱ����ת120��
	
	Complex_Addition(&tempIA, &tempIB, &tempI2);
	Complex_Addition(&tempI2, &tempIC, &I2);
	Complex_Multiplication_by_constant(1.0/3.0, &I2);
	
	//����I3 �������
	tempIA = IA;
	tempIB = IB;
	tempIC = IC;
	
	Complex_Addition(&tempIA, &tempIB, &tempI3);
	Complex_Addition(&tempI3, &tempIC, &I3);
	Complex_Multiplication_by_constant(1.0/3.0, &I3);
	
	I1.Module = sqrt(pow(I1.Real, 2) + pow(I1.Image, 2));
	I2.Module = sqrt(pow(I2.Real, 2) + pow(I2.Image, 2));
	I3.Module = sqrt(pow(I3.Real, 2) + pow(I3.Image, 2));
	
	I1.Phase = atan(I1.Image/I1.Real) / PI * 180;
	I2.Phase = atan(I2.Image/I2.Real) / PI * 180;
	I3.Phase = atan(I3.Image/I3.Real) / PI * 180;
	
	target[0] = I1;
	target[1] = I2;
	target[2] = I3;
}


/************************************************************ 
����ԭ�ͣ�void main()  
�������ܣ�����FFT�任����ʾ����ʹ�÷��� ����������� ����������� 
************************************************************/ 
void FFT_test()
{
	int i; 
	
  for(i=0;i<FFT_N;i++)                    						//���ṹ�帳ֵ   
	{
			s[i].real= 1*cos(2*PI*50* (i * 0.00048828125)- PI*30/180);// + 1.5*cos(2*PI*75* ( i * 0.0009765625)+PI*90/180);	//ʵ��Ϊ���Ҳ�FFT_N���������ֵΪ1     
			s[i].imag=0;																		//�鲿Ϊ0  
	}
	
	//printf("---------------\n\r");
	
	//FFT_show();
  
	FFT(s);                                        	//���п��ٸ���Ҷ�任
	
	//printf("---------------\n\r");
	
	//FFT_show();
	
  for(i=0;i<FFT_N;i++)                           	//��任������ģֵ�����븴����ʵ������ 
  	s[i].real=sqrt(s[i].real*s[i].real+s[i].imag*s[i].imag); 
}

void FFT_show()
{
	int i;
	
	for(i = 0; i < FFT_N; i++)
	{
		printf("%d: %f\t%f\n\r",i + 1, s[i].real, s[i].imag);
	}
}
