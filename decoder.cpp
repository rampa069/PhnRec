/**************************************************************************
 *  copyright           : (C) 2004-2006 by Lukas Burget UPGM,FIT,VUT,Brno *
 *  email               : burget@fit.vutbr.cz                             *
 **************************************************************************
 *                                                                        *
 *  This program is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 **************************************************************************/

#include "decoder.h"
#include "matrix.h"
#include "filename.h"

Decoder::Decoder() :
  callbackFunc(0),
  callbackTmpParam(0),
  mode(DECMODE_DECODE)
{
}

Decoder::~Decoder()
{
}

int Decoder::ProcessFile(char *features, char *label_file, float *score)
{
  Mat<float> M;
  if(!M.loadHTK(features))
  {
    return DECERR_INPUTFILE;
  }
	
  int ret = Init(label_file);
  if(ret != DECERR_NONE)
    return ret;

  int i;
  float *frame = (float *)M.getMem();
  for(i = 0; i < M.rows(); i++)
  {
    ProcessFrame(frame);
    frame += M.columns();
  }

  float score1 = Done();

  if(score)
    *score = score1;

  return DECERR_NONE;
}

int Decoder::Init(char *label_file)
{
puts("decoder init");
  return DECERR_NONE;
}

void Decoder::ProcessFrame(float *frame)
{
}

float Decoder::Done()
{
  return (float)0;
}

