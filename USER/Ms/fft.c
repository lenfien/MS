
#include <stdio.h>
#include <math.h>
#include "ms.h"
#include "fft.h"
#include "com.h"


struct compx {float real,imag;};                                    //定义一个复数结构 
struct compx s[FFT_N];                                             	//FFT输入和输出：从S[1]开始存放，根据大小自己定义 

void FFT_show(void);

/******************************************************************* 
函数原型：struct compx EE(struct compx b1,struct compx b2)   
函数功能：对两个复数进行乘法运算 输入参数：两个以联合体定义的复数a,b 
输出参数：a和b的乘积，以联合体的形式输出 
*******************************************************************/ 
struct compx EE(struct compx a,struct compx b)       
{ 
	struct compx c; 
	c.real=a.real*b.real-a.imag*b.imag;  
	c.imag=a.real*b.imag+a.imag*b.real;  
	return(c);
}

/*****************************************************************
 函数原型：void FFT(struct compx *xin,int N) 
函数功能：对输入的复数组进行快速傅里叶变换（FFT） 
输入参数：*xin复数结构体组的首地址指针，struct型 
*****************************************************************/ 
void FFT(struct compx *xin) 
{
	int f,m,nv2,nm1,i,k,l,j=0;   
	struct compx u,w,t;     
	nv2=FFT_N/2;                  //变址运算，即把自然顺序变成倒位序，采用雷德算法 
	nm1=FFT_N-1;   
	for(i=0;i<nm1;i++)            
	{ 
		if(i<j)                    //如果i<j,即进行变址      
		{ 
			t=xin[j];                  
			xin[j]=xin[i];       
			xin[i]=t;      
		} 
		k=nv2;                    //求j的下一个倒位序 
		while(k<=j)               //如果k<=j,表示j的最高位为1        
		{            
			j=j-k;                 //把最高位变成0 
			k=k/2;                 //k/2，比较次高位，依次类推，逐个比较，直到某个位为0      
		} 
		j=j+k;                   //把0改为1   
	} 
                    
	{
		int le,lei,ip;                            		//FFT运算核，使用蝶形运算完成FFT运算     
		f=FFT_N; 
		for (l = 1; (f = f / 2) != 1; l++);       		//计算l的值，即计算蝶形级数            

		for(m=1;m<=l;m++)                         		// 控制蝶形结级数 
		{                                        		//m表示第m级蝶形，l为蝶形级总数l=log(2）N 
			le=2<<(m-1);                            	//le蝶形结距离，即第m级蝶形的蝶形结相距le点 
			lei=le/2;                               	//同一蝶形结中参加运算的两点的距离     
			u.real=1.0;                             	//u为蝶形结运算系数，初始值为1     
			u.imag=0.0; 
			w.real=cos(PI/lei);                     	//w为系数商，即当前系数与前一个系数的商    
			w.imag=-sin(PI/lei); 
			for(j=0;j<=lei-1;j++)                   	//控制计算不同种蝶形结，即计算系数不同的蝶形结 
			{
				for(i=j;i<=FFT_N-1;i=i+le)            	//控制同一蝶形结运算，即计算系数相同蝶形结
				{
					ip=i+lei;                           //i，ip分别表示参加蝶形运算的两个节点         
					t=EE(xin[ip],u);                    //蝶形运算，详见公式         
					xin[ip].real=xin[i].real-t.real;         
					xin[ip].imag=xin[i].imag-t.imag;         
					xin[i].real=xin[i].real+t.real;         
					xin[i].imag=xin[i].imag+t.imag;       
				} 
				u=EE(u,w);                           	//改变系数，进行下一个蝶形运算      
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
	计算负序分量
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
	
	Complex_Multiplication(&a, &a, &tempaa);			//计算a平方
	
	//计算I1 负序分量
	tempIA = IA;
	Complex_Multiplication(&a, &IB, &tempIB);			//计算 a * IB		IB逆时针旋转120度
	Complex_Multiplication(&tempaa, &IC, &tempIC);		//计算 a ^ 2 * IC	IC顺时针旋转120度
	
	Complex_Addition(&tempIA, &tempIB, &tempI1);		//计算 IA + a * IB	
	Complex_Addition(&tempI1, &tempIC, &I1);			//计算 IA + a * IB + a ^ 2 * IC
	Complex_Multiplication_by_constant(1.0/3.0, &I1);	//计算 1/3 * (IA + a * IB + a ^ 2 * IC)
	
	//计算I2 正序分量
	tempIA = IA;
	Complex_Multiplication(&tempaa, &IB, &tempIB);	  	//IB 顺时针旋转120度
	Complex_Multiplication(&a, &IC, &tempIC);			//IC 逆时针旋转120度
	
	Complex_Addition(&tempIA, &tempIB, &tempI2);
	Complex_Addition(&tempI2, &tempIC, &I2);
	Complex_Multiplication_by_constant(1.0/3.0, &I2);
	
	//计算I3 令序分量
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
函数原型：void main()  
函数功能：测试FFT变换，演示函数使用方法 输入参数：无 输出参数：无 
************************************************************/ 
void FFT_test()
{
	int i; 
	
  for(i=0;i<FFT_N;i++)                    						//给结构体赋值   
	{
			s[i].real= 1*cos(2*PI*50* (i * 0.00048828125)- PI*30/180);// + 1.5*cos(2*PI*75* ( i * 0.0009765625)+PI*90/180);	//实部为正弦波FFT_N点采样，赋值为1     
			s[i].imag=0;																		//虚部为0  
	}
	
	//printf("---------------\n\r");
	
	//FFT_show();
  
	FFT(s);                                        	//进行快速福利叶变换
	
	//printf("---------------\n\r");
	
	//FFT_show();
	
  for(i=0;i<FFT_N;i++)                           	//求变换后结果的模值，存入复数的实部部分 
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
