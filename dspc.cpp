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

#include <stdio.h>
#include <stdlib.h>
#include "dspc.h"


/* Fast Fourier Transformation
 - the zeroth data's index is not used
 - data = [0 real1, imag1, real2, imag2 ...]
 - isign = -1 ~ FFT; isign = 1 ~ IFFT
*/
void cFour1(float *data, unsigned int nn, int isign)
{
	unsigned int n, mmax, m, j, istep, i;
	double wtemp, wr, wpr, wpi, wi, theta;

	float tempr,tempi;

	n = nn << 1;
	j = 1;
	for (i = 1; i < n; i += 2)
	{
		if (j > i)
		{
			SWAP(data[j],data[i]);
			SWAP(data[j + 1],data[i + 1]);
			//printf("%f %f %d %d\n", data[j], data[i], j, i);
		}
		m = n >> 1;
		while(m >= 2 && j > m)
		{
			j -= m;
			m >>= 1;
		}
		j += m;
	}

	mmax=2;
	while (n > mmax)
	{
		istep = mmax << 1;
		theta = isign*(6.28318530717959 / mmax);
		wtemp  =sin(0.5 * theta);
		wpr = -2.0 * wtemp * wtemp;
		wpi = sin(theta);
		wr = 1.0;
		wi = 0.0;
		for (m = 1; m < mmax; m += 2)
		{
			for (i = m; i <= n; i += istep)
			{
				j = i + mmax;
				tempr = wr * data[j] - wi * data[j + 1];
				tempi = wr * data[j + 1] + wi * data[j];
				data[j] = data[i] - tempr;
				data[j + 1] = data[i + 1] - tempi;
				data[i] += tempr;
				data[i+1] += tempi;
			}

			wr = (wtemp = wr) * wpr - wi * wpi + wr;
			wi = wi * wpr + wtemp * wpi + wi;
		}
		mmax = istep;
	}
}

struct _MelBanks *_mbInit(int Count, int WindowLength, int SampleFreq, float fmin, float fmax, bool Debug)
{
	if(Count < 3)
	{
		fprintf(stderr, "ERROR: _mbPrepare - number of filters has be greater than three");
		exit(1);
	}

	if(fmin < 0.0f)
		fmin = 0.0f;

	if(fmax > (float)SampleFreq / 2.0f)
		fmax = (float)SampleFreq / 2.0f;

	/* Memory allocation */
	struct _MelBanks *Prep = new struct _MelBanks;
	if(!Prep)
	{
        	fprintf(stderr, "ERROR: _mbPrepare - memory allocation error");
		exit(1);
	}

	Prep->Count = Count;
	Prep->WindowLength = WindowLength;
	Prep->SampleFreq = SampleFreq;
	Prep->WindowLength_2 = WindowLength / 2;

	Prep->f0 = 0;
	Prep->Coeffs = 0;
	Prep->Banks = 0;

	Prep->f0 = new float[Count+1];
	Prep->f0m = new float[Count+1];
	Prep->Coeffs = new float [Prep->WindowLength_2];
	Prep->Banks = new short [Prep->WindowLength_2];

	if(!Prep->f0 || !Prep->Coeffs || !Prep->Banks || !Prep->f0m)
	{
		delete [] Prep->f0;
		delete [] Prep->f0m;
		delete [] Prep->Coeffs;
		delete [] Prep->Banks;
        	fprintf(stderr, "ERROR: _mbPrepare - memory allocation error");
		exit(1);
	}

	int i, j;

	for(j = 0; j < Prep->WindowLength_2; ++j)
	{
		Prep->Coeffs[j] = 0.0f;
		Prep->Banks[j] = -1;
	}

	float bf = (float)SampleFreq / (float)WindowLength;
	Prep->lo = fmin;
	Prep->hi = fmax;
	Prep->mlo = Scale_Mel(fmin);
	Prep->mhi = Scale_Mel(fmax);
	Prep->fftlo = (int)(Prep->lo / bf + 1.5f);
	Prep->ffthi = (int)(Prep->hi / bf - 0.5f);
	if(Prep->fftlo < 1)
		Prep->fftlo = 1;
	if(Prep->ffthi >= Prep->WindowLength_2)
		Prep->ffthi = Prep->WindowLength_2 - 1;
	float mel_freq_delta = (Prep->mhi - Prep->mlo) / (Count + 1);
	float mel_freq = Prep->mlo;

	/*  Filter coefficients are calculated on foollowing few lines.
	 *  The right part of each filter is storead only.
	 *
	 *      /\  |\  |\  |\      - 1
	 *     /  \ | \ | \ | \
	 *    /    \|  \|  \|  \    - 0
	 */

	/* centers of bands */
	for(i = 0; i <= Count; ++i)
	{
		mel_freq = mel_freq + mel_freq_delta;
		Prep->f0m[i] = mel_freq;
		Prep->f0[i] = Scale_MelToLinear(mel_freq);
	}

	/* fft indexes */
	int ch = 0;
	for(i = 0; i < Prep->WindowLength_2; ++i)
	{
		if(i < Prep->fftlo || i > Prep->ffthi)
		{
			Prep->Banks[i] = -1;
		}
		else
		{
			mel_freq = Scale_Mel((float)i*bf);
			while(mel_freq > Prep->f0m[ch] && ch <= Prep->Count)
				++ch;
			Prep->Banks[i] = ch;
		}
	}

	/* weights */
	for(i = 0; i < Prep->WindowLength_2; ++i)
	{
		ch = Prep->Banks[i];

		if(i < Prep->fftlo || i > Prep->ffthi)
	        {
			Prep->Coeffs[i] = 0;
		}
		else
		{
			if(ch == 0)
				Prep->Coeffs[i] = (Prep->f0m[0] - Scale_Mel((float)i*bf)) / (Prep->f0m[0] - Prep->mlo);
			else
				Prep->Coeffs[i] = (Prep->f0m[ch] - Scale_Mel((float)i*bf)) / (Prep->f0m[ch] - Prep->f0m[ch-1]);
		}
	}

	if(Debug)
	{
		printf("MelBanksPrepare\n");
		printf("        Count           %d\n", Prep->Count);
		printf("        SampleFreq      %d\n", Prep->SampleFreq);
		printf("        WindowLength    %d\n", Prep->WindowLength);
		printf("Prep->f0\n");
		for(i = 0; i <= Prep->Count; ++i)
			printf(" %f", Prep->f0[i]);
		printf("\n");
		printf("Prep->f0m\n");
		for(i = 0; i <= Prep->Count; ++i)
		{
			printf(" %f", Prep->f0m[i]);
		}
		printf("\n");
		printf("Prep->Banks\n");
		for(i = 0; i < Prep->WindowLength / 2; ++i)
			printf(" %d", Prep->Banks[i]);
		printf("\n");
		printf("Prep->Coeffs\n");
		for(i = 0; i < Prep->WindowLength / 2; ++i)
			printf(" %f", Prep->Coeffs[i]);
		printf("\n");
	}
	return Prep;
}

void _mbFree(_MelBanks *Prep)
{
	delete [] Prep->Coeffs;
	delete [] Prep->Banks;
	delete [] Prep->f0;
	delete [] Prep->f0m;
	delete Prep;
}

void _mbApply(_MelBanks *Prep, float *data, float *out, int Debug)
{
	int i;
	float v;
	short *Banks = Prep->Banks;
	float *Coeffs = Prep->Coeffs;
	short Bank;

	if(Debug)
		printf("APPLY MEL BANKS:\n");

	for(i = 0; i < Prep->Count; ++i)
		out[i] = 0.0f;

	for(i = Prep->fftlo; i <= Prep->ffthi; ++i)
	{
		v = Coeffs[i] * data[i];
		Bank = Banks[i];
		if(Bank > 0)
			out[Bank - 1] += v;
        	if(Bank < Prep->Count)
			out[Bank] += (data[i] - v);    /* (1-coef)*data */
		if(Debug)
			printf("%f %f %d\n", data[i], Coeffs[i], Bank);
	}

	if(Debug)
	{
		printf("MEL BANKS:\n");
		for(i = 0; i < Prep->Count - 1; ++i)
			printf(" %f", out[i]);
		printf("\n");
	}
}



// Durbin's recursion - cnversion autocorrelation coefficients to LPC

float sDurbin(float *pAC, float *pLP, float *pTmp, int Order)
{
   float ki;    // reflection coefficient
   int i;
   int j;
 
   float E = pAC[0]; 

   for(i = 0; i < Order; i++)
   {
      // next reflection coefficient
      ki = pAC[i + 1];
      for(j = 0; j < i; j++)
      {
         ki = ki + pLP[j] * pAC[i - j];
      }
      ki = ki / E;   

      // new error
      E *= 1 - ki * ki;

      // new LP coefficients
      pTmp[i] = -ki;       
      for (j = 0; j < i; j++)
         pTmp[j] = pLP[j] - ki * pLP[i - j - 1];

      for (j = 0; j <= i; j++)  
      { 
         pLP[j] = pTmp[j];
      }
   }

   return E;
}

void sLPC2Cepstrum(int N, float *pLPC, float *pCepst)
{
   int i;
   for (i = 0; i < N; i++)  
   { 
      float sum = 0.0f;
      int j;
      for (j = 0; j < i; j++)
      { 
         sum += (float)(i - j) * pLPC[j] * pCepst[i - j - 1];
      }
      pCepst[i] = -pLPC[i] - sum / (float)(i + 1);
   }
}


void sLifteringWindow(int N, float *pWin, int Q)
{
   int i;
   
   for (i = 0; i < N; i++)
   {
      pWin[i] = 1.0f + 0.5f * Q * sinf(M_PI * (float)(i + 1) / (float)Q);
   }
}  
