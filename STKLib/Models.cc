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

#include "Models.h"
#include "stkstream.h"
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

namespace STK
{
  const char *    gpHListFilter;
  bool            gHmmsIgnoreMacroRedefinition = true;
  FLOAT           gWeightAccumDen;
  
  
  FunctionTable gFuncTable[] = 
  {
    {sigmoid_vec, KID_Sigmoid}, {log_vec,     KID_Log},
    {exp_vec,     KID_Exp},     {sqrt_vec,    KID_Sqrt},
    {softmax_vec, KID_SoftMax}
  };
  
  size_t gFuncTableSize = sizeof(gFuncTable)/sizeof(*gFuncTable);
  enum StatType {MEAN_STATS, COV_STATS};
  
  
  //***************************************************************************
  //***************************************************************************
  Mixture&
  Mixture::
  AddToAccumCAT(Matrix<FLOAT>* pGw, Matrix<FLOAT>* pKw)
  {
    FLOAT          tmp_val;
    FLOAT          occ_counts;                                                  // occupation counts for current mixture
    Matrix<FLOAT>  aux_mat1(mpMean->mClusterMatrix);                            // auxiliary matrix
    
    aux_mat1.DiagScale(mpVariance->mpVectorO);
    
    // we go through each accumulator set
    for (size_t sub=0; sub < mpMean->mCwvAccum.Rows(); sub++)
    {
      occ_counts = mpMean->mpOccProbAccums[sub];                
      
      // we accumulate Gw here
      for (size_t i=0; i < pGw[sub].Rows(); i++)
      {
        for (size_t j=0; j < pGw[sub].Cols(); j++)
        {
          tmp_val = 0;
          for (size_t k=0; k < aux_mat1.Cols(); k++)
          {
            tmp_val += aux_mat1[i][k] * mpMean->mClusterMatrix[i][k];
          } // k
          pGw[sub][i][j] += occ_counts * tmp_val;
        } // j
      } // i
      
      // accumulate kw here
      for (size_t i=0; i < aux_mat1.Rows(); i++)
      {   
        for (size_t k=0; k < aux_mat1.Cols(); k++)
        {
          pKw[sub][0][i] += aux_mat1[i][k] * mpMean->mCwvAccum[sub][k];
        }      
      }
    } // for (size_t sub=0; sub < mpMean->mCwvAccum.Rows())
        
    return *this;
  }
  
  void 
  ComputeClusterWeightVectorAccums(
    int               macro_type, 
    HMMSetNodeName    nodeName, 
    MacroData*        pData, 
    void*             pUserData)
  {
    ClusterWeightAccums* cwa = reinterpret_cast<ClusterWeightAccums*>(pUserData);
    Mixture*             mix = reinterpret_cast<Mixture*>(pData);
    
    for (int i = 0; i < cwa->mNClusterWeights; i++)
    {
      mix->AddToAccumCAT(cwa->mpGw, cwa->mpKw);
    }
  }  
  
  //***************************************************************************
  //***************************************************************************
  Mixture&
  Mixture::
  ResetAccumCAT()
  {
    *(mpMean->mpOccProbAccums) = 0.0;
    mpMean->mCwvAccum.Clear();
    return *this;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  ReplaceItem(
    int               macro_type, 
    HMMSetNodeName    nodeName, 
    MacroData *       pData, 
    void *            pUserData)
  {
    ReplaceItemUserData*   ud = (ReplaceItemUserData*) pUserData;
    size_t                  i;
    size_t                  j;
  
    if (macro_type == 'h') 
    {
      Hmm *hmm = (Hmm *) pData;
      
      if (ud->mType == 's') 
      {
        for (i = 0; i < hmm->mNStates-2; i++) 
        {
          if (hmm->mpState[i] == ud->mpOldData) 
          {
            hmm->mpState[i] = (State*) ud->mpNewData;
          }
        }
      } 
      else if (hmm->mpTransition == ud->mpOldData) 
      {
        hmm->mpTransition = (Transition*) ud->mpNewData;
      }
    } 
    
    else if (macro_type == 's') 
    {
      State *state = (State *) pData;
      if (state->mOutPdfKind != KID_PDFObsVec) 
      {
        for (i = 0; i < state->mNumberOfMixtures; i++) 
        {
          if (state->mpMixture[i].mpEstimates == ud->mpOldData) 
          {
            state->mpMixture[i].mpEstimates = (Mixture*) ud->mpNewData;
          }
        }
      }
    } 
    
    else if (macro_type == 'm') 
    {
      Mixture* mixture = static_cast<Mixture*>(pData);
      if (mixture->mpMean      == ud->mpOldData) mixture->mpMean       = (Mean*)          ud->mpNewData;
      if (mixture->mpVariance  == ud->mpOldData) mixture->mpVariance   = (Variance*)      ud->mpNewData;
      if (mixture->mpInputXform== ud->mpOldData) mixture->mpInputXform = (XformInstance*) ud->mpNewData;
    } 
    
    else if (macro_type == 'u')
    {
      Mean* mean = static_cast<Mean*>(pData);
      
      // For cluster adaptive training, BiasXform holds the weights vector. If
      // new bias is defined, we want to check all means and potentially update
      // the weights refference and recalculate the values
      if ('x' == ud->mType                                               
      && (XT_BIAS == static_cast<Xform*>(ud->mpNewData)->mXformType))
      {
        // find if any of the mpWeights points to the old data
        for (i = 0; i < mean->mNWeights && (mean->mpWeights[i] != 
             static_cast<BiasXform*>(ud->mpOldData)) ; i++)
        {}
        
        // if i < mean->mNWeights
        if (i < mean->mNWeights)
        {
          BiasXform* new_weights = static_cast<BiasXform*>(ud->mpNewData);
          
          mean->mpWeights[i] = new_weights;
          mean->RecalculateCAT();
        }
        
        
      }
    }
        
    else if (macro_type == 'x') 
    {
      CompositeXform *cxf = (CompositeXform *) pData;
      if (cxf->mXformType == XT_COMPOSITE) 
      {
        for (i = 0; i < cxf->mNLayers; i++) 
        {
          for (j = 0; j < cxf->mpLayer[i].mNBlocks; j++) 
          {
            if (cxf->mpLayer[i].mpBlock[j] == ud->mpOldData) 
            {
              cxf->mpLayer[i].mpBlock[j] = (Xform*) ud->mpNewData;
            }
          }
        }
      }
    } 
    
    else if (macro_type == 'j') 
    {
      XformInstance *xformInstance = (XformInstance *) pData;
      if (xformInstance->mpInput == ud->mpOldData) xformInstance->mpInput = (XformInstance*) ud->mpNewData;
      if (xformInstance->mpXform == ud->mpOldData) xformInstance->mpXform = (Xform*)         ud->mpNewData;
    }
  }
    
  //***************************************************************************
  //***************************************************************************
  bool 
  IsXformIn1stLayer(Xform *xform, Xform *topXform)
  {
    size_t      i;
    if (topXform == NULL)  return false;
    if (topXform == xform) return true;
  
    if (topXform->mXformType == XT_COMPOSITE) 
    {
      CompositeXform *cxf = static_cast<CompositeXform *>(topXform);
      
      for (i=0; i < cxf->mpLayer[0].mNBlocks; i++) 
      {
        if (IsXformIn1stLayer(xform, cxf->mpLayer[0].mpBlock[i])) 
          return true;
      }
    }
    
    return false;
  }
  
  //***************************************************************************
  //***************************************************************************
  bool 
  Is1Layer1BlockLinearXform(Xform * pXform)
  {
    CompositeXform *cxf = static_cast<CompositeXform *>(pXform);
    
    if (cxf == NULL)                                        return false;
    if (cxf->mXformType == XT_LINEAR)                       return true;
    if (cxf->mXformType != XT_COMPOSITE)                    return false;
    if (cxf->mNLayers > 1 || cxf->mpLayer[0].mNBlocks > 1)  return false;
    
    return Is1Layer1BlockLinearXform(cxf->mpLayer[0].mpBlock[0]);
  }
  
  
  //***************************************************************************
  //***************************************************************************
  Macro *
  FindMacro(MyHSearchData *macro_hash, const char *name) 
  {
    ENTRY e, *ep;
    e.key = (char *) name;
    my_hsearch_r(e, FIND, &ep, macro_hash);
    return (Macro *) (ep ? ep->data : NULL);
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  ReleaseMacroHash(MyHSearchData *macro_hash) 
  {
    unsigned int i;
    for (i = 0; i < macro_hash->mNEntries; i++) 
    {
      Macro *macro = (Macro *) macro_hash->mpEntry[i]->data;
      free(macro->mpName);
      free(macro->mpFileName);
      delete macro;
      macro_hash->mpEntry[i]->data = NULL;
    }
    my_hdestroy_r(macro_hash, 0);
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void 
  ReleaseItem(int macro_type,HMMSetNodeName nodeName,MacroData * pData, void * pUserData)
  {
    delete pData;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  qsmacrocmp(const void *a, const void *b) 
  {
    return strcmp(((Macro *)a)->mpName, ((Macro *)b)->mpName);
  }
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  ResetAccum(int macro_type, HMMSetNodeName nodeName,
             MacroData * pData, void *pUserData) 
  {
    size_t    i;
    size_t    size    = 0;
    FLOAT *   vector  = NULL;
  
    if (macro_type == mt_mean || macro_type == mt_variance) {
      if (macro_type == mt_mean) {
        size   = ((Mean *)pData)->VectorSize();
        vector = ((Mean *)pData)->mpVectorO + size;
        size   = (size + 1) * 2;
      } else if (macro_type == mt_variance) {
        size   = ((Variance *)pData)->VectorSize();
        vector = ((Variance *)pData)->mpVectorO + size;
        size   = (size * 2 + 1) * 2;
      }
  
      for (i = 0; i < size; i++) vector[i] = 0;
  
    } else if (macro_type == mt_state) {
      State *state = (State *) pData;
      if (state->mOutPdfKind == KID_DiagC) {
        for (i = 0; i < state->mNumberOfMixtures; i++) {
          state->mpMixture[i].mWeightAccum     = 0;
          state->mpMixture[i].mWeightAccumDen  = 0;
        }
      }
    } else if (macro_type == mt_transition) {
      size   = SQR(((Transition *) pData)->mNStates);
      vector = ((Transition *) pData)->mpMatrixO + size;
  
      for (i = 0; i < size; i++) vector[i] = LOG_0;
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  GlobalStats(
    int                 macro_type, 
    HMMSetNodeName      nn, 
    MacroData*          pData, 
    void*               pUserData)
  {
    size_t        i;
    
    GlobalStatsUserData *ud = (GlobalStatsUserData *) pUserData;
  
    if (macro_type == mt_state) 
    {
      State * state = (State *) pData;
      
      if (state->mOutPdfKind == KID_DiagC) 
      {
        for (i = 0; i < state->mNumberOfMixtures; i++) 
        {
          state->mpMixture[i].mWeightAccum += 1;
        }
      }
    } 
    else 
    { // macro_type == mt_mixture
      Mixture * mixture   = (Mixture *) pData;
      size_t    vec_size  = mixture->mpMean->VectorSize();
      FLOAT *   obs       = XformPass(mixture->mpInputXform,ud->observation,ud->mTime,FORWARD);
      
      for (i = 0; i < vec_size; i++) 
      {
        mixture->mpMean->mpVectorO[vec_size + i] += obs[i];
      }
      
      mixture->mpMean->mpVectorO[2 * vec_size] += 1;
  
      for (i = 0; i < vec_size; i++) 
      {
        mixture->mpVariance->mpVectorO[vec_size  +i] += SQR(obs[i]);
        mixture->mpVariance->mpVectorO[2*vec_size+i] += obs[i];
      }
      mixture->mpVariance->mpVectorO[3 * vec_size] += 1;
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  FLOAT *
  XformPass(
    XformInstance   *     pXformInst, 
    FLOAT           *     pInputVector, 
    int                   time, 
    PropagDirectionType   dir)
  {
    if (pXformInst == NULL) 
      return pInputVector;
  
    if (time != UNDEF_TIME && pXformInst->mTime == time) 
      return pXformInst->mpOutputVector;
  
    pXformInst->mTime = time;
  
    // recursively pass previous transformations
    if (pXformInst->mpInput)
      pInputVector = XformPass(pXformInst->mpInput, pInputVector, time, dir);
  
    // evaluate this transformation
    pXformInst->mpXform->Evaluate(
                          pInputVector,
                          pXformInst->mpOutputVector,
                          pXformInst->mpMemory,dir);
  
    return pXformInst->mpOutputVector;
  }

    
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  AllocXformStatAccums(
    XformStatAccum **     xformStatAccum,
    size_t         *      nxformStatAccums,
    XformInstance  *      xformInstance,
    enum StatType         stat_type) 
  {
    size_t    i;
    size_t    j;
    
    if (xformInstance == NULL) 
      return;
  
    for (i = 0; i < xformInstance->mNumberOfXformStatCaches; i++) 
    {
      XformStatCache *xfsc = &xformInstance->mpXformStatCache[i];
      XformStatAccum *xfsa = *xformStatAccum;
  
      for (j = 0; j < *nxformStatAccums; j++, xfsa++) 
      {
        if (xfsa->mpXform == xfsc->mpXform) 
          break;
      }
  
      if (j == *nxformStatAccums) 
      {
        size_t size = xfsc->mpXform->mInSize; //mean : mean+covariance
        size = (stat_type == MEAN_STATS) ? size : size+size*(size+1)/2;
  
        *xformStatAccum =
          (XformStatAccum *) realloc(*xformStatAccum,
                                    sizeof(XformStatAccum) * ++*nxformStatAccums);
  
        if (*xformStatAccum == NULL)
          Error("Insufficient memory");
  
        xfsa = *xformStatAccum + *nxformStatAccums - 1;
  
        if ((xfsa->mpStats = (FLOAT *) malloc(sizeof(FLOAT) * size)) == NULL)
          Error("Insufficient memory");
  
        xfsa->mpXform    = xfsc->mpXform;
        xfsa->mNorm     = 0.0;
        
        for (j = 0; j < size; j++) 
          xfsa->mpStats[j] = 0.0;
      }
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  AllocateXformStatCachesAndAccums(
    int                 macro_type, 
    HMMSetNodeName      nodeName,
    MacroData       *   pData, 
    void            *   hmm_set)
  {
    if (macro_type == mt_XformInstance) 
    {
      //Allocate Xform stat caches for XformInstance
  
      XformInstance *   xfi =(XformInstance *) pData;
      size_t            i;
      size_t            j;
  
      for (i=0; i < ((ModelSet *) hmm_set)->mNumberOfXformsToUpdate; i++) 
      {
        Xform *xform = ((ModelSet *) hmm_set)->mpXformToUpdate[i].mpXform;
        int instanceContainXfrom = IsXformIn1stLayer(xform, xfi->mpXform);
  
        //Does instance one level up contain cache for this xform
        XformStatCache *upperLevelStats = NULL;
        if (xfi->mpInput != NULL) {
          for (j=0; j < xfi->mpInput->mNumberOfXformStatCaches; j++) {
            if (xfi->mpInput->mpXformStatCache[j].mpXform == xform) {
              upperLevelStats = &xfi->mpInput->mpXformStatCache[j];
              break;
            }
          }
        }
  
        if (instanceContainXfrom || upperLevelStats != NULL) 
        {
          XformStatCache *xfsc;
  
          xfi->mpXformStatCache = (XformStatCache *)
            realloc(xfi->mpXformStatCache,
                    sizeof(XformStatCache) * ++xfi->mNumberOfXformStatCaches);
  
          if (xfi->mpXformStatCache == NULL) {
            Error("Insufficient memory");
          }
  
          xfsc = &xfi->mpXformStatCache[xfi->mNumberOfXformStatCaches-1];
  
          if (instanceContainXfrom) {
            int size = xform->mInSize;
            size = size+size*(size+1)/2;
  
            if ((xfsc->mpStats = (FLOAT *) malloc(sizeof(FLOAT) * size))==NULL) {
              Error("Insufficient memory");
            }
          } else {
            xfsc->mpStats = upperLevelStats->mpStats;
          }
  
          xfsc->mNorm = 0;
          xfsc->mpXform = xform;
          xfsc->mpUpperLevelStats = upperLevelStats;
        }
      }
    } 
    
    else if (macro_type == mt_mixture) 
    {
      //Allocate Xform stat accumulators for mean and covariance
  
      Mixture *mix = (Mixture *) pData;
      AllocXformStatAccums(&mix->mpMean->mpXformStatAccum,
                          &mix->mpMean->mNumberOfXformStatAccums,
                          mix->mpInputXform, MEAN_STATS);
  
      AllocXformStatAccums(&mix->mpVariance->mpXformStatAccum,
                          &mix->mpVariance->mNumberOfXformStatAccums,
                          mix->mpInputXform, COV_STATS);
  
      if (mix->mpInputXform == NULL || mix->mpInputXform->mNumberOfXformStatCaches == 0)
        return;
        
      if (mix->mpInputXform->mNumberOfXformStatCaches != 1 ||
        !Is1Layer1BlockLinearXform(mix->mpInputXform->mpXform) ||
        mix->mpInputXform->mpXformStatCache[0].mpUpperLevelStats != NULL) 
      {
        mix->mpVariance->mUpdatableFromStatAccums = false;
        mix->mpMean    ->mUpdatableFromStatAccums = false;
        ((ModelSet *) hmm_set)->mAllMixuresUpdatableFromStatAccums = false;  
      } 
      
      else if (mix->mpMean->mNumberOfXformStatAccums != 1) 
      {
        assert(mix->mpMean->mNumberOfXformStatAccums > 1);
        mix->mpMean->mUpdatableFromStatAccums = false;
        ((ModelSet *) hmm_set)->mAllMixuresUpdatableFromStatAccums = false;
      } 
      
      else if (mix->mpVariance->mNumberOfXformStatAccums != 1) 
      {
        assert(mix->mpVariance->mNumberOfXformStatAccums > 1);
        mix->mpVariance->mUpdatableFromStatAccums = false;
        ((ModelSet *) hmm_set)->mAllMixuresUpdatableFromStatAccums = false;
      }
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  NormalizeStatsForXform(int macro_type, HMMSetNodeName nodeName,
                         MacroData * pData, void * pUserData) 
  {
    XformStatAccum *xfsa = NULL;
    int i, j, k, nxfsa = 0, size;
    FLOAT *mean, *cov, inorm;
  
    if (macro_type == mt_mean) 
    {
      xfsa  = ((Mean *)pData)->mpXformStatAccum;
      nxfsa = ((Mean *)pData)->mNumberOfXformStatAccums;
    } 
    
    else if (macro_type == mt_variance) 
    {
      xfsa  = ((Variance *)pData)->mpXformStatAccum;
      nxfsa = ((Variance *)pData)->mNumberOfXformStatAccums;
    }
  
    for (i = 0; i < nxfsa; i++) 
    {
      size = xfsa[i].mpXform->mInSize;
      mean = xfsa[i].mpStats;
      cov  = xfsa[i].mpStats + size;
      inorm = 1.0 / xfsa[i].mNorm;
  
      for (j = 0; j < size; j++) 
        mean[j] *= inorm; //normalize means
  
      if (macro_type == mt_variance) 
      {
        for (k=0; k < size; k++) 
        {
          for (j=0; j <= k; j++) 
          {                 //normalize covariances
            cov[k*(k+1)/2+j] = cov[k*(k+1)/2+j] * inorm - mean[k] * mean[j];
          }
        }
      }
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  WriteStatsForXform(int macro_type, HMMSetNodeName nodeName,
                     MacroData * pData, void * pUserData) 
  {
    XformStatsFileNames*     file = NULL;
    XformStatAccum*               xfsa = NULL;
    size_t                        i;
    size_t                        j;
    size_t                        k;
    size_t                        nxfsa = 0;
    int                           cc = 0;
    size_t                        size;
    FLOAT*                        mean;
    FLOAT*                        cov;
    WriteStatsForXformUserData*   ud = (WriteStatsForXformUserData *) pUserData;
  
    if (macro_type == mt_mean) 
    {
      file  = &ud->mMeanFile;
      xfsa  = ((Mean *)pData)->mpXformStatAccum;
      nxfsa = ((Mean *)pData)->mNumberOfXformStatAccums;
    } 
    else if (macro_type == mt_variance) 
    {
      file  = &ud->mCovFile;
      xfsa  = ((Variance *)pData)->mpXformStatAccum;
      nxfsa = ((Variance *)pData)->mNumberOfXformStatAccums;
    }
  
    for (i = 0; i < nxfsa && xfsa[i].mpXform != (Xform *) ud->mpXform; i++)
      ;
    
    if (i == nxfsa) 
      return;
  
    if (fprintf(file->mpOccupP, "%s "FLOAT_FMT"\n", nodeName, xfsa[i].mNorm) < 0) 
    {
      Error("Cannot write to file: %s", file->mpOccupN);
    }
  
    size = xfsa[i].mpXform->mInSize;
    mean = xfsa[i].mpStats;
    cov  = xfsa[i].mpStats + size;
  
    if (macro_type == mt_mean) 
    {
      if (ud->mBinary) 
      {
        if (!isBigEndian()) 
          for (i = 0; i < size; i++) swapFLOAT(mean[i]);
          
        cc |= (fwrite(mean, sizeof(FLOAT), size, file->mpStatsP) != size);
        
        if (!isBigEndian()) 
          for (i = 0; i < size; i++) swapFLOAT(mean[i]);
      } 
      else 
      {
        for (j=0;j<size;j++) 
        {
          cc |= fprintf(file->mpStatsP, FLOAT_FMT" ", mean[j]) < 0;
        }
        
        cc |= fputs("\n", file->mpStatsP) == EOF;
      }
    } 
    else 
    {
      if (ud->mBinary) 
      {
        size = size*(size+1)/2;
        if (!isBigEndian()) for (i = 0; i < size; i++) swapFLOAT(cov[i]);
        cc |= fwrite(cov, sizeof(FLOAT), size, file->mpStatsP) != size;
        if (!isBigEndian()) for (i = 0; i < size; i++) swapFLOAT(cov[i]);
      } 
      else
      {
        for (k=0; k < size; k++) 
        {
          for (j=0;j<=k;j++) {
            cc |= fprintf(file->mpStatsP, FLOAT_FMT" ", cov[k*(k+1)/2+j]) < 0;
          }
  
          for (;j<size; j++) {
            cc |= fprintf(file->mpStatsP, FLOAT_FMT" ", cov[j*(j+1)/2+k]) < 0;
          }
  
          cc |= fputs("\n", file->mpStatsP) == EOF;
        }
        cc |= fputs("\n", file->mpStatsP) == EOF;
      }
    }
  
    if (cc) {
      Error("Cannot write to file %s", file->mpStatsN);
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  ReadStatsForXform(int macro_type, HMMSetNodeName nodeName,
                    MacroData * pData, void *pUserData) 
  {
    char                        buff[128];
    XformStatsFileNames *  file = NULL;
    XformStatAccum *            xfsa = NULL;
    size_t                      i;
    size_t                      j;
    size_t                      k;
    size_t                      nxfsa = 0;
    int                         cc = 0;
    size_t                      size;
    FLOAT *                     mean;
    FLOAT *                     cov;
    FLOAT                       f;
    WriteStatsForXformUserData * ud = (WriteStatsForXformUserData *) pUserData;
  
    if (macro_type == mt_mean) 
    {
      file  = &ud->mMeanFile;
      xfsa  = ((Mean *)pData)->mpXformStatAccum;
      nxfsa = ((Mean *)pData)->mNumberOfXformStatAccums;
    } 
    else if (macro_type == mt_variance) 
    {
      file  = &ud->mCovFile;
      xfsa  = ((Variance *)pData)->mpXformStatAccum;
      nxfsa = ((Variance *)pData)->mNumberOfXformStatAccums;
    }
  
    for (i = 0; i < nxfsa && xfsa[i].mpXform != (Xform *) ud->mpXform; i++)
      ;
      
    if (i == nxfsa) 
      return;
  
    j = fscanf(file->mpOccupP, "%128s "FLOAT_FMT"\n", buff, &xfsa[i].mNorm);
    
    if (j < 1) 
      Error("Unexpected end of file: %s", file->mpOccupN);
    
    else if (strcmp(buff, nodeName)) 
      Error("'%s' expected but '%s' found in file: %s",nodeName,buff,file->mpOccupN);
    
    else if (j < 2) 
      Error("Decimal number expected after '%s'in file: %s", buff, file->mpOccupN);
    
    size = xfsa[i].mpXform->mInSize;
    mean = xfsa[i].mpStats;
    cov  = xfsa[i].mpStats + size;
  
    if (macro_type == mt_mean) 
    {
      if (ud->mBinary) 
      {
        j = fread(mean, sizeof(FLOAT), size, file->mpStatsP);
        cc |= j != size;
        if (!isBigEndian()) for (i = 0; i < size; i++) swapFLOAT(mean[i]);
      } 
      else 
      {
        for (j=0;j<size;j++)
        {
          cc |= (fscanf(file->mpStatsP, FLOAT_FMT" ", &mean[j]) != 1);
        }
      }
    } 
    else 
    {
      if (ud->mBinary) 
      {
        size = size*(size+1)/2;
        cc |= (fread(cov, sizeof(FLOAT), size, file->mpStatsP) != size);
        
        if (!isBigEndian()) 
          for (i = 0; i < size; i++) swapFLOAT(cov[i]);
      } 
      else
      {
        for (k=0; k < size; k++) 
        {
          for (j=0;j<k;j++) 
          {
            cc |= (fscanf(file->mpStatsP, FLOAT_FMT" ", &f) != 1);
            if (f != cov[k*(k+1)/2+j]) 
            {
              Error("Covariance matrix '%s' in file '%s' must be symetric",
                    nodeName, file->mpStatsP);
            }
          }
  
          for (;j<size; j++) {
            cc |= (fscanf(file->mpStatsP, FLOAT_FMT" ", &cov[j*(j+1)/2+k]) != 1);
          }
        }
      }
    }
  
    if (ferror(file->mpStatsP)) 
    {
      Error("Cannot read file '%s'", file->mpStatsN);
    } 
    else if (cc) 
    {
      Error("Invalid file with Xform statistics '%s'", file->mpStatsN);
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  WriteAccum(int macro_type, HMMSetNodeName nodeName,
             MacroData * pData, void *pUserData) 
  {
    size_t                i;
    size_t                size;
    FLOAT *               vector = NULL;
  //  FILE *fp = (FILE *) pUserData;
    WriteAccumUserData *  ud = (WriteAccumUserData * ) pUserData;
    Macro *               macro = static_cast <MacroData *> (pData)->mpMacro;
  
    if (macro &&
      (fprintf(ud->mpFp, "~%c \"%s\"", macro->mType, macro->mpName) < 0 ||
       fwrite(&macro->mOccurances, sizeof(macro->mOccurances), 1, ud->mpFp) != 1)) 
    {
      Error("Cannot write accumulators to file: '%s'", ud->mpFileName);
    }
  
    if (macro_type == mt_mean || macro_type == mt_variance) 
    {
      XformStatAccum *    xfsa = NULL;
      UINT_32             nxfsa = 0;
  
      if (macro_type == mt_mean) 
      {
        xfsa   = ((Mean *)pData)->mpXformStatAccum;
        nxfsa  = ((Mean *)pData)->mNumberOfXformStatAccums;
        size   = ((Mean *)pData)->VectorSize();
        vector = ((Mean *)pData)->mpVectorO+size;
        size   = size + 1;
      } 
      else if (macro_type == mt_variance) 
      {
        xfsa   = ((Variance *)pData)->mpXformStatAccum;
        nxfsa  = ((Variance *)pData)->mNumberOfXformStatAccums;
        size   = ((Variance *)pData)->VectorSize();
        vector = ((Variance *)pData)->mpVectorO+size;
        size   = size * 2 + 1;
      }
  
  //    if (ud->mMmi) vector += size; // Move to MMI accums, which follows ML accums
  
      if (fwrite(vector, sizeof(FLOAT), size, ud->mpFp) != size ||
          fwrite(&nxfsa, sizeof(nxfsa),    1, ud->mpFp) != 1) 
      {
        Error("Cannot write accumulators to file: '%s'", ud->mpFileName);
      }
  
  //    if (!ud->mMmi) { // MMI estimation of Xform statistics has not been implemented yet
      for (i = 0; i < nxfsa; i++) 
      {
        size = xfsa[i].mpXform->mInSize;
        size = (macro_type == mt_mean) ? size : size+size*(size+1)/2;
        assert(xfsa[i].mpXform->mpMacro != NULL);
        if (fprintf(ud->mpFp, "\"%s\"", xfsa[i].mpXform->mpMacro->mpName) < 0 ||
          fwrite(&size,           sizeof(size),        1, ud->mpFp) != 1    ||
          fwrite(xfsa[i].mpStats, sizeof(FLOAT), size, ud->mpFp) != size ||
          fwrite(&xfsa[i].mNorm,  sizeof(FLOAT),    1, ud->mpFp) != 1) 
        {
          Error("Cannot write accumulators to file: '%s'", ud->mpFileName);
        }
      }
  //    }
    }
    
    else if (macro_type == mt_state) 
    {
      State *state = (State *) pData;
      if (state->mOutPdfKind == KID_DiagC) 
      {
        for (i = 0; i < state->mNumberOfMixtures; i++) 
        {
          if (fwrite(&state->mpMixture[i].mWeightAccum,
                    sizeof(FLOAT), 1, ud->mpFp) != 1 ||
            fwrite(&state->mpMixture[i].mWeightAccumDen,
                    sizeof(FLOAT), 1, ud->mpFp) != 1) 
          {
            Error("Cannot write accumulators to file: '%s'", ud->mpFileName);
          }
        }
      }
    } 
    
    else if (macro_type == mt_transition) 
    {
      size   = SQR(((Transition *) pData)->mNStates);
      vector = ((Transition *) pData)->mpMatrixO + size;
  
      if (fwrite(vector, sizeof(FLOAT), size, ud->mpFp) != size) 
      {
        Error("Cannot write accumulators to file: '%s'", ud->mpFileName);
      }
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  NormalizeAccum(int macro_type, HMMSetNodeName nodeName,
                 MacroData * pData, void *pUserData) 
  {
    size_t      i;
    size_t      j;
    size_t      size   = 0;
    FLOAT *     vector = NULL;
  
    if (macro_type == mt_mean || macro_type == mt_variance) 
    {
      XformStatAccum *  xfsa = NULL;
      size_t            nxfsa = 0;
  
      if (macro_type == mt_mean) 
      {
        xfsa   = ((Mean *)pData)->mpXformStatAccum;
        nxfsa  = ((Mean *)pData)->mNumberOfXformStatAccums;
        size   = ((Mean *)pData)->VectorSize();
        vector = ((Mean *)pData)->mpVectorO+size;
        size   = size + 1;
      } 
      else if (macro_type == mt_variance) 
      {
        xfsa   = ((Variance *)pData)->mpXformStatAccum;
        nxfsa  = ((Variance *)pData)->mNumberOfXformStatAccums;
        size   = ((Variance *)pData)->VectorSize();
        vector = ((Variance *)pData)->mpVectorO+size;
        size   = size * 2 + 1;
      }
  
      for (i=0; i < size; i++) 
        vector[i] /= vector[size-1];
  
      for (i = 0; i < nxfsa; i++) 
      {
        size = xfsa[i].mpXform->mInSize;
        size = (macro_type == mt_mean) ? size : size+size*(size+1)/2;
  
        for (j=0; j < size; j++) 
          xfsa[i].mpStats[j] /= xfsa[i].mNorm;
        
        xfsa[i].mNorm = 1.0;
      }
    } 
    else if (macro_type == mt_state) 
    {
      State *state = (State *) pData;
      
      if (state->mOutPdfKind == KID_DiagC) 
      {
        FLOAT accum_sum = 0.0;
  
        for (i = 0; i < state->mNumberOfMixtures; i++)
          accum_sum += state->mpMixture[i].mWeightAccum;
  
        if (accum_sum > 0.0) 
        {
          for (i = 0; i < state->mNumberOfMixtures; i++)
            state->mpMixture[i].mWeightAccum /= accum_sum;
        }
      }
    } 
    else if (macro_type == mt_transition) 
    {
      size_t nstates = ((Transition *) pData)->mNStates;
      vector = ((Transition *) pData)->mpMatrixO + SQR(nstates);
  
      for (i=0; i < nstates; i++) 
      {
        FLOAT nrm = LOG_0;
        
        for (j=0; j < nstates; j++) 
        {
          LOG_INC(nrm, vector[i * nstates + j]);
        }
        
        if (nrm < LOG_MIN) nrm = 0.0;
        
        for (j=0; j < nstates; j++) 
        {
          vector[i * nstates + j] -= nrm;
        }
      }
    }
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  unsigned int faddfloat(FLOAT *vec, size_t size, float mul_const, FILE *fp) 
  {
    size_t    i;
    FLOAT     f;
  
    for (i = 0; i < size; i++) 
    {
      if (fread(&f, sizeof(FLOAT), 1, fp) != 1) break;
      vec[i] += f * mul_const;
    }
    
    return i;
  }
  
  
  //*****************************************************************************
  //*****************************************************************************
  void ReadAccum(int macro_type, HMMSetNodeName nodeName,
                  MacroData * pData, void *pUserData) 
  {
    unsigned int        i;
    unsigned int        j;
    int                 c;
    unsigned int        size   = 0;
    FLOAT *             vector = NULL;
    Macro *             macro;
    ReadAccumUserData * ud = (ReadAccumUserData *) pUserData;
  //  FILE *fp =        ((ReadAccumUserData *) pUserData)->fp;
  //  char *fn =        ((ReadAccumUserData *) pUserData)->fn;
  //  ModelSet *hmm_set = ((ReadAccumUserData *) pUserData)->hmm_set;
  //  float weight =    ((ReadAccumUserData *) pUserData)->weight;
    char                xfName[128];
  
    xfName[sizeof(xfName)-1] = '\0';
  
    if (macro_type == mt_mean || macro_type == mt_variance) 
    {
      XformStatAccum *  xfsa = NULL;
      UINT_32           size_inf;
      UINT_32           nxfsa_inf;
      size_t            nxfsa = 0;
  
      if (macro_type == mt_mean) 
      {
        xfsa   = ((Mean *)pData)->mpXformStatAccum;
        nxfsa  = ((Mean *)pData)->mNumberOfXformStatAccums;
        size   = ((Mean *)pData)->VectorSize();
        vector = ((Mean *)pData)->mpVectorO+size;
        size   = size + 1;
      } 
      else if (macro_type == mt_variance) 
      {
        xfsa   = ((Variance *)pData)->mpXformStatAccum;
        nxfsa  = ((Variance *)pData)->mNumberOfXformStatAccums;
        size   = ((Variance *)pData)->VectorSize();
        vector = ((Variance *)pData)->mpVectorO+size;
        size   = size * 2 + 1;
      }
  
      if (ud->mMmi) 
        vector += size;
  
      if (faddfloat(vector, size, ud->mWeight,    ud->mpFp) != size ||
          fread(&nxfsa_inf, sizeof(nxfsa_inf), 1, ud->mpFp) != 1) 
      {
        Error("Incompatible accumulator file: '%s'", ud->mpFileName);
      }
  
      if (!ud->mMmi) { // MMI estimation of Xform statistics has not been implemented yet
        for (i = 0; i < nxfsa_inf; i++) 
        {
          if (getc(ud->mpFp) != '"') 
            Error("Incompatible accumulator file: '%s'", ud->mpFileName);
  
          for (j=0; (c=getc(ud->mpFp)) != EOF && c != '"' && j < sizeof(xfName)-1; j++) 
            xfName[j] = c;
  
          xfName[j] = '\0';
          
          if (c == EOF)
            Error("Incompatible accumulator file: '%s'", ud->mpFileName);
  
          macro = FindMacro(&ud->mpModelSet->mXformHash, xfName);
  
          if (fread(&size_inf, sizeof(size_inf), 1, ud->mpFp) != 1) 
            Error("Incompatible accumulator file: '%s'", ud->mpFileName);
  
          if (macro != NULL) 
          {
            size = ((LinearXform *) macro->mpData)->mInSize;
            size = (macro_type == mt_mean) ? size : size+size*(size+1)/2;
  
            if (size != size_inf)
              Error("Incompatible accumulator file: '%s'", ud->mpFileName);
  
            for (j = 0; j < nxfsa && xfsa[j].mpXform != macro->mpData; j++)
              ;
            
            if (j < nxfsa) 
            {
              if (faddfloat(xfsa[j].mpStats, size, ud->mWeight, ud->mpFp) != size  ||
                  faddfloat(&xfsa[j].mNorm,     1, ud->mWeight, ud->mpFp) != 1) 
              {
                Error("Invalid accumulator file: '%s'", ud->mpFileName);
              }
            } 
            else 
            {
              macro = NULL;
            }
          }
  
          // Skip Xform accumulator
          if (macro == NULL) 
          { 
            FLOAT f;
            for (j = 0; j < size_inf+1; j++) 
              fread(&f, sizeof(f), 1, ud->mpFp);
          }
        }
      }
    } else if (macro_type == mt_state) {
      State *state = (State *) pData;
      if (state->mOutPdfKind == KID_DiagC) {
        FLOAT junk;
        for (i = 0; i < state->mNumberOfMixtures; i++) {
          if (ud->mMmi == 1) {
            if (faddfloat(&state->mpMixture[i].mWeightAccumDen, 1, ud->mWeight, ud->mpFp) != 1 ||
                faddfloat(&junk,                                1, ud->mWeight, ud->mpFp) != 1) {
              Error("Incompatible accumulator file: '%s'", ud->mpFileName);
            }
          } else if (ud->mMmi == 2) {
            if (faddfloat(&junk,                                1, ud->mWeight, ud->mpFp) != 1 ||
                faddfloat(&state->mpMixture[i].mWeightAccumDen, 1, ud->mWeight, ud->mpFp) != 1) {
              Error("Incompatible accumulator file: '%s'", ud->mpFileName);
            }
          } else {
            if (faddfloat(&state->mpMixture[i].mWeightAccum,     1, ud->mWeight, ud->mpFp) != 1 ||
                faddfloat(&state->mpMixture[i].mWeightAccumDen,  1, ud->mWeight, ud->mpFp) != 1) {
              Error("Incompatible accumulator file: '%s'", ud->mpFileName);
            }
          }
        }
      }
    } else if (macro_type == mt_transition) {
      FLOAT f;
      size   = SQR(((Transition *) pData)->mNStates);
      vector =     ((Transition *) pData)->mpMatrixO + size;
  
      for (i = 0; i < size; i++) {
        if (fread(&f, sizeof(FLOAT), 1, ud->mpFp) != 1) {
        Error("Incompatible accumulator file: '%s'", ud->mpFileName);
        }
        if (!ud->mMmi) { // MMI estimation of transition probabilities has not been implemented yet
          f += log(ud->mWeight);
          LOG_INC(vector[i], f);
        }
      }
    }
  }
  

  //**************************************************************************
  //**************************************************************************
  //   Hmm class section
  //**************************************************************************
  //**************************************************************************
  Hmm::
  Hmm(size_t nStates) : 
    mpTransition(NULL)
  {
    // we allocate pointers for states. The -2 is for the non-emmiting states
    mpState   = new State*[nStates - 2];
    mNStates  = nStates;
  }
  
  //**************************************************************************
  //**************************************************************************
  Hmm::
  ~Hmm()
  {
    delete [] mpState;
  }
  
  //**************************************************************************  
  //**************************************************************************
  void
  Hmm::
  UpdateFromAccums(const ModelSet * pModelSet)
  {
    size_t i;
  
    for (i = 0; i < mNStates - 2; i++) 
    {
      if (!this->mpState[i]->mpMacro) 
        mpState[i]->UpdateFromAccums(pModelSet, this);
    }
  
    if (!mpTransition->mpMacro) 
      mpTransition->UpdateFromAccums(pModelSet);
  } // UpdateFromAccums(const ModelSet * pModelSet)

  //*****************************************************************************
  //*****************************************************************************
  void 
  Hmm::
  Scan(int mask, HMMSetNodeName nodeName,
       ScanAction action, void *pUserData)
  {
    size_t    i;
    size_t    n = 0;
    char *    chptr = NULL;
  
    if (nodeName != NULL) 
    {
      n = strlen(nodeName);
      chptr = nodeName + n;
      n = sizeof(HMMSetNodeName) - 4 - n;
    }
  
    
    if (mask & MTM_HMM && mask & MTM_PRESCAN) 
      action(mt_hmm, nodeName, this, pUserData);
  
    if (mask & (MTM_ALL & ~(MTM_HMM | MTM_TRANSITION))) 
    {
      for (i=0; i < mNStates-2; i++) 
      {
        if (!mpState[i]->mpMacro) 
        {
          if (n > 0 ) snprintf(chptr, n, ".state[%d]", (int) i+2);
          mpState[i]->Scan(mask, nodeName, action, pUserData);
        }
      }
    }
  
    if (mask & MTM_TRANSITION && !mpTransition->mpMacro) 
    {
      if (n > 0) strncpy(chptr, ".transP", n);
      action(mt_transition, nodeName, mpTransition, pUserData);
    }
  
    if (mask & MTM_HMM && !(mask & MTM_PRESCAN)) 
    {
      if (n > 0) chptr = '\0';
      action(mt_hmm, nodeName, this, pUserData);
    }
  }
  
  
  //**************************************************************************
  //**************************************************************************
  //   Mixture section
  //**************************************************************************
  //**************************************************************************
  void 
  Mixture::
  ComputeGConst()
  {
    FLOAT cov_det = 0;
    size_t i;
  
    for (i = 0; i < mpVariance->VectorSize(); i++) 
    {
      cov_det -= log(mpVariance->mpVectorO[i]);
    }
    mGConst = cov_det + M_LOG_2PI * mpVariance->VectorSize();
  }  

  //***************************************************************************
  //***************************************************************************
  Variance *
  Mixture::
  FloorVariance(const ModelSet * pModelSet)
  {
    size_t   i;                                   // element index
    FLOAT    g_floor = pModelSet->mMinVariance;   // global varfloor
    Variance *f_floor = NULL;                     // flooring vector
    
    
    f_floor = (mpInputXform != NULL) ? mpInputXform->mpVarFloor 
                                     : pModelSet->mpVarFloor;
    
    // none of the above needs to be set, so 
    if (f_floor != NULL)
    {
      assert(f_floor->VectorSize() == mpVariance->VectorSize());
      
      // go through each element and update
      for (i = 0; i < mpVariance->VectorSize(); i++)
      {
        mpVariance->mpVectorO[i] = LOWER_OF(mpVariance->mpVectorO[i],
                                            f_floor->mpVectorO[i]);
      }
    }
    
    // we still may have global varfloor constant set
    if (g_floor > 0.0)
    {
printf("%f", g_floor);
      // go through each element and update
      g_floor =  1.0 / g_floor;
      for (i = 0; i < mpVariance->VectorSize(); i++)
      {
        mpVariance->mpVectorO[i] = LOWER_OF(mpVariance->mpVectorO[i], g_floor);
      }
    }
    
    return mpVariance;
  } // FloorVariance(...)
  
  //**************************************************************************
  //**************************************************************************
  void 
  Mixture::
  UpdateFromAccums(const ModelSet * pModelSet) 
  {
    double  Djm;
    int     i;

    if (pModelSet->mMmiUpdate == 1 || pModelSet->mMmiUpdate == -1) 
    {
      int vec_size    = mpVariance->VectorSize();
      FLOAT *mean_vec = mpMean->mpVectorO;
      FLOAT *var_vec  = mpVariance->mpVectorO;
  
      FLOAT *vac_num  = var_vec + 1 * vec_size;
      FLOAT *mac_num  = var_vec + 2 * vec_size;
      FLOAT *nrm_num  = var_vec + 3 * vec_size;
  
      FLOAT *vac_den  = vac_num + 2 * vec_size + 1;
      FLOAT *mac_den  = mac_num + 2 * vec_size + 1;
      FLOAT *nrm_den  = nrm_num + 2 * vec_size + 1;
  
      if (pModelSet->mMmiUpdate == 1) 
      {
        // I-smoothing
        for (i = 0; i < vec_size; i++) 
        {
          mac_num[i] *= (*nrm_num + pModelSet->MMI_tauI) / *nrm_num;
          vac_num[i] *= (*nrm_num + pModelSet->MMI_tauI) / *nrm_num;
        }
        *nrm_num   += pModelSet->MMI_tauI;
      }
  
      Djm = 0.0;
      
      // Find minimum Djm leading to positive update of variances
      for (i = 0; i < vec_size; i++) {
        double macn_macd = mac_num[i]-mac_den[i];
        double vacn_vacd = vac_num[i]-vac_den[i];
        double nrmn_nrmd = *nrm_num - *nrm_den;
        double a  = 1/var_vec[i];
        double b  = vacn_vacd + nrmn_nrmd * (1/var_vec[i] + SQR(mean_vec[i])) -
                  2 * macn_macd * mean_vec[i];
        double c  = nrmn_nrmd * vacn_vacd - SQR(macn_macd);
        double Dd = (- b + sqrt(SQR(b) - 4 * a * c)) / (2 * a);
  
        Djm = HIGHER_OF(Djm, Dd);
      }
  
      Djm = HIGHER_OF(pModelSet->MMI_h * Djm, pModelSet->MMI_E * *nrm_den);
  
      if (pModelSet->mMmiUpdate == -1) 
      {
        // I-smoothing
        for (i = 0; i < vec_size; i++) 
        {
          mac_num[i] *= (*nrm_num + pModelSet->MMI_tauI) / *nrm_num;
          vac_num[i] *= (*nrm_num + pModelSet->MMI_tauI) / *nrm_num;
        }
        *nrm_num   += pModelSet->MMI_tauI;
      }
      
      for (i = 0; i < vec_size; i++) 
      {
        double macn_macd = mac_num[i]-mac_den[i];
        double vacn_vacd = vac_num[i]-vac_den[i];
        double nrmn_nrmd = *nrm_num - *nrm_den;
  
        double new_mean = (macn_macd + Djm * mean_vec[i]) / (nrmn_nrmd + Djm);
        var_vec[i]     = 1/((vacn_vacd + Djm * (1/var_vec[i] + SQR(mean_vec[i]))) /
                        (nrmn_nrmd + Djm) - SQR(new_mean));
        mean_vec[i]    = new_mean;
  
        /* We will floor the variance separately at the end of this member function
        if (pModelSet->mpVarFloor) {
          var_vec[i] = LOWER_OF(var_vec[i], pModelSet->mpVarFloor->mpVectorO[i]);
        }
        */
      }
    } 
    
    // //////////
    // MFE update
    else if (pModelSet->mMmiUpdate == 2 || pModelSet->mMmiUpdate == -2 ) 
    { 
      int vec_size    = mpVariance->VectorSize();
      FLOAT *mean_vec = mpMean->mpVectorO;
      FLOAT *var_vec  = mpVariance->mpVectorO;
  
      FLOAT *vac_mle  = var_vec + 1 * vec_size;
      FLOAT *mac_mle  = var_vec + 2 * vec_size;
      FLOAT *nrm_mle  = var_vec + 3 * vec_size;
  
      FLOAT *vac_mfe  = vac_mle + 2 * vec_size + 1;
      FLOAT *mac_mfe  = mac_mle + 2 * vec_size + 1;
      FLOAT *nrm_mfe  = nrm_mle + 2 * vec_size + 1;
  
      if (pModelSet->mMmiUpdate == 2) 
      {
        // I-smoothing
        for (i = 0; i < vec_size; i++) {
          mac_mfe[i] += (pModelSet->MMI_tauI / *nrm_mle * mac_mle[i]);
          vac_mfe[i] += (pModelSet->MMI_tauI / *nrm_mle * vac_mle[i]);
        }
        *nrm_mfe += pModelSet->MMI_tauI;
      }
      
      Djm = 0.0;
      
      // Find minimum Djm leading to positive update of variances
      for (i = 0; i < vec_size; i++) 
      {
        double macn_macd = mac_mfe[i];
        double vacn_vacd = vac_mfe[i];
        double nrmn_nrmd = *nrm_mfe;
        double a  = 1/var_vec[i];
        double b  = vacn_vacd + nrmn_nrmd * (1/var_vec[i] + SQR(mean_vec[i])) -
                  2 * macn_macd * mean_vec[i];
        double c  = nrmn_nrmd * vacn_vacd - SQR(macn_macd);
        double Dd = (- b + sqrt(SQR(b) - 4 * a * c)) / (2 * a);
  
        Djm = HIGHER_OF(Djm, Dd);
      }
  
      // gWeightAccumDen is passed using quite ugly hack that work
      // only if mixtures are not shared by more states - MUST BE REWRITEN
      Djm = HIGHER_OF(pModelSet->MMI_h * Djm, pModelSet->MMI_E * gWeightAccumDen);
  
      if (pModelSet->mMmiUpdate == -2) 
      {
        // I-smoothing
        for (i = 0; i < vec_size; i++) 
        {
          mac_mfe[i] += (pModelSet->MMI_tauI / *nrm_mle * mac_mle[i]);
          vac_mfe[i] += (pModelSet->MMI_tauI / *nrm_mle * vac_mle[i]);
        }
        *nrm_mfe += pModelSet->MMI_tauI;
      }
  
      for (i = 0; i < vec_size; i++) 
      {
        double macn_macd = mac_mfe[i];
        double vacn_vacd = vac_mfe[i];
        double nrmn_nrmd = *nrm_mfe;
        double new_mean  = (macn_macd + Djm * mean_vec[i]) / (nrmn_nrmd + Djm);
        var_vec[i]       = 1 / ((vacn_vacd + Djm * (1/var_vec[i] + SQR(mean_vec[i]))) /
                               (nrmn_nrmd + Djm) - SQR(new_mean));
        mean_vec[i]      = new_mean;
  
        /* We will floor variance separately in the end
        if (pModelSet->mpVarFloor) 
          var_vec[i] = LOWER_OF(var_vec[i], pModelSet->mpVarFloor->mpVectorO[i]);
        */
      }
    }
     
    // ///////
    // ordinary update    
    else 
    {
      if (!mpVariance->mpMacro)
        mpVariance->UpdateFromAccums(pModelSet);
  
      if (!mpMean->mpMacro)
        mpMean->UpdateFromAccums(pModelSet);
    }
    
    // Perform some variance flooring
    FloorVariance(pModelSet);
    
    // recompute the GConst for the Gaussian mixture
    ComputeGConst();
  }; // UpdateFromAccums(const ModelSet * pModelSet) 

  
  //***************************************************************************
  //***************************************************************************
  void 
  Mixture::
  Scan(int mask, HMMSetNodeName nodeName, ScanAction action, void *pUserData)
  {
    int n = 0;
    char *chptr = NULL;
    
    if (nodeName != NULL) 
    {
      n = strlen(nodeName);
      chptr = nodeName + n;
      n = sizeof(HMMSetNodeName) - 4 - n;
    }
  
    if (mask & MTM_MIXTURE && mask & MTM_PRESCAN) {
      action(mt_mixture, nodeName, this, pUserData);
    }
  
    if (mask & MTM_MEAN && !mpMean->mpMacro) {
      if (n > 0) strncpy(chptr, ".mean", n);
      action(mt_mean, nodeName, mpMean, pUserData);
    }
  
    if (mask & MTM_VARIANCE && !mpVariance->mpMacro) {
      if (n > 0) strncpy(chptr, ".cov", n);
      action(mt_variance, nodeName, mpVariance, pUserData);
    }
  
    if (mask & MTM_XFORM_INSTANCE && mpInputXform &&
      !mpInputXform->mpMacro) {
      if (n > 0) strncpy(chptr, ".input", n);
      mpInputXform->Scan(mask, nodeName, action, pUserData);
    }
  
    if (mask & MTM_MIXTURE && !(mask & MTM_PRESCAN)) {
      if (n > 0) chptr = '\0';
      action(mt_mixture, nodeName, this, pUserData);
    }
  }

        
  
  //**************************************************************************  
  //**************************************************************************  
  // Mean section
  //**************************************************************************  
  //**************************************************************************  
  Mean::
  Mean(size_t vectorSize, bool allocateAccums)
  {
    size_t accum_size = 0;
    size_t size;
    size_t skip;
    void* free_vec;
    
    if (allocateAccums) 
      accum_size = (vectorSize + 1) * 2; // * 2 for MMI accums

    size = (vectorSize + accum_size)*sizeof(FLOAT);
        
    mpVectorO = static_cast<FLOAT*>
      stk_memalign(16, size, &free_vec);
#ifdef STK_MEMALIGN_MANUAL
    mpVectorOFree = static_cast<FLOAT*>(free_vec);
#endif
    
    mVectorSize               = vectorSize;    
    mpXformStatAccum          = NULL;
    mNumberOfXformStatAccums  = 0;
    mUpdatableFromStatAccums  = true;
    mpWeights                 = NULL;
    mNWeights                 = 0;
    mpOccProbAccums           = NULL;
  }  
  
  
  //**************************************************************************  
  //**************************************************************************  
  Mean::
  ~Mean()
  {
    if (mpXformStatAccum != NULL)
    {
      free(mpXformStatAccum->mpStats);
      free(mpXformStatAccum);
    }
    
    // delete refferences to weights Bias xforms
    if (NULL != mpWeights)
    { 
      delete [] mpWeights;
    }
    
    if (NULL != mpOccProbAccums)
    {
      delete [] mpOccProbAccums;
    }
    
#ifdef STK_MEMALIGN_MANUAL
    free(mpVectorOFree);
#else
    free(mpVectorO);
#endif
  }  
  
  //**************************************************************************  
  //**************************************************************************  
  void
  Mean::
  UpdateFromAccums(const ModelSet * pModelSet)
  {
    size_t   i;
    FLOAT *  vec  = mpVectorO;
    FLOAT *  acc  = mpVectorO + 1 * VectorSize();
    FLOAT    nrm  = mpVectorO  [2 * VectorSize()];
  
    if (pModelSet->mUpdateMask & UM_MEAN) 
    {
      if (mNumberOfXformStatAccums == 0) 
      {
        for (i = 0; i < VectorSize(); i++) 
        {
          vec[i] = acc[i] / nrm;
        }
      } 
      else 
      { // Updating from xform statistics
        size_t r;
        size_t c;
        size_t pos = 0;
        
        if (!mUpdatableFromStatAccums) 
          return;
        
        // If 'mUpdatableFromStatAccums' is true then 'mNumberOfXformStatAccums' is equal to 1
        for (i=0; i < mNumberOfXformStatAccums; i++) 
        {
          LinearXform * xform   = (LinearXform *) mpXformStatAccum[i].mpXform;
          size_t        in_size = xform->mInSize;
          FLOAT *       mnv     = mpXformStatAccum[i].mpStats;
  
          assert(xform->mXformType == XT_LINEAR);
          assert(pos + xform->mOutSize <= VectorSize());
          
          for (r = 0; r < xform->mOutSize; r++) 
          {
            mpVectorO[pos + r] = 0;
            for (c = 0; c < in_size; c++) 
            {
              mpVectorO[pos + r] += mnv[c] * xform->mpMatrixO[in_size * r + c];
            }
          }
          pos += xform->mOutSize;
        }
        
        assert(pos == VectorSize());
      }
    }
  } // UpdateFromAccums(const ModelSet * pModelSet)
  
  
  //**************************************************************************  
  //**************************************************************************  
  void 
  Mean::
  RecalculateCAT()
  {
    if (NULL != mpWeights)
    {
      std::cerr << "Recalculating..." << std::endl;
      //:KLUDGE: optimize this
      memset(mpVectorO, 0, sizeof(FLOAT) * VectorSize());      
      int v = 0;
      
      // go through weights vectors
      for (size_t w = 0; w < mNWeights; w++)
      {
        // go through rows in the cluster matrix
        for (size_t i = 0; i < mpWeights[w]->mInSize; i++)
        {
          // go through cols in the cluster matrix
          for (size_t j = 0; j < VectorSize(); j++)
          {
            std::cerr << "Recalculating using " << mpWeights[w]->mpMacro->mpName << "  " << mpVectorO[j] << "+=" << mClusterMatrix(v, j) << "*" << mpWeights[w]->mVector[0][i] << std::endl;
            mpVectorO[j] += mClusterMatrix(v, j) * mpWeights[w]->mVector[0][i];
          }
          // move to next cluster mean vector
          v++;
        }
      }
      
    }
  }
  
  
  //**************************************************************************  
  //**************************************************************************  
  // Variance section
  //**************************************************************************  
  //**************************************************************************  
  Variance::
  Variance(size_t vectorSize, bool allocateAccums)
  {
    size_t accum_size = 0;
    void* free_vec;
    
    if (allocateAccums) 
      accum_size = (2*vectorSize + 1) * 2; // * 2 for MMI accums
    
    mpVectorO = static_cast<FLOAT*>
      (stk_memalign(16, (vectorSize + accum_size) * sizeof(FLOAT), &free_vec));
#ifdef STK_MEMALIGN_MANUAL
    mpVectorOFree = static_cast<FLOAT*>(free_vec);
#endif
    
    mVectorSize               = vectorSize;    
    mpXformStatAccum          = NULL;
    mNumberOfXformStatAccums  = 0;
    mUpdatableFromStatAccums  = true;
  }  
  
  //**************************************************************************  
  //**************************************************************************  
  Variance::
  ~Variance()
  {
    if (mpXformStatAccum != NULL)
    {
      free(mpXformStatAccum->mpStats);
      free(mpXformStatAccum);
    }
    
#ifdef STK_MEMALIGN_MANUAL
    free(mpVectorOFree);
#else
    free(mpVectorO);
#endif
  }  
  
  //**************************************************************************  
  //**************************************************************************  
  void
  Variance::
  UpdateFromAccums(const ModelSet * pModelSet)
  {
    size_t    i;
  
    if (pModelSet->mUpdateMask & UM_VARIANCE) 
    {
      if (mNumberOfXformStatAccums == 0) 
      {
        FLOAT *vec  = mpVectorO;
        FLOAT *vac  = mpVectorO + 1 * VectorSize(); // varriance accum 
        FLOAT *mac  = mpVectorO + 2 * VectorSize(); // mean accum 
        FLOAT *nrm  = mpVectorO + 3 * VectorSize(); // norm - occupation count
        
        for (i = 0; i < VectorSize(); i++) 
        {
          if (pModelSet->mUpdateMask & UM_OLDMEANVAR) 
          {
            vec[i] = *nrm / vac[i];
          } 
          else 
          {
            vec[i] = 1 / HIGHER_OF(0.0, vac[i] / *nrm - SQR(mac[i] / *nrm)); 
                      // HIGHER_OF is to avoid negative variances
          }
          
          /* This block is moved one level higher
          
          // !!! Need for transformation dependent varFloors
          if (pModelSet->mpVarFloor && 
              pModelSet->mpVarFloor->VectorSize() == VectorSize()) 
          {
            vec[i] = LOWER_OF(vec[i], pModelSet->mpVarFloor->mpVectorO[i]);
          }
          */
        }
      } 
      
      else // Updating from xform statistics
      { 
        size_t r;
        size_t c;
        size_t t;
        size_t pos = 0;
        
        if (!mUpdatableFromStatAccums) 
          return;
  
  //    If 'mUpdatableFromStatAccums' is true then 'mNumberOfXformStatAccums' is equal to 1
        for (i=0; i<mNumberOfXformStatAccums; i++) 
        {
          LinearXform * xform = (LinearXform *) mpXformStatAccum[i].mpXform;
          size_t        in_size = xform->mInSize;
          FLOAT *       cov  = mpXformStatAccum[i].mpStats + in_size;
  
          assert(xform->mXformType == XT_LINEAR);
          assert(pos + xform->mOutSize <= VectorSize());
          
          for (r = 0; r < xform->mOutSize; r++) 
          {
            mpVectorO[pos + r] = 0.0;
  
            for (c = 0; c < in_size; c++) 
            {
              FLOAT aux = 0;
              for (t = 0; t <= c; t++) 
              {
                aux += cov[c * (c+1)/2 + t] * xform->mpMatrixO[in_size * r + t];
              }
  
              for (; t < in_size; t++) 
              {
                aux += cov[t * (t+1)/2 + c] * xform->mpMatrixO[in_size * r + t];
              }
              
              mpVectorO[pos + r] += aux     * xform->mpMatrixO[in_size * r + c];
            }
            
            mpVectorO[pos + r] = 1 / HIGHER_OF(0.0, mpVectorO[pos + r]);

            /* This block is moved one level higher
            if (pModelSet->mpVarFloor && 
                pModelSet->mpVarFloor->VectorSize() == VectorSize()) // !!! Need for transformation dependent varFloors
            {
                 mpVectorO[pos + r] = LOWER_OF(mpVectorO[pos + r], pModelSet->mpVarFloor->mpVectorO[pos + r]);
            }
            */
          } // for (r = 0; r < xform->mOutSize; r++) 
          
          pos += xform->mOutSize;
        } // for (i=0; i<mNumberOfXformStatAccums; i++) 
        
        assert(pos == VectorSize());
      }
    }
  }

  
  //**************************************************************************  
  //**************************************************************************  
  // Transition section
  //**************************************************************************  
  //**************************************************************************  
  Transition::
  Transition(size_t nStates, bool allocateAccums)
  {
    size_t  alloc_size;
    alloc_size = allocateAccums ? 2 * SQR(nStates) + nStates: SQR(nStates);
    
    mpMatrixO = new FLOAT[alloc_size];
    mNStates = nStates;
  }
  
  //**************************************************************************  
  //**************************************************************************  
  Transition::
  ~Transition()
  {
    delete [] mpMatrixO;
  }

  //**************************************************************************  
  //**************************************************************************  
  void
  Transition::
  UpdateFromAccums(const ModelSet * pModelSet)
  {
    int       i;
    int       j;
    int       nstates = mNStates;
    FLOAT *   vec = mpMatrixO;
    FLOAT *   acc = mpMatrixO + 1 * SQR(mNStates);
  
    if (pModelSet->mUpdateMask & UM_TRANSITION) 
    {
      for (i=0; i < nstates; i++) 
      {
        FLOAT nrm = LOG_0;
        for (j=0; j < nstates; j++) 
        {
          LOG_INC(nrm, acc[i * nstates + j]);
        }
        
        if (nrm == LOG_0) 
          nrm = 0;
        
        for (j=0; j < nstates; j++) 
        {
          vec[i * nstates + j] = acc[i * nstates + j] - nrm; // it is in log
        }
      }
    }
  }
  
  
  //**************************************************************************  
  //**************************************************************************  
  // Transition section
  //**************************************************************************  
  //**************************************************************************  
  
  //***************************************************************************
  //***************************************************************************
  State::
  State(size_t numMixtures)
  {
    mpMixture = (numMixtures > 0) ? new MixtureLink[numMixtures] : NULL;    
  }
  
  
  //***************************************************************************
  //***************************************************************************
  State::
  ~State()
  {
    if (mpMixture != NULL)
      delete [] mpMixture;
  }

  
  //***************************************************************************
  //***************************************************************************
  void 
  State::
  UpdateFromAccums(const ModelSet * pModelSet, const Hmm * pHmm) 
  {
    size_t i;
  
    if (mOutPdfKind == KID_DiagC) 
    {
      FLOAT accum_sum = 0;
  
//    if (hmm_set->mUpdateMask & UM_WEIGHT) {
      for (i = 0; i < mNumberOfMixtures; i++) 
      {
        accum_sum += mpMixture[i].mWeightAccum;
      }

      if (accum_sum <= 0.0) 
      {
        if (mpMacro) 
        {
          Warning("No occupation of '%s', state is not updated",
                  mpMacro->mpName);
        } 
        else 
        {
          size_t j; // find the state number
          for (j=0; j < pHmm->mNStates && pHmm->mpState[j] != this; j++)
            ;
          Warning("No occupation of '%s[%d]', state is not updated",
                  pHmm->mpMacro->mpName, (int) j + 1);
        }
        return;
      }
  
        // Remove mixtures with low weight
      if (pModelSet->mUpdateMask & UM_WEIGHT) 
      {
        for (i = 0; i < mNumberOfMixtures; i++) 
        {
          if (mpMixture[i].mWeightAccum / accum_sum < pModelSet->mMinMixWeight) 
          {
            if (pHmm)
            {
              size_t j;
              for (j=0; j < pHmm->mNStates && pHmm->mpState[j] != this; j++)
                ; // find the state number
              Warning("Discarding mixture %d of Hmm %s state %d because of too low mixture weight",
                      (int) i, pHmm->mpMacro->mpName, (int) j + 1);
            } 
            else 
            {
              assert(mpMacro);
              Warning("Discarding mixture %d of state %s because of too low mixture weight",
                      (int) i, mpMacro->mpName);
            }
            accum_sum -= mpMixture[i].mWeightAccum;
  
            if (!mpMixture[i].mpEstimates->mpMacro) 
            {
              mpMixture[i].mpEstimates->Scan(MTM_ALL,NULL,ReleaseItem,NULL);
            }
  
            mpMixture[i--] = mpMixture[--mNumberOfMixtures];
            continue;
          }
        }
      }
  
      for (i = 0; i < mNumberOfMixtures; i++) 
      {
  //      printf("Weight Acc: %f\n", (float) state->mpMixture[i].mWeightAccum);
        if (pModelSet->mUpdateMask & UM_WEIGHT) 
        {
          mpMixture[i].mWeight = log(mpMixture[i].mWeightAccum / accum_sum);
        }
        
        if (!mpMixture[i].mpEstimates->mpMacro) 
        {
        //!!! This is just too ugly hack
          gWeightAccumDen = mpMixture[i].mWeightAccumDen;
          mpMixture[i].mpEstimates->UpdateFromAccums(pModelSet);
        }
      }
    }
  }
  

  //***************************************************************************
  //***************************************************************************
  void 
  State::
  Scan(int mask, HMMSetNodeName nodeName,  ScanAction action, void *pUserData)
  {
    size_t    i;
    size_t    n = 0;
    char *    chptr = NULL;
  
    if (nodeName != NULL) 
    {
      n = strlen(nodeName);
      chptr = nodeName + n;
      n = sizeof(HMMSetNodeName) - 4 - n;
    }
  
    if (mask & MTM_STATE && mask & MTM_PRESCAN) 
    {
      action(mt_state, nodeName, this, pUserData);
    }
  
    if (mOutPdfKind != KID_PDFObsVec &&
      mask & (MTM_ALL & ~(MTM_STATE | MTM_HMM | MTM_TRANSITION))) 
    {
      for (i=0; i < mNumberOfMixtures; i++) 
      {
        if (!mpMixture[i].mpEstimates->mpMacro) 
        {
          if (n > 0 ) snprintf(chptr, n, ".mix[%d]", (int) i+1);
          mpMixture[i].mpEstimates->Scan(mask, nodeName,
                      action, pUserData);
        }
      }
    }
    if (mask & MTM_STATE && !(mask & MTM_PRESCAN)) 
    {
      if (n > 0) chptr = '\0';
      action(mt_state, nodeName, this, pUserData);
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  // XformInstance seciton
  //***************************************************************************
  //***************************************************************************
  XformInstance::
  XformInstance(size_t vectorSize)
  {
    mpOutputVector  = new FLOAT[vectorSize];
    mpVarFloor      = NULL;
    mOutSize        = vectorSize;
    mpXformStatCache = NULL;

    mpInput          = NULL;
    mpXform          = NULL;
    mTime            = 0;
    mpNext           = NULL;
    mNumberOfXformStatCaches = 0;
    mStatCacheTime   = 0;
    mpMemory         = NULL;
    mTotalDelay      = 0;
  }
  
  //***************************************************************************
  //***************************************************************************
  XformInstance::
  XformInstance(Xform * pXform, size_t vectorSize)
  {
    mpOutputVector      = new FLOAT[vectorSize];
    mpVarFloor          = NULL;
    mOutSize            = vectorSize;
    mpXformStatCache    = NULL;

    mpInput             = NULL;
    mpXform             = pXform;
    mTime               = 0;
    mpNext              = NULL;
    mNumberOfXformStatCaches = 0;
    mStatCacheTime      = 0;
    mpMemory            = NULL;
    mTotalDelay         = 0;
  }
    
  
  //***************************************************************************
  //***************************************************************************
  XformInstance::
  ~XformInstance()
  {
    if (mpXformStatCache != NULL)
    {
      free(mpXformStatCache->mpStats);
      free(mpXformStatCache);
    }
    delete [] mpOutputVector;
    delete [] mpMemory;
  }
  
  //***************************************************************************
  //***************************************************************************
  FLOAT *
  XformInstance::
  XformPass(FLOAT * pInputVector, int time, PropagDirectionType dir)
  {
    if (time != UNDEF_TIME && this->mTime == time) 
      return mpOutputVector;
  
    this->mTime = time;
  
    if (this->mpInput) {
      pInputVector = mpInput->XformPass(pInputVector, time, dir);
    }
  
    mpXform->Evaluate(pInputVector, mpOutputVector, mpMemory, dir);
  
  /*  {int i;
    for (i=0; i<xformInst->mOutSize; i++)
      printf("%.2f ", xformInst->mpOutputVector[i]);
    printf("%s\n", dir == FORWARD ? "FORWARD" : "BACKWARD");}*/
  
    return mpOutputVector;
  }; // XformPass(FLOAT *in_vec, int time, PropagDirectionType dir)
  
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  XformInstance::
  Scan(int mask, HMMSetNodeName nodeName, ScanAction action, void *pUserData)
  {
    int n = 0;
    char *chptr = NULL;
    
    if (nodeName != NULL) 
    {
      n = strlen(nodeName);
      chptr = nodeName + n;
      n = sizeof(HMMSetNodeName) - 4 - n;
    }
  
    if (mask & MTM_XFORM_INSTANCE && mask & MTM_PRESCAN) {
      action(mt_XformInstance, nodeName, this, pUserData);
    }
    
    if (mpInput != NULL) {
      if (!mpInput->mpMacro) {
      if (n > 0) strncpy(chptr, ".input", n);
        mpInput->Scan(mask, nodeName, action, pUserData);
      }
    }
  
    if (mask & MTM_XFORM && mpXform != NULL && 
      !mpXform->mpMacro) {
      if (n > 0) strncpy(chptr, ".mpXform", n);
      mpXform->Scan(mask, nodeName, action, pUserData);
    }
    
    if (mask & MTM_XFORM_INSTANCE && !(mask & MTM_PRESCAN)) {
      if (n > 0) chptr = '\0';
      action(mt_XformInstance, nodeName, this, pUserData);
    }
  }
  

  //***************************************************************************
  //***************************************************************************
  // Xform seciton
  //***************************************************************************
  //***************************************************************************
  
  //****************************************************************************
  //****************************************************************************
  void 
  Xform::
  Scan(int mask, HMMSetNodeName nodeName, ScanAction action, void *pUserData)
  {
    size_t      n = 0;
    char *      chptr = NULL;
  
    if (nodeName != NULL) 
    {
      n = strlen(nodeName);
      chptr = nodeName + n;
      n = sizeof(HMMSetNodeName) - 4 - n;
    }
  
    if (mask & MTM_PRESCAN) 
    {
      action(mt_Xform, nodeName, this, pUserData);
    }
    
    if (mXformType == XT_COMPOSITE) 
    {
      CompositeXform *  cxf = dynamic_cast<CompositeXform *> (this);
      size_t            i;
      size_t            j;
  
      for (i=0; i < cxf->mNLayers; i++) 
      {
        for (j = 0; j < cxf->mpLayer[i].mNBlocks; j++) 
        {
          if (!cxf->mpLayer[i].mpBlock[j]->mpMacro) 
          {
            if (n > 0) 
              snprintf(chptr, n, ".part[%d,%d]", (int) i+1, (int) j+1);
            cxf->mpLayer[i].mpBlock[j]->Scan(mask, nodeName, action, pUserData);
          }
        }
      }
    }
  
    if (!(mask & MTM_PRESCAN)) 
    {
      if (n > 0) 
        chptr = '\0';
        
      action(mt_Xform, nodeName, this, pUserData);
    }
  }
  
    
    
  //**************************************************************************  
  //**************************************************************************  
  // XformLayer section
  //**************************************************************************  
  //**************************************************************************  
  XformLayer::
  XformLayer() : mpOutputVector(NULL), mNBlocks(0), mpBlock(NULL) 
  {
    // we don't do anything here, all is done by the consructor call
  }
  
  //**************************************************************************  
  //**************************************************************************  
  XformLayer::
  ~XformLayer()
  {
    // delete the pointers
    delete [] mpOutputVector;
    delete [] mpBlock;
  }

  //**************************************************************************  
  //**************************************************************************  
  Xform **
  XformLayer::
  InitBlocks(size_t nBlocks)
  {
    mpBlock  = new Xform*[nBlocks];
    mNBlocks = nBlocks;
    
    for (size_t i = 0; i < nBlocks; i++) 
      mpBlock[i] = NULL;
      
    return mpBlock;
  }
  
  
  //**************************************************************************  
  //**************************************************************************  
  // CompositeXform section
  //**************************************************************************  
  //**************************************************************************  
  CompositeXform::
  CompositeXform(size_t nLayers)
  {
    mpLayer = new XformLayer[nLayers];
    
    // set some properties
    mMemorySize = 0;
    mDelay      = 0;
    mNLayers    = nLayers;
    mXformType = XT_COMPOSITE;    
  }
  
  //**************************************************************************  
  //**************************************************************************  
  CompositeXform::
  ~CompositeXform()
  {
    
  }
  
  //**************************************************************************  
  //**************************************************************************  
  //virtual 
  FLOAT *
  CompositeXform::
  Evaluate(FLOAT *    pInputVector, 
           FLOAT *    pOutputVector,
           char *     pMemory,
           PropagDirectionType  direction)
  {
    size_t  i;
    size_t  j;
  
    for (i = 0; i < mNLayers; i++) 
    {
      FLOAT *in  = (i == 0)          ? pInputVector  : mpLayer[i-1].mpOutputVector;
      FLOAT *out = (i == mNLayers-1) ? pOutputVector : mpLayer[i].mpOutputVector;
      
      for (j = 0; j < mpLayer[i].mNBlocks; j++) 
      {
        mpLayer[i].mpBlock[j]->Evaluate(in, out, pMemory, direction);
        in  += mpLayer[i].mpBlock[j]->mInSize;
        out += mpLayer[i].mpBlock[j]->mOutSize;
        pMemory += mpLayer[i].mpBlock[j]->mMemorySize;
      }
    }
    
    return pOutputVector;
  }; //Evaluate(...)

  
  
  //**************************************************************************  
  //**************************************************************************  
  // LinearXform section
  //**************************************************************************  
  //**************************************************************************  
  LinearXform::
  LinearXform(size_t inSize, size_t outSize):
    mMatrix(inSize, outSize, STORAGE_TRANSPOSED)
  {
    //ret = (LinearXform *) malloc(sizeof(LinearXform)+(out_size*in_size-1)*sizeof(ret->mpMatrixO[0]));
    //if (ret == NULL) Error("Insufficient memory");
    mpMatrixO     = new FLOAT[inSize * outSize];    
    mOutSize      = outSize;
    mInSize       = inSize;
    mMemorySize   = 0;
    mDelay        = 0;
    mXformType    = XT_LINEAR;
  }
    
  //**************************************************************************  
  //**************************************************************************  
  LinearXform::
  ~LinearXform()
  {
    delete [] mpMatrixO;
  }
  
  //**************************************************************************  
  //**************************************************************************  
  //virtual 
  FLOAT *
  LinearXform::
  Evaluate(FLOAT *    pInputVector, 
           FLOAT *    pOutputVector,
           char *     pMemory,
           PropagDirectionType  direction)
  {
    size_t c; // column counter
    size_t r; // row counter
    
    // matrix multiplication
    for (r = 0; r < mOutSize; r++) 
    {
      pOutputVector[r] = 0.0;
      for (c = 0; c < mInSize; c++) 
      {
        pOutputVector[r] += pInputVector[c] * mpMatrixO[mInSize * r + c];
      }
    }
    return pOutputVector;  
  }; //Evaluate(...)
  

  //**************************************************************************  
  //**************************************************************************  
  // BiasXform section
  //**************************************************************************  
  //**************************************************************************  
  BiasXform::
  BiasXform(size_t vectorSize) :
    mVector(1, vectorSize)
  {
    //mpVectorO     = new FLOAT[vectorSize];  
    mInSize       = vectorSize;
    mOutSize      = vectorSize;
    mMemorySize   = 0;
    mDelay        = 0;
    mXformType    = XT_BIAS;
  }
  
  //**************************************************************************  
  //**************************************************************************  
  BiasXform::
  ~BiasXform()
  {
    //delete [] mpVectorO;
  }
  
  //**************************************************************************  
  //**************************************************************************  
  //virtual 
  FLOAT *
  BiasXform::
  Evaluate(FLOAT *    pInputVector, 
           FLOAT *    pOutputVector,
           char *     pMemory,
           PropagDirectionType  direction)
  {
    size_t i;
    
    for (i = 0; i < mOutSize; i++) 
    {
      pOutputVector[i] = pInputVector[i] + mVector[0][i];
    }
    return pOutputVector;
  }; //Evaluate(...)
  

  //**************************************************************************  
  //**************************************************************************  
  // FuncXform section
  //**************************************************************************  
  //**************************************************************************  
  FuncXform::
  FuncXform(size_t size, int funcId)
  {
    mFuncId       = funcId;
    mInSize       = size;
    mOutSize      = size;
    mMemorySize   = 0;
    mDelay        = 0;
    mXformType    = XT_FUNC;
  }  
  
  //**************************************************************************  
  //**************************************************************************  
  FuncXform::
  ~FuncXform()
  {}
  
  //**************************************************************************  
  //**************************************************************************  
  //virtual 
  FLOAT *
  FuncXform::
  Evaluate(FLOAT *    pInputVector, 
           FLOAT *    pOutputVector,
           char *     pMemory,
           PropagDirectionType  direction)
  {
    gFuncTable[mFuncId].funcPtr(pInputVector, pOutputVector, mOutSize);
    return pOutputVector;
  }; //Evaluate(...)
  
      
  //**************************************************************************  
  //**************************************************************************  
  // CopyXform section
  //**************************************************************************  
  //**************************************************************************  
  CopyXform::
  CopyXform(size_t inSize, size_t outSize)
  {
    mpIndices     = new int[outSize];
    mOutSize      = outSize;
    mInSize       = inSize;
    mMemorySize   = 0;
    mDelay        = 0;
    mXformType    = XT_COPY;
  }
  
  //**************************************************************************  
  //**************************************************************************  
  CopyXform::
  ~ CopyXform()
  {
    delete [] mpIndices;
  }
  
  //**************************************************************************  
  //**************************************************************************  
  //virtual 
  FLOAT *
  CopyXform::
  Evaluate(FLOAT *    pInputVector, 
           FLOAT *    pOutputVector,
           char *     pMemory,
           PropagDirectionType  direction)
  {
    size_t i;
    
    for (i = 0; i < mOutSize; i++) 
    {
      pOutputVector[i] = pInputVector[mpIndices[i]];
    }
    return pOutputVector;
  }; //Evaluate(...)
  
  
  //**************************************************************************  
  //**************************************************************************  
  // StackingXform section
  //**************************************************************************  
  //**************************************************************************  
  StackingXform::
  StackingXform(size_t stackSize, size_t inSize)
  {
    size_t out_size = stackSize * inSize;
  
    mOutSize      = out_size;
    mInSize       = inSize;
    mMemorySize   = out_size * sizeof(FLOAT);
    mDelay        = stackSize - 1;
    mHorizStack   = 0;
    mXformType    = XT_STACKING;      
  }
  
  //**************************************************************************  
  //**************************************************************************  
  StackingXform::
  ~StackingXform()
  { }
  
  //**************************************************************************  
  //**************************************************************************  
  //virtual 
  FLOAT *
  StackingXform::
  Evaluate(FLOAT *    pInputVector, 
           FLOAT *    pOutputVector,
           char *     pMemory,
           PropagDirectionType  direction)
  {
    FLOAT *stack = reinterpret_cast <FLOAT *> (pMemory);
  
    memmove(stack + (direction == BACKWARD ? mInSize : 0),
            stack + (direction ==  FORWARD ? mInSize : 0),
            (mOutSize - mInSize) * sizeof(FLOAT));
    
    memmove(stack + (direction ==  FORWARD ? mOutSize - mInSize : 0),
            pInputVector, mInSize * sizeof(FLOAT));
  
    if (!mHorizStack) 
    {
      memmove(pOutputVector, stack, mOutSize * sizeof(FLOAT));
    } 
    else 
    { // stacking == HORZ_STACK
      size_t t;
      size_t c;
      size_t stack_size = mOutSize / mInSize;
      
      for (t = 0; t < stack_size; t++) 
      {
        for (c = 0; c < mInSize; c++) 
        {
          pOutputVector[c * stack_size + t] = stack[t * mInSize + c];
        }
      }
    }
    
    return pOutputVector;
  }; //Evaluate(...)
    
  
 
  //**************************************************************************
  //**************************************************************************
  //   ModelSet section
  //**************************************************************************
  //**************************************************************************
  
  //**************************************************************************
  //**************************************************************************  
  void
  ModelSet::
  Init(FlagType flags)
  {
    if (!my_hcreate_r(100, &mHmmHash)            ||
        !my_hcreate_r(100, &mStateHash)          ||
        !my_hcreate_r( 10, &mMixtureHash)        ||
        !my_hcreate_r( 10, &mMeanHash)           ||
        !my_hcreate_r( 10, &mVarianceHash)       ||
        !my_hcreate_r( 10, &mTransitionHash)     ||
        !my_hcreate_r( 10, &mXformInstanceHash)  ||
        !my_hcreate_r( 10, &mXformHash)) 
    {
      Error("Insufficient memory");
    }
  
    /// :TODO: 
    /// Get rid of this by making STK use Matrix instead of FLOAT arrays
    mUseNewMatrix               = false;
    
    this->mpXformInstances      = NULL;
    this->mpInputXform          = NULL;
    this->mpFirstMacro          = NULL;
    this->mpLastMacro           = NULL;
    this->mInputVectorSize      = -1;
    this->mParamKind            = -1;
    this->mOutPdfKind           = KID_UNSET;
    this->mDurKind              = KID_UNSET;
    this->mNMixtures            = 0;
    this->mNStates              = 0;
    this->mAllocAccums          = flags & MODEL_SET_WITH_ACCUM ? true : false;
    this->mTotalDelay           = 0;
    InitLogMath();
  
    //Reestimation params
    this->mMinOccurances        = 3;
    this->mMinMixWeight         = MIN_WEGIHT;
    this->mpVarFloor            = NULL;
    this->mMinVariance          = 0.0;
    this->mUpdateMask           = UM_TRANSITION | UM_MEAN | UM_VARIANCE |
                                  UM_WEIGHT | UM_XFSTATS | UM_XFORM;
    this->mpXformToUpdate       = NULL;
    this->mNumberOfXformsToUpdate     = 0;
    this->mGaussLvl2ModelReest  = 0;
    this->mMmiUpdate            = 0;
    this->MMI_E                 = 2.0;
    this->MMI_h                 = 2.0;
    this->MMI_tauI              = 100.0;
  
    mClusterWeightUpdate        = false;
    mpClusterWeights            = NULL;
    mNClusterWeights            = 0;
    InitKwdTable();    
  } // Init(...);


  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  Release()
  {
    size_t i;  
    
    Scan(MTM_REVERSE_PASS | MTM_ALL, NULL, ReleaseItem, NULL);
  
    ReleaseMacroHash(&mHmmHash);
    ReleaseMacroHash(&mStateHash);
    ReleaseMacroHash(&mMixtureHash);
    ReleaseMacroHash(&mMeanHash);
    ReleaseMacroHash(&mVarianceHash);
    ReleaseMacroHash(&mTransitionHash);
    ReleaseMacroHash(&mXformHash);
    ReleaseMacroHash(&mXformInstanceHash);
  
    for (i = 0; i < mNumberOfXformsToUpdate; i++) 
    {
      if (NULL != mpXformToUpdate[i].mpShellCommand)
        free(mpXformToUpdate[i].mpShellCommand);
    }
  
    if (NULL != mpXformToUpdate)
      free(mpXformToUpdate);
  } // Release();
    
  
  //*****************************************************************************
  //*****************************************************************************
  void 
  ModelSet::
  Scan(int mask, HMMSetNodeName nodeName,
       ScanAction action, void *pUserData)
  {
    Macro *macro;
    
    if (nodeName != NULL) 
      strcpy(nodeName+sizeof(HMMSetNodeName)-4, "...");
  
    // walk through the list of macros and decide what to do... 
    // we also decide which direction to walk based on the MTM_REVERSE_PASS flag
    for (macro = mask & MTM_REVERSE_PASS ? mpLastMacro   : mpFirstMacro;
        macro != NULL;
        macro = mask & MTM_REVERSE_PASS ? macro->prevAll : macro->nextAll) 
    {
      if (macro->mpData != NULL && macro->mpData->mpMacro != macro) 
        continue;
      
      if (nodeName != NULL) 
        strncpy(nodeName, macro->mpName, sizeof(HMMSetNodeName)-4);
  
      switch (macro->mType) 
      {
        case mt_Xform:
          if (!(mask & MTM_XFORM)) break;
          macro->mpData->Scan(mask, nodeName, action, pUserData);
          break;
        case mt_XformInstance:
          if (!(mask & (MTM_XFORM | MTM_XFORM_INSTANCE))) break;
          macro->mpData->Scan(mask, nodeName, action, pUserData);
          break;
        case mt_mean:
          if (!(mask & MTM_MEAN)) break;
          action(mt_mean, nodeName, macro->mpData, pUserData);
          break;
        case mt_variance:
          if (!(mask & MTM_VARIANCE)) break;
          action(mt_variance, nodeName, macro->mpData, pUserData);
          break;
        case mt_transition:
          if (!(mask & MTM_TRANSITION)) break;
          action(mt_transition, nodeName, macro->mpData, pUserData);
          break;
        case mt_mixture:
          if (!(mask & (MTM_ALL & ~(MTM_STATE | MTM_HMM | MTM_TRANSITION)))) break;
          macro->mpData->Scan(mask, nodeName, action, pUserData);
          break;
        case mt_state:
          if (!(mask & (MTM_ALL & ~(MTM_HMM | MTM_TRANSITION)))) break;
          macro->mpData->Scan(mask, nodeName, action, pUserData);
          break;
        case mt_hmm:
          if (!(mask & MTM_ALL)) break;
          macro->mpData->Scan(mask, nodeName, action, pUserData);
          break;
        default: assert(0);
      }
    }
  }
  
  
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  ReadAccums(const char * pFileName, 
             float        weight,
             long *       totFrames, 
             FLOAT *      totLogLike, 
             int          mmiDenominatorAccums)
  {
    IStkStream                in;
    FILE*                     fp;
    char                      macro_name[128];
    MyHSearchData*            hash;
    unsigned int              i;
    int                       t = 0;
    int                       c;
    int                       skip_accum = 0;
    INT_32                    occurances;
    ReadAccumUserData         ud;
    Macro*                    macro;
    int                       mtm = MTM_PRESCAN | 
                                    MTM_STATE | 
                                    MTM_MEAN | 
                                    MTM_VARIANCE | 
                                    MTM_TRANSITION;
  
    macro_name[sizeof(macro_name)-1] = '\0';
  
    // open the file
    in.open(pFileName, ios::binary);
    if (!in.good())
    {
      Error("Cannot open input accumulator file: '%s'", pFileName);
    }
    
    fp = in.file();
    
    INT_32 i32;
    if (fread(&i32,       sizeof(i32),  1, fp) != 1 ||
        fread(totLogLike, sizeof(FLOAT), 1, fp) != 1) 
    {
      Error("Invalid accumulator file: '%s'", pFileName);
    }
    *totFrames = i32;
    
//    *totFrames  *= weight; // Not sure whether we should report weighted quantities or not
//    *totLogLike *= weight;
  
    ud.mpFileName   = pFileName;
    ud.mpFp         = fp;
    ud.mpModelSet   = this;
    ud.mWeight      = weight;
    ud.mMmi         = mmiDenominatorAccums;
  
    for (;;) 
    {
      if (skip_accum) 
      { // Skip to the begining of the next macro accumulator
        for (;;) 
        {
          while ((c = getc(fp)) != '~' && c != EOF)
            ;
          
          if (c == EOF) 
            break;
            
          if (strchr("hsmuvt", t = c = getc(fp)) &&
            (c = getc(fp)) == ' ' && (c = getc(fp)) == '"')
          {  
            break;
          }
          
          ungetc(c, fp);
        }
        
        if (c == EOF) 
          break;
      } 
      else 
      {
        if ((c = getc(fp)) == EOF) break;
        
        if (c != '~'      || !strchr("hsmuvt", t = getc(fp)) ||
          getc(fp) != ' ' || getc(fp) != '"') 
        {
          Error("Incomatible accumulator file: '%s'", pFileName);
        }
      }
  
      for (i=0; (c = getc(fp))!=EOF && c!='"' && i<sizeof(macro_name)-1; i++) 
      {
        macro_name[i] = c;
      }
      macro_name[i] = '\0';
  
      hash = t == 'h' ? &mHmmHash :
             t == 's' ? &mStateHash :
             t == 'm' ? &mMixtureHash :
             t == 'u' ? &mMeanHash :
             t == 'v' ? &mVarianceHash :
             t == 't' ? &mTransitionHash : NULL;
  
      assert(hash);
      if ((macro = FindMacro(hash, macro_name)) == NULL) 
      {
        skip_accum = 1;
        continue;
      }
  
      skip_accum = 0;
      if (fread(&occurances, sizeof(occurances), 1, fp) != 1) 
      {
        Error("Invalid accumulator file: '%s'", pFileName);
      }
  
      if (!mmiDenominatorAccums) macro->mOccurances += occurances;
      switch (t) 
      {
        case 'h': macro->mpData->Scan(mtm, NULL, ReadAccum, &ud); break;
        case 's': macro->mpData->Scan(mtm, NULL, ReadAccum, &ud); break;
        case 'm': macro->mpData->Scan(mtm, NULL, ReadAccum, &ud); break;
        case 'u': ReadAccum(mt_mean, NULL, macro->mpData, &ud);                break;
        case 'v': ReadAccum(mt_variance, NULL, macro->mpData, &ud);            break;
        case 't': ReadAccum(mt_transition, NULL, macro->mpData, &ud);          break;
        default:  assert(0);
      }
    }
    
    in.close();
    //delete [] ud.mpFileName;
    //free(ud.mpFileName);    
  }; // ReadAccums(...)

  
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  WriteAccums(const char * pFileName, 
              const char * pOutputDir,
              long         totFrames, 
              FLOAT        totLogLike)
  {
    FILE *                fp;
    char                  file_name[1024];
    WriteAccumUserData    ud;  
    
    MakeFileName(file_name, pFileName, pOutputDir, NULL);
  
    if ((fp = fopen(file_name, "wb")) == NULL) 
    {
      Error("Cannot open output file: '%s'", file_name);
    }
  
    INT_32 i32 = totFrames;
    if (fwrite(&i32,  sizeof(i32),  1, fp) != 1 ||
        fwrite(&totLogLike, sizeof(FLOAT), 1, fp) != 1) 
    {
      Error("Cannot write accumulators to file: '%s'", file_name);
    }
  
    ud.mpFp         = fp;
    ud.mpFileName   = file_name;
  //  ud.mMmi = MMI_denominator_accums;
  
    Scan(MTM_PRESCAN | (MTM_ALL & ~(MTM_XFORM_INSTANCE|MTM_XFORM)),
              NULL, WriteAccum, &ud);
  
    fclose(fp);
  }; // WriteAccums(...)
  
  
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  NormalizeAccums()
  {
    Scan(MTM_ALL & ~(MTM_XFORM_INSTANCE|MTM_XFORM), NULL,
               NormalizeAccum, NULL);
  }; // NormalizeAccums
  
    
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  ResetAccums()
  {
    if (mAllocAccums)
    {
      Scan(MTM_STATE | MTM_MEAN | MTM_VARIANCE | MTM_TRANSITION,
              NULL, ResetAccum, NULL);
    }
  }; // ResetAccums()
  
  
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  DistributeMacroOccurances()
  {
    Macro *   macro;
    size_t    i;
    size_t    j;
    size_t    k;
  
    for (k = 0; k < mHmmHash.mNEntries; k++) 
    {
      macro = static_cast <Macro *> (mHmmHash.mpEntry[k]->data);
      
      if (macro->mpData->mpMacro != macro) 
      { 
        macro->mpData->mpMacro->mOccurances += macro->mOccurances;
      }
      
      Hmm *hmm = static_cast <Hmm *> (macro->mpData);
  
      for (i = 0; i < hmm->mNStates - 2; i++) 
      {
        State *state = hmm->mpState[i];
  
        if (state->mpMacro) 
          state->mpMacro->mOccurances += macro->mOccurances;
  
        if (state->mOutPdfKind == KID_DiagC) 
        {
          for (j = 0; j < state->mNumberOfMixtures; j++) 
          {
            Mixture *mixture = state->mpMixture[j].mpEstimates;
  
            if (mixture->mpMacro)
              mixture->mpMacro->mOccurances += macro->mOccurances;
            
            if (mixture->mpMean->mpMacro)
              mixture->mpMean->mpMacro->mOccurances += macro->mOccurances;
            
            if (mixture->mpVariance->mpMacro) 
              mixture->mpVariance->mpMacro->mOccurances += macro->mOccurances;
          }
        }
      }
  
      if (hmm->mpTransition->mpMacro) 
        hmm->mpTransition->mpMacro->mOccurances += macro->mOccurances;
    }    
  }; // DistributeMacroOccurances()
  
  
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  ComputeGlobalStats(FLOAT *observation, int time)
  {
    GlobalStatsUserData ud = {observation, time};
    Scan(MTM_STATE | MTM_MIXTURE, NULL, GlobalStats, &ud);
  }; // ComputeGlobalStats(...)
  
  
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  UpdateFromAccums(const char * pOutputDir)
  {
    Macro * macro;
    size_t  i;
    
    for (i = 0; i < mMeanHash.mNEntries; i++) 
    {
      macro = (Macro *) mMeanHash.mpEntry[i]->data;
  //  for (macro = mean_list; macro != NULL; macro = macro->mpNext) {
      if (macro->mpData->mpMacro != macro) continue;
      
      if (macro->mOccurances < mMinOccurances) 
      {
        WARN_FEW_EXAMPLES("Mean vector", macro->mpName, macro->mOccurances);
      } 
      else 
      {
        ((Mean *) macro->mpData)->UpdateFromAccums(this);
      }
    }
  
    for (i = 0; i < mVarianceHash.mNEntries; i++) 
    {
      macro = (Macro *) mVarianceHash.mpEntry[i]->data;
  //  for (macro = variance_list; macro != NULL; macro = macro->mpNext) {
      if (macro->mpData->mpMacro != macro) continue;
      
      if (strcmp(macro->mpName, "varFloor1")) 
      {
        if (macro->mOccurances < mMinOccurances) 
        {
          WARN_FEW_EXAMPLES("Variance vector", macro->mpName, macro->mOccurances);
        } 
        else 
        {
          ((Variance *) macro->mpData)->UpdateFromAccums(this);
        }
      }
    }
  
    for (i = 0; i < mTransitionHash.mNEntries; i++) 
    {
      macro = (Macro *) mTransitionHash.mpEntry[i]->data;
  //  for (macro = transition_list; macro != NULL; macro = macro->mpNext) {
      if (macro->mpData->mpMacro != macro) 
        continue;
        
      if (macro->mOccurances < mMinOccurances) 
      {
        WARN_FEW_EXAMPLES("Transition matrix ", macro->mpName, macro->mOccurances);
      } 
      else 
      {
        ((Transition *) macro->mpData)->UpdateFromAccums(this);
      }
    }               
      
    for (i = 0; i < mMixtureHash.mNEntries; i++) 
    {
      macro = (Macro *) mMixtureHash.mpEntry[i]->data;
  //  for (macro = mixture_list; macro != NULL; macro = macro->mpNext) {
      if (macro->mpData->mpMacro != macro)
        continue;
      
      if (macro->mOccurances < mMinOccurances) 
      {
        WARN_FEW_EXAMPLES("Mixture", macro->mpName, macro->mOccurances);
      } 
      else 
      {
        ((Mixture *) macro->mpData)->UpdateFromAccums(this);
      }
    }
  
    for (i = 0; i < mStateHash.mNEntries; i++) 
    {
      macro = (Macro *) mStateHash.mpEntry[i]->data;
  //  for (macro = hmm_set->state_list; macro != NULL; macro = macro->mpNext) {
      if (macro->mpData->mpMacro != macro) 
        continue;
      
      if (macro->mOccurances < mMinOccurances) 
      {
        WARN_FEW_EXAMPLES("State", macro->mpName, macro->mOccurances);
      } 
      else 
      {
        ((State *) macro->mpData)->UpdateFromAccums(this, NULL);
      }
    }
  
    for (i = 0; i < mHmmHash.mNEntries; i++) 
    {
      macro = (Macro *) mHmmHash.mpEntry[i]->data;
  //  for (macro = hmm_set->hmm_list; macro != NULL; macro = macro->mpNext) {
      if (macro->mpData->mpMacro != macro) 
        continue;
      
      if (macro->mOccurances < mMinOccurances) 
      {
        WARN_FEW_EXAMPLES("Model", macro->mpName, macro->mOccurances);
      } 
      else 
      {
        ((Hmm *) macro->mpData)->UpdateFromAccums(this);
      }
    }    
  }; // UpdateHMMSetFromAccums(...)
  
  
  //**************************************************************************  
  //**************************************************************************  
  Macro *
  ModelSet::
  pAddMacro(const char type, const std::string & rNewName)
  {
    Macro *                  macro;
    MyHSearchData * hash;
    ENTRY                    e;
    ENTRY *                  ep;
  
    switch (type)
    {
      case 'h': hash = &mHmmHash; break;
      case 's': hash = &mStateHash; break;
      case 'm': hash = &mMixtureHash; break;
      case 'u': hash = &mMeanHash; break;
      case 'v': hash = &mVarianceHash; break;
      case 't': hash = &mTransitionHash; break;
      case 'j': hash = &mXformInstanceHash; break;
      case 'x': hash = &mXformHash; break;
      default:  hash = NULL; break;
    }
    
    if (hash == NULL) 
    {
      return NULL;
    }
  
    if ((macro = FindMacro(hash, rNewName.c_str())) != NULL)
    {
      return macro;
    }
    
    // create new macro
    macro = new Macro;
    
    //if ((macro = (Macro *) malloc(sizeof(Macro))) == NULL ||
    if ((macro->mpName = strdup(rNewName.c_str())) == NULL              
    ||  (macro->mpFileName = NULL, gpCurrentMmfName
    &&  (macro->mpFileName = strdup(gpCurrentMmfName)) == NULL)) 
    {
      Error("Insufficient memory");
    }
    
    std::cerr << "Macro " << macro->mpName << " defined in file " 
              << macro->mpFileName << std::endl;
    
    e.key  = macro->mpName;
    e.data = macro;
  
    if (!my_hsearch_r(e, ENTER, &ep, hash)) 
    {
      Error("Insufficient memory");
    }
  
    macro->mpData = NULL;
    macro->mOccurances = 0;
    macro->mType = type;
    // List of all macros is made to be able to save macros in proper order
    macro->nextAll = NULL;
    macro->prevAll = mpLastMacro;
    
    if (!mpFirstMacro /* => !hmm_set->mpLastMacro  */) 
    {
      mpFirstMacro = macro;
    } 
    else 
    {
      mpLastMacro->nextAll = macro;
    }
    
    mpLastMacro = macro;
    
    return macro;
  }; // pAddMacro(...)


  //**************************************************************************  
  //**************************************************************************  
  void 
  ModelSet::
  AllocateAccumulatorsForXformStats() 
  {
    mAllMixuresUpdatableFromStatAccums = true;
    Scan(MTM_XFORM_INSTANCE | MTM_MIXTURE, NULL,
         AllocateXformStatCachesAndAccums, this);
  }


  //***************************************************************************
  //**************************************************************************  
  void 
  ModelSet::
  WriteXformStatsAndRunCommands(const char * pOutDir, bool binary)
  {
    HMMSetNodeName                nodeNameBuffer;
    WriteStatsForXformUserData    userData;
    char                          fileName[1024];
    size_t                        i;
    size_t                        j;
    size_t                        k;
    FILE *                        fp;
  
    struct XfStatsHeader 
    {
      #define PRECISION_FLOAT    0
      #define PRECISION_DOUBLE   1
      #define PRECISION_UNKNOWN 15
      char precision;
      #define STATS_MEAN         0
      #define STATS_COV_LOW_TRI  1
      char stats_type;
      short size;
    } header = {sizeof(FLOAT) == sizeof(float)  ? PRECISION_FLOAT  :
                sizeof(FLOAT) == sizeof(double) ? PRECISION_DOUBLE :
                                                  PRECISION_UNKNOWN};
    userData.mBinary = binary;
    for (k = 0; k < mNumberOfXformsToUpdate; k++) 
    {
      char *    ext;
      char *    shellCommand = mpXformToUpdate[k].mpShellCommand;
      
      userData.mpXform = (LinearXform *) mpXformToUpdate[k].mpXform;
      assert(userData.mpXform->mXformType == XT_LINEAR);
  
      MakeFileName(fileName, userData.mpXform->mpMacro->mpName, pOutDir, NULL);
      ext = fileName + strlen(fileName);
      
      strcpy(ext, ".xms"); userData.mMeanFile.mpStatsN = strdup(fileName);
      strcpy(ext, ".xmo"); userData.mMeanFile.mpOccupN = strdup(fileName);
      strcpy(ext, ".xcs"); userData.mCovFile.mpStatsN = strdup(fileName);
      strcpy(ext, ".xco"); userData.mCovFile.mpOccupN = strdup(fileName);
  
      if (userData.mMeanFile.mpStatsN == NULL || userData.mMeanFile.mpOccupN == NULL||
        userData.mCovFile.mpStatsN  == NULL || userData.mCovFile.mpOccupN  == NULL) 
      {
        Error("Insufficient memory");
      }
  
      userData.mMeanFile.mpStatsP = fopen(userData.mMeanFile.mpStatsN, binary?"w":"wt");
      if (userData.mMeanFile.mpStatsP == NULL) 
      {
        Error("Cannot open output file %s",
              userData.mMeanFile.mpStatsN);
      }
  
      if (binary) 
      {
        header.stats_type = STATS_MEAN;
        header.size = userData.mpXform->mInSize;
        
        if (!isBigEndian()) 
          swap2(header.size);
        
        if (fwrite(&header, sizeof(header), 1, userData.mMeanFile.mpStatsP) != 1) 
        {
          Error("Cannot write to file: %s", userData.mMeanFile.mpStatsN);
        }
      }
  
      userData.mMeanFile.mpOccupP = fopen(userData.mMeanFile.mpOccupN, "wt");
      if (userData.mMeanFile.mpOccupP == NULL) 
      {
        Error("Cannot open output file %s", userData.mMeanFile.mpOccupN);
      }
  
      userData.mCovFile.mpStatsP = fopen(userData.mCovFile.mpStatsN, binary?"w":"wt");
      if (userData.mCovFile.mpStatsP == NULL) 
      {
        Error("Cannot open output file %s", userData.mCovFile.mpStatsN);
      }
  
      if (binary) 
      {
        header.stats_type = STATS_COV_LOW_TRI;
        header.size = userData.mpXform->mInSize;
        if (!isBigEndian()) 
          swap2(header.size);
        
        if (fwrite(&header, sizeof(header), 1, userData.mCovFile.mpStatsP) != 1) 
        {
          Error("Cannot write to file: %s",
                userData.mCovFile.mpStatsN);
        }
      }
  
      userData.mCovFile.mpOccupP = fopen(userData.mCovFile.mpOccupN, "wt");
      if (userData.mCovFile.mpOccupP == NULL) 
      {
        Error("Cannot open output file %s", userData.mCovFile.mpOccupN);
      }
  
      Scan(MTM_MEAN | MTM_VARIANCE, nodeNameBuffer,
           WriteStatsForXform, &userData);
  
  
      fclose(userData.mMeanFile.mpStatsP); fclose(userData.mMeanFile.mpOccupP);
      fclose(userData.mCovFile.mpStatsP);  fclose(userData.mCovFile.mpOccupP);
      free(userData.mMeanFile.mpStatsN);   free(userData.mMeanFile.mpOccupN);
      free(userData.mCovFile.mpStatsN);    free(userData.mCovFile.mpOccupN);
  
      strcpy(ext, ".xfm");
      if ((fp = fopen(fileName, "wt")) == NULL) 
      {
        Error("Cannot open output file %s", fileName);
      }
  
      for (i=0; i < userData.mpXform->mOutSize; i++) 
      {
        for (j=0; j < userData.mpXform->mInSize; j++) 
        {
          fprintf(fp, " "FLOAT_FMT,
                  userData.mpXform->mpMatrixO[i*userData.mpXform->mInSize+j]);
        }
        fputs("\n", fp);
      }
  
      fclose(fp);
  
      if (shellCommand != NULL && *shellCommand) 
      {
        TraceLog("Executing command: %s", shellCommand);
        system(shellCommand);
      }
    }
  } // WriteXformStatsAndRunCommands(const std::string & rOutDir, bool binary)
  //***************************************************************************

  
  //***************************************************************************
  //*****************************************************************************  
  void 
  ModelSet::
  ReadXformStats(const char * pOutDir, bool binary) 
  {
    HMMSetNodeName                nodeNameBuffer;
    WriteStatsForXformUserData   userData;
    char                          fileName[1024];
    size_t                        i;
    size_t                        k;
    FILE *                        fp;
  
    struct XfStatsHeader 
    {
      char    precision;
      char    stats_type;
      short   size;
    } header;
  
    userData.mBinary = binary;
    
    for (k = 0; k < mNumberOfXformsToUpdate; k++) 
    {
      char *  ext;
      userData.mpXform = (LinearXform *) mpXformToUpdate[k].mpXform;
      assert(userData.mpXform->mXformType == XT_LINEAR);
  
      MakeFileName(fileName, userData.mpXform->mpMacro->mpName, pOutDir, NULL);
      ext = fileName + strlen(fileName);
      strcpy(ext, ".xms"); userData.mMeanFile.mpStatsN = strdup(fileName);
      strcpy(ext, ".xmo"); userData.mMeanFile.mpOccupN = strdup(fileName);
      strcpy(ext, ".xcs"); userData.mCovFile.mpStatsN = strdup(fileName);
      strcpy(ext, ".xco"); userData.mCovFile.mpOccupN = strdup(fileName);
  
      if (userData.mMeanFile.mpStatsN == NULL || userData.mMeanFile.mpOccupN == NULL||
        userData.mCovFile.mpStatsN  == NULL || userData.mCovFile.mpOccupN  == NULL) 
      {
        Error("Insufficient memory");
      }
  
      userData.mMeanFile.mpStatsP = fopen(userData.mMeanFile.mpStatsN, "r");
      
      if (userData.mMeanFile.mpStatsP == NULL) 
      {
        Error("Cannot open input file '%s'", userData.mMeanFile.mpStatsN);
      }
  
      if (binary) 
      {
        header.precision = -1;
        fread(&header, sizeof(header), 1, userData.mMeanFile.mpStatsP);
        
        if (!isBigEndian()) 
          swap2(header.size);
        
        if (ferror(userData.mMeanFile.mpStatsP))
        {
          Error("Cannot read input file '%s'", userData.mMeanFile.mpStatsN);
        }        
        else if (header.stats_type                != STATS_MEAN
             || static_cast<size_t>(header.size)  != userData.mpXform->mInSize
             || header.precision                  != (sizeof(FLOAT) == sizeof(FLOAT_32)
                                                     ? PRECISION_FLOAT : PRECISION_DOUBLE))
        {
          Error("Invalid header in file '%s'", userData.mMeanFile.mpStatsN);
        }
      }
  
      userData.mMeanFile.mpOccupP = fopen(userData.mMeanFile.mpOccupN, "r");
      
      if (userData.mMeanFile.mpOccupP == NULL) 
      {
        Error("Cannot open input file '%s'", userData.mMeanFile.mpOccupN);
      }
  
      userData.mCovFile.mpStatsP = fopen(userData.mCovFile.mpStatsN, "r");
      if (userData.mCovFile.mpStatsP == NULL) 
      {
        Error("Cannot open input file '%s'", userData.mCovFile.mpStatsN);
      }
  
      if (binary) 
      {
        header.precision = -1;
        fread(&header, sizeof(header), 1, userData.mCovFile.mpStatsP);
        if (!isBigEndian()) swap2(header.size);
        if (ferror(userData.mCovFile.mpStatsP)) {
          Error("Cannot read input file '%s'", userData.mCovFile.mpStatsN);
        } else if (header.stats_type               != STATS_COV_LOW_TRI
               || static_cast<size_t>(header.size) != userData.mpXform->mInSize
               || header.precision                 != (sizeof(FLOAT) == sizeof(float)
                                                     ? PRECISION_FLOAT : PRECISION_DOUBLE))
        {
          Error("Invalid header in file '%s'", userData.mCovFile.mpStatsN);
        }
      }
  
      userData.mCovFile.mpOccupP = fopen(userData.mCovFile.mpOccupN, "r");
      if (userData.mCovFile.mpOccupP == NULL) {
        Error("Cannot open output file '%s'", userData.mCovFile.mpOccupN);
      }
  
      Scan(MTM_MEAN | MTM_VARIANCE, nodeNameBuffer,
           ReadStatsForXform, &userData);
  
  
      fclose(userData.mMeanFile.mpStatsP); fclose(userData.mMeanFile.mpOccupP);
      fclose(userData.mCovFile.mpStatsP);  fclose(userData.mCovFile.mpOccupP);
      free(userData.mMeanFile.mpStatsN);   free(userData.mMeanFile.mpOccupN);
      free(userData.mCovFile.mpStatsN);    free(userData.mCovFile.mpOccupN);
  
  
      strcpy(ext, ".xfm");
      if ((fp = fopen(fileName, "r")) == NULL) {
        Error("Cannot open input xforn file '%s'", fileName);
      }
  
      int c =0;
      for (i=0; i < userData.mpXform->mOutSize * userData.mpXform->mInSize; i++) {
        c |= fscanf(fp, FLOAT_FMT, &userData.mpXform->mpMatrixO[i]) != 1;
      }
      if (ferror(fp)) {
        Error("Cannot read xform file '%s'", fileName);
      } else if (c) {
        Error("Invalid xform file '%s'", fileName);
      }
      fclose(fp);
    }
  }


  //*****************************************************************************  
  //*****************************************************************************  
  void 
  ModelSet::
  WriteHMMStats(const char * pFileName)
  {
    Macro *     macro;
    size_t      i = 0;
    size_t      j;
    size_t      k;
    FILE *      fp;
  
    if ((fp = fopen(pFileName, "wt")) == NULL) 
    {
      Error("Cannot open output file: '%s'", pFileName);
    }
  
    for (macro = mpFirstMacro; macro != NULL; macro = macro->nextAll) 
    {
      if (macro->mpData->mpMacro != macro) 
        continue;
        
      if (macro->mType != mt_hmm) 
        continue;
        
      Hmm *hmm = (Hmm *) macro->mpData;
  
      fprintf(fp, "%4d%*c\"%s\" %4ld ", (int) ++i,
                  HIGHER_OF(0,13 - strlen(macro->mpName)), ' ',
                  macro->mpName, macro->mOccurances);
  
      for (j = 0; j < hmm->mNStates-2; j++) 
      {
        State *state = hmm->mpState[j];
        FLOAT stOccP = 0;
        for (k = 0; k < state->mNumberOfMixtures; k++) 
        {
          stOccP += state->mpMixture[k].mWeightAccum;
        }
        fprintf(fp, " %10.6f", stOccP);
      }
      fputs("\n", fp);
    }
  
    fclose(fp);
  }
  
  
  //**************************************************************************  
  //**************************************************************************  
  void 
  ModelSet::
  ResetXformInstances()
  {
    XformInstance *inst;
    for (inst = mpXformInstances; inst != NULL; inst = inst->mpNext) 
    {
      inst->mStatCacheTime = UNDEF_TIME;
      inst->mTime          = UNDEF_TIME;
    }
  }

    
  //**************************************************************************  
  //**************************************************************************  
  void 
  ModelSet::
  UpdateStacks(FLOAT *obs, int time,  PropagDirectionType dir) 
  {
    XformInstance *inst;
    
    for (inst = mpXformInstances; inst != NULL; inst = inst->mpNext) 
    {
      if (inst->mpXform->mDelay > 0) {
        XformPass(inst, obs, time, dir);
      }
    }
  }
      
    
  //**************************************************************************  
  //**************************************************************************  
  MyHSearchData 
  ModelSet::
  MakeCIPhoneHash()
  {
    unsigned int              i;
    int                       nCIphns = 0;
    MyHSearchData    tmpHash;
    MyHSearchData    retHash;
    ENTRY                     e;
    ENTRY *                   ep;
  
    if (!my_hcreate_r(100, &tmpHash)) 
      Error("Insufficient memory");
  
    // Find CI HMMs and put them into hash
    for (i = 0; i < mHmmHash.mNEntries; i++) 
    {
      Macro *macro = (Macro *) mHmmHash.mpEntry[i]->data;
      if (strpbrk(macro->mpName, "+-")) continue;
  
      e.key  = macro->mpName;
      e.data = macro->mpData;
      
      if (!my_hsearch_r(e, ENTER, &ep, &tmpHash)) 
        Error("Insufficient memory");
      
      nCIphns++;
    }
  
    // Find CD HMMs and mark corresponding CI HMMs in the hash
    for (i = 0; i < mHmmHash.mNEntries; i++) 
    {
      char *    ciname;
      char      chr;
      int       cinlen;
      Macro *   macro = (Macro *) mHmmHash.mpEntry[i]->data;
      
      if (!strpbrk(macro->mpName, "+-")) 
        continue;
  
      ciname = strrchr(macro->mpName, '-');
      
      if (ciname == NULL) ciname = macro->mpName;
      else ciname++;
      
      cinlen = strcspn(ciname, "+");
      chr = ciname[cinlen];
      ciname[cinlen] = '\0';
      e.key  = ciname;
      my_hsearch_r(e, FIND, &ep, &tmpHash);
      ciname[cinlen] = chr;
  
      if (ep != NULL && ep->data != 0) 
      {
        ep->data = NULL;
        nCIphns--;
      }
    }
    
    if (!my_hcreate_r(nCIphns, &retHash)) 
      Error("Insufficient memory");
  
    // To the new hash, collect only those CI HMMs from tmpHash not having CD versions
    for (i = 0; i < tmpHash.mNEntries; i++) 
    {
      if (tmpHash.mpEntry[i]->data != NULL) 
      {
        Hmm *hmm  = (Hmm *) tmpHash.mpEntry[i]->data;
        size_t isTee = hmm->mpTransition->mpMatrixO[hmm->mNStates - 1] > LOG_MIN;
        e.key  = tmpHash.mpEntry[i]->key;
        e.data = reinterpret_cast<void *>(isTee);
        
        if (!my_hsearch_r(e, ENTER, &ep, &retHash)) 
          Error("Insufficient memory");
      }
    }
    
    my_hdestroy_r(&tmpHash, 0);
    return retHash;
  }
  
    
  //**************************************************************************  
  //**************************************************************************  
  void
  ModelSet::
  ComputeClusterWeightVectorCAT(size_t w)
  {
    // offset to the accumulator matrix row
    size_t start = 0;
    size_t end;
    
    for (size_t i = 0; i < w; i++)
    {
      start += mpClusterWeights[i]->mInSize;
    }
    
    end = start + mpClusterWeights[w]->mInSize;
    
    mpGw[w].Invert();
    mpClusterWeights[w]->mVector.Clear();
    
    for (size_t i = start; i < end; i++)
    {
      for (size_t j=0; j < mpGw[w].Cols(); j++)
      {
        mpClusterWeights[w]->mVector[0][i-start] += mpGw[w][i][j] * mpKw[w][0][j];
      }
    }
  }

  
  //**************************************************************************  
  //**************************************************************************  
  void 
  ModelSet::
  ResetClusterWeightAccumsCAT(size_t i)
  {
    mpGw[i].Clear();
    mpKw[i].Clear();
  }
  
}; //namespace STK  
  
