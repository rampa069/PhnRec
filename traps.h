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

#ifndef TRAPS_H
#define TRAPS_H

#include "nn.h"

typedef enum {st3bt, st1bt, st1bt_dct, stlcrc} system_type;

class Traps
{
	protected:
		bool initialized;
		float *be_mat;
		float *be_mat_hamm;
		float *be_win;
		float *hamming;
		float **band_input;
		float **band_output;
		float *merger_input;
		float *lc_mat;
		float *lc_mat_win;
		float *rc_mat;
		float *rc_mat_win;
		
		int nbanks;
		int trap_len;
		int merger_input_shift;
		bool useHamming;
		bool add_c0;
		int trap_bands;
		int delay;
		int bunchSize;
		system_type system;

		NeuralNet *band_classifier;
		NeuralNet merger;
		
		void LoadWindow(char *file, int len, float *win);
		
		void AddVectorToBEMatrix(float *band_energies);
		void CalcInputFeaturesForBandNets(int bunchPos);
		void ForwardPassBandNets(int actBunchSize);
		void CalcInputFeaturesForMerger(int actBunchSize);
		void ForwardPassMerger(int actBunchSize, float *retFeatures);

	public:
		Traps();
		~Traps();
		void Init(char *dir);
		void CalcFeatures(float *band_energies, float *features, int n = 1, bool neededFea = true);
                void CalcFeaturesBunched(float *band_energies, float *features, int n = 1, bool neededFea = true);
		void Reset();
		void DumpTr();
		int GetNumOuts()          {return merger.GetOutputSize();};
		int GetTrapShift()        {return (trap_len - 1) / 2;};
		int GetDelay()            {return delay;};
		// these functions should be called before Init
		bool SetSystem(char *sys);
		void SetTrapLen(int v)    {trap_len = v;}; 
		void SetHamming(bool ham) {useHamming = ham;};
		void SetNBanks(int v)     {nbanks = v;};
		void SetAddC0(bool v)     {add_c0 = v;};
		void SetBunchSize(int v)  {bunchSize = v;};
};

#endif
