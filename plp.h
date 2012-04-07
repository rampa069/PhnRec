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

#ifndef PLP_H
#define PLP_H

#include "melbanks.h"

#define PLP_DEF_LPCORDER      12
#define PLP_DEF_COMPRESSFACT   0.3333333
#define PLP_DEF_CEPLIFTER     22
#define PLP_DEF_CEPSCALE      10
#define PLP_DEF_ADDC0         true

class PLPCoefs : public MelBanks
{
  public:
    PLPCoefs();
    ~PLPCoefs();

    void SetLPCOrder(int Order)          {Free(); mLPCOrder = Order;};
    void SetCompressFactor(float Factor) {Free(); mCompressFactor = Factor;};
    void SetCepstralLifter(float Lifter) {Free(); mCepstralLifter = Lifter;};
    void SetCepstralScale(float Scale)   {Free(); mCepstralScale = Scale;};
    void SetAddC0(bool flag)             {Free(); mAddC0 = flag;}

    virtual int GetNParams()             {return (mAddC0 ? mLPCOrder + 1 : mLPCOrder);}

  protected:
    virtual void Init();   // intitialize buffers
    virtual void Free();   // free buffers before setting of new parameters or before deallocation of the instance
    virtual void ProcessFrame(float *pInpFrame, float *pOutFrame);

  private:
    int mLPCOrder;
    float mCompressFactor;
    float mCepstralLifter;
    float mCepstralScale; 
    bool mAddC0;

    float *mpEnergies;   
    float *mpEqualLaudnessCurve;
    float **mppIDFTBases;
    float *mpAutocorrCoefs;
    float *mpLPC;
    float *mpTmp;
    float *mpCepst;
    float *mpLifteringWindow;

    float **CreateIDFTMatrix(int NBases, int Dimension);
    void FreeMatrix(float **ppM, int NBases);
    void ApplyLinTransform(float **ppM, int NBases, int Dimension, float *pIn, float *pOut);
};

#endif

