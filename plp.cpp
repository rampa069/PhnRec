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

#include <assert.h>
#include <stdio.h>
#include "plp.h"
#include "dspc.h"


PLPCoefs::PLPCoefs() : 
    MelBanks(),
    mLPCOrder(PLP_DEF_LPCORDER),
    mCompressFactor(PLP_DEF_COMPRESSFACT),
    mCepstralLifter(PLP_DEF_CEPLIFTER),
    mCepstralScale(PLP_DEF_CEPSCALE),
    mAddC0(PLP_DEF_ADDC0),
    mpEnergies(0),
    mppIDFTBases(0), 
    mpAutocorrCoefs(0),
    mpLifteringWindow(0)
{
}

PLPCoefs::~PLPCoefs()
{
   Free();
}

void PLPCoefs::Init()
{
   MelBanks::Init();

   int nbanks = GetNBanks();

   // space for mel-bank enerfies
   mpEnergies = new float [nbanks + 2];

   // centers of mel banks (Hz)
   float *fcenters = GetCenters();
   
   // generate equal laudness curve
   mpEqualLaudnessCurve = new float [nbanks];
   sEqualLaudnessCurve(nbanks, fcenters, mpEqualLaudnessCurve);

   // IDFT matrix
   mppIDFTBases = CreateIDFTMatrix(mLPCOrder + 1, nbanks + 2);

   // space for autocoretation coefficients, LPC coefficients, 
   // and Durbin's recursion
   mpAutocorrCoefs = new float [mLPCOrder + 1];
   mpLPC = new float [mLPCOrder + 1];
   mpTmp = new float [mLPCOrder + 1];
   mpCepst = new float [mLPCOrder + 1];

   // generate liftering window
   mpLifteringWindow = new float [mLPCOrder];
   sLifteringWindow(mLPCOrder, mpLifteringWindow, (int)mCepstralLifter);

   SetTakeLog(false);
}

void PLPCoefs::Free()
{
   if(mpEnergies != 0)
   {
      delete [] mpEnergies;
      mpEnergies = 0;   
      FreeMatrix(mppIDFTBases, mLPCOrder + 1);
      delete [] mpAutocorrCoefs;
      delete [] mpLPC;
      delete [] mpTmp;
      delete [] mpCepst; 
      delete [] mpLifteringWindow;    
   }

   MelBanks::Free();

}


void PLPCoefs::ProcessFrame(float *pInpFrame, float *pOutFrame)
{

   MelBanks::ProcessFrame(pInpFrame, mpEnergies);

//float a[] = {6.728697e+005, 9.397764e+005, 1.477376e+006, 2.738733e+005, 4.895320e+004, 8.109916e+004, 9.704898e+004, 2.696904e+004, 4.563343e+004, 2.192602e+005, 4.360400e+005, 6.248051e+005, 2.641279e+005, 5.159555e+004, 4.087839e+004, 5.961117e+004, 1.200557e+005, 4.983299e+004, 2.235987e+004, 2.777589e+004, 4.005698e+004, 2.035172e+004, 3.305208e+004, 6.939732e+004};
//sCopy(24, mpEnergies, a);

   int nbanks = GetNBanks();

   // limit energies
   sLowerFloor(nbanks, mpEnergies, 1.0f);

   // apply equal laudness curve
   sMultVect(nbanks, mpEnergies, mpEqualLaudnessCurve);

   // compression of energies
   sPower(nbanks, mpEnergies, mCompressFactor);

   // duplicate the first and the last value 
   sShiftRight(nbanks + 1, mpEnergies, 1);
   mpEnergies[0] = mpEnergies[1];
   mpEnergies[nbanks + 1] = mpEnergies[nbanks];

   // IDCT
   ApplyLinTransform(mppIDFTBases, mLPCOrder + 1, GetNBanks() + 2, mpEnergies, mpAutocorrCoefs);
   
   // autocerrelation -> LPC
   float gain = sDurbin(mpAutocorrCoefs, mpLPC, mpTmp, mLPCOrder);
   assert(gain > 0.0f);

   // LPC -> cepstrum
   sLPC2Cepstrum(mLPCOrder, mpLPC, mpCepst);

   // calculate C0
   mpCepst[mLPCOrder] = -logf(1.0f/gain); 

   // appply liftering window
   if(mCepstralLifter != 0.0f)
   {
      sMultVect(mLPCOrder, mpCepst, mpLifteringWindow);
   }

   // cepstral scaling
   if(mCepstralScale != 1.0f)
   {
      sMultiplication(mLPCOrder + 1, mpCepst, mCepstralScale);
   }

   sCopy((mAddC0 ? mLPCOrder + 1 : mLPCOrder), pOutFrame, mpCepst);
}

float **PLPCoefs::CreateIDFTMatrix(int NBases, int Dimension)
{
   // allocate the matrix
   float **ppM = new float * [NBases];
   int i;
   for(i = 0; i < NBases; i++)
   {
      ppM[i] = new float [Dimension];
   }

   float angle = M_PI / (float)(Dimension - 1);
   float scale = 1.0f / (2.0f * (Dimension - 1));
   for (i = 0; i < NBases; i++) 
   {
      ppM[i][0] = 1.0f * scale;
      int j;
      for(j = 1; j < Dimension - 1; j++)
      {
         ppM[i][j] = 2.0 * scale * cos(angle * (float)i * (float)j);
      }
      ppM[i][Dimension - 1] = scale * cos(angle * (float)i * (float)(Dimension-1));
   }

   return ppM;
}

void PLPCoefs::FreeMatrix(float **ppM, int NBases)
{
   int i;
   for(i = 0; i < NBases; i++)
   {
      delete [] ppM[i];
   }
   delete [] ppM;
}

void PLPCoefs::ApplyLinTransform(float **ppM, int NBases, int Dimension, float *pIn, float *pOut)
{
   int i;
   for(i = 0; i < NBases; i++)
   {
      pOut[i] = 0.0f;
      int j;
      for(j = 0; j < Dimension; j++)
      {
         pOut[i] += pIn[j] *ppM[i][j];
      }         
   }
}


