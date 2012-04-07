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

#ifndef _GPTRANS_H
#define _GPTRANS_H

#include <list>
#include <string>
#include <map>
#include <set>
#include "fsm.h"

#define GPT_OK              0
#define GTP_UNKGRAPHEME     1
#define GTP_INPUTFILE       2

#define GTP_MAX_WORD_LEN  100

class GPTrans
{
	public:
		typedef struct TE
		{
			std::string trans;
			float prob;
			bool operator < (const TE &v) const
			{
				if(prob < v.prob)
					return true;
				else if(prob > v.prob)
					return false;
				return trans < v.trans;
			}
		} trans_entry;

	protected:

		typedef struct
		{
			int target;
			float prob;
		} rule;

		FSM R;

		std::map<std::string, int> symbols;
		std::map<int, std::string> symbolsBack;
                std::set<std::string> acceptedLetters;

		int maxProns;
		bool scalePronProb;
		float lowestPronProb;
                bool initialized;

		void InsertSpaces(char *str);
		bool Str2Idxs(char *str, int *idxs);
		void CreateKeyIdxs(int *word_idxs, int i, int *key_idxs);
		void FindRules(int *key_idxs, std::list<rule> *rules_list);
		void FilterPron(char *pron);
		void ScalePronVarsProb(std::list<trans_entry> *trans_list);
		void CutTransList(std::list<trans_entry> *trans_list);
                void CreateListOfAcceptedLetters();
	public:

		GPTrans();
		~GPTrans();
		int LoadRules(char *file);
		int LoadSymbols(char *file);
		int Generate(char *word, std::list<trans_entry> *trans_list);
		int GenerateBest(char *word, char *trans);

		void SetMaxProns(int v) {maxProns = v;};
		void SetScalePronProb(bool v) {scalePronProb = v;};
		void SetLowestPronProb(float v) {lowestPronProb = v;};

                bool GetInitialized() {return initialized;};
                void UnInitialize() {initialized = false;}
                bool FilterInputString(char *str);
};

#endif
