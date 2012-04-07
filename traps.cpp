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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "traps.h"
#include "dspc.h"
#include "nn.h"
#include "config.h"

Traps::Traps() :
		initialized(false),
		be_mat(0),
		be_mat_hamm(0),
		hamming(0),
		nbanks(MB_MELBANKS_DEF),
		trap_len(TRAP_LEN),
		merger_input_shift(0),
		useHamming(true),
		add_c0(false),
		trap_bands(0),
		delay(0),
		bunchSize(BUNCH_SIZE)
{
}


Traps::~Traps()
{
	if(!initialized)
		return;

    int i;

	switch(system)
	{
		case st3bt:
		case st1bt:
			delete [] band_classifier;
			delete [] be_mat;
			delete [] be_mat_hamm;
			delete [] hamming;
			for(i = 0; i < (system == st3bt ? nbanks - 2 : nbanks); i++)
			{
				delete [] band_input[i];
				delete [] band_output[i];
			}
			delete [] band_input;    
			delete [] band_output;
			break;
		case st1bt_dct:
			delete [] be_mat;
			delete [] be_mat_hamm;
			delete [] hamming;
			break;
		case stlcrc:
			delete [] be_mat;
			delete [] band_classifier;
			delete [] lc_mat;
			delete [] lc_mat_win;
			delete [] rc_mat; 
			delete [] rc_mat_win;
			delete [] be_win;
			for(i = 0; i < 2; i++)
			{
				delete [] band_input[i];
				delete [] band_output[i];
			}
            delete [] band_input;
			break;

	}
}


void Traps::Init(char *dir)
{
	initialized = false;

	// Matrix of mel-banks energies
	int half_context = (trap_len - 1) / 2 + 1;
	
	trap_bands = nbanks;
	if(system == st3bt)
		trap_bands -= 2;

	switch(system)
	{
		case st3bt:
		case st1bt:
		case st1bt_dct:
			be_mat = new float [nbanks * trap_len + 1];
			be_mat_hamm = new float [bunchSize * nbanks * trap_len + 1];

			// Hamming window
			hamming = new float [trap_len];
			sSet(trap_len, hamming, 1.0f);
			sWindow_Hamming(trap_len, hamming);
			break;
		case stlcrc:
			be_mat = new float [nbanks * trap_len + 1];
			lc_mat = new float [nbanks * half_context + 1];
			lc_mat_win = new float [bunchSize * nbanks * half_context + 1];
			rc_mat = new float [nbanks * half_context + 1];
			rc_mat_win = new float [bunchSize * nbanks * half_context + 1];
			be_win = new float [half_context * 2];
			break;
	}

	// Initialize neural nets and windows
	char fweights[256];
	char fnorms[256];
	char fwin[256];

	if(system == st3bt || system == st1bt || system == stlcrc)
	{
		int nnets = (system == stlcrc ? 2 : trap_bands);
		band_classifier = new NeuralNet [nnets];
		float *win = be_win;

                band_input = new float * [nnets];
                band_output = new float * [nnets];

		int i;
		for(i = 0; i < nnets; i++)
		{
			sprintf(fweights, "%s%s%d%s", dir, BANDS_WEIGHTS_FILE_PREFIX, i, BANDS_WEIGHTS_FILE_SUFFIX);
			sprintf(fnorms, "%s%s%d%s", dir, BANDS_NORMS_FILE_PREFIX, i, BANDS_NORMS_FILE_SUFFIX);
			if(band_classifier[i].Load(fweights, fnorms, bunchSize) != NN_OK)
                        {
                           fprintf(stderr, "ERROR: Loading neural network: weights %s, norms %s\n", fweights, fnorms);
                           exit(1);
                        }

			// band classifier input vector
			band_input[i] = new float [bunchSize * band_classifier[i].GetInputSize()];
			band_output[i] = new float [bunchSize * band_classifier[i].GetOutputSize()];

			if(system == stlcrc)
			{	
				sprintf(fwin, "%s%s%d%s", dir, BANDS_WINDOW_FILE_PREFIX, i, BANDS_WINDOW_FILE_SUFFIX);
				LoadWindow(fwin, half_context, win);
				win += half_context;
			}
		}		
	}

	sprintf(fweights, "%s%s", dir, MERGER_WEIGHTS_FILE);
	sprintf(fnorms, "%s%s", dir, MERGER_NORMS_FILE);
	if(merger.Load(fweights, fnorms, bunchSize) != NN_OK)
        {
           fprintf(stderr, "ERROR: Loading neural network: weights %s, norms %s\n", fweights, fnorms);
           exit(1);
        }

	// A vector for storing band classifiers output (merger input)
	merger_input = new float [bunchSize * merger.GetInputSize()];
	merger_input_shift = merger.GetInputSize() / trap_bands;
}


void Traps::Reset()
{
	initialized = false;
}


void Traps::AddVectorToBEMatrix(float *band_energies)
{
        float *offset;
        int i, j;
	if(!initialized)
	{
		initialized = true;

		// Set the same energy for whole temporal pattern
		offset = be_mat;
		for(i = 0; i < nbanks; i++)
		{
			for(j = 0; j < trap_len; ++j)
			{
				*offset = band_energies[i];
				offset++;
			}
		}

		delay = 0;
	}
	else
	{
		// Shift the matrix left and add the vector of band energies at
		// the end
		for(i = 0; i < nbanks * trap_len; i++)
			be_mat[i] = be_mat[i + 1];

		offset = be_mat;
		for(i = 0; i < nbanks; i++)
		{
			offset[trap_len - 1] = band_energies[i];
			offset += trap_len;
		}

		delay++;
		if(delay > 9999)
			delay = 9999;
	}
}

void Traps::CalcInputFeaturesForBandNets(int bunchPos)
{
	int i, j;
        float *offset;

	if(system != stlcrc)
	{
		// copy to new matrix
		for(i = 0; i < nbanks * trap_len; i++)
			be_mat_hamm[i] = be_mat[i];

		// apply hamming windows
		if(useHamming)
		{
			offset = be_mat_hamm;
			for(i = 0; i < nbanks; i++)
			{
				sMultVect(trap_len, offset, hamming);
				offset += trap_len;
			}
		}
	}

	// prepare memory block for NN forward pass in bunch mode
	float *inp, *outp;

	switch(system)
	{
		case st3bt:
		case st1bt:

			// => band_input
			inp = be_mat_hamm;
		
			for(i = 0; i < trap_bands; ++i)
			{
				outp = band_input[i] + bunchPos * trap_len;
				sCopy(trap_len, outp, inp);
				inp += trap_len;
			}
			break;

		case st1bt_dct:
			
			// => memory classifiers are not used at all => merger input
			inp = be_mat_hamm;
			outp = merger_input + bunchPos * merger.GetInputSize();

 			for(i = 0; i < trap_bands; ++i)
			{
				if(add_c0)
				{
					*outp = CalcC0(trap_len, inp);
					sDCT(trap_len, inp, merger_input_shift - 1, outp + 1);
				}
				else
				{
					sDCT(trap_len, inp, merger_input_shift, outp);
				}
				outp += merger_input_shift;
				inp += trap_len;
			}
			break;

		case stlcrc:

			// => band_input
			inp = be_mat;

			int half_context = (trap_len - 1) / 2 + 1;
			float *outp1 = lc_mat;
			float *outp2 = rc_mat;
			// C -> LC, RC
			for(i = 0; i < nbanks; i++)
			{
				for(j = 0; j < half_context; j++)
				{
					*outp1 = *inp;
					*outp2 = *(inp + (half_context - 1));
					outp1++;
					outp2++;
					inp++;
				}
				inp += (half_context - 1);
			}
			// Apply windows
			outp1 = lc_mat;
			outp2 = rc_mat;
			for(i = 0; i < nbanks; i++)
			{
				sMultVect(half_context, outp1, be_win);
				sMultVect(half_context, outp2, be_win + half_context);
				outp1 += half_context;
				outp2 += half_context;
			}
			// Apply DCT
			outp1 = band_input[0] + bunchPos * band_classifier[0].GetInputSize();
			outp2 = band_input[1] + bunchPos * band_classifier[1].GetInputSize();
			float *inp1 = lc_mat;
			float *inp2 = rc_mat;
			for(i = 0; i < nbanks; i++)
			{
				int nOut;
				if(add_c0)
				{
					*outp1 = CalcC0(half_context, inp1);
					*outp2 = CalcC0(half_context, inp2);
					outp1++;
					outp2++;
					nOut = band_classifier[0].GetInputSize() / nbanks - 1;
				}
				else
				{
					nOut = band_classifier[0].GetInputSize() / nbanks;
				}
				sDCT(half_context, inp1, nOut, outp1);
				sDCT(half_context, inp2, nOut, outp2);
				outp1 += nOut;
				outp2 += nOut;
				inp1 += half_context;
				inp2 += half_context;
			}
	}
}


void Traps::ForwardPassBandNets(int actBunchSize)
{
	int i;
	switch(system)
	{
		case st3bt:
		case st1bt:
			for(i = 0; i < trap_bands; i++)
                        {
				band_classifier[i].Forward(band_input[i], band_output[i], actBunchSize);
			}
                        break;

		case st1bt_dct:
			
			// band classifiers are not used at all
			break;

		case stlcrc:
			for(i = 0; i < 2; i++)
			{
/*
if(i == 1)
{       
puts("zzzz");
	int k;
	for(k = 0; k < actBunchSize; k++)
	{
        	int j;
		for(j = 0; j < band_classifier[i].GetInputSize(); j++)
		{
			printf(" %e", (band_input[i] + k * band_classifier[i].GetInputSize())[j]);
	        }
		puts("");
	}
}*/

				band_classifier[i].Forward(band_input[i], band_output[i], actBunchSize);

/*
if(i == 1)
{       
puts("zzzz");
	int k;
	for(k = 0; k < actBunchSize; k++)
	{
        	int j;
		for(j = 0; j < band_classifier[i].GetOutputSize(); j++)
		{
			printf(" %e", (band_output[i] + k * band_classifier[i].GetOutputSize())[j]);
	        }
		puts("");
	}
}*/

			}
	}
}


void Traps::CalcInputFeaturesForMerger(int actBunchSize)
{
	int i, j;
	float *inp, *outp;
	switch(system)
	{
		case st3bt:
		case st1bt:
			outp = merger_input;
			for(j = 0; j < actBunchSize; j++)
			{
				for(i = 0; i < trap_bands; i++)
				{
					int n = band_classifier[i].GetOutputSize();
					inp = band_output[i] + j * n;
					sCopy(n, outp, inp);
					outp += n;
				}
			}
			sLn(actBunchSize * merger.GetInputSize(), merger_input);
			sMultiplication(actBunchSize * merger.GetInputSize(), merger_input, -1);
			break;

		case st1bt_dct:
			
			// band classifiers are not used at all, merger input is alredy calculated
			break;

		case stlcrc:
			outp = merger_input;
			for(j = 0; j < actBunchSize; j++)
			{
				for(i = 0; i < 2; i++)
				{
					int n = band_classifier[i].GetOutputSize();
					inp = band_output[i] + j * n;
					sCopy(n, outp, inp);
					outp += n;
				}
			}

/*
//printf("### ");
int k;
for(k = 0; k < actBunchSize; k++)
{
for(i = 0; i < merger.GetInputSize(); i++)
{
	printf(" %e", (merger_input + k * merger.GetInputSize())[i]);
}
printf("\n");
}*/
			sLn(actBunchSize * merger.GetInputSize(), merger_input);

			break;
	}	
}

void Traps::ForwardPassMerger(int actBunchSize,	 float *retFeatures)
{
	merger.Forward(merger_input, retFeatures, actBunchSize);
}

void Traps::CalcFeatures(float *band_energies, float *features, int n, bool neededFea)
{
	assert(n != 0 && n <= bunchSize);

        int i;

/*
printf("XXX: %d\n", nbanks);

int k;
for(k = 0; k < n; k++)
{
printf(">> ");
for(i = 0; i < nbanks; i++)
{
	printf(" %e", (band_energies + k * nbanks)[i]);
}
puts("");
}
*/

	float *be = band_energies;
	for(i = 0; i < n; i++)
	{
		AddVectorToBEMatrix(be);
                if(neededFea)
        		CalcInputFeaturesForBandNets(i);
		be += nbanks;
	}

        if(neededFea)
        {
	        ForwardPassBandNets(n);
	        CalcInputFeaturesForMerger(n);
	        ForwardPassMerger(n, features);
/*
int k;
for(k = 0; k < n; k++)
{
for(i = 0; i < merger.GetOutputSize(); i++)
{
	printf(" %e", (features + k * merger.GetOutputSize())[i]);
}
puts("");
}*/
        }
}

void Traps::CalcFeaturesBunched(float *band_energies, float *features, int n, bool neededFea)
{
        float *be = band_energies;
        float *fe = features;

        int i;
        int nbunchs = n / bunchSize;
        int rem = n % bunchSize;

        for(i = 0; i < nbunchs; i++)
        {
                CalcFeatures(be, fe, bunchSize, neededFea);
                be += (bunchSize * nbanks);
                fe += (bunchSize * merger.GetOutputSize());
        }
        if(rem != 0)
                CalcFeatures(be, fe, rem, neededFea);
}

void Traps::DumpTr()
{
	int i, j;
	for(i = 0; i < nbanks; i++)
	{
		printf("band %d\n", i);
		for(j = 0; j < trap_len; ++j)
			printf(" %f", be_mat_hamm[i * trap_len + j]);
		printf("\n");
	}
}

void Traps::LoadWindow(char *file, int len, float *win)
{
	FILE *fp;
	fp = fopen(file, "r");
	if(!fp)
	{
		fprintf(stderr, "ERROR: Unable load window: %s\n", file);
		exit(1);
	}

	int i;
	for(i = 0; i < len; i++)
	{
		if(fscanf(fp, "%f", &win[i]) != 1)
		{
			fprintf(stderr, "ERROR: Weight file corrupted: %s\n", file);
			exit(1);
		}
	}

	fclose(fp);
}

bool Traps::SetSystem(char *sys)
{
	if(strcmp(sys, "3BT") == 0)
		system = st3bt;
	else if(strcmp(sys, "1BT") == 0)
		system = st1bt;
	else if(strcmp(sys, "1BT_DCT") == 0)
		system = st1bt_dct;
	else if(strcmp(sys, "LCRC") == 0)
		system = stlcrc;
	else
		return false;

	return true;
}
