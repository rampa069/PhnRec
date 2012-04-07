/**************************************************************************
 *  copyright            : (C) 2004-2006 by Petr Schwarz & Pavel Matejka  *
 *                                        UPGM,FIT,VUT,Brno               *
 *  email                : {schwarzp,matejkap}@fit.vutbr.cz               *
 **************************************************************************
 *                                                                        *
 *  This program is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 **************************************************************************/

#ifndef DSPC_H
#define DSPC_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
	#if defined(__BORLANDC__) && ! defined(__MATHF__)
		#define __MATHF__
		inline float logf(float arg) { return log(arg); }
		inline float sqrtf(float arg) { return sqrt(arg); }
		inline float cosf(float arg) { return cos(arg); }
		inline float expf(float arg) { return exp(arg); }
	#endif
#endif

#define SWAP(a,b) tempr=(a); (a)=(b); (b)=tempr

#ifndef M_PI
#define M_PI  3.1415926535897932384626433832795
#endif

inline void sCopy(int n, float *to, float *from)
{
        int i;
        for(i = 0; i < n; ++i)
                to[i] = from[i];
}

inline void sShiftLeft(int n, float *re, int m)
{
	int i, j;
	for(i = m, j = 0; i < n; ++i, ++j)
		re[j] = re[i];
	for( ;j < n; ++j)
		re[j] = 0;
}

inline void sShiftRight(int n, float *re, int m)
{
    int i, j;
    for(i = n - 1, j = n - 1 - m; j >= 0; --i, --j)
	re[i] = re[j];
    for( ;i >= 0; --i)
    	re[i] = 0.0f;

}


inline void sSubtractAverage(int n, float *data)
{
	float Average;
	int i;
	Average = 0.0;
	for(i = 0; i < n; ++i)
		Average += data[i];
	Average /= (float)n;

	for(i = 0; i < n; ++i)
		data[i] -= Average;
}

inline void sPreemphasisBW(int N, float *x, float a)
{
	int n;

	for (n = N - 1; n > 0; --n)
		x[n] -= a * x[n - 1];
	x[0] *= (1.0f - a);
}

inline void sMultiplication(int n, float *re, float c)
{
	int     i;
	for (i = 0; i < n; ++i)
		re[i] = re[i] * c;
}

inline void sAddition(int n, float *re, float c)
{
	int 	i;
	for (i = 0; i < n; ++i)
		re[i] = re[i] + c;
}

inline void sAddNoise(int n, float *re, float level)
{
	int i;
	for (i = 0; i < n; ++i)
		re[i] += (level * 2.0f * ((float)rand() / (float)RAND_MAX - 0.5f));
}

inline void sMultVect(int n, float *vect1, float *vect2)
{
	int i;
	for(i = 0; i < n; ++i)
		vect1[i] *= vect2[i];
}

inline void sSet(int n, float *vector, float v)
{
	int i;
        for(i = 0; i < n; ++i)
                vector[i] = v;
}

inline void cComplex22N(int n, float *re, float *im, float *s2N)
{
	int i, j;
	for(i = 0, j = 0; i < n; ++i, j += 2)
	{
		s2N[j] = re[i];
		s2N[j+1] = im[i];
	}
}

inline void c2N2Complex(int n, float *s2N, float *re, float *im)
{
	int i, j;
	for(i = 0, j = 0; i < n; ++i, j+=2)
	{
		re[i] = s2N[j];
		im[i] = s2N[j+1];
	}
}

inline void cPower(int n, float *re, float *imag)
{
        int i;
        for (i = 0; i < n; ++i)
                re[i] = (re[i] * re[i] + imag[i] * imag[i]);
}

inline void sSqrt(int n, float *re)
{
        int i;
        for (i = 0; i < n; ++i)
                re[i] = sqrtf(re[i]);
}

inline void sLn(int n, float *re)
{
	int i;
	for (i = 0; i < n; ++i)
		re[i] = (re[i] > 0.0f ? logf(re[i]) : 0.0f);
}

inline void sWindow_Hamming(int n, float *re)
{
	int i;
	for (i = 0; i < n; ++i)
		re[i] = re[i] * (0.54f - 0.46f * cosf(2.0f * (float)M_PI * i / (n - 1)));
}

inline float Scale_MelToLinear(float freq)
{
	return 700.0f * (expf(freq / 1127.0f) - 1.0f);
}

inline float Scale_Mel(float freq)
{
	return 1127.0f * logf(1.0f + freq / 700.0f);
}


void cFour1(float *data, unsigned int nn, int isign);

struct _MelBanks
{
	int Count;
	int SampleFreq;
	int WindowLength;
	int WindowLength_2;
	float *f0;
	float *f0m;         /* in mel scale */
	float *Coeffs;
	short *Banks;
	float lo;
	float hi;
	float mlo;
	float mhi;
	int fftlo;
	int ffthi;
};

void _mbApply(_MelBanks *Prep, float *data, float *out, int Debug);

struct _MelBanks *_mbInit(int Count, int WindowLength, int SampleFreq, float fmin, float fmax, bool Debug);

void _mbFree(_MelBanks *Prep);

inline void sDCT(int n, float *re, int nOut, float *Out)
{
	int j, k;
	float NormC = sqrtf(2.0f/(float)n); 
	float PiByN = (float)M_PI/(float)n;
	float v;

	for(k = 0; k < nOut; ++k)
	{
		Out[k] = 0;
		v = PiByN * (float)(k + 1);
		for(j = 0; j < n; ++j)
			Out[k] += re[j] * cosf(v * ((float)j + 0.5f));
		Out[k] *= NormC;
	}
}

inline float CalcC0(int n, float *data)
{
	float NormC = sqrtf(2.0f/(float)n);

	float sum = 0.0f;
	int i;
	for(i = 0; i < n; ++i)
		sum += data[i];
	sum *= NormC;
	return sum;
}

inline void sEqualLaudnessCurve(int N, float *pFreqCenters, float *pRetCurve)
{
	int i;
	for(i = 0; i < N; i++)
	{
		float fsq = (pFreqCenters[i] * pFreqCenters[i]);
		float fsub = fsq / (fsq + 1.6e5f);
		pRetCurve[i] = fsub * fsub * ((fsq + 1.44e6f) / (fsq + 9.61e6f));
	}	
}

inline void sPower(int N, float *pVct, float PowerConst)
{
        int i;
        for (i = 0; i < N; ++i)
	{
                pVct[i] = powf(pVct[i], PowerConst);
	}
}

inline void sLowerFloor(int N, float *pVct, float Floor)
{
        int i;
        for (i = 0; i < N; ++i)
	{
                if(pVct[i] < Floor)
		{
			pVct[i] = Floor;
		}
	}
}

float sDurbin(float *pAC, float *pLP, float *pTmp, int Order);
void sLPC2Cepstrum(int N, float *pLPC, float *pCepst);
void sLifteringWindow(int N, float *pWin, int Q);

#endif
