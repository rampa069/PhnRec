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

#ifndef FILEIO_H
#define FILEIO_H

#include "common.h"
#include <string>
#include <cstdio>

//using namespace std;
namespace STK
{
  /**
  * Structure for HTK header
  */
  struct HtkHeader
  {
    INT_32    mNSamples;              
    INT_32    mSamplePeriod;
    INT_16    mSampleSize;
    UINT_16   mSampleKind;
  };
  
  struct RHFBuffer
  {
    char *    mpLastFileName;
    char *    mpLastCmnFile;
    char *    mpLastCvnFile;
    char *    mpLastCvgFile;
    FILE *    mpFp;
    FLOAT *   cmn;
    FLOAT *   cvn;
    FLOAT *   cvg;
    HtkHeader last_header;
    FLOAT *   A;
    FLOAT *   B;
  } ;
  
  
  int WriteHTKHeader  (FILE * fp_out, HtkHeader header, bool swap);
  int WriteHTKFeature (FILE * fp_out, FLOAT *out, size_t fea_len, bool swap);
  int ReadHTKHeader   (FILE * fp_in, HtkHeader *header, bool swap);
  int ReadHTKFeature  (FILE * fp_in, FLOAT *in, size_t fea_len, bool swap,
                       bool   decompress, FLOAT *A, FLOAT *B);
  
  int Mkdir4File(const char * file_name);
  
  
  FLOAT *ReadHTKFeatures(
    char *        file_name,
    bool          swap,
    int           extLeft,
    int           extRight,
    int           targetKind,
    int           derivOrder,
    int *         derivWinLen,
    HtkHeader *   header,
    const char *  cmn_file,
    const char *  cvn_file,
    const char *  cvg_file,
    RHFBuffer *   buff);
  
}; // namespace STK  

#endif // FILEIO_H
