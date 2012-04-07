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

#include "fileio.h"
#include "stkstream.h"
#include "common.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#include <direct.h>
#endif

namespace STK
{
  enum CNFileType 
  {
    CNF_Mean, 
    CNF_Variance, 
    CNF_VarScale
  };
  
  //***************************************************************************
  //***************************************************************************
  void ReadCepsNormFile(
    const char *  pFileName, 
    char **       last_file, 
    FLOAT **      vec_buff,
    int           sampleKind, 
    CNFileType    type, 
    int           coefs);
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  WriteHTKHeader (FILE * pOutFp, HtkHeader header, bool swap)
  {
    int cc;
  
    if (swap) {
      swap4(header.mNSamples);
      swap4(header.mSamplePeriod);
      swap2(header.mSampleSize);
      swap2(header.mSampleKind);
    }
  
    fseek (pOutFp, 0L, SEEK_SET);
    cc = fwrite(&header, sizeof(HtkHeader), 1, pOutFp);
  
    if (swap) {
      swap4(header.mNSamples);
      swap4(header.mSamplePeriod);
      swap2(header.mSampleSize);
      swap2(header.mSampleKind);
    }
  
    return cc == 1 ? 0 : -1;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  WriteHTKFeature(FILE * pOutFp, FLOAT * pOut, size_t feaLen, bool swap) 
  {
    size_t    i;
    size_t    cc = 0;
    
  #if !DOUBLEPRECISION
    if (swap) 
      for (i = 0; i < feaLen; i++) 
        swap4(pOut[i]);
    
    cc = fwrite(pOut, sizeof(FLOAT_32), feaLen, pOutFp);
    
    if (swap) 
      for (i = 0; i < feaLen; i++) 
        swap4(pOut[i]);
  #else
    FLOAT_32 f;
  
    for (i = 0; i < feaLen; i++) 
    {
      f = pOut[i];
      if (swap) 
        swap4(f);
      cc += fwrite(&f, sizeof(FLOAT_32), 1, pOutFp);
    }
  #endif
    return cc == feaLen ? 0 : -1;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  ReadHTKHeader (FILE * pInFp, HtkHeader * pHeader, bool swap)
  {
    if (!fread(&pHeader->mNSamples,     sizeof(INT_32),  1, pInFp)) return -1;
    if (!fread(&pHeader->mSamplePeriod, sizeof(INT_32),  1, pInFp)) return -1;
    if (!fread(&pHeader->mSampleSize,   sizeof(INT_16),  1, pInFp)) return -1;
    if (!fread(&pHeader->mSampleKind,   sizeof(UINT_16), 1, pInFp)) return -1;
  
    if (swap) 
    {
      swap4(pHeader->mNSamples);
      swap4(pHeader->mSamplePeriod);
      swap2(pHeader->mSampleSize);
      swap2(pHeader->mSampleKind);
    }
  
    if (pHeader->mSamplePeriod < 0 || pHeader->mSamplePeriod > 100000 ||
        pHeader->mNSamples     < 0 || pHeader->mSampleSize   < 0) 
    {
      return -1;
    }
  
    return 0;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  ReadHTKFeature(
      FILE *    pInFp, 
      FLOAT *   pIn, 
      size_t    feaLen, 
      bool      swap,
      bool      decompress, 
      FLOAT *   pScale, 
      FLOAT *   pBias)
  {
    size_t i;
    
    if (decompress) 
    {
      INT_16 s;
  //    FLOAT pScale = (xmax - xmin) / (2*32767);
  //    FLOAT pBias  = (xmax + xmin) / 2;
  
      for (i = 0; i < feaLen; i++) 
      {
        if (fread(&s, sizeof(INT_16), 1, pInFp) != 1) 
          return -1;
        
        if (swap) swap2(s);
        pIn[i] = (s + pBias[i]) / pScale[i];
      }
      
      return 0;
    }
  
  #if !DOUBLEPRECISION
    if (fread(pIn, sizeof(FLOAT_32), feaLen, pInFp) != feaLen) 
      return -1;
    
    if (swap) 
      for (i = 0; i < feaLen; i++) 
        swap4(pIn[i]);
  #else
    FLOAT_32 f;
  
    for (i = 0; i < feaLen; i++) 
    {
      if (fread(&f, sizeof(FLOAT_32), 1, pInFp) != 1)
        return -1;
      
      if (swap) 
        swap4(f);
        
      pIn[i] = f;
    }
  #endif
    return 0;
  }  // int ReadHTKFeature
  
  
  //***************************************************************************
  //***************************************************************************
  int Mkdir4File(const char * pFileName)
  {
    struct stat   stat_buff;
    char          dir_name[260];
    char *        chptr;
  
    if ((chptr=strrchr(pFileName, '/')) == NULL)
      return 0;
    
    
    strncpy(dir_name, pFileName, chptr - pFileName);
    
    dir_name[chptr-pFileName]='\0';
    chptr=dir_name;
    
    if (*chptr == '/')
      chptr++;
    
    if (!chptr[0])
      return 0;
    
    for (;;) 
    {
      if ((chptr=strchr(chptr, '/')) != NULL)
        *chptr='\0';
      
      if ((access(dir_name, 0) || stat(dir_name, &stat_buff) == -1 ||
          !(S_IFDIR & stat_buff.st_mode))) 
      {
  #ifndef WIN32
        if (mkdir(dir_name, 0777)) 
          return -1;
  #else
        if (mkdir(dir_name)) 
          return -1;
  #endif
      }
      
      if (chptr)
        *chptr = '/';
      
      if (!chptr || !chptr[1])
        return 0;
      
      chptr++;
    }
  }
  
  /*
  FUNCTION
  
    FLOAT *ReadHTKFeatures(char *pFileName, bool swap, int extLeft, int extRight,
                          int targetKind, int derivOrder, int *derivWinLen,
                          HtkHeader *pHeader);
  
  DESCRIPTION
  
    Function opens HTK feature file, reads its contents, optionally augment
    features with their derivatives (of any order), optionally extend (augmented)
    features by repeating initial or final frames and return pointer to buffer
    with results allocated by malloc.
  
  PARAMETERS
  
    pFileName:
      Name of features file optionally augmented with frame range specification
      in the form 'pFileName[start,end]', where 'start' and 'end' are integers.
      If range is specified, only the corresponding segment of feature frames is
      extracted from file. Range [0,0] corresponds to only first frame. The range
      can also extend over feature file boundaries (e.g. start can be negative).
      In such case initial/final frames are repeated for the extending frames, so
      that resulting features have allays end-start+1 frames.
  
    swap:
      Boolean value specifies whether to swap bytes when reading file or not.
  
    extLeft:
      Features read from file are extended with extLeft initial frames. Normally,
      these frames are repetitions of the first feature frame in file (with its
      derivative, if derivatives are preset in the file).  However, if segment of
      feature frames is extracted according to range specification, the true
      feature frames from beyond the segment boundary are used, wherever it is
      possible. Note that value of extLeft can be also negative. In such case
      corresponding number of initial frames is discarded.
  
    extRight:
      The paramerer is complementary to parameter extLeft and has obvious
      meaning. (Controls extensions over the last frame, last frame from file is
      repeated only if necessary).
  
    targetKind:
      The parameters is used to check whether pHeader->mSampleKind match to
      requited targetKind and to control suppression of 0'th cepstral or energy
      coefficients accorging to modifiers _E, _0 and _N. Modifiers _D, _A and _T
      are ignored; Computation of derivatioves is controled by parameters
      derivOrder and derivWinLen. Value PARAMKIND_ANON ensures that function do
      not result in targetKind mismatch error and cause no _E or _0 suppression.
  
    derivOrder:
      Final features will be augmented with their derivatives up to 'derivOrder'
      order. If 'derivOrder' is negative value, no new derivatives are appended
      and derivatives that already present in feature file are preserved.
      Straight features are considered to be of zero order. If some
      derivatives are already present in feature file, these are not computed
      again, only higher order derivatives are appended if required. Note, that
      HTK feature file cannot contain higher order derivatives (e.g. double
      delta) without containing lower ones (e.g. delta). Derivative present
      in feature file that are of higher order than is required are discarded.
      Derivatives are computed in the final stage from (extracted segment of)
      feature frames possibly extended by repeated frames. Derivatives are
      computed using the same formula that is employed also by HTK tools. Lengths
      of windows used for computation of derivatives are passed in parameter
      derivWinLen. To compute derivatives for frames close to boundaries,
      frames before the first and after the last frame (of the extracted segment)
      are considered to be (yet another) repetitions of the first and the
      last frame, respectively. If the segment of frames is extracted according to
      range specification and parameters extLeft and extLeft are set to zero,
      the first and the last frames of the segment are considered to be repeated,
      eventough the true feature frames from beyond the segment boundary can be
      available in the file. Therefore, segment extracted from features that were
      before augmented with derivatives will differ from the same segment augmented
      with derivatives by this function. Difference will be of course only on
      boundaries and only in derivatives. This "incorrect" behavior was chosen
      to fully simulate behavior of HTK tools. To obtain more correct computation
      of derivatives, use parameters extLeft and extRight, which correctly extend
      segment with the true frames (if possible) and in resulting feature matrix
      ignore first extLeft and last extRight frames. For this purpose, both
      extLeft and extRight should be set to sum of all values in the array
      derivWinLen.
  
    derivWinLen:
      Array of size derivOrder specifying lengths of windows used for computation
      of derivatives. Individual values represents one side context used in
      the computation. The each window length is therefore twice the value
      from array plus one. Value at index zero specify window length for first
      order derivatives (delta), higher indices corresponds to higher order
      derivatives.
  
    pHeader:
      Structure *header is filled by information read from header of HTK feature
      file. The information is changed according to modifications made to
      features: bits representing added/removed derivatives are set/unset in
      header->mSampleKind, header->mNSamples is set to correspond to the number of
      frames in the resulting feature matrix.
  
  RETURN VALUE
  
      Function return pointer to buffer (allocated by malloc) with resulting
      feature matrix.
  */
  
  
  //***************************************************************************
  //***************************************************************************
  FLOAT *ReadHTKFeatures(
    char *                pFileName,
    bool                  swap,
    int                   extLeft,
    int                   extRight,
    int                   targetKind,
    int                   derivOrder,
    int *                 derivWinLen,
    HtkHeader *           pHeader,
    const char *          pCmnFile,
    const char *          pCvnFile,
    const char *          pCvgFile,
    RHFBuffer *           pBuff)
  {
    FLOAT *               fea_mx;
    int                   from_frame;
    int                   to_frame;
    int                   tot_frames;
    int                   trg_vec_size;
    int                   src_vec_size;
    int                   src_deriv_order;
    int                   lo_src_tgz_deriv_order;
    int                   i;
    int                   j;
    int                   k;
    int                   e;
    int                   coefs;
    int                   trg_E;
    int                   trg_0;
    int                   trg_N;
    int                   src_E;
    int                   src_0;
    int                   src_N;
    int                   comp;
    int                   coef_size;
    char *                chptr;
    //IStkStream            istr;
  
    // remove final spaces from file name
    for (i = strlen(pFileName) - 1; i >= 0 && isspace(pFileName[i]); i--)
    { 
      pFileName[i] = '\0';
    }
      
   
    // read frame range definition if any ( physical_file.fea[s,e] )
    if ((chptr = strrchr(pFileName, '[')) == NULL ||
        ((i=0), sscanf(chptr, "[%d,%d]%n", &from_frame, &to_frame, &i), chptr[i] != '\0')) 
    {
      chptr = NULL;
    }
    
    if (chptr != NULL) 
      *chptr = '\0';
  
    if ((strcmp(pFileName, "-"))
    &&  (pBuff->mpLastFileName != NULL) 
    &&  (!strcmp(pBuff->mpLastFileName, pFileName))) 
    {
      *pHeader = pBuff->last_header;
    } 
    else 
    {
      if (pBuff->mpLastFileName) 
      {
        if (pBuff->mpFp != stdin) 
          fclose(pBuff->mpFp);
        
        free(pBuff->mpLastFileName);
        pBuff->mpLastFileName = NULL;
      }
      
      
      if (!strcmp(pFileName, "-")) 
        pBuff->mpFp = stdin;
      else 
        pBuff->mpFp = fopen(pFileName, "rb");
      
      if (pBuff->mpFp == NULL) 
        Error("Cannot open feature file: '%s'", pFileName);
      
      /*
      istr.open(pFileName, ios::binary);
      if (!istr.good())
        Error("Cannot open feature file: '%s'", pFileName);
      pBuff->mpFp = istr.file();  
      */
      
      if (ReadHTKHeader(pBuff->mpFp, pHeader, swap)) 
        Error("Invalid HTK header in feature file: '%s'", pFileName);
      
      if (pHeader->mSampleKind & PARAMKIND_C) 
      {
        // File is in compressed form, scale and pBias vectors
        // are appended after HTK header.
  
        int coefs = pHeader->mSampleSize/sizeof(INT_16);
        pBuff->A = (FLOAT*) realloc(pBuff->A, coefs * sizeof(FLOAT_32));
        pBuff->B = (FLOAT*) realloc(pBuff->B, coefs * sizeof(FLOAT_32));
        if (pBuff->A == NULL || pBuff->B == NULL) Error("Insufficient memory");
  
        e  = ReadHTKFeature(pBuff->mpFp, pBuff->A, coefs, swap, 0, 0, 0);
        e |= ReadHTKFeature(pBuff->mpFp, pBuff->B, coefs, swap, 0, 0, 0);
        
        if (e) 
          Error("Cannot read feature file: '%s'", pFileName);
        
        pHeader->mNSamples -= 2 * sizeof(FLOAT_32) / sizeof(INT_16);
      }
      
      if ((pBuff->mpLastFileName = strdup(pFileName)) == NULL) 
        Error("Insufficient memory");
      
      pBuff->last_header = * pHeader;
    }
    
    if (chptr != NULL) 
      *chptr = '[';
  
    if (chptr == NULL) 
    { // Range [s,e] was not specified
      from_frame = 0;
      to_frame   = pHeader->mNSamples-1;
    }
    
    src_deriv_order = PARAMKIND_T & pHeader->mSampleKind ? 3 :
                      PARAMKIND_A & pHeader->mSampleKind ? 2 :
                      PARAMKIND_D & pHeader->mSampleKind ? 1 : 0;
    src_E =  (PARAMKIND_E & pHeader->mSampleKind) != 0;
    src_0 =  (PARAMKIND_0 & pHeader->mSampleKind) != 0;
    src_N = ((PARAMKIND_N & pHeader->mSampleKind) != 0) * (src_E + src_0);
    comp =    PARAMKIND_C & pHeader->mSampleKind;
    
    pHeader->mSampleKind &= ~PARAMKIND_C;
  
    if (targetKind == PARAMKIND_ANON) 
    {
      targetKind = pHeader->mSampleKind;
    } 
    else if ((targetKind & 077) == PARAMKIND_ANON) 
    {
      targetKind &= ~077;
      targetKind |= pHeader->mSampleKind & 077;
    }
    
    trg_E = (PARAMKIND_E & targetKind) != 0;
    trg_0 = (PARAMKIND_0 & targetKind) != 0;
    trg_N =((PARAMKIND_N & targetKind) != 0) * (trg_E + trg_0);
  
    coef_size     = comp ? sizeof(INT_16) : sizeof(FLOAT_32);
    coefs         = (pHeader->mSampleSize/coef_size + src_N) / 
                    (src_deriv_order+1) - src_E - src_0;
    src_vec_size  = (coefs + src_E + src_0) * (src_deriv_order+1) - src_N;
  
    //Is coefs dividable by 1 + number of derivatives specified in header
    if (src_vec_size * coef_size != pHeader->mSampleSize) 
    {
      Error("Invalid HTK header in feature file: '%s'. "
            "mSampleSize do not match with parmKind", pFileName);
    }
    
    if (derivOrder < 0) 
      derivOrder = src_deriv_order;
  
  
    if ((!src_E && trg_E) || (!src_0 && trg_0) || (src_N && !trg_N) ||
        (trg_N && !trg_E && !trg_0) || (trg_N && !derivOrder) ||
        (src_N && !src_deriv_order && derivOrder) ||
        ((pHeader->mSampleKind & 077) != (targetKind & 077) &&
         (pHeader->mSampleKind & 077) != PARAMKIND_ANON)) 
    {
      char srcParmKind[64], trgParmKind[64];
      ParmKind2Str(pHeader->mSampleKind, srcParmKind);
      ParmKind2Str(targetKind,       trgParmKind);
      Error("Cannot convert %s to %s", srcParmKind, trgParmKind);
    }
  
    lo_src_tgz_deriv_order = LOWER_OF(src_deriv_order, derivOrder);
    trg_vec_size  = (coefs + trg_E + trg_0) * (derivOrder+1) - trg_N;
    
    i =  LOWER_OF(from_frame, extLeft);
    from_frame  -= i;
    extLeft     -= i;
  
    i =  LOWER_OF(pHeader->mNSamples-to_frame-1, extRight);
    to_frame    += i;
    extRight    -= i;
  
    if (from_frame > to_frame || from_frame >= pHeader->mNSamples || to_frame < 0)
      Error("Invalid frame range for feature file: '%s'", pFileName);
    
    tot_frames = to_frame - from_frame + 1 + extLeft + extRight;
    fea_mx = (FLOAT *) malloc((trg_vec_size * tot_frames + 1) * sizeof(FLOAT));
    // + 1 needed for safe reading unwanted _E and _0
    
    if (fea_mx == NULL) 
      Error("Insufficient memory");
    
    for (i = 0; i <= to_frame - from_frame; i++) 
    {
      FLOAT *A = pBuff->A, *B = pBuff->B;
      FLOAT *mxPtr = fea_mx + trg_vec_size * (i+extLeft);
      
      fseek(pBuff->mpFp, sizeof(HtkHeader) + 
                         (comp ? src_vec_size * 2 * sizeof(FLOAT_32) : 0) +
                         (from_frame + i) * src_vec_size * coef_size, 
            SEEK_SET);
  
      e  = ReadHTKFeature(pBuff->mpFp, mxPtr, coefs, swap, comp, A, B);
      
      mxPtr += coefs; 
      A     += coefs; 
      B     += coefs;
        
      if (src_0 && !src_N) e |= ReadHTKFeature(pBuff->mpFp, mxPtr, 1, swap, comp, A++, B++);
      if (trg_0 && !trg_N) mxPtr++;
      if (src_E && !src_N) e |= ReadHTKFeature(pBuff->mpFp, mxPtr, 1, swap, comp, A++, B++);
      if (trg_E && !trg_N) mxPtr++;
  
      for (j = 0; j < lo_src_tgz_deriv_order; j++) 
      {
        e |= ReadHTKFeature(pBuff->mpFp, mxPtr, coefs, swap, comp, A, B);
        mxPtr += coefs; 
        A     += coefs; 
        B     += coefs;
        
        if (src_0) e |= ReadHTKFeature(pBuff->mpFp, mxPtr, 1, swap, comp, A++, B++);
        if (trg_0) mxPtr++;
        if (src_E) e |= ReadHTKFeature(pBuff->mpFp, mxPtr, 1, swap, comp, A++, B++);
        if (trg_E) mxPtr++;
      }
      
      if (e) 
        Error("Cannot read feature file: '%s' frame %d/%d", pFileName, i, to_frame - from_frame + 1);
    }
  
    // From now, coefs includes also trg_0 + trg_E !
    coefs += trg_0 + trg_E; 
  
    for (i = 0; i < extLeft; i++) 
    {
      memcpy(fea_mx + trg_vec_size * i,
            fea_mx + trg_vec_size * extLeft,
            (coefs * (1+lo_src_tgz_deriv_order) - trg_N) * sizeof(FLOAT));
    }
    
    for (i = tot_frames - extRight; i < tot_frames; i++) 
    {
      memcpy(fea_mx + trg_vec_size * i,
            fea_mx + trg_vec_size * (tot_frames - extRight - 1),
            (coefs * (1+lo_src_tgz_deriv_order) - trg_N) * sizeof(FLOAT));
    }
    
    // Sentence cepstral mean normalization
    if(pCmnFile == NULL && 
      !(PARAMKIND_Z & pHeader->mSampleKind) && 
        (PARAMKIND_Z & targetKind)) 
    {
      // for each coefficient
      for(j=0; j < coefs; j++) 
      {          
        FLOAT norm = 0.0;
        for(i=0; i < tot_frames; i++)      // for each frame
          norm += fea_mx[i*trg_vec_size - trg_N + j];
  
        norm /= tot_frames;
  
        for(i=0; i < tot_frames; i++)      // for each frame
          fea_mx[i*trg_vec_size - trg_N + j] -= norm;
      }
    }
    
    // Compute missing derivatives
    for (; src_deriv_order < derivOrder; src_deriv_order++) 
    {
      int winLen = derivWinLen[src_deriv_order];
      FLOAT norm = 0.0;
      
      for (k = 1; k <= winLen; k++) 
      {
        norm += 2 * k * k;
      }
      
      // for each frame
      for (i=0; i < tot_frames; i++) 
      {        
        // for each coefficient
        for (j=0; j < coefs; j++) 
        {          
          FLOAT *src = fea_mx + i*trg_vec_size + src_deriv_order*coefs - trg_N + j;
          *(src + coefs) = 0.0;
          
          if (i < winLen || i >= tot_frames-winLen) 
          { // boundaries need special treatment
            for (k = 1; k <= winLen; k++) 
            {  
              *(src+coefs) += k*(src[ LOWER_OF(tot_frames-1-i,k)*trg_vec_size]
                                -src[-LOWER_OF(i,            k)*trg_vec_size]);
            }
          } 
          else 
          { // otherwice use more efficient code
            for (k = 1; k <= winLen; k++) 
            {  
              *(src+coefs) += k*(src[ k * trg_vec_size]
                                -src[-k * trg_vec_size]);
            }
          }
          *(src + coefs) /= norm;
        }
      }
    }
    
    pHeader->mNSamples    = tot_frames;
    pHeader->mSampleSize  = trg_vec_size * sizeof(FLOAT_32);
    pHeader->mSampleKind  = targetKind & ~(PARAMKIND_D | PARAMKIND_A | PARAMKIND_T);
  
    /////////////// Cepstral mean and variance normalization ///////////////////
  
    if (pCmnFile != NULL) 
    {
      ReadCepsNormFile(pCmnFile, &pBuff->mpLastCmnFile, &pBuff->cmn,
                      pHeader->mSampleKind & ~PARAMKIND_Z, CNF_Mean, coefs);
                      
      for (i=0; i < tot_frames; i++) 
      {
        for (j=trg_N; j < coefs; j++) 
        {
          fea_mx[i*trg_vec_size + j - trg_N] -= pBuff->cmn[j];
        }
      }
    }
  
    pHeader->mSampleKind |= derivOrder==3 ? PARAMKIND_D | PARAMKIND_A | PARAMKIND_T :
                            derivOrder==2 ? PARAMKIND_D | PARAMKIND_A :
                            derivOrder==1 ? PARAMKIND_D : 0;
  
    if (pCvnFile != NULL) 
    {
      ReadCepsNormFile(pCvnFile, &pBuff->mpLastCvnFile, &pBuff->cvn,
                      pHeader->mSampleKind, CNF_Variance, trg_vec_size);
                      
      for (i=0; i < tot_frames; i++) 
      {
        for (j=trg_N; j < trg_vec_size; j++) 
        {
          fea_mx[i*trg_vec_size + j - trg_N] *= pBuff->cvn[j];
        }
      }
    }
    
    if (pCvgFile != NULL) 
    {
      ReadCepsNormFile(pCvgFile, &pBuff->mpLastCvgFile, &pBuff->cvg,
                      -1, CNF_VarScale, trg_vec_size);
                      
      for (i=0; i < tot_frames; i++) 
      {
        for (j=trg_N; j < trg_vec_size; j++) 
        {
          fea_mx[i*trg_vec_size + j - trg_N] *= pBuff->cvg[j];
        }
      }
    }
    
    return fea_mx;
  }
  
  //***************************************************************************
  //***************************************************************************
  void ReadCepsNormFile(
    const char *  pFileName, 
    char **       pLastFileName, 
    FLOAT **      vec_buff,
    int           sampleKind, 
    CNFileType    type, 
    int           coefs)
  {
    FILE *  fp;
    int     i;
    char    s1[9];
    char    s2[64];
    char *  typeStr = (char*) (type == CNF_Mean     ? "MEAN" :
                    type == CNF_Variance ? "VARIANCE" : "VARSCALE");
  
    char *  typeStr2 = (char*) (type == CNF_Mean     ? "CMN" :
                    type == CNF_Variance ? "CVN" : "VarScale");
  
    if (*pLastFileName != NULL && !strcmp(*pLastFileName, pFileName)) {
      return;
    }
    free(*pLastFileName);
    *pLastFileName=strdup(pFileName);
    *vec_buff = (FLOAT*) realloc(*vec_buff, coefs * sizeof(FLOAT));
  
    if (*pLastFileName == NULL || *vec_buff== NULL) 
      Error("Insufficient memory");
    
    if ((fp = fopen(pFileName, "r")) == NULL) 
      Error("Cannot open %s pFileName: '%s'", typeStr2, pFileName);
    
    if ((type != CNF_VarScale
        && (fscanf(fp, " <%64[^>]> <%64[^>]>", s1, s2) != 2
          || strcmp(strtoupper(s1), "CEPSNORM")
          || ReadParmKind(s2, false) != sampleKind))
        || fscanf(fp, " <%64[^>]> %d", s1, &i) != 2
        || strcmp(strtoupper(s1), typeStr)
        || i != coefs) 
    {
      ParmKind2Str(sampleKind, s2);
      Error("%s%s%s <%s> %d ... expected in %s pFileName %s",
            type == CNF_VarScale ? "" : "<CEPSNORM> <",
            type == CNF_VarScale ? "" : s2,
            type == CNF_VarScale ? "" : ">",
            typeStr, coefs, typeStr2, pFileName);
    }
    
    for (i = 0; i < coefs; i++) 
    {
      if (fscanf(fp, " "FLOAT_FMT, *vec_buff+i) != 1) 
      {
        if (fscanf(fp, "%64s", s2) == 1) 
        {
          Error("Decimal number expected but '%s' found in %s file %s",
                s2, typeStr2, pFileName);
        } 
        else if (feof(fp)) 
        {
          Error("Unexpected end of %s file %s", typeStr2, pFileName);
        } 
        else 
        {
          Error("Cannot read %s file %s", typeStr2, pFileName);
        }
      }
      
      if (type == CNF_Variance)      
        (*vec_buff)[i] = 1 / sqrt((*vec_buff)[i]);
      else if (type == CNF_VarScale) 
        (*vec_buff)[i] =     sqrt((*vec_buff)[i]);
    }
    
    if (fscanf(fp, "%64s", s2) == 1) 
    {
      Error("End of file expected but '%s' found in %s file %s",
            s2, typeStr2, pFileName);
    }
  } // ReadCepsNormFile(...)
  
} // namespace STK
  
