/***************************************************************************
 *   copyright            : (C) 2004 by Lukas Burget,UPGM,FIT,VUT,Brno     *
 *   email                : burget@fit.vutbr.cz                            *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


//#############################################################################
//#############################################################################
// PROJECT INCLUDES
//#############################################################################
//#############################################################################
#include "Viterbi.h"
#include "labels.h"
#include "common.h"


//#############################################################################
//#############################################################################
// SYSTEM INCLUDES
//#############################################################################
//#############################################################################
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <stdarg.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>
#include <iostream>

using namespace std;

#ifdef MOTIF
#include "imagesc.h"
#endif

//#define TRACE_TOKENS
#define SQR(x) ((x) * (x))


//#############################################################################
//#############################################################################
// CODE
//#############################################################################
//#############################################################################

namespace STK
{
  int  BackwardPruning(int time, Node *node, int state);
  
  #ifdef DEBUG_MSGS
  WordLinkRecord * firstWLR;
  #endif
  
  
  //***************************************************************************
  //***************************************************************************
  WordLinkRecord *
  TimePruning(Network *net, int frame_delay)
  {
    size_t              i;
    Node *              node;
    Token *             token = net->mpBestToken;
    WordLinkRecord *    twlr;
    WordLinkRecord *    rwlr = NULL;
  
    if (frame_delay > net->mTime-1 || !token) 
      return NULL;
  
    if (token->mpTWlr != NULL &&
      token->mpTWlr->mTime == net->mTime-1 - frame_delay) 
    {
      rwlr = token->mpTWlr;
    }
  
    for (node = net->mpFirst; node != NULL; node = node->mpNext) 
    {
      if (!(node->mType & NT_MODEL)) 
        continue;
  
      for (i = 0; i < node->mpHmm->mNStates-1; i++) 
      {
        if (node->mpTokens[i].IsActive()) 
        {
          Token *token = &node->mpTokens[i];
  
          if (token->mpTWlr != NULL &&
            token->mpTWlr->mTime == net->mTime-1 - frame_delay) 
          {
            if (rwlr != token->mpTWlr) 
            {
              KillToken(token);
            } 
            else 
            {
              if (token->mpWlr == token->mpTWlr) 
              {
                token->mpTWlr = NULL;
              } 
              else 
              {
                for (twlr = token->mpWlr; 
                     twlr->mpNext != token->mpTWlr; 
                     twlr = twlr->mpNext)
                {}
                token->mpTWlr = twlr;
              }
            }
          } 
          else if (rwlr) 
          {
            KillToken(token);
          }
        }
      }
    }
    return rwlr;
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  PhoneNodesToModelNodes(Node *  pFirst, ModelSet * pHmms, ModelSet *pHmmsToUpdate)
  {
    Node *node;
  
    if (pHmmsToUpdate == NULL) 
      pHmmsToUpdate = pHmms;
  
    for (node =  pFirst; node != NULL; node = node->mpNext) 
    {
      if (node->mType & NT_PHONE) 
      {
        Macro *macro;
  
        node->mType &= ~NT_PHONE;
        node->mType |= NT_MODEL;
        macro = FindMacro(&pHmms->mHmmHash, node->mpName);
        if (macro == NULL) {
          Error("Model %s not defined in %sHMM set", node->mpName,
                pHmmsToUpdate != pHmms ? "alignment " : "");
        }
        node->mpHmm = node->mpHmmToUpdate = (Hmm *) macro->mpData;
  
        if (pHmmsToUpdate != pHmms) {
          macro = FindMacro(&pHmmsToUpdate->mHmmHash, node->mpName);
          if (macro == NULL) {
            Error("Model %s not defined in HMM set", node->mpName,
                  pHmmsToUpdate != pHmms ? "" : "target ");
          }
          node->mpHmmToUpdate = (Hmm *) macro->mpData;
        }
      }
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  int 
  cmplnk(const void *a, const void *b)
  {
    return ((Link *) a)->mpNode->mAux - ((Link *) b)->mpNode->mAux;
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  WriteAlpha(int time, Node *node, int state, Token *token)
  {
    if (node->mpAlphaBetaListReverse == NULL ||
      node->mpAlphaBetaListReverse->mTime != time) 
    {
      size_t    i;
      FWBWR *   newrec;
      
      newrec  = (FWBWR*) malloc(sizeof(FWBWR) +
                                sizeof(newrec->mpState[0]) * (node->mpHmm->mNStates-1));
      if (newrec == NULL) 
        Error("Insufficient memory");
        
      newrec->mpNext = node->mpAlphaBetaListReverse;
      newrec->mTime = time;
      
      for (i=0; i<node->mpHmm->mNStates; i++) 
      {
        newrec->mpState[i].mAlpha = newrec->mpState[i].mBeta = LOG_0;
      }
      
      node->mpAlphaBetaListReverse = newrec;
    }
    node->mpAlphaBetaListReverse->mpState[state].mAlpha = token->mLike;
    node->mpAlphaBetaListReverse->mpState[state].mAlphaAccuracy = token->mAccuracy;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  WriteBeta(int time, Node *node, int state, Token *token)
  {  
    // Record for current time must be already moved to
    // mpAlphaBetaList by function BackwardPruning
    assert(node->mpAlphaBetaListReverse == NULL ||
          node->mpAlphaBetaListReverse->mTime < time);
  
    if (node->mpAlphaBetaList != NULL && node->mpAlphaBetaList->mTime == time) {
      node->mpAlphaBetaList->mpState[state].mBeta = token->mLike;
      node->mpAlphaBetaList->mpState[state].mBetaAccuracy = token->mAccuracy;
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  BackwardPruning(int time, Node *node, int state)
  {
    while (node->mpAlphaBetaListReverse != NULL &&
          node->mpAlphaBetaListReverse->mTime > time) 
    {
      FWBWR *fwbwr = node->mpAlphaBetaListReverse;
      node->mpAlphaBetaListReverse = fwbwr->mpNext;
      free(fwbwr);
    }
  
    if (node->mpAlphaBetaListReverse != NULL &&
      node->mpAlphaBetaListReverse->mTime == time) 
    {
      FWBWR *fwbwr = node->mpAlphaBetaListReverse;
      node->mpAlphaBetaListReverse = fwbwr->mpNext;
      fwbwr->mpNext = node->mpAlphaBetaList;
      node->mpAlphaBetaList = fwbwr;
    }
  
    return !(node->mpAlphaBetaList != NULL &&
            node->mpAlphaBetaList->mTime == time &&
            node->mpAlphaBetaList->mpState[state].mAlpha > LOG_MIN);
  }
  
  
  
  
  #ifndef NDEBUG
  int test_for_cycle = 0;
  int HasCycleCounter = 100000;
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  Network::
  HasCycle() 
  {
    Node *      node;
    
    HasCycleCounter++;
    
    if (!test_for_cycle) 
      return 0;
      
    for (node = mpActiveNodes; node; node= node->mpNextActiveNode) 
    {
      int     i;
      int     n_links = InForwardPass() ? node->mNLinks : node->mNBackLinks;
      Link *  links   = InForwardPass() ? node->mpLinks : node->mpBackLinks;
      
      if (node->mAux2 == HasCycleCounter) 
      {
        printf("Cycle in list of active nodes\n");
        return 1;
      }
      
      node->mAux2 = HasCycleCounter;
  
      for (i = 0; i < n_links; i++)
      {
        if (links[i].mpNode->mAux2 == HasCycleCounter &&
            (!(links[i].mpNode->mType & NT_MODEL)
              || links[i].mpNode->mType & NT_TEE)) 
        {
          printf("Active node %d listed after his non-model succesor %d\n",
                (int) (node - mpFirst), (int) (links[i].mpNode - mpFirst));
  
          return 2;
        }
      }
    }
    return 0;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  bool
  Network:: 
  AllWordSuccessorsAreActive() 
  {
    Node *  node;
    
    if (!test_for_cycle) 
      return true;
  
    for (node = mpActiveNodes; node; node= node->mpNextActiveNode) 
    {
      int       i;
      int       n_links = InForwardPass() ? node->mNLinks : node->mNBackLinks;
      Link *    links   = InForwardPass() ? node->mpLinks : node->mpBackLinks;
  
      for (i=0; i <n_links; i++)
      {
        if (links[i].mpNode->mAux2 != HasCycleCounter &&
          links[i].mpNode != (InForwardPass() ? mpLast : mpFirst) &&
          (!(links[i].mpNode->mType & NT_MODEL)
          || links[i].mpNode->mType & NT_TEE)) 
        {
          printf("Active node %d has nonactive non-model succesor %d\n",
                 (int) (node - mpFirst), (int) (links[i].mpNode - mpFirst));
          return false;
        }
      }
    }
    return true;
  }
  #endif
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  MarkWordNodesLeadingFrom(Node *node)
  {
    int       i;
    int       n_links = InForwardPass() ? node->mNLinks : node->mNBackLinks;
    Link *    links   = InForwardPass() ? node->mpLinks : node->mpBackLinks;
  
    for (i = 0; i < n_links; i++) 
    {
      Node *lnode = links[i].mpNode;
      
      if ((lnode->mType & NT_MODEL && !(lnode->mType & NT_TEE))
        || (lnode == (InForwardPass() ? mpLast : mpFirst))) 
      {
        continue;
      }
        
      if (lnode->mIsActiveNode > 0) 
        continue;
  
      if (lnode->mIsActive) 
      {
        assert(lnode->mType & NT_TEE);
        continue;
      }
      
      if(lnode->mIsActiveNode-- == 0) 
      {
        lnode->mAux = 0;
        MarkWordNodesLeadingFrom(lnode);
      }
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  Node *
  Network::
  pActivateWordNodesLeadingFrom(Node * pNode)
  {
    int     i;
    int     n_links = InForwardPass() ? pNode->mNLinks : pNode->mNBackLinks;
    Link *  links   = InForwardPass() ? pNode->mpLinks : pNode->mpBackLinks;
  
    for (i = 0; i < n_links; i++) 
    {
      Node * lnode = links[i].mpNode;
      
      if ((lnode->mType & NT_MODEL && !(lnode->mType & NT_TEE))
        || (lnode == (InForwardPass() ? mpLast : mpFirst))) 
      {
        continue;
      }
        
      if (lnode->mIsActiveNode++ > 0) 
        continue;
  
      if (lnode->mIsActive) 
      {
        assert(lnode->mType & NT_TEE);
        continue;
      }
  
      lnode->mAux++;
      
      if (lnode->mIsActiveNode < 0) 
        continue;
  
      assert(lnode->mIsActiveNode == 0);
      
      lnode->mIsActiveNode    = lnode->mAux;
      lnode->mpNextActiveNode = pNode->mpNextActiveNode;
      lnode->mpPrevActiveNode = pNode;
      
      if (pNode->mpNextActiveNode) 
        pNode->mpNextActiveNode->mpPrevActiveNode = lnode;
        
      pNode->mpNextActiveNode  = lnode;
      pNode = pActivateWordNodesLeadingFrom(lnode);
    }
    
    assert(!HasCycle());
    return pNode;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void
  Network:: 
  ActivateModel(Node * pNode)
  {
    if (pNode->mIsActive) return;
    pNode->mIsActive = 1;
    pNode->mpPrevActiveModel = NULL;
    pNode->mpNextActiveModel = mpActiveModels;
    if (mpActiveModels != NULL) {
      mpActiveModels->mpPrevActiveModel = pNode;
    }
    mpActiveModels = pNode;
  
    if (pNode->mIsActiveNode) {
      assert(pNode->mType & NT_TEE);
      return;
    }
  
    pNode->mIsActiveNode = 1;
    pNode->mpPrevActiveNode = NULL;
    pNode->mpNextActiveNode = mpActiveNodes;
    if (mpActiveNodes != NULL) {
      mpActiveNodes->mpPrevActiveNode = pNode;
    }
    mpActiveNodes = pNode;
  
    assert(!HasCycle());
    MarkWordNodesLeadingFrom(pNode);
    pActivateWordNodesLeadingFrom(pNode);
    assert(AllWordSuccessorsAreActive());
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void
  Network:: 
  DeactivateWordNodesLeadingFrom(Node *pNode)
  {
    int       i;
    int       n_links = InForwardPass() ? pNode->mNLinks : pNode->mNBackLinks;
    Link *    links   = InForwardPass() ? pNode->mpLinks : pNode->mpBackLinks;
  
    for (i = 0; i < n_links; i++) 
    {
      Node *lnode = links[i].mpNode;
      
      if (lnode->mType & NT_MODEL && !(lnode->mType & NT_TEE)) 
        continue;
      
      assert(!(lnode->mType & NT_TEE) || lnode->mIsActiveNode);
      
      if (--lnode->mIsActiveNode) 
        continue;
  
      if (lnode->mType & NT_TEE && lnode->mIsActive) 
        return;
  
      DeactivateWordNodesLeadingFrom(lnode);
      assert(lnode->mpPrevActiveNode);
      
      lnode->mpPrevActiveNode->mpNextActiveNode = lnode->mpNextActiveNode;
      if (lnode->mpNextActiveNode)
        lnode->mpNextActiveNode->mpPrevActiveNode = lnode->mpPrevActiveNode;
    }
    assert(!HasCycle());
    assert(AllWordSuccessorsAreActive());
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void
  Network:: 
  DeactivateModel(Node *pNode)
  {
    if (!pNode->mIsActive) 
      return;
      
    pNode->mIsActive = 0;
  
    if (pNode->mpNextActiveModel != NULL) {
      pNode->mpNextActiveModel->mpPrevActiveModel = pNode->mpPrevActiveModel;
    }
  
    if (pNode->mpPrevActiveModel != NULL) {
      pNode->mpPrevActiveModel->mpNextActiveModel = pNode->mpNextActiveModel;
    } else {
      assert(mpActiveModels == pNode);
      mpActiveModels = pNode->mpNextActiveModel;
    }
  
    assert(!HasCycle());
    if (pNode->mType & NT_TEE && pNode->mIsActiveNode) return;
  
    pNode->mIsActiveNode = 0;
    DeactivateWordNodesLeadingFrom(pNode);
  
    if (pNode->mpNextActiveNode != NULL) {
      pNode->mpNextActiveNode->mpPrevActiveNode = pNode->mpPrevActiveNode;
    }
  
    if (pNode->mpPrevActiveNode != NULL) {
      pNode->mpPrevActiveNode->mpNextActiveNode = pNode->mpNextActiveNode;
    } else {
      assert(mpActiveNodes == pNode);
      mpActiveNodes = pNode->mpNextActiveNode;
    }
  
    assert(!HasCycle());
  }
  
  
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  PassTokenMax(Token *from, Token *to, FLOAT mLike)
  {
    int ret = 0;
    
  #ifdef TRACE_TOKENS
    printf("(%.2f + %.2f -> %.2f = ", from->mLike, mLike, to->mLike);
  #endif
    
    if (!to->IsActive() || from->mLike + mLike > to->mLike) 
    {
      KillToken(to);
  
      ret = 1;
      *to = *from;
      to->mLike += mLike;
      
      if (to->mpWlr) 
        to->mpWlr->mNReferences++;
    }
  #ifdef TRACE_TOKENS
    printf("%.2f)\n", to->mLike);
  #endif
    return ret;
  }
  
  /*int PassTokenSum(Token *from, Token *to, FLOAT mLike)
  {
    double tl;
    int ret = 0;
  #ifdef TRACE_TOKENS
    printf("(%.2f + %.2f -> %.2f = ", from->mLike, mLike, to->mLike);
  #endif
    if (IS_ACTIVE(*to)) {
      tl = LogAdd(to->mLike, from->mLike + mLike);
    } else {
      tl = from->mLike + mLike;
    }
  
    if (!IS_ACTIVE(*to) || from->mLike + mLike > to->mBestLike) {
      KillToken(to);
  
      ret = 1;
      *to = *from;
      to->mBestLike = from->mLike + mLike;
      if (to->mpWlr) {
        to->mpWlr->mNReferences++;
      }
    }
  
    to->mLike = tl;
  #ifdef TRACE_TOKENS
    printf("%.2f)\n", to->mLike);
  #endif
    return ret;
  }*/
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  PassTokenSum(Token *from, Token *to, FLOAT like)
  {
    double        tl;
    FloatInLog    fe;
    FloatInLog    fil_from_like = {like, 0};
    int           ret = 0;
    
  #ifdef TRACE_TOKENS
    printf("(%.2f + %.2f -> %.2f = ", from->mLike, like, to->mLike);
  #endif
    
    if (to->IsActive()) 
    {
      tl = LogAdd(to->mLike, from->mLike + like);
      fe = FIL_Add(to->mAccuracy, FIL_Mul(from->mAccuracy, fil_from_like));
    } 
    else 
    {
      tl = from->mLike + like;
      fe = FIL_Mul(from->mAccuracy, fil_from_like);
    }
  
    if (!to->IsActive() || from->mLike + like > to->mBestLike) 
    {
      KillToken(to);
  
      ret = 1;
      *to = *from;
      to->mBestLike = from->mLike + like;
      
      if (to->mpWlr) 
      {
        to->mpWlr->mNReferences++;
      }
    }
  
    to->mLike     = tl;
    to->mAccuracy = fe;
  #ifdef TRACE_TOKENS
    printf("%.2f)\n", to->mLike);
  #endif
    return ret;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  FreeWordLinkRecords(WordLinkRecord * wlr)
  {
    if (wlr != NULL) {
      --wlr->mNReferences;
      assert(wlr->mNReferences >= 0);
  
      if (wlr->mNReferences == 0) {
        FreeWordLinkRecords(wlr->mpNext);
  #ifdef DEBUG_MSGS
        assert(wlr->mIsFreed == false);
        wlr->mIsFreed = true;
  #else
        free(wlr);
  #endif
      }
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  KillToken(Token *token)
  {
    token->mLike = LOG_0;
    FreeWordLinkRecords(token->mpWlr);
    token->mpWlr  = NULL;
    token->mpTWlr = NULL;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  DiagCGaussianDensity(Mixture *mix, FLOAT  *obs, Network *net)
  {
    FLOAT mLike = 0.0;
    size_t j;
  
    if (net && net->mpMixPCache[mix->mID].mTime == net->mTime) 
    {
      return net->mpMixPCache[mix->mID].mValue;
    }
  
    for (j = 0; j < mix->mpMean->VectorSize(); j++) 
    {
      mLike += SQR(obs[j] - mix->mpMean->mpVectorO[j]) * mix->mpVariance->mpVectorO[j];
    }
  
    mLike = -0.5 * (mix->GConst() + mLike);
  
    if (net) 
    {
      net->mpMixPCache[mix->mID].mTime  = net->mTime;
      net->mpMixPCache[mix->mID].mValue = mLike;
    }
  
    return mLike;
  }
  
#ifdef DEBUG_MSGS
  int gaus_computaions = 0;
#endif
  
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  DiagCGaussianMixtureDensity(State *state, FLOAT *obs, Network *net)
  {
    size_t  i;
    FLOAT   mLike = LOG_0;
  
    assert(state->mOutPdfKind == KID_DiagC);
  
    if (net && net->mpOutPCache[state->mID].mTime == net->mTime) 
    {
      return net->mpOutPCache[state->mID].mValue;
    }
  
    for (i = 0; i < state->mNumberOfMixtures; i++) 
    {
      FLOAT glike;
      Mixture *mix = state->mpMixture[i].mpEstimates;
  
      obs = XformPass(mix->mpInputXform, obs,
                      net ? net->mTime : UNDEF_TIME,
                      net ? net->mPropagDir : FORWARD);
  
      assert(obs != NULL);
      glike = DiagCGaussianDensity(mix, obs, net);
      mLike  = LogAdd(mLike, glike + state->mpMixture[i].mWeight);
    }
  
#ifdef DEBUG_MSGS
    gaus_computaions++;
#endif
  
    if (net) {
      net->mpOutPCache[state->mID].mTime = net->mTime;
      net->mpOutPCache[state->mID].mValue = mLike;
    }
    return mLike;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  FromObservationAtStateId(State *state, FLOAT *obs, Network *net)
  {
    obs = XformPass(net->mpModelSet->mpInputXform, obs,
                    net ? net->mTime : UNDEF_TIME,
                    net ? net->mPropagDir : FORWARD);
    assert(obs != NULL);
    return obs[state->PDF_obs_coef];
  }
  
  
  
  //***************************************************************************
  //***************************************************************************
  #ifdef DEBUG_MSGS
  void 
  PrintNumOfWLR() 
  {
    WordLinkRecord *  wlr = firstWLR;
    int               nwlrs = 0;
    int               nwlrsnf = 0;
    
    while (wlr) 
    {
      nwlrs++;
      
      if (!wlr->mIsFreed)
        nwlrsnf++;
      
      wlr = wlr->mpTmpNext;
    }
    printf("%d Released: %d\n", nwlrsnf, nwlrs - nwlrsnf);
  }
  #endif
  
  
  
  //***************************************************************************
  //***************************************************************************
  /*void BaumWelchInit(ModelSet *pHmms) 
  {
    //ResetAccumsForHMMSet(pHmms);
    pHmms.ResetAccums();
  }*/
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  UpdateXformStatCache(XformStatCache * xfsc,
                       Xform *          topXform, //to locate positions in input vector
                       FLOAT *          input) 
  {
    size_t    i;
    size_t    j;
    size_t    size = xfsc->mpXform->mInSize;
  
    if (topXform == NULL)
      return;
  
    if (topXform == xfsc->mpXform) 
    {
      if (xfsc->mNorm > 0) 
      {
        for (i=0; i < size; i++) 
        {
          xfsc->mpStats[i] += input[i];
          for (j=0; j <= i; j++) {
            xfsc->mpStats[size + i*(i+1)/2 + j] += input[i] * input[j];
          }
        }
      } 
      else 
      {
        for (i=0; i < size; i++) 
        {
          xfsc->mpStats[i] = input[i];
          for (j=0; j <= i; j++) 
          {
            xfsc->mpStats[size + i*(i+1)/2 + j] = input[i] * input[j];
          }
        }
      }
      xfsc->mNorm++;
    } 
    
    else if (topXform->mXformType == XT_COMPOSITE) 
    {
      CompositeXform *cxf = (CompositeXform *) topXform;
      for (i=0; i<cxf->mpLayer[0].mNBlocks; i++) 
      {
        UpdateXformStatCache(xfsc, cxf->mpLayer[0].mpBlock[i], input);
        input += cxf->mpLayer[0].mpBlock[i]->mInSize;
      }
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  UpdateXformInstanceStatCaches(XformInstance *xformInstance,
                                FLOAT *observation, int time)
  {
    size_t i, j;
    FLOAT *obs;
  
    if (xformInstance == NULL || xformInstance->mStatCacheTime == time) return;
  
    xformInstance->mStatCacheTime = time;
  
    if (xformInstance->mNumberOfXformStatCaches == 0) return;
  
    if (xformInstance->mpInput) {
      UpdateXformInstanceStatCaches(xformInstance->mpInput, observation, time);
    }
  
    for (i = 0; i < xformInstance->mNumberOfXformStatCaches; i++) {
      XformStatCache *xfsc = &xformInstance->mpXformStatCache[i];
  
      if (xfsc->mpUpperLevelStats != NULL &&
        xfsc->mpUpperLevelStats->mpStats == xfsc->mpStats) { //just link to upper level?
        xfsc->mNorm = xfsc->mpUpperLevelStats->mNorm;
        continue;
      }
  
      obs = XformPass(xformInstance->mpInput, observation, time, FORWARD);
      xfsc->mNorm = 0;
      UpdateXformStatCache(xfsc, xformInstance->mpXform, obs);
      if (xfsc->mpUpperLevelStats != NULL) {
        size_t size = xfsc->mpXform->mInSize;
        for (j = 0; j < size + size*(size+1)/2; j++) {
          xfsc->mpStats[j] += xfsc->mpUpperLevelStats->mpStats[j];
        }
        xfsc->mNorm += xfsc->mpUpperLevelStats->mNorm;
      }
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  ReestState(
    Node*         pNode,
    int           stateIndex, 
    FLOAT         logPriorProb, 
    FLOAT         updateDir,
    FLOAT*        pObs, 
    FLOAT*        pObs2) 
  {
    size_t        i;
    size_t        j;
    size_t        k;
    size_t        m;
    State*        state  = pNode->mpHmm->        mpState[stateIndex];
    State*        state2 = pNode->mpHmmToUpdate->mpState[stateIndex];
    FLOAT         bjtO   = LOG_0;
    size_t        n_mixtures;
  
    if (!mpModelSetToUpdate->mGaussLvl2ModelReest 
    && ( mpModelSet != mpModelSetToUpdate))
    {
      // Occupation probabilities of mixtures are computed using target model
      state = state2; pObs = pObs2;
    } 
    else if (state->mNumberOfMixtures <= state2->mNumberOfMixtures) 
    {
      bjtO = mpOutPCache[mpModelSet->mNStates * mTime + state->mID].mValue;
    }
  
    n_mixtures = LOWER_OF(state->mNumberOfMixtures, state2->mNumberOfMixtures);
  
    if (bjtO < LOG_MIN && n_mixtures > 1)
    {
      // State likelihood was not available in cache because
      // - occupation probabilities of mixtures are computed using target model
      // - not all mixtures of mAlignment model are used for computation of
      //   occupation probabilities (state2->num_mix < state->num_mix)
      for (m = 0; m < n_mixtures; m++)
      {
        Mixture *mix  = state->mpMixture[m].mpEstimates;
        FLOAT  cjm    = state->mpMixture[m].mWeight;
        FLOAT* xobs   = XformPass(mix->mpInputXform, pObs, mTime, FORWARD);
        FLOAT  bjmtO  = DiagCGaussianDensity(mix, xobs, this);
        bjtO          = LogAdd(bjtO, cjm + bjmtO);
      }
    }
  
    //for every emitting state
    for (m = 0; m < n_mixtures; m++)
    { 
      FLOAT Lqjmt = logPriorProb;
      
      if (n_mixtures > 1)
      {
        Mixture*  mix   = state->mpMixture[m].mpEstimates;
        FLOAT     cjm   = state->mpMixture[m].mWeight;
        FLOAT*    xobs  = XformPass(mix->mpInputXform, pObs, mTime, FORWARD);
        FLOAT     bjmtO = DiagCGaussianDensity(mix, xobs, this);
        
        Lqjmt +=  -bjtO + cjm + bjmtO;
      }
  
      if (Lqjmt > MIN_LOG_WEGIHT) 
      {
        Mixture*  mix      = state2->mpMixture[m].mpEstimates;
        size_t    vec_size = mix->mpMean->VectorSize();
        FLOAT*    mnvec    = mix->mpMean->mpVectorO;
        FLOAT*    mnacc    = mix->mpMean->mpVectorO     +     vec_size;
        FLOAT*    vvacc    = mix->mpVariance->mpVectorO +     vec_size;
        FLOAT*    vmacc    = mix->mpVariance->mpVectorO + 2 * vec_size;
        XformInstance* ixf = mix->mpInputXform;
        FLOAT*    xobs     = XformPass(ixf, pObs2, mTime, FORWARD);
  
/*      if (mmi_den_pass) {
          mnacc += vec_size + 1;
          vvacc += 2 * vec_size + 1;
          vmacc += 2 * vec_size + 1;
        }*/
  
        // the real state occupation probability
        Lqjmt = exp(Lqjmt) * updateDir;
  
        // Update
        for (i = 0; i < vec_size; i++) 
        {                 
          mnacc[i] += Lqjmt * xobs[i];                  // mean
          if (mpModelSetToUpdate->mUpdateMask & UM_OLDMEANVAR) 
          {
            vvacc[i] += Lqjmt * SQR(xobs[i]-mnvec[i]);  // var
          } 
          else 
          {
            vvacc[i] += Lqjmt * SQR(xobs[i]);           // scatter
            vmacc[i] += Lqjmt * xobs[i];                // mean for var.
          }
        }
  
        mnacc[vec_size] += Lqjmt; //norms for mean
        vmacc[vec_size] += Lqjmt; //norms for variance
        
        // collect statistics for cluster weight vectors 
        if (0 < mix->mpMean->mCwvAccum.Cols())
        {
          for (size_t vi = 0; vi < mix->mpMean->mCwvAccum.Rows(); vi++)
          {
            mix->mpMean->mpOccProbAccums[vi] += Lqjmt;
            for (size_t vj = 0; vj < mix->mpMean->mCwvAccum.Cols(); vj++)
            {
              mix->mpMean->mCwvAccum[vi][vj] += xobs[vj] * Lqjmt;
            }
          }
        }
  
//      if (mmi_den_pass) {
//        state2->mpMixture[m].mWeightAccumDen += Lqjmt;
//      } else {
        state2->mpMixture[m].mWeightAccum     += Lqjmt; //  Update weight accum
//      }
  
        if (Lqjmt < 0)
          state2->mpMixture[m].mWeightAccumDen -= Lqjmt;
  
        if (ixf == NULL || ixf->mNumberOfXformStatCaches == 0) 
          continue;
  
        UpdateXformInstanceStatCaches(ixf, pObs2, mTime);
  
        for (i = 0; i < ixf->mNumberOfXformStatCaches; i++) 
        {
          XformStatCache *xfsc = &ixf->mpXformStatCache[i];
          Variance *var  = state2->mpMixture[m].mpEstimates->mpVariance;
          Mean     *mean = state2->mpMixture[m].mpEstimates->mpMean;
  
          for (j = 0; j < var->mNumberOfXformStatAccums; j++) 
          {
            XformStatAccum *xfsa = &var->mpXformStatAccum[j];
            
            if (xfsa->mpXform == xfsc->mpXform) 
            {
              size_t size = xfsc->mpXform->mInSize;
              for (k = 0; k < size+size*(size+1)/2; k++) 
              {
                xfsa->mpStats[k] += xfsc->mpStats[k] * Lqjmt;
              }
              xfsa->mNorm += xfsc->mNorm * Lqjmt;
              break;
            }
          }
  
          for (j = 0; j < mean->mNumberOfXformStatAccums; j++) 
          {
            XformStatAccum *xfsa = &mean->mpXformStatAccum[j];
            
            if (xfsa->mpXform == xfsc->mpXform) 
            {
              size_t size = xfsc->mpXform->mInSize;
              for (k = 0; k < size; k++) 
              {
                xfsa->mpStats[k] += xfsc->mpStats[k] * Lqjmt;
              }
              xfsa->mNorm += xfsc->mNorm * Lqjmt;
              break;
            }
          }
        }
      }
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  /*FLOAT *
  StateOccupationProbability(Network *net, FLOAT *obsMx, ModelSet *hmms,
                             int nFrames, FLOAT **outProbOrMahDist, int getMahalDist)
  {
    int i, j, k;
    int nNetModels;
    int nEmitingStates;
    FLOAT totalLike;
    FLOAT *occupProb;
  
    FLOAT *beta = (FLOAT *) malloc(net->mNumberOfNetStates * (nFrames+1) * sizeof(FLOAT));
    FLOAT *alfa = (FLOAT *) malloc(net->mNumberOfNetStates * (nFrames+1) * sizeof(FLOAT));
    Cache *outPCache = (Cache *) malloc(nFrames * hmms->mNStates * sizeof(Cache));
  
    if (outPCache == NULL || beta == NULL || alfa == NULL) {
      Error("Insufficient memory");
    }
  
    for (i = 0; i < net->mNumberOfNetStates * (nFrames+1); i++) {
      beta[i] = alfa[i] = LOG_0;
    }
  
    nNetModels = 0;
    for (i=0; i < net->nnodes; i++) {
      if (net->mpNodes[i].type & NT_MODEL) nNetModels++;
    }
  
    nEmitingStates = net->mNumberOfNetStates - 2 * nNetModels;
  
    for (i = 0; i < nFrames * hmms->mNStates; i++) {
      outPCache[i].mTime = UNDEF_TIME;
      outPCache[i].mValue = LOG_0;
    }
  
    free(net->mpOutPCache);
  
  
    occupProb = (FLOAT *) malloc(nEmitingStates * nFrames * sizeof(FLOAT));
    if (occupProb == NULL) {
      Error("Insufficient memory");
    }
  
    if (outProbOrMahDist != NULL) {
      *outProbOrMahDist = (FLOAT *) malloc(nEmitingStates * nFrames * sizeof(FLOAT));
      if (*outProbOrMahDist == NULL) {
        Error("Insufficient memory");
      }
    }
  
    net->PassTokenInModel   = &PassTokenSum;
    net->PassTokenInNetwork = &PassTokenSum;
    net->mAlignment          = NO_ALIGNMENT;
  
    //Backward Pass
    net->mPropagDir = BACKWARD;
  
    net->mTime = nFrames;
    TokenPropagationInit(net,
                        beta + net->mNumberOfNetStates * net->mTime,
                        NULL);
  
    for (i = nFrames-1; i >= 0; i--) {
  
      net->mpOutPCache = outPCache + hmms->mNStates * i;
      TokenPropagationInModels(net,  obsMx + hmms->mInputVectorSize * i,
                              beta + net->mNumberOfNetStates * net->mTime,
                              NULL);
  
      if (outProbOrMahDist != NULL && getMahalDist !=4) {
        int state_counter = 0;
  
        for (k=0; k < net->nnodes; k++) {
          Node *node = &net->mpNodes[k];
          if (node->mType & NT_MODEL) {
            for (j = 0; j < node->mpHmm->mNStates - 2; j++, state_counter++) {
              FLOAT tmpf = net->OutputProbability(node->mpHmm->mpState[j],
                                        obsMx + hmms->mInputVectorSize * i, net);
              switch (getMahalDist) {
                case 0:
                  break;
                case 1:
                tmpf = log((tmpf / -0.5) - node->mpHmm->mpState[j]->mpMixture[0].mpEstimates->GConst()) * -0.5;
                break;
                case 2:
                tmpf = log((tmpf / -0.5) - node->mpHmm->mpState[j]->mpMixture[0].mpEstimates->GConst()) * -1;
                break;
                case 3:
                tmpf += node->mpHmm->mpState[j]->mpMixture[0].mpEstimates->GConst() * 0.5;
  //               tmpf /= hmms->mInputVectorSize;
                break;
              }
  
              (*outProbOrMahDist)[i * nEmitingStates + state_counter] = tmpf;
            }
          }
        }
      }
  
      net->mTime--;
      TokenPropagationInNetwork(net,
                                beta + net->mNumberOfNetStates * net->mTime, NULL);
    }
  
    if (!IS_ACTIVE(*net->mpFirst->mpExitToken)) {
      TokenPropagationDone(net);
      net->mpOutPCache = outPCache;
      free(alfa);
      for (i=0; i < net->mNumberOfNetStates * nFrames; i++) {
        beta[i] = LOG_0;
      }
      return beta; // No token survivered
    }
  
    totalLike = net->mpFirst->mpExitToken->mLike;
  //  totalLike = 0;
    TokenPropagationDone(net);
  
    //Forward Pass
    net->mPropagDir = FORWARD;
    net->mTime = 0;
    TokenPropagationInit(net,
                        beta + net->mNumberOfNetStates * net->mTime,
                        alfa + net->mNumberOfNetStates * net->mTime);
  
    for (i = 0; i < nFrames; i++) {
      net->mpOutPCache = outPCache + hmms->mNStates * i;
      net->mTime++;
      TokenPropagationInModels(net,  obsMx + hmms->mInputVectorSize * i,
                              beta + net->mNumberOfNetStates * net->mTime,
                              alfa + net->mNumberOfNetStates * net->mTime);
  
      TokenPropagationInNetwork(net,
                                beta + net->mNumberOfNetStates * net->mTime,
                                alfa + net->mNumberOfNetStates * net->mTime);
  
      if (outProbOrMahDist != NULL && getMahalDist == 4) {
        for (j=0; j < nEmitingStates; j++) {
          (*outProbOrMahDist)[i * nEmitingStates + j] = totalLike;
        }
      }
    }
  
    net->mpOutPCache = outPCache;
  
    TokenPropagationDone(net);
  
    for (i = 1; i <= nFrames; i++) {
      int state_counter = 0;
  
      for (k=0; k < net->nnodes; k++) {
        Node *node = &net->mpNodes[k];
        if (node->mType & NT_MODEL) {
          for (j = 0; j < node->mpHmm->mNStates - 2; j++, state_counter++) {
            int idx = (net->mNumberOfNetStates * i) + node->mEmittingStateId + j + 1;
            FLOAT occp = beta[idx] + alfa[idx] - totalLike;
            occupProb[(i-1) * nEmitingStates + state_counter] = occp < LOG_MIN ?
                                                                LOG_0 : occp;
          }
        }
      }
    }
  
    free(alfa); free(beta);
    return occupProb;
  }*/
  
  

  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  TokenPropagationInit()
  {
    size_t  i;
    Node *  node;
  
    InitLogMath();
  
    for (node = mpFirst; node != NULL; node = node->mpNext) {
      size_t numOfTokens = (node->mType & NT_MODEL) ? node->mpHmm->mNStates : 1;
  
      for (i=0; i < numOfTokens; i++) {
        node->mpTokens[i].mLike = LOG_0;
        node->mpTokens[i].mpWlr = NULL;
  #ifdef bordel_staff
        node->mpTokens[i].mpTWlr = NULL;
  #endif // bordel_staff
      }
      node->mIsActive = 0;
      node->mIsActiveNode = 0;
    }
  
    if (mpOutPCache != NULL) {
      for (i = 0; i < mpModelSet->mNStates; i++) {
        mpOutPCache[i].mTime = UNDEF_TIME;
      }
    }
  
    if (mpMixPCache != NULL) {
      for (i = 0; i < mpModelSet->mNMixtures; i++) {
        mpMixPCache[i].mTime = UNDEF_TIME;
      }
    }

    mpBestToken  = NULL;
    mWordThresh = LOG_MIN;
    mpActiveModels = NULL;
    mpActiveNodes = NULL;
    mActiveTokens = 0;
  //  mTime = 0;
    node = InForwardPass() ? mpFirst : mpLast;
    node->mpTokens[0].mLike = 0;
    node->mpTokens[0].mAccuracy.logvalue = LOG_0;
    node->mpTokens[0].mAccuracy.negative = 0;
    node->mpTokens[0].mpWlr = NULL;
  #ifdef bordel_staff
    node->mpTokens[0].mpTWlr = NULL;
    node->mpTokens[0].mBestLike = 0;
  #endif
  
    if (mCollectAlphaBeta && InForwardPass()) 
    {
      for (node = mpFirst; node != NULL; node = node->mpNext) 
      {
        if (!(node->mType & NT_MODEL)) 
          continue;
          
        node->mpAlphaBetaList = node->mpAlphaBetaListReverse = NULL;
      }
    }
    
    // Needed to load last FWBWRs to mpAlphaBetaList
    if (mCollectAlphaBeta && !InForwardPass()) 
    {
      for (node = mpFirst; node != NULL; node = node->mpNext) 
      {
        if (!(node->mType & NT_MODEL)) 
          continue;
          
        BackwardPruning(mTime, node, node->mpHmm->mNStates-1);
      }
    }
  
    node = InForwardPass() ? mpFirst : mpLast;
    mpActiveNodes = node;
    node->mpPrevActiveNode = node->mpNextActiveNode = NULL;
    node->mIsActiveNode = 1;
  
    MarkWordNodesLeadingFrom(node);
    pActivateWordNodesLeadingFrom(node);
    TokenPropagationInNetwork();
    DeactivateWordNodesLeadingFrom(node);
    
    node->mIsActiveNode = 0;
    
    if (node->mpPrevActiveNode) 
    {
      node->mpPrevActiveNode->mpNextActiveNode = node->mpNextActiveNode;
    }  
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  TokenPropagationInNetwork()
  {
    Node *  node;
    Link *  links;
    int     i;
    int     n_links;
  
    // Beam pruning is not active in backward pass. First, it is not necessary
    // since only token that fit into the forward pass beam are allowed (backward
    // pruning after forward pass). Second, it could be dangerous. For example,
    // when training from lattices where LM weights are pushed to the begining,
    // the lowest weight (the biggest penalty) will be on the wery first link.
    // If this weight was lower than minus pruning treshold, it would result in
    // always killing token in the first node, when pruning during the backward pass.
    //                                     |
    mBeamThresh = InForwardPass() && // <--'
                  mpBestToken &&
                  mpBestToken->mLike - mPruningThresh > LOG_MIN
                      ? mpBestToken->mLike - mPruningThresh : LOG_MIN;
  
    node = InForwardPass() ? mpLast : mpFirst;
    
    KillToken(node->mpExitToken);
  
  //  Node *Xnode = mpActiveNodes;
  /*  for (node = InForwardPass() ? mpFirst : mpLast;
        node != NULL;
        node = InForwardPass() ? node->mpNext : node->mpBackNext) { //*/
    for (node = mpActiveNodes; node != NULL; node = node->mpNextActiveNode) 
    {
      if ((node->mType & NT_TEE) && node->mpTokens[0].IsActive()) 
      {
  //        assert(node->mIsActiveNode || (node->mType & NT_TEE && node->mIsActive));
  //        for (Xnode = mpActiveNodes; Xnode && Xnode != node; Xnode = Xnode->mpNextActiveNode);
  //        assert(Xnode);
  
        if (!(mCollectAlphaBeta && !InForwardPass() && // backward pruning
            BackwardPruning(mTime, node, 0))) 
        {   // after forward pass
          Hmm *hmm = node->mpHmm;
          FLOAT transP = hmm->mpTransition->mpMatrixO[hmm->mNStates - 1];
    #ifdef TRACE_TOKENS
          printf("Tee model State 0 -> Exit State ");
    #endif
          PassTokenInModel(&node->mpTokens[0], node->mpExitToken,
                                          transP * mTranScale);
        }
      }
  
      if (node->mpExitToken->IsActive()) 
      {
  //      assert(node->mIsActiveNode || (node->mType & NT_TEE && node->mIsActive));
  //      for (Xnode = mpActiveNodes; Xnode && Xnode != node; Xnode = Xnode->mpNextActiveNode);
  //      assert(Xnode);
        if (node->mType & NT_MODEL) 
        {
          if (mCollectAlphaBeta) 
          {
            if (InForwardPass()) 
              WriteAlpha(mTime, node, node->mpHmm->mNStates-1, node->mpExitToken);
            else 
              WriteBeta(mTime, node, 0, node->mpExitToken);
          }
          node->mpExitToken->mLike += mMPenalty;
        } 
        else if (node->mType & NT_WORD && node->mpPronun != NULL) 
        {
          node->mpExitToken->mLike += mWPenalty +
                                  mPronScale * node->mpPronun->prob;
          /*if (node->mpExitToken->mLike < mWordThresh) {
            node->mpExitToken->mLike = LOG_0;
          }*/
        }
  
        if (node->mpExitToken->mLike > mBeamThresh) 
        {
          if (node->mType & NT_WORD && node->mpPronun != NULL &&
            mAlignment & WORD_ALIGNMENT) 
          {
            node->mpExitToken->AddWordLinkRecord(node, -1, mTime);
          } 
          else if (node->mType & NT_MODEL && mAlignment & MODEL_ALIGNMENT) 
          {
            node->mpExitToken->AddWordLinkRecord(node, -1, mTime);
          }
  
          n_links = InForwardPass() ? node->mNLinks : node->mNBackLinks;
          links  = InForwardPass() ? node->mpLinks  : node->mpBackLinks;
  
          for (i = 0; i < n_links; i++) 
          {
            FLOAT lmLike = links[i].mLike * mLmScale;
            
            if (node->mpExitToken->mLike + lmLike > mBeamThresh 
                && (/*links[i].mpNode->mStart == UNDEF_TIME ||*/
                      links[i].mpNode->mStart <= mTime) 
                      
                && (  links[i].mpNode->mStop  == UNDEF_TIME        ||
                      links[i].mpNode->mStop  >= mTime)
                      
                && (  mSearchPaths != SP_TRUE_ONLY || 
                      (node->mType & NT_TRUE)                   || 
                      !(links[i].mpNode->mType & NT_MODEL))) 
            {
  #           ifdef TRACE_TOKENS
              printf("Node %d -> Node %d ", node->mAux, links[i].mpNode->mAux);
  #           endif
              PassTokenInNetwork(node->mpExitToken,
                                      &links[i].mpNode->mpTokens[0], lmLike);
              if (links[i].mpNode->mType & NT_MODEL) 
              {
                ActivateModel(links[i].mpNode);
              } 
              else 
              {
                assert(links[i].mpNode->mIsActiveNode ||
                       links[i].mpNode == (InForwardPass() ? mpLast : mpFirst));
              }
            }
          }
        }
        
        if (!(node->mType & NT_STICKY)) 
          KillToken(node->mpExitToken);
      }  
    }
  
  
    if (mCollectAlphaBeta) 
    {
      if (InForwardPass()) 
      {
        for (node = mpActiveModels; node != NULL; node = node->mpNextActiveModel) 
        {
          if (/*!(node->mType & NT_MODEL) ||*/ node->mpTokens[0].mLike < LOG_MIN) 
            continue;
            
          WriteAlpha(mTime, node, 0, &node->mpTokens[0]);
        }
      } 
      else 
      {
        for (node = mpActiveModels; node != NULL; node = node->mpNextActiveModel) 
        {
          if (/*!(node->mType & NT_MODEL) ||*/ node->mpTokens[0].mLike < LOG_MIN
            || BackwardPruning(mTime, node, node->mpHmm->mNStates-1)) 
            continue;
            
          WriteBeta(mTime, node, node->mpHmm->mNStates-1, &node->mpTokens[0]);          
        }
      }
    }
  
  //  Go through newly activeted models and costruct list of active nodes
  //  for (node=mpActiveModels; node&&!node->mIsActive; node=node->mpNextActiveModel) {
  //    ActivateNode(net, node);
  //  }
    assert(!HasCycle());
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void
  Network::
  TokenPropagationInModels(FLOAT *observation)
  {
    Node *  node;
    Hmm *   hmm;
    size_t  winingToken = 0;
    size_t  i;
    size_t  j;
    int     from;
    int     to;
  //  int estate_id;
    int     state_idx;
  //  FLOAT threshOutProb = LOG_0;
  
    mpBestToken = NULL;
  /*  if (mpThreshState) {
      threshOutProb = OutputProbability(mpThreshState, observation, net);
    } */
  
    for (node = mpActiveModels; node != NULL; node = node->mpNextActiveModel) {
  //  for (node = mpFirst; node != NULL; node = node->mpNext) {
  //    if (!(node->mType & NT_MODEL)) continue;
  
      hmm = node->mpHmm;
  
      if (    (/*node->mStart != UNDEF_TIME &&*/node->mStart >= mTime)
          ||  (  node->mStop  != UNDEF_TIME &&  node->mStop  <  mTime)
          ||  mSearchPaths == SP_TRUE_ONLY && !(node->mType & NT_TRUE)) 
      {
        for (i = 0; i < hmm->mNStates-1; i++) 
        {
          KillToken(&node->mpTokens[i]);
        }
        
        DeactivateModel(node);
        continue;
      }
  
      if (mAccumType == AT_MPE && InForwardPass() && node->mpTokens[0].IsActive()) 
      {
        FloatInLog fil_lmpa =
          {node->mpTokens[0].mLike + log(fabs(node->mPhoneAccuracy)),
          node->mPhoneAccuracy < 0};
  
        node->mpTokens[0].mAccuracy = FIL_Add(node->mpTokens[0].mAccuracy, fil_lmpa);
      }
  
      for (i = 0; i < hmm->mNStates-1; i++) 
      {
        assert(!mpAuxTokens[i].IsActive());
        if (node->mpTokens[i].IsActive()) 
        {
          mpAuxTokens[i] = node->mpTokens[i];
          node->mpTokens[i].mLike = LOG_0;
          node->mpTokens[i].mpWlr  = NULL;
        }
      }
  
      int keepModelActive = false;
  
      assert(!node->mpTokens[hmm->mNStates-1].IsActive());
  
      for (j = 1; j < hmm->mNStates-1; j++) 
      {
        state_idx = (InForwardPass() ? j : hmm->mNStates-1 - j);
  
        if (mCollectAlphaBeta &&
          !InForwardPass() &&
          BackwardPruning(mTime, node, state_idx)) 
        {
          continue; // backward pruning after forward pass
        }
  
        for (i = 0; i < hmm->mNStates-1; i++) 
        {
          from = InForwardPass() ? i : hmm->mNStates-1 - j;
          to   = InForwardPass() ? j : hmm->mNStates-1 - i;
  
          assert(!mpAuxTokens[i].IsActive() || node->mIsActive);
  
          if (hmm->mpTransition->mpMatrixO[from * hmm->mNStates + to] > LOG_MIN &&
              mpAuxTokens[i].IsActive()) 
          {
            FLOAT transP = hmm->mpTransition->mpMatrixO[from * hmm->mNStates + to];
  
  #ifdef TRACE_TOKENS
            printf("Model %d State %d -> State %d ",  node->mAux, (int) i, (int) j);
  #endif
            if (PassTokenInModel(&mpAuxTokens[i], &node->mpTokens[j],
                                    transP * mTranScale)) {
              winingToken = i;
            }
          }
        }
  
        // if (IS_ACTIVE(node->mpTokens[j])) {
        if (node->mpTokens[j].mLike > mBeamThresh) 
        {
          FLOAT outProb = OutputProbability(hmm->mpState[state_idx-1],
                                                    observation, this);
          outProb *= mOutpScale;
  
          /*if (outProb < threshOutProb) {
            outProb = threshOutProb;
          }*/
  
          if (mCollectAlphaBeta && !InForwardPass())
            WriteBeta(mTime, node, state_idx, &node->mpTokens[j]);
          
          if (mAccumType == AT_MFE && node->mType & NT_TRUE) 
          {
            FloatInLog fil_like = {node->mpTokens[j].mLike, 0};
            node->mpTokens[j].mAccuracy = FIL_Add(node->mpTokens[j].mAccuracy, fil_like);
          }
  
          node->mpTokens[j].mAccuracy.logvalue += outProb;
          node->mpTokens[j].mLike              += outProb;
  
          if (mCollectAlphaBeta && InForwardPass()) 
            WriteAlpha(mTime, node, state_idx, &node->mpTokens[j]);
  
          if (mAlignment & STATE_ALIGNMENT && winingToken > 0 &&
            (winingToken != j || mAlignment & FRAME_ALIGNMENT)) 
          {
            node->mpTokens[j].AddWordLinkRecord(
                node,
                (InForwardPass() ? winingToken : hmm->mNStates-1 - winingToken)-1,
                mTime-1);            
          }
  
          mActiveTokens++;
          keepModelActive = true;
          assert(node->mIsActive);
        } 
        else 
        {
          assert(node->mIsActive || !node->mpTokens[j].IsActive());
          KillToken(&node->mpTokens[j]);
        }
      }
  
      for (i = 0; i < hmm->mNStates-1; i++) 
      {
        KillToken(&mpAuxTokens[i]);
      }
  
      if (!keepModelActive) 
        DeactivateModel(node);
  
      state_idx = (InForwardPass() ? hmm->mNStates - 1 : 0);
      assert(!node->mpTokens[hmm->mNStates - 1].IsActive());
  
      if (!keepModelActive ||
        (mCollectAlphaBeta && !InForwardPass() &&
          BackwardPruning(mTime-1, node, state_idx))) 
      {
        // backward pruning after forward pass
        continue;
        //KillToken(&node->mpTokens[hmm->mNStates - 1]);
      }
  
      for (i = 1; i < hmm->mNStates-1; i++) 
      {
        from = InForwardPass() ? i : 0;
        to   = InForwardPass() ? hmm->mNStates-1 : hmm->mNStates-1 - i;
  
        if (node->mpTokens[i].IsActive()) 
        {
          if (!mpBestToken || mpBestToken->mLike < node->mpTokens[i].mLike) 
          {
            mpBestToken = &node->mpTokens[i];
            mpBestNode  = node;
          }
  
          if (hmm->mpTransition->mpMatrixO[from * hmm->mNStates + to] > LOG_MIN) 
          {
            FLOAT transP = hmm->mpTransition->mpMatrixO[from * hmm->mNStates + to];
  #ifdef TRACE_TOKENS
            printf("Model %d State %d -> Exit State ",  node->mAux, (int) i);
  #endif
            if (PassTokenInModel(&node->mpTokens[i],
                                    &node->mpTokens[hmm->mNStates - 1],
                                    transP * mTranScale)) 
            {
              winingToken = i;
            }
          }
        }
      }
  
      if (node->mpTokens[hmm->mNStates - 1].IsActive()) 
      {
        if (mAccumType == AT_MPE && !InForwardPass()) 
        {
          FloatInLog fil_lmpa =
            {node->mpTokens[hmm->mNStates - 1].mLike + log(fabs(node->mPhoneAccuracy)),
            node->mPhoneAccuracy < 0};
  
          node->mpTokens[hmm->mNStates - 1].mAccuracy =
            FIL_Add(node->mpTokens[hmm->mNStates - 1].mAccuracy, fil_lmpa);
        }
  
    //    ActivateNode(net, node);
        if (mAlignment & STATE_ALIGNMENT) 
        {
          node->mpTokens[hmm->mNStates - 1].AddWordLinkRecord(
              node,
              (InForwardPass() ? winingToken : hmm->mNStates-1 - winingToken-1)-1,
              mTime);
        }
      }
    }
    assert(!HasCycle());
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  TokenPropagationDone()
  {
    int     j;
    Node *  node;
  
    KillToken(InForwardPass() ? 
              mpLast->mpExitToken : 
              mpFirst->mpExitToken);
  
  //  for (i=0; i < nnodes; i++) {
  //    node = &mpNodes[i];
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      int numOfTokens;
  
      if (!(node->mType & NT_MODEL)) 
      {
        assert(!node->mpExitToken->IsActive());
        continue;
      }
  
      numOfTokens = node->mpHmm->mNStates;
  
      for (j=0; j < numOfTokens; j++) 
      {
        KillToken(&node->mpTokens[j]);
      }
    }
  }
    
  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  FreeFWBWRecords()
  {
    Node *node;
    
    // go through all nodes in the network and free the alpha/beta list
    // and alpha/beta reverse list
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      if (!(node->mType & NT_MODEL)) 
        continue;
  
      while (node->mpAlphaBetaList) 
      {
        FWBWR *fwbwr = node->mpAlphaBetaList;
        node->mpAlphaBetaList = fwbwr->mpNext;
        free(fwbwr);
      }
  
      while (node->mpAlphaBetaListReverse) 
      {
        FWBWR *fwbwr = node->mpAlphaBetaListReverse;
        node->mpAlphaBetaListReverse = fwbwr->mpNext;
        free(fwbwr);
      }
    }
  }

    
  //***************************************************************************
  //***************************************************************************
  void
  Network:: 
  SortNodes()
  {
    int       i;
    int       j;
    Node *    chain;
    Node *    last;
    Node *    node;
  
    // Sort nodes for forward (Viterbi) propagation
  
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      node->mAux = node->mNBackLinks;
    }
  
    for (i = 0; i < mpFirst->mNLinks; i++) 
    {
      mpFirst->mpLinks[i].mpNode->mAux--;
    }
  
    last = mpFirst;
    chain = mpFirst->mpNext;
  
    while (chain) 
    {
      bool    short_curcuit = true;
      Node ** curPtr = &chain;
      i = 0;
  
      while (*curPtr) 
      {
        if ((((*curPtr)->mType & NT_MODEL) && !((*curPtr)->mType & NT_TEE))
          || (*curPtr)->mAux == 0) 
        {
          for (j = 0; j < (*curPtr)->mNLinks; j++) 
          {
            (*curPtr)->mpLinks[j].mpNode->mAux--;
          }
  
          last = (last->mpNext = *curPtr);
          last->mAux = i++;
          *curPtr = (*curPtr)->mpNext;
          short_curcuit = false;
        } 
        else 
        {
          curPtr = &(*curPtr)->mpNext;
        }
      }
  
      if (short_curcuit) 
      {
  //      fprintf(stderr, "Nodes in loop: ");
  //      for (curPtr = &chain; *curPtr; curPtr = &(*curPtr)->next)
  //        fprintf(stderr, "%d %d", *curPtr - mpNodes, (*curPtr)->mType);
  //      fprintf(stderr, "\n");
        Error("Loop of non-emiting nodes found in network");
      }
    }
  
    last->mpNext = NULL;
  
    /// !!! What is this sorting links good for ???
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      if (node->mNLinks > 1)
        qsort(node->mpLinks, node->mNLinks, sizeof(Link), cmplnk);
    }
  
  // Sort nodes for backward propagation
  
    for (node = mpFirst; node != NULL; node = node->mpNext)
      node->mAux = node->mNLinks;
  
    for (i = 0; i < mpLast->mNBackLinks; i++)
      mpLast->mpBackLinks[i].mpNode->mAux--;
  
    last = mpLast;
    chain = mpLast->mpBackNext;
    i = 0;
  
    while (chain) 
    {
      bool short_curcuit = true;
      Node **curPtr = &chain;
  
      while (*curPtr) 
      {
        if ((((*curPtr)->mType & NT_MODEL) && !((*curPtr)->mType & NT_TEE))
          || (*curPtr)->mAux == 0) 
        {
          for (j = 0; j < (*curPtr)->mNBackLinks; j++) 
          {
            (*curPtr)->mpBackLinks[j].mpNode->mAux--;
          }
  
          last = (last->mpBackNext = *curPtr);
          last->mAux = i++;
          *curPtr = (*curPtr)->mpBackNext;
          short_curcuit = false;
        } 
        else 
        {
          curPtr = &(*curPtr)->mpBackNext;
        }
      }
  
      /*if (short_curcuit) {
        fprintf(stderr, "Nodes in loop: ");
        for (curPtr = &chain; *curPtr; curPtr = &(*curPtr)->mpBackNext)
          fprintf(stderr, "%d ", *curPtr - mpNodes);
        fprintf(stderr, "\n");
        Error("Loop of non-emiting nodes found in net");
      }*/
  
      assert(!short_curcuit); // Shouldn't happen, since it didnot happen before
    }
  
    last->mpBackNext = NULL;
  
    /// !!! What is this sorting links good for ???
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      if (node->mNBackLinks > 1)
        qsort(node->mpBackLinks, node->mNBackLinks, sizeof(Link), cmplnk);
    }
  } // Network::SortNodes();
  
  
  //***************************************************************************
  //***************************************************************************
  void
  Network:: 
  Init(Node * pFirstNode, ModelSet * pHmms, ModelSet *pHmmsToUpdate) 
  {
    Node *node;
    int maxStatesInModel = 0;
    int i;
  
    mpFirst = mpLast = pFirstNode;
  
    PhoneNodesToModelNodes(pFirstNode, pHmms, pHmmsToUpdate);
  
    //Allocate tokens and count emiting states
    mNumberOfNetStates = 0;
    
    for (node = pFirstNode; node != NULL; mpLast = node, node = node->mpNext) 
    {
      
  #ifndef NDEBUG
      node->mAux2 = 0;
  #endif
      int numOfTokens = 1;
      if (node->mType & NT_MODEL) 
      {
        numOfTokens = node->mpHmm->mNStates;
        node->mpHmmToUpdate->mpMacro->mOccurances++;
        if (node->mpHmm->mpTransition->mpMatrixO[numOfTokens - 1] > LOG_MIN) {
          node->mType |= NT_TEE;
        }
      } 
      else if (node->mType & NT_WORD) 
      {
        numOfTokens = 1;
      } 
      else 
      {
        Error("Fatal: Incorect node type");
      }
  
      node->mpTokens = (Token *) malloc(numOfTokens * sizeof(Token));
      
      if (node->mpTokens == NULL) 
        Error("Insufficient memory");
  
      node->mpExitToken = &node->mpTokens[numOfTokens-1];
      node->mEmittingStateId = mNumberOfNetStates;
      
      if (node->mType & NT_MODEL) {
        int nstates = node->mpHmm->mNStates;
  
        if (maxStatesInModel < nstates) maxStatesInModel = nstates;
        assert(nstates >= 2); // two non-emiting states
        mNumberOfNetStates += nstates;
      }
    }
  
    SortNodes();  
    
    mpAuxTokens = (Token *) malloc((maxStatesInModel-1) * sizeof(Token));
    mpOutPCache = (Cache *) malloc(pHmms->mNStates   * sizeof(Cache));
    mpMixPCache = (Cache *) malloc(pHmms->mNMixtures * sizeof(Cache));
  
    if (mpAuxTokens == NULL ||
        mpOutPCache == NULL || mpMixPCache == NULL) 
    {
      Error("Insufficient memory");
    }
  
    for (i = 0; i < maxStatesInModel-1; i++) 
    {
      mpAuxTokens[i].mLike = LOG_0;
      mpAuxTokens[i].mpWlr = NULL;
    }
  
    mWPenalty          = 0.0;
    mMPenalty          = 0.0;
    mPronScale         = 1.0;
    mTranScale         = 1.0;
    mOutpScale         = 1.0;
    mOcpScale          = 1.0;
    mLmScale           = 1.0;
    
    OutputProbability =
      pHmms->mOutPdfKind == KID_DiagC     ? &DiagCGaussianMixtureDensity :
      pHmms->mOutPdfKind == KID_PDFObsVec ? &FromObservationAtStateId    : NULL;
  
    PassTokenInNetwork  = &PassTokenMax;
    PassTokenInModel    = &PassTokenMax;
    
    mPropagDir          = FORWARD;
    mAlignment          = WORD_ALIGNMENT;
    mpThreshState       = NULL;
    mPruningThresh      = -LOG_0;
    mpModelSet          = pHmms;
    mpModelSetToUpdate  = pHmmsToUpdate;
    mCollectAlphaBeta   = 0;
    mAccumType          = AT_ML;            
    mSearchPaths        = SP_ALL;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  Release()
  {
    Node *node;
  
    for (node = mpFirst; node != NULL; node = node->mpNext) 
      free(node->mpTokens);
      
    FreeNetwork(mpFirst);
    
    free(mpAuxTokens);
    free(mpOutPCache);
    free(mpMixPCache);
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  Network::
  ViterbiInit()
  {
    PassTokenInModel    = &PassTokenMax;
    PassTokenInNetwork  = &PassTokenMax;
    mPropagDir          = FORWARD;
    mpModelSet->ResetXformInstances();
  
    mTime = 0; // Must not be set to -mpModelSet->mTotalDelay yet
               // otherwise token cannot enter first model node
               // with start set to 0
               
    TokenPropagationInit();
    mTime = -mpModelSet->mTotalDelay;
  }  
  
  //***************************************************************************
  //***************************************************************************
  void
  Network:: 
  ViterbiStep(FLOAT *observation)
  {
    mTime++;
    mpModelSet->UpdateStacks(observation, mTime, mPropagDir);
  
    if (mTime <= 0)
      return;
  
    TokenPropagationInModels(observation);
    TokenPropagationInNetwork();
  #ifdef DEBUG_MSGS
    printf("Frame: %ld Nuberm of WordLinkRecord Active: ", mTime); PrintNumOfWLR();
  #endif
  
  }
  
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  Network::
  ViterbiDone(Label **labels)
  {
    FLOAT totLike = LOG_0;
    if (labels) 
    {
      if (mpLast->mpExitToken && 
          mpLast->mpExitToken->IsActive()) 
      {
        totLike = mpLast->mpExitToken->mLike;
        *labels = mpLast->mpExitToken->pGetLabels();
      } 
      else 
      {
        *labels = NULL;
      }
    }
    
    TokenPropagationDone();
  
  #ifdef DEBUG_MSGS
    printf("Number of WordLinkRecord Unreleased: "); PrintNumOfWLR();
    printf("Number of output prob. computations: %d\n", gaus_computaions);
  #endif
    return totLike;
  }


  //***************************************************************************
  //***************************************************************************
  Network::FWBWRet
  Network::
  ForwardBackward(FLOAT *obsMx, int nFrames)
  {
    int         i;
    Cache *     p_out_p_cache;
    struct      FWBWRet ret;
    ModelSet *  hmms = mpModelSet;
  
    p_out_p_cache = (Cache *) malloc(nFrames * hmms->mNStates * sizeof(Cache));
  
    if (p_out_p_cache == NULL) {
      Error("Insufficient memory");
    }
  
    for (i = 0; i < static_cast<int>(nFrames * hmms->mNStates); i++) {
      p_out_p_cache[i].mTime  = UNDEF_TIME;
      p_out_p_cache[i].mValue = LOG_0;
    }
  
    free(mpOutPCache);
    mpOutPCache = NULL;
  
    PassTokenInModel   = &PassTokenSum;
    PassTokenInNetwork = &PassTokenSum;
    mAlignment          = NO_ALIGNMENT;
  
  
    //Forward Pass
    mPropagDir = FORWARD;
    mCollectAlphaBeta = 1;
    mTime = -hmms->mTotalDelay;
  
    mpModelSet->ResetXformInstances();
    for (i = 0; i < hmms->mTotalDelay; i++) {
      mTime++;
      hmms->UpdateStacks(obsMx + hmms->mInputVectorSize * i, mTime, FORWARD);
    }
  
    
    // tady nekde je chyba
    // !!!!
    TokenPropagationInit();
  
    for (i = hmms->mTotalDelay; i < nFrames+hmms->mTotalDelay; i++) 
    {
      mTime++;
      mpOutPCache = p_out_p_cache + hmms->mNStates * (mTime-1);
  
      mpModelSet->UpdateStacks(obsMx + hmms->mInputVectorSize * i,
                                mTime, mPropagDir);
  
      TokenPropagationInModels(obsMx + hmms->mInputVectorSize * i);
      TokenPropagationInNetwork();
    }
  
    if (!mpLast->mpExitToken->IsActive())  
    { // No token survivered
      TokenPropagationDone();
      FreeFWBWRecords();
      mpOutPCache = p_out_p_cache;
      ret.totLike = LOG_0;
      return ret;
    }
  
    ret.totLike = mpLast->mpExitToken->mLike; //  totalLikelihood;
    TokenPropagationDone();
  
    //Backward Pass
    mPropagDir = BACKWARD;
    mTime      = nFrames+hmms->mTotalDelay;
  
    for (i = nFrames + hmms->mTotalDelay - 1; i >= nFrames; i--) 
    {
  //  We do not need any features, in backward prop. All output probab. are cached.
  //  UpdateStacks(hmms, obsMx + hmms->mInputVectorSize * i,
  //                 mTime, BACKWARD);
      mTime--;
    }
  
    mpOutPCache = NULL;  // TokenPropagationInit would reset valid 1st frm cache
    TokenPropagationInit();
  
    for (i = nFrames-1; i >= 0; i--) {
      mpOutPCache = p_out_p_cache + hmms->mNStates * (mTime-1);
  
  //  We do not need any features, in backward prop. All output probab. are cached.
  //  UpdateStacks(mpModelSet, obsMx + hmms->mInputVectorSize * i,     |
  //                 mTime, mPropagDir);                   |
  //                                                               V
      TokenPropagationInModels(NULL); //obsMx + hmms->mInputVectorSize * i);
      mTime--;
      TokenPropagationInNetwork();
    }
  
    mpOutPCache = p_out_p_cache;
  
    if (!mpFirst->mpExitToken->IsActive()) { // No token survivered
      TokenPropagationDone();
      FreeFWBWRecords();
      ret.totLike = LOG_0;
      return ret;
    }
  
    ret.totLike = HIGHER_OF(ret.totLike, mpFirst->mpExitToken->mLike); //  totalLikelihood;
    // Backward pass P can differ from forward pass P because of the precision
    // problems. Take the higher one to decrease the possibility of getting
    // an occupation probability (when normalizing by P) higher that one.
  
    FloatInLog fil_ret_totLike = {ret.totLike, 0};
    ret.avgAccuracy  = FIL_Div(mpFirst->mpExitToken->mAccuracy, fil_ret_totLike);
    TokenPropagationDone();
  
    // There may be remaining records in mpAlphaBetaListReverse unused in
    // backward pass. Free them and set mpAlphaBetaListReverse's to NULLs;
  
    Node *node;
    for (node = mpFirst; node != NULL; node = node->mpNext) {
      if (!(node->mType & NT_MODEL)) continue;
  
      while (node->mpAlphaBetaListReverse) {
        FWBWR *fwbwr = node->mpAlphaBetaListReverse;
        node->mpAlphaBetaListReverse = fwbwr->mpNext;
        free(fwbwr);
      }
    }
  
    return ret;
  }
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  Network::
  MCEReest(FLOAT * pObsMx, FLOAT * pObsMx2, int nFrames, FLOAT weight, FLOAT sigSlope)
  {
    struct FWBWRet    fwbw;
    FLOAT             TP;
    FLOAT             P;
    FLOAT             F;
    
    int               i;
    int               j;
    int               k;
    int               t;
    
    ModelSet *        p_hmms_alig = mpModelSet;
    ModelSet *        p_hmms_upd = mpModelSetToUpdate;
    Node     *        node;
  
    AccumType origAccumType = mAccumType;
    
    mAccumType          = AT_ML;
    mPropagDir          = FORWARD;
    mAlignment          = NO_ALIGNMENT;
    
    // pointers to functions
    PassTokenInModel    = &PassTokenSum;  
    PassTokenInNetwork  = &PassTokenSum;
  
    mSearchPaths        = SP_TRUE_ONLY;
    mpModelSet->ResetXformInstances();
  
    mTime = 0; // Must not be set to -mpModelSet->totalDelay yet
               // otherwise token cannot enter first model node
               // with start set to 0
               
    TokenPropagationInit();
    
    mTime = -mpModelSet->mTotalDelay;
  
    for (t = 0; t < nFrames + p_hmms_alig->mTotalDelay; t++) 
    {
      //ViterbiStep(net, pObsMx + p_hmms_alig->mInputVectorSize * t);
      ViterbiStep(pObsMx + p_hmms_alig->mInputVectorSize * t);
    }
  
    TP = mpLast->mpExitToken->mLike;
    
    ViterbiDone(NULL);
  
    if (TP <= LOG_MIN) 
      return LOG_0;
  
  
    ////////////////// Denominator accumulation //////////////////
    mSearchPaths = SP_ALL;
  
    fwbw = ForwardBackward(pObsMx, nFrames);
    P = fwbw.totLike;
  
    assert(P >= TP);
    if(sigSlope > 0.0) 
    {
      F = TP - LogSub(P, TP);
      printf("MCE distance: %g; ", F);
      F = exp(-sigSlope * F);
      F = (sigSlope*F) / SQR(1+F);
      printf("weight: %g\n", F);
      weight *= F;
    }

    if (P < LOG_MIN) return LOG_0;
  
    for (node = mpFirst; node != NULL; node = node->mpNext) {
      if (node->mType & NT_MODEL && node->mpAlphaBetaList != NULL &&
        node->mpAlphaBetaList->mTime == 0) {
        node->mpAlphaBetaListReverse = node->mpAlphaBetaList;
        node->mpAlphaBetaList = node->mpAlphaBetaList->mpNext;
      }
    }
  
    p_hmms_alig->ResetXformInstances();
  
    if (p_hmms_alig != p_hmms_upd) {
      p_hmms_upd->ResetXformInstances();
    }
  
    for (i = 0; i < p_hmms_alig->mTotalDelay; i++) {
      p_hmms_alig->UpdateStacks(pObsMx+p_hmms_alig->mInputVectorSize*i,
                  i-p_hmms_alig->mTotalDelay, FORWARD);
    }
  
    if (p_hmms_alig != p_hmms_upd) {
      for (i = 0; i < p_hmms_upd->mTotalDelay; i++) {
        p_hmms_upd->UpdateStacks(pObsMx2+p_hmms_upd->mInputVectorSize*i,
                    i-p_hmms_upd->mTotalDelay, FORWARD);
      }
    }
  
  
    // mpMixPCache might be used to cache likelihoods of mixtures of target
    // models. Reallocate the cache to fit mixtures of both models and reset it.
    k = HIGHER_OF(p_hmms_upd->mNMixtures, p_hmms_alig->mNMixtures);
    mpMixPCache = (Cache *) realloc(mpMixPCache, k * sizeof(Cache));
    if (mpMixPCache == NULL) Error("Insufficient memory");
  
    for (i = 0; i < k; i++) mpMixPCache[i].mTime = UNDEF_TIME;
  
  // Update accumulators
    for (mTime = 0; mTime < nFrames; mTime++) {//for every frame
      FLOAT *obs  =pObsMx +p_hmms_alig->mInputVectorSize*(mTime+p_hmms_alig->mTotalDelay);
      FLOAT *obs2 =pObsMx2+p_hmms_upd->mInputVectorSize*(mTime+p_hmms_upd->mTotalDelay);
      p_hmms_alig->UpdateStacks(obs, mTime, FORWARD);
      if (p_hmms_alig != p_hmms_upd) {
        p_hmms_upd->UpdateStacks(obs2, mTime, FORWARD);
      }
  
      for (node = mpFirst; node != NULL; node = node->mpNext) { //for every model
  //    for (k=0; k < nnodes; k++) {
  //      Node *node = &mpNodes[k];
        if (node->mType & NT_MODEL &&
          node->mpAlphaBetaList != NULL &&
          node->mpAlphaBetaList->mTime == mTime+1) {
  
          struct AlphaBeta *st;
          int Nq       = node->mpHmm->mNStates;
          st = node->mpAlphaBetaList->mpState;
  
          for (j = 1; j < Nq - 1; j++) {                   //for every emitting state
            if (st[j].mAlpha + st[j].mBeta - P > MIN_LOG_WEGIHT) {
              assert(node->mpAlphaBetaListReverse->mTime == mTime);
  
              ReestState(
                  node,
                  j-1,
                  (st[j].mAlpha + st[j].mBeta - P)  * mOcpScale,
                  -weight,
                  obs,
                  obs2);
            }
          }
          
          if (node->mpAlphaBetaListReverse) 
            free(node->mpAlphaBetaListReverse);
          
          node->mpAlphaBetaListReverse = node->mpAlphaBetaList;
          node->mpAlphaBetaList = node->mpAlphaBetaList->mpNext;
        }
      }
    }
  
    for (node = mpFirst; node != NULL; node = node->mpNext) {
      if (node->mpAlphaBetaListReverse != NULL)
        free(node->mpAlphaBetaListReverse);
    }
  
  
    ////////////////// Numerator accumulation //////////////////
    mSearchPaths = SP_TRUE_ONLY;
  
  
    ForwardBackward(pObsMx, nFrames);
  
    for (node = mpFirst; node != NULL; node = node->mpNext) {
      if (node->mType & NT_MODEL && node->mpAlphaBetaList != NULL &&
        node->mpAlphaBetaList->mTime == 0) {
        node->mpAlphaBetaListReverse = node->mpAlphaBetaList;
        node->mpAlphaBetaList = node->mpAlphaBetaList->mpNext;
      }
    }
  
    p_hmms_alig->ResetXformInstances();
  
    if (p_hmms_alig != p_hmms_upd) {
      p_hmms_upd->ResetXformInstances();
    }
  
    for (i = 0; i < p_hmms_alig->mTotalDelay; i++) {
      p_hmms_alig->UpdateStacks(pObsMx+p_hmms_alig->mInputVectorSize*i,
                  i-p_hmms_alig->mTotalDelay, FORWARD);
    }
  
    if (p_hmms_alig != p_hmms_upd) {
      for (i = 0; i < p_hmms_upd->mTotalDelay; i++) {
        p_hmms_upd->UpdateStacks(pObsMx2+p_hmms_upd->mInputVectorSize*i,
                    i-p_hmms_upd->mTotalDelay, FORWARD);
      }
    }
  
  
    // mpMixPCache might be used to cache likelihoods of mixtures of target
    // models. Reallocate the cache to fit mixtures of both models and reset it.
    k = HIGHER_OF(p_hmms_upd->mNMixtures, p_hmms_alig->mNMixtures);
    mpMixPCache = (Cache *) realloc(mpMixPCache, k * sizeof(Cache));
    if (mpMixPCache == NULL) Error("Insufficient memory");
  
    for (i = 0; i < k; i++) mpMixPCache[i].mTime = UNDEF_TIME;
  
  // Update accumulators
    for (mTime = 0; mTime < nFrames; mTime++) {//for every frame
      FLOAT *obs  = pObsMx +p_hmms_alig->mInputVectorSize*(mTime+p_hmms_alig->mTotalDelay);
      FLOAT *obs2 = pObsMx2+p_hmms_upd->mInputVectorSize*(mTime+p_hmms_upd->mTotalDelay);
      p_hmms_alig->UpdateStacks(obs, mTime, FORWARD);
      if (p_hmms_alig != p_hmms_upd) {
        p_hmms_upd->UpdateStacks(obs2, mTime, FORWARD);
      }
  
      for (node = mpFirst; node != NULL; node = node->mpNext) 
      { //for every model
        if (node->mType & NT_MODEL &&
          node->mpAlphaBetaList != NULL &&
          node->mpAlphaBetaList->mTime == mTime+1) 
        {
  
          struct AlphaBeta *st;
          int Nq       = node->mpHmm->mNStates;
          FLOAT *aq    = node->mpHmm->        mpTransition->mpMatrixO;
          FLOAT *aqacc = node->mpHmmToUpdate->mpTransition->mpMatrixO + SQR(Nq);
  //        int qt_1 = (mNumberOfNetStates * mTime) + node->mEmittingStateId;
  //        int qt = qt_1 + mNumberOfNetStates;
  
          st = node->mpAlphaBetaList->mpState;
  
          if (//!mmi_den_pass &&
            st[Nq-1].mAlpha + st[Nq-1].mBeta - TP > MIN_LOG_WEGIHT) 
          {
            for (i = 0; i < Nq - 1; i++) 
            {
              LOG_INC(aqacc[i * Nq + Nq-1], aq[i * Nq + Nq-1]  * mTranScale +
                                          (st[i].mAlpha                         +
                                            st[Nq-1].mBeta - TP) * mOcpScale);
            }
          }
  
          for (j = 1; j < Nq - 1; j++) 
          {                   //for every emitting state
            if (st[j].mAlpha + st[j].mBeta - TP > MIN_LOG_WEGIHT) {
              FLOAT bjtO =mpOutPCache[p_hmms_alig->mNStates * mTime +
                                        node->mpHmm->mpState[j-1]->mID].mValue;
              // ForwardBackward() set mpOutPCache to contain out prob. for all frames
  
              assert(node->mpAlphaBetaListReverse->mTime == mTime);
  
  //            if (!mmi_den_pass) {
              for (i = 0; i < Nq - 1; i++) {
                LOG_INC(aqacc[i * Nq + j],
                        aq[i * Nq + j]    * mTranScale +
                        (node->mpAlphaBetaListReverse->mpState[i].mAlpha +
                        bjtO              * mOutpScale +
                        st[j].mBeta - TP)   * mOcpScale);
              }
  //            }
  
              if (origAccumType == AT_MMI)
              {
                ReestState(
                    node,
                    j-1,
                    (st[j].mAlpha + st[j].mBeta - TP)  * mOcpScale,
                    weight, 
                    obs, 
                    obs2);
              }
              else // origAccumType == AT_MCE
              {
                ReestState(node, j-1,
                    (st[j].mAlpha + st[j].mBeta - TP + LogAdd(TP,P) - P)  * mOcpScale,
                    weight, 
                    obs, 
                    obs2);
              }
            }
          }
          
          if (node->mpAlphaBetaListReverse) 
            free(node->mpAlphaBetaListReverse);
            
          node->mpAlphaBetaListReverse = node->mpAlphaBetaList;
          node->mpAlphaBetaList = node->mpAlphaBetaList->mpNext;
        }
      }
    }
  
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      if (node->mpAlphaBetaListReverse != NULL)
        free(node->mpAlphaBetaListReverse);
    }
  
    mAccumType = origAccumType;
    return TP;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  Network::
  BaumWelchReest(FLOAT *pObsMx, FLOAT *pObsMx2, int nFrames, FLOAT weight)
  {
    struct FWBWRet          fwbw;
    FLOAT                   P;
    FLOAT                   update_dir;
    int                     i;
    int                     j;
    int                     k;
    ModelSet*               p_hmms_alig = mpModelSet;
    ModelSet*               p_hmms_upd = mpModelSetToUpdate;
    Node*                   node;
  
    fwbw = ForwardBackward(pObsMx, nFrames);
    P    = fwbw.totLike;
    
    if (P < LOG_MIN) 
      return LOG_0;
  
  #ifdef MOTIF
    FLOAT *ocprob = (FLOAT*) malloc(mNumberOfNetStates * (nFrames+1) * sizeof(FLOAT));
    for (i=0; i<mNumberOfNetStates * (nFrames+1); i++) ocprob[i] = 0;
  #endif
  
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      if (node->mType & NT_MODEL && node->mpAlphaBetaList != NULL &&
        node->mpAlphaBetaList->mTime == 0) 
      {
        node->mpAlphaBetaListReverse = node->mpAlphaBetaList;
        node->mpAlphaBetaList = node->mpAlphaBetaList->mpNext;
      }
    }
  
    p_hmms_alig->ResetXformInstances();
  
    if (p_hmms_alig != p_hmms_upd) 
      p_hmms_upd->ResetXformInstances();
  
    for (i = 0; i < p_hmms_alig->mTotalDelay; i++) 
    {
      p_hmms_alig->UpdateStacks(pObsMx+p_hmms_alig->mInputVectorSize*i,
                            i-p_hmms_alig->mTotalDelay, FORWARD);
    }
  
    if (p_hmms_alig != p_hmms_upd) 
    {
      for (i = 0; i < p_hmms_upd->mTotalDelay; i++) 
      {
        p_hmms_upd->UpdateStacks(pObsMx2+p_hmms_upd->mInputVectorSize*i,
                    i-p_hmms_upd->mTotalDelay, FORWARD);
      }
    }
  
  
    // mpMixPCache might be used to cache likelihoods of mixtures of target
    // models. Reallocate the cache to fit mixtures of both models and reset it.
    k = HIGHER_OF(p_hmms_upd->mNMixtures, p_hmms_alig->mNMixtures);
    mpMixPCache = (Cache *) realloc(mpMixPCache, k * sizeof(Cache));
    
    if (mpMixPCache == NULL)
    { 
      Error("Insufficient memory");
    }
    
    for (i = 0; i < k; i++)
    { 
      mpMixPCache[i].mTime = UNDEF_TIME;
    }
    
  // Update accumulators
    for (mTime = 0; mTime < nFrames; mTime++) 
    { //for every frame
      FLOAT* obs  = pObsMx +p_hmms_alig->mInputVectorSize*(mTime+p_hmms_alig->mTotalDelay);
      FLOAT* obs2 = pObsMx2+p_hmms_upd->mInputVectorSize*(mTime+p_hmms_upd->mTotalDelay);
      
      p_hmms_alig->UpdateStacks(obs, mTime, FORWARD);
      
      if (p_hmms_alig != p_hmms_upd)
        p_hmms_upd->UpdateStacks(obs2, mTime, FORWARD);
  
      for (node = mpFirst; node != NULL; node = node->mpNext) 
      { //for every model
  //    for (k=0; k < nnodes; k++) {
  //      Node *node = &mpNodes[k];
        if (node->mType & NT_MODEL &&
          node->mpAlphaBetaList != NULL &&
          node->mpAlphaBetaList->mTime == mTime+1) 
        {
          struct AlphaBeta *st;
          int Nq       = node->mpHmm->mNStates;
          FLOAT* aq    = node->mpHmm->        mpTransition->mpMatrixO;
          FLOAT* aqacc = node->mpHmmToUpdate->mpTransition->mpMatrixO + SQR(Nq);
//        int qt_1 = (mNumberOfNetStates * mTime) + node->mEmittingStateId;
//        int qt = qt_1 + mNumberOfNetStates;
  
          st = node->mpAlphaBetaList->mpState;
  
          if (//!mmi_den_pass &&
            st[Nq-1].mAlpha + st[Nq-1].mBeta - P > MIN_LOG_WEGIHT) 
          {
            for (i = 0; i < Nq - 1; i++) 
            {
              LOG_INC(aqacc[i * Nq + Nq-1], aq[i * Nq + Nq-1]  * mTranScale +
                                          (st[i].mAlpha + st[Nq-1].mBeta - P) * mOcpScale);
            }
          }
  
          for (j = 1; j < Nq - 1; j++) 
          { //for every emitting state
            if (st[j].mAlpha + st[j].mBeta - P > MIN_LOG_WEGIHT) 
            {
#ifdef MOTIF
            ocprob[mNumberOfNetStates * (mTime+1) + node->mEmittingStateId + j]
//            = node->mPhoneAccuracy;
              = exp(st[j].mAlpha+st[j].mBeta-P) *
                ((1-2*static_cast<int>(st[j].mAlphaAccuracy.negative)) * exp(st[j].mAlphaAccuracy.logvalue - st[j].mAlpha) +
                (1-2*static_cast<int>(st[j].mBetaAccuracy.negative))  * exp(st[j].mBetaAccuracy.logvalue  - st[j].mBeta)
                - (1-2*static_cast<int>(fwbw.avgAccuracy.negative)) * exp(fwbw.avgAccuracy.logvalue)
                );
  
#endif
//            int qt_1   = qt - mNumberOfNetStates;
              FLOAT bjtO =mpOutPCache[p_hmms_alig->mNStates * mTime +
                                        node->mpHmm->mpState[j-1]->mID].mValue;
              // ForwardBackward() set mpOutPCache to contain out prob. for all frames
  
              assert(node->mpAlphaBetaListReverse->mTime == mTime);
  
//            if (!mmi_den_pass) {
              for (i = 0; i < Nq - 1; i++) 
              {
                LOG_INC(aqacc[i * Nq + j],
                        aq[i * Nq + j] * mTranScale +
                        (node->mpAlphaBetaListReverse->mpState[i].mAlpha +
                        bjtO * mOutpScale + st[j].mBeta - P) * mOcpScale);
              }
//            }
  
              if (mAccumType == AT_MFE || mAccumType == AT_MPE) 
              {
                update_dir = (1-2*static_cast<int>(st[j].mAlphaAccuracy.negative)) * exp(st[j].mAlphaAccuracy.logvalue - st[j].mAlpha) +
                             (1-2*static_cast<int>(st[j].mBetaAccuracy.negative))  * exp(st[j].mBetaAccuracy.logvalue  - st[j].mBeta)  -
                             (1-2*static_cast<int>(fwbw.avgAccuracy.negative))     * exp(fwbw.avgAccuracy.logvalue);
              } 
              else 
              {
                update_dir = 1.0;
              }
  
              ReestState(
                node, 
                j - 1,
                (st[j].mAlpha + st[j].mBeta - P)  * mOcpScale,
                update_dir*weight, 
                obs, 
                obs2);
            }
          }
          
          if (node->mpAlphaBetaListReverse) 
            free(node->mpAlphaBetaListReverse);
            
          node->mpAlphaBetaListReverse = node->mpAlphaBetaList;
          node->mpAlphaBetaList = node->mpAlphaBetaList->mpNext;
        }
      }
    }
  
    for (node = mpFirst; node != NULL; node = node->mpNext) 
    {
      if (node->mpAlphaBetaListReverse != NULL)
        free(node->mpAlphaBetaListReverse);
    }
  
#ifdef MOTIF
    FLOAT max = LOG_0;
    printf("mTranScale: %f\noutpScale: %f\n",mTranScale, mOutpScale);
    for (i = 0; i < mNumberOfNetStates * (nFrames+1); i++) 
      max = HIGHER_OF(max, ocprob[i]);
  
    imagesc(ocprob, mNumberOfNetStates, (nFrames+1),
            sizeof(FLOAT) == 4 ? "float" : "double",
            NULL,  cm_color, "OccProb");
  
    for (j=0; j<nFrames+1; j++) 
    {
      for (i=0; i<mNumberOfNetStates; i++) 
      {
        printf("%6.3g ", (ocprob[mNumberOfNetStates * j + i]));
      }
      printf("\n");
    }
//  for (i = 0; i < mNumberOfNetStates * (nFrames+1); i++) {
//    if (ocprob[i] < LOG_MIN) ocprob[i] = max;
//  }
//  imagesc(ocprob, mNumberOfNetStates, (nFrames+1), STR(FLOAT), NULL, cm_color, "Log OccProb");
    free(ocprob);
#endif
  
    return P;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  Network::
  ViterbiReest(FLOAT *pObsMx, FLOAT *pObsMx2, int nFrames, FLOAT weight)
  {
    int                     t;
    WordLinkRecord *        wlr;
    Node *prevnode =        NULL;
    FLOAT                   P;
    Cache *                 p_out_p_cache;
    ModelSet *              p_hmms_alig =   mpModelSet;
    ModelSet *              p_hmms_upd =    mpModelSetToUpdate;
  
    p_out_p_cache = 
      (Cache *) malloc(nFrames * p_hmms_alig->mNStates * sizeof(Cache));
      
    if (p_out_p_cache == NULL) 
      Error("Insufficient memory");
  
    for (t = 0; t < static_cast<int>(nFrames * p_hmms_alig->mNStates); t++) 
    {
      p_out_p_cache[t].mTime  = UNDEF_TIME;
      p_out_p_cache[t].mValue = LOG_0;
    }
  
    free(mpOutPCache);
    
    mpOutPCache = NULL;
    mAlignment = STATE_ALIGNMENT;
    
    ViterbiInit();
    
    nFrames += p_hmms_alig->mTotalDelay;
  
    for (t = 0; t < nFrames; t++) 
    {
      if (t >= p_hmms_alig->mTotalDelay)
        mpOutPCache = p_out_p_cache + p_hmms_alig->mNStates*(t-p_hmms_alig->mTotalDelay);
      
      ViterbiStep(pObsMx + p_hmms_alig->mInputVectorSize * t);
    }
  
    mpOutPCache = p_out_p_cache;
  
    if (!mpLast->mpExitToken->IsActive()) 
    {
      ViterbiDone(NULL);
      return LOG_0;
    }
  
    p_hmms_alig->ResetXformInstances();
  
    if (p_hmms_alig != p_hmms_upd)
      p_hmms_upd->ResetXformInstances();
  
    // invert order of WRLs
    wlr = mpLast->mpExitToken->mpWlr;
    
    while (wlr->mpNext != NULL) 
    {
      WordLinkRecord *tmp = wlr->mpNext->mpNext;
      wlr->mpNext->mpNext = mpLast->mpExitToken->mpWlr;
      mpLast->mpExitToken->mpWlr = wlr->mpNext;
      wlr->mpNext = tmp;
    }
  
    for (mTime = -p_hmms_alig->mTotalDelay; mTime < 0; mTime++) 
    {
      FLOAT *obs = pObsMx+p_hmms_alig->mInputVectorSize*(mTime+p_hmms_alig->mTotalDelay);
      p_hmms_alig->UpdateStacks(obs, mTime, FORWARD);
    }
  
    if (p_hmms_alig != p_hmms_upd) 
    {
      FLOAT *obs2 = pObsMx2+p_hmms_upd->mInputVectorSize*(mTime+p_hmms_upd->mTotalDelay);
      for (mTime = -p_hmms_upd->mTotalDelay; mTime < 0; mTime++) 
      {
        p_hmms_upd->UpdateStacks(obs2, mTime, FORWARD);
      }
    }
  
  // Update accumulators
    for (wlr = mpLast->mpExitToken->mpWlr; wlr != NULL; wlr = wlr->mpNext) 
    {
      Node *node   = wlr->mpNode;
      int Nq       = node->mpHmmToUpdate->mNStates;
      FLOAT *aqacc = node->mpHmmToUpdate->mpTransition->mpMatrixO + SQR(Nq);
      int currstate = wlr->mStateIdx+1;
      int nextstate = (wlr->mpNext && node == wlr->mpNext->mpNode)
                      ? wlr->mpNext->mStateIdx+1 : Nq-1;
      int duration  = wlr->mTime - mTime;
  
  
  
      if (prevnode != node) {
  //      if (!mmi_den_pass)
        LOG_INC(aqacc[currstate], 0 /*ln(1)*/);
        prevnode = node;
      }
  
  //    if (!mmi_den_pass) { // So far we dont do any MMI estimation of trasitions
      LOG_INC(aqacc[currstate * Nq + currstate], log(duration-1));
      LOG_INC(aqacc[currstate * Nq + nextstate], 0 /*ln(1)*/);
  //    }
  
      for (; mTime < wlr->mTime; mTime++) 
      {
        //for every frame of segment
        FLOAT *obs  = pObsMx  + p_hmms_alig->mInputVectorSize*(mTime+p_hmms_alig->mTotalDelay);
        FLOAT *obs2 = pObsMx2 + p_hmms_upd->mInputVectorSize*(mTime+p_hmms_upd->mTotalDelay);
  
        p_hmms_alig->UpdateStacks(obs, mTime, FORWARD);
        
        if (p_hmms_alig != p_hmms_upd)
          p_hmms_upd->UpdateStacks(obs2, mTime, FORWARD);
        
        ReestState(node, currstate-1, 0.0, 1.0*weight, obs, obs2);
      }
    }
  
    P = mpLast->mpExitToken->mpWlr->mLike;
    //ViterbiDone(net, NULL);
    ViterbiDone(NULL);
    return P;
  }
  
  
    
    
    
  //***************************************************************************
  //***************************************************************************
  // Token section
  //***************************************************************************
  //***************************************************************************
  
  //***************************************************************************
  //***************************************************************************
  Label *
  Token::
  pGetLabels()
  {
    WordLinkRecord *  wlr;
    Label *           tmp;
    Label *           level[3] = {NULL, NULL, NULL};
    int               li = 0;
      
    if (!this->IsActive()) 
      return NULL;
    
    for (wlr = this->mpWlr; wlr != NULL; wlr = wlr->mpNext) 
    {
      if ((tmp = (Label *) malloc(sizeof(Label))) == NULL)
        Error("Insufficient memory");
  
      tmp->mScore = wlr->mLike;
      tmp->mStop  = wlr->mTime;
      tmp->mId    = wlr->mStateIdx;
  
      if (wlr->mpNode->mType & NT_MODEL) 
      {
        li = wlr->mStateIdx >= 0 ? 0 : 1;
        tmp->mpData = wlr->mpNode->mpHmm;
        tmp->mpName = wlr->mpNode->mpHmm->mpMacro->mpName;
        tmp->mpNextLevel = level[li+1] ? level[li+1] : level[2];
      } 
      else //if (wlr->mpNode->mpPronun->outSymbol) 
      {
        li = 2;
        tmp->mpData = wlr->mpNode->mpPronun->mpWord;
        tmp->mpName = wlr->mpNode->mpPronun->outSymbol;
        tmp->mpNextLevel = NULL;
      } 
      //else 
      //{
      //  free(tmp);
      //  continue;
      //}
  
      if (level[li]) 
      {
  
    // if previous label mis its label on lower level, just make it
  /*      if (li > 0 && (!level[li-1] || level[li-1]->mpNextLevel != level[li])) {
          Label *tmp2;
          if ((tmp2 = (Label *) malloc(sizeof(Label))) == NULL) {
            Error("Insufficient memory");
          }
          tmp2->mpNextLevel = level[li];
          tmp2->mpName  = level[li]->mpName;
          tmp2->mScore = level[li]->mScore;
          tmp2->mStop  = level[li]->stop;
          tmp2->mId    = level[li]->mId;
  
          if (level[li-1]) {
            level[li-1]->mScore -= tmp2->mScore;
            level[li-1]->mStart  = tmp2->mStop;
          }
  
          tmp2->mpNext  = level[li-1];
          level[li-1] = tmp2;
        }*/
  
        level[li]->mScore -= tmp->mScore;
        level[li]->mStart = tmp->mStop;
      }
  
      tmp->mpNext = level[li];
      level[li] = tmp;
    }
    
    for (li = 0; li < 3; li++) 
    {
      if (level[li]) 
        level[li]->mStart = 0;
    }
    
    return level[0] ? level[0] : level[1] ? level[1] : level[2];
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  Token::
  AddWordLinkRecord(Node *node, int stateIndex, int time)
  {
    WordLinkRecord *wlr;
  
    if ((wlr = (WordLinkRecord *) malloc(sizeof(WordLinkRecord))) == NULL)
      Error("Insufficient memory");
  
    wlr->mStateIdx  = stateIndex;
    wlr->mLike      = mLike;
    wlr->mpNode     = node;
    wlr->mTime      = time;
    wlr->mpNext     = mpWlr;
    wlr->mNReferences       = 1;
    mpWlr      = wlr;
    
# ifdef bordel_staff
    if (!mpTWlr) 
      mpTWlr = wlr;
# endif // bordel_staff
  
# ifdef DEBUG_MSGS
    wlr->mpTmpNext = firstWLR;
    firstWLR = wlr;
    wlr->mIsFreed = false;
# endif
  }
  
}; // namespace STK
  
