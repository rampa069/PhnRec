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

#include "stkinterface.h"
#include "matrix.h"
#include "filename.h"

using namespace STK;

StkInterface::StkInterface() :
  Decoder(),
  NetworkLoaded(false),
  HMMSetLoaded(false),
  lastTime(-1),
  timePruning(99999),
  beamPruning(-99999.0f),
  wPenalty(0.0f),
  lmScale(1.0f),
  improveKwdEstim(false),
  fp_lab(0),
  lrt(0),
  nKeywords(0),
  kwsScorePruning(LOG_0)
{
  if (!my_hcreate_r(100,  &dictHash)
  ||  !my_hcreate_r(100,  &phoneHash)) 
  {
    Error("Insufficient memory");
  }
}


StkInterface::~StkInterface()
{
  if(NetworkLoaded)
    net.Release();
  if(HMMSetLoaded)
    hset.Release();

  // keyword spotting
  if(lrt)
    delete [] lrt;
}


int StkInterface::ReadHMMSet(char *mmFileName)
{
  if(!FileExistence(mmFileName))
  {
    char msg[256];
    sprintf(msg, "HMM definition file does not exist - %s", mmFileName);
    ErrorMessage(msg);
    return DECERR_INPUTFILE;
  }

  if(HMMSetLoaded)
    hset.Release();

  hset.Init(0);
  hset.ParseMmf(mmFileName, NULL);

  HMMSetLoaded = true;

  return DECERR_NONE;
}


int StkInterface::LoadNetwork(char *file_name)
{
  if(!FileExistence(file_name))
  {
    char msg[256];
    sprintf(msg, "Network file does not exist - %s", file_name);
    ErrorMessage(msg);
    return DECERR_INPUTFILE;
  }

  if(NetworkLoaded)
    net.Release();
  
  
  LabelFormat in_lbl_fmt  = {0};
  FILE* ilfp;
  ilfp = fopen(file_name, "rt");
  
  if (ilfp  == NULL) 
    Error("Cannot open network file: %s", file_name);

  Node *n = ReadSTKNetwork(ilfp, &dictHash, &phoneHash,
                           WORD_NOT_IN_DIC_UNSET, in_lbl_fmt,
                           1, file_name, NULL);

  net.Init (n, &hset, NULL);
  fclose(ilfp);
                                  
  // keyword spotting initialization
  if(mode == DECMODE_KWS)
  {
    Node *node;

    // check whether a null node is contained in the network
    // = end of filler model
    for(node = net.mpFirst; node != 0; node = node->mpNext) 
    {
      if(node->mType == (NT_WORD | NT_STICKY) && node->mpPronun == NULL) 
        break;
    }

    if(node == NULL)
    {
      ErrorMessage("Network must contain one final null node, the end of reference model");
      return DECERR_NETWORK;
    }

    filler_end = node->mpExitToken;
    nKeywords = 0;

    for(node = net.mpFirst; node != NULL; node = node->mpNext)
    {
      if(node->mType == (NT_WORD | NT_STICKY) && node->mpPronun != NULL) 
      nKeywords++;
    }

    if(nKeywords == 0)
    {
      ErrorMessage("Network must contain an final word nodes, the end of hypotesis model");
      return DECERR_NETWORK;
    }

    // allocate a trace structure for each keyword
    if(lrt)
      delete [] lrt;

    lrt = new LRTrace [nKeywords];

    int i;
    for(i = 0, node = net.mpFirst; node != NULL; node = node->mpNext)
    {
      if(node->mType == (NT_WORD | NT_STICKY) && node->mpPronun != NULL)
      {
        lrt[i].mpWordEnd = node;
        i++;
      }
    }
  }

  NetworkLoaded = true;

  return DECERR_NONE;
}
		

int StkInterface::Init(char *label_file)
{
  if(!HMMSetLoaded || !NetworkLoaded)
  {
    ErrorMessage("The HMM set or network is not loaded.");
    return DECERR_ALREADYINIT;
  }
  lastTime = -1;
  net.ViterbiInit();

  // open output label file
  if(label_file)
  {
    fp_lab = fopen(label_file, "w");
    if(!fp_lab)
    {
      char msg[256];
      sprintf(msg, "Can not create output label file - %s", label_file);
      ErrorMessage(msg);
      return DECERR_OUTPUTFILE;
    }
  }
  else
  {
    fp_lab = 0;
  }

  // keyword spotting
  if(mode == DECMODE_KWS)
  {
    int i;
    for(i = 0; i < nKeywords; i++)
    {
      lrt[i].lastLR = -FLT_MAX;
      lrt[i].candidateLR = -FLT_MAX;
      lrt[i].candidateStartTime = 0;
      lrt[i].candidateEndTime = 0;
      lrt[i].prevCandidateEndTime = 0;
      lrt[i].dumped = false;
    }

    KillToken(filler_end);
    int j;
    for(j = 0; j < nKeywords; j++)
      KillToken(lrt[j].mpWordEnd->mpExitToken);
  }

  return DECERR_NONE;
}


void StkInterface::ProcessFrame(float *frame)
{
  if(!HMMSetLoaded || !NetworkLoaded)
    return;
  net.ViterbiStep(frame);

  if(mode == DECMODE_DECODE)      // classical decoding mode
  {
    if(timePruning < 99999.0 && callbackFunc != 0)
    {
      WordLinkRecord*  wlr;
      if((wlr = TimePruning(&net, timePruning)) != 0) 
      {
        long prevTime = 0;
        if(wlr->mpNext)
          prevTime = wlr->mpNext->mTime;
        (*callbackFunc)(DECMSG_WORD, 
                        wlr->mpNode->mpPronun->mpWord->mpName,
                        (long_long)prevTime * (long_long)100000l, 
                        (long_long)wlr->mTime * (long_long)100000l, 
                        wlr->mLike, 
                        callbackTmpParam);
                        lastTime = wlr->mTime;
      }
    }
  }
  else                            // keyword spotting mode
  {
    int j;
    for(j = 0; j < nKeywords; j++) 
    {
      long_long wordStartTime;
      float lhRatio;
      Token *word_end = lrt[j].mpWordEnd->mpExitToken;

      if(!IS_ACTIVE(*word_end) || !IS_ACTIVE(*filler_end))
      {
        lrt[j].lastLR = -FLT_MAX;
        continue;
      }

      lhRatio = word_end->mLike - filler_end->mLike;

      if(lhRatio >= lrt[j].lastLR)    // LR is growing.
      { 
        wordStartTime = (long_long) (word_end->mpWlr && word_end->mpWlr->mpNext
											? word_end->mpWlr->mpNext->mTime : 0);
			
        // new candidate is greater then previous one or a new hypothesis
        // was found
        if(lhRatio >= lrt[j].candidateLR || lrt[j].candidateEndTime <= wordStartTime)
        {
          // if a new hypothesis was found, dump the previous one
          if(lrt[j].candidateEndTime <= wordStartTime)
          {
            PutKWSCandidateToLabels(&lrt[j]);
            lrt[j].dumped = false;
          }
          
          lrt[j].candidateStartTime = wordStartTime;
          lrt[j].candidateEndTime = (long_long) net.mTime;
          lrt[j].candidateLR = lhRatio;
        }
      }
      lrt[j].lastLR = lhRatio;
      KillToken(word_end);

      // time pruning
      if(timePruning < 99999.0)
      {
        if(lrt->candidateEndTime != 0 && net.mTime - lrt->candidateEndTime >= timePruning)
          PutKWSCandidateToLabels(&lrt[j]);
      }
    }
    KillToken(filler_end);
  }
}


float StkInterface::Done()
{
  if(!HMMSetLoaded || !NetworkLoaded)
    return DECERR_NOTINIT;

  Label *labels;
  Label *label;

  float like;

  if(mode == DECMODE_DECODE)   // classical decoder
  {
    like = net.ViterbiDone(&labels);

    if(callbackFunc)
    {
      for(label=labels; label != 0; label = label->mpNext)
      {
        if(label->mStop > lastTime)
        {
          (*callbackFunc)(DECMSG_WORD, 
                          label->mpName,
                          (long_long)label->mStart * (long_long)100000l, 
                          (long_long)label->mStop * (long_long)100000l, 
                          label->mScore, 
                          callbackTmpParam);				
        }	
      }
    }

    LabelFormat lf = {0};
    
    if(fp_lab)
      WriteLabels(fp_lab, labels, lf, 100000, "", 0);

    ReleaseLabels(labels);
  }
  else                         // keyword spotting 
  {
    like = net.ViterbiDone(0);

    int j;
    for(j = 0; j < nKeywords; j++)
    {
      if(!lrt[j].dumped)
        PutKWSCandidateToLabels(&lrt[j]);
    }
  }
		
  if(fp_lab)
    fclose(fp_lab);

  return like;
}	


void StkInterface::PutKWSCandidateToLabels(LRTrace *lrt)
{
  if(lrt->candidateEndTime != 0
    && (!lrt->dumped || (improveKwdEstim && lrt->candidateEndTime != lrt->prevCandidateEndTime)))
  {
    Label label = init_label;
    label.mStart = lrt->candidateStartTime;
    label.mStop  = lrt->candidateEndTime;
    label.mpName  = lrt->mpWordEnd->mpPronun->mpWord->mpName;
    label.mScore = lrt->candidateLR;

    LabelFormat lf = {0};
        
    if(fp_lab)
      WriteLabels(fp_lab, &label, lf, 100000, "", 0);

    if(callbackFunc)
    {
      (*callbackFunc)(!lrt->dumped ? DECMSG_WORD : DECMSG_NEWESTIM, 
                      label.mpName,
                      (long_long)label.mStart * (long_long)100000l, 
                      (long_long)label.mStop * (long_long)100000l, 
                      label.mScore, 
                      callbackTmpParam);
    }
    lrt->dumped = true;
    lrt->prevCandidateEndTime = lrt->candidateEndTime;
  }
}


void StkInterface::ErrorMessage(char *msg)
{
  #if defined(__WIN32) && defined(__VISUAL)
    MessageBox(0, msg, "Error", MB_OK);         
  #else
    printf(msg);
  #endif
}

