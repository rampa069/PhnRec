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

#ifndef MELBANK_H
#define MELBANK_H

#include "dspc.h"

// default seting
#define MB_DEF_SAMLEFREQ               8000
#define MB_DEF_VECTORSIZE               200  // = 25 ms
#define MB_DEF_STEP                      80  // = 10 ms
#define MB_DEF_PREEMCOEF                  0
#define MB_DEF_MELBANKS                  23
#define MB_DEF_LOFREQ                    64
#define MB_DEF_HIFREQ                  4000


class MelBanks
{
  private:
    // setting
    int sample_freq;            
    int vector_size;
    int fft_vector_size;
    int fft_vector_size_2;
    int step;
    float preem_coef;
    int nbanks;
    int nbanks_full;
    float lo_freq;
    float hi_freq;
    bool z_mean_source;
    bool take_log;    

    //
    bool initialized;
    bool first_frame;
    
    // buffers
    float	*input_vector;
    int	input_vector_pos;
    int	input_vector_len;
    int	frame_pos;
    float *frame;
    float	*frame_backup;
    float *FFT_vector;
    float *FFT_real;
    float *FFT_imag;
    float *hamming;
    float *out_en;
    struct _MelBanks *mel_banks;  
    
    //
  protected:
    virtual void Init();   // intitialize buffers
    virtual void Free();   // free buffers before setting of new parameters or before deallocation of the instance
    virtual void ProcessFrame(float *inp_frame, float *ret_features);

  public:
    MelBanks();
    virtual ~MelBanks();
    void AddWaveform(float *waveform, int len);
    int GetFeatures(float *ret_features);
    void Reset();

    void SetSampleFreq(int freq)   {Free(); sample_freq = freq;}
    void SetVectorSize(int vs)     {Free(); vector_size = vs;}
    void SetStep(int s)            {Free(); step = s;}
    void SetPreemCoef(float c)     {Free(); preem_coef = c;}
    void SetBanksNum(int n)        {Free(); nbanks = n; nbanks_full = -1;}
    void SetBanksFullNum(int n)    {Free(); nbanks_full = n;}
    void SetLowFreq(float freq)    {Free(); lo_freq = freq;}
    void SetHighFreq(float freq)   {Free(); hi_freq = freq;}
    void SetZMeanSource(bool flag) {Free(); z_mean_source = flag;}
    void SetTakeLog(bool flag)     {take_log = flag;}
    int GetNBanks()                {return nbanks;}
    int GetVectorSize()            {return vector_size;}
    int GetStep()                  {return step;}
    int GetSampleFreq()            {return sample_freq;}
    virtual int GetNParams()       {return GetNBanks();}
    float *GetCenters()            {return (mel_banks != 0 ? mel_banks->f0 : 0);}
};

#endif

