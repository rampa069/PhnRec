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
#include "melbanks.h"

MelBanks::MelBanks()
  : sample_freq(MB_DEF_SAMLEFREQ),            
    vector_size(MB_DEF_VECTORSIZE),
    step(MB_DEF_STEP),
    preem_coef(MB_DEF_PREEMCOEF),
    nbanks(MB_DEF_MELBANKS),
    nbanks_full(-1),
    lo_freq(MB_DEF_LOFREQ),
    hi_freq(MB_DEF_HIFREQ),
    take_log(true)
{
  initialized = false;
}

MelBanks::~MelBanks()
{
    Free();
}

void MelBanks::Init()
{  
  if(initialized)
    return;
    
  // get wector size which is power of two 
  fft_vector_size = 1;
  while(fft_vector_size < vector_size)
    fft_vector_size *= 2;
  
  fft_vector_size_2 = fft_vector_size / 2; 
    
	input_vector = 0;
	input_vector_pos = 0;
	first_frame = true;

	// space for one waveform frame
	frame = new float [vector_size];
	frame_backup = new float [vector_size];
	frame_pos = 0;

	// coefficients of hamming window
	hamming = new float[vector_size];
	sSet(vector_size, hamming, 1.0f);
	sWindow_Hamming(vector_size, hamming);

	// space for FFT
	FFT_vector = new float [2 * fft_vector_size + 1];     // real + imag + not used entry
	FFT_real = new float [fft_vector_size];
	FFT_imag = new float [fft_vector_size];

        if(nbanks_full == -1)
        {
           nbanks_full = nbanks;
        }    

        out_en = new float [nbanks_full];

	// calculate mel-bank
	mel_banks = _mbInit(nbanks_full, fft_vector_size, sample_freq, lo_freq, hi_freq, 0);   


  initialized = true;
}

void MelBanks::Free()
{
  if(!initialized)
    return;
    
	_mbFree(mel_banks);
	delete [] frame;
	delete [] frame_backup;
	delete [] hamming;
	delete [] FFT_vector;
	delete [] FFT_real;
	delete [] FFT_imag; 
        delete [] out_en;
	
	initialized = false;
}

void MelBanks::AddWaveform(float *waveform, int len)
{
  if(!initialized)
    Init();
    
	assert(waveform != 0);
	
  input_vector = waveform;
	input_vector_pos = 0;
	input_vector_len = len;
}

void MelBanks::ProcessFrame(float *inp_frame, float *ret_features)
{

	// preprocessing in the time domain
	if(z_mean_source)
        {
           sSubtractAverage(vector_size, inp_frame);
        }

        if(preem_coef != 0.0f)
        {
	   sPreemphasisBW(vector_size, inp_frame, preem_coef);
        }

	sMultVect(vector_size, inp_frame, hamming);

	// FFT
	sSet(fft_vector_size, FFT_real, 0.0f);
	sSet(fft_vector_size, FFT_imag, 0.0f);
	sCopy(vector_size, FFT_real, inp_frame);
	cComplex22N(fft_vector_size, FFT_real, FFT_imag, FFT_vector + 1);
	FFT_vector[0] = 0.0f;

	cFour1(FFT_vector, fft_vector_size, -1);

	c2N2Complex(fft_vector_size_2, FFT_vector + 1, FFT_real, FFT_imag);

	cPower(fft_vector_size_2, FFT_real, FFT_imag);
	//sSqrt(MB_FFTVECTORSIZE_2, FFTReal);

	// Get logarithms of energies in critical bands
	_mbApply(mel_banks, FFT_real, out_en, false);

        if(take_log)
        {
	   sLn(nbanks, out_en);
        }
        sCopy(nbanks, ret_features, out_en);
}

int MelBanks::GetFeatures(float *ret_features)
{
	if(input_vector == 0)
		return 0;                                  // we do not have input data

	// copy one frame from the input buffer to the temporary one during the first pass through this function,
	// shift the temporal buffer left an copy next 10 ms from the input buffer in all follow passes.
	if(first_frame)
	{
		sCopy(vector_size, frame, input_vector);
                                               // input_vector -> frame

		// backup the frame
		sCopy(vector_size, frame_backup, frame);

		frame_pos = vector_size;
		input_vector_pos += vector_size;

		first_frame = false;
	}
	else
	{
		// restore the last frame and shift it 10 ms left 
		if(frame_pos == vector_size - step)
		{
			sCopy(vector_size, frame, frame_backup);  // frame_backup -> frame
			sShiftLeft(vector_size, frame, step);
		}

		int n_input = input_vector_len - input_vector_pos; 
		int n_output = vector_size - frame_pos;
		int n_process = (n_input < n_output ? n_input : n_output);
		sCopy(n_process, frame + frame_pos, input_vector + input_vector_pos);
		input_vector_pos += n_process;
		frame_pos += n_process;

		if(input_vector_pos > input_vector_len)
			input_vector = 0;
	}

	if(frame_pos == vector_size)
	{
		// backup the frame
		sCopy(vector_size, frame_backup, frame);

		frame_pos = vector_size - step;

		// perform parameterisation of one frame
		ProcessFrame(frame, ret_features);
		return 1;
	}

	return 0;
}

void MelBanks::Reset()
{
	input_vector = (float *)0;
	first_frame = true;
}

