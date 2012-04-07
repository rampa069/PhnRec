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

#ifndef STK_StkInterface_h
#define STK_StkInterface_h

#undef USE_BLAS
  
#undef min
#undef max
           
//namespace std
//{
//  #define min(a,b) (a < b ? a : b)
//}

#include <fileio.h>
#include <common.h>
#include <Models.h>
#include <Viterbi.h>
#include <labels.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "decoder.h"


//  #undef min
 

using namespace STK;
    
  
class StkInterface : public Decoder
{
  protected:
    MyHSearchData     phoneHash;
    MyHSearchData     dictHash;
            
    bool              NetworkLoaded;
    bool              HMMSetLoaded;
    long              lastTime;
  
    long              timePruning;
    float             beamPruning;
    float             wPenalty;
    float             lmScale;
    bool              improveKwdEstim;
  
    FILE*             fp_lab;
      
    // stk structures
    ModelSet          hset;
    Network           net;
    
    // kws
    struct _LRTrace 
    {
      Node *mpWordEnd;
      float lastLR;
      float candidateLR;
      long_long candidateStartTime;
      long_long candidateEndTime;
      long_long prevCandidateEndTime;
      bool dumped;
    } *lrt;
      
    typedef struct _LRTrace LRTrace;
      
    Token*          filler_end;
    int             nKeywords;
    float           kwsScorePruning;
      
            
    void PutKWSCandidateToLabels(LRTrace *lrt);
  
      
  public:
    StkInterface();
    ~StkInterface();
  
    int ReadHMMSet(char *mmFileName);
    int LoadNetwork(char *file_name);
      
    // Batch processing
    //  int ProcessFile(char *features, char *label_file, float *score);
      
    // Per frame processing
    int Init(char *label_file = 0);
    void ProcessFrame(float *frame);
    float Done();	
    
    // Configuration
    void SetTimePruning(long v) {timePruning = v;};
    void SetBeamPruning(float v) {beamPruning = v; net.mPruningThresh = v;};
    void SetKwsScorePruning(float v) {kwsScorePruning = v;};
    void SetWPenalty(float v) {wPenalty = v; net.mWPenalty = (double)v;};
    void SetLmScale(float v) {lmScale = v; net.mLmScale = (double)v;};
    void SetImproveKwdEstim(bool v) {improveKwdEstim = v;};
    void SetLMScale(float v) {lmScale = v;};
    void ErrorMessage(char *msg);
};


#endif // #define STK_StkInterface_h
