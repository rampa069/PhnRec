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

#ifndef NN_H
#define NN_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<malloc.h>
#include "filename.h"

#ifdef WIN32
	#if defined(__BORLANDC__) && ! defined(__MATHF)
		#define __MATHF
		inline float logf(float arg) { return log(arg); }
		inline float sqrtf(float arg) { return sqrt(arg); }
		inline float cosf(float arg) { return cos(arg); }
		inline float expf(float arg) { return exp(arg); }
	#endif
#endif

// Error codes for the Load function 
const int NN_OK            = 0;
const int NN_NOWEIGHTS     = 1;
const int NN_BADWEIGHTS    = 2;
const int NN_NONORMS       = 3;
const int NN_BADNORMS      = 4;
const int NN_MEMORY        = 5;
const int NN_CREATEERR     = 6;
const int NN_WRITEERR      = 7;

// Blas setting
const int NN_BLAS_MINBUNCH = 15;

class NeuralNet
{
   public:
      NeuralNet();
      ~NeuralNet();
      int Load(char *pWeightFile, char *pNormFile, int bunchSize);
      int LoadAscii(char *pWeightFile, char *pNormFile, int bunchSize);
      int LoadBinary(char *pWeightFile, int bunchSize);
	  int SaveBinary(char *pWeightFile);
	  void Forward(float *pInp, float *pOut, int nVct);
      int GetInputSize();
      int GetOutputSize();
      int GetHiddenSize();
   protected:
      int mNInp;
      int mNHid;
      int mNOut;
      int mNInp16;                 // rounded up to x16 bytes
      int mNHid16;
      int mNOut16;
      float *mpInputPatterns;
      float *mpHiddenPatterns;
      float *mpOutputPatterns;
      float *mpInputPatterns16;     // rounded up to x16 bytes
      float *mpHiddenPatterns16;
      float *mpOutputPatterns16;
      float *mpInpHidMatrix;
      float *mpHidOutMatrix;
      float *mpInpHidMatrix16;      // rounded up to x16 bytes
      float *mpHidOutMatrix16;
      float *mpHiddenBiases;
      float *mpOutputBiases;            
      float *mpHiddenBiases16;      // rounded up to x16 bytes
      float *mpOutputBiases16;            
      float *mpNormMeans;
      float *mpNormDevs;
      float *mpNormMeans16;         // rounded up to x16 bytes
      float *mpNormDevs16;      
      bool mUseNorms;
      bool mAllocated;
      int mBunchSize;
      char *SkipSpaces(char *pText);
      char *SkipText(char *pText); 
      char *SkipLines(char *pText, int n);
      bool GetFloatValue(char *pText, float *pRet);
      bool GetIntValue(char *pText, int *pRet);
      char *LoadText(char *pFile);                 
      bool GetInfo(char *pData, int *pNInp, int *pNHid, int *pNOut);
      bool ParseWeights(char *pData);
      bool ParseNorms(char *pData);
      int Align16(int n);
      float *MemAlign16(float *pBuff);
      void Alloc();
      void Free();     
      void Normalize(int bunchOccupation, float *pData, int len, float *pMeans, float *pDevs);
      void PrepareBiases(int bunchOccupation, int len, float *pBiases, float *pPatterns);
      void MatrixMultiplyAndAdd(int nARows, int nACols, int nTRBRows, float *pA, float *pB, float *pC);
      void Sigmoid(int bunchOccupation, float *pData, int len, int lenAligned);
      void SoftMax(int bunchOccupation, float *pData, int len, int lenAligned);            
      void ForwardPass1Bunch(int bunchOccupation, float *pInp, float *pOut);
      void DumpMatrix(float *pMat, int nRows, int nColumns, int nColumnsAligned);
};

#endif
