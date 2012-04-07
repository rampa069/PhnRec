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

#ifndef _KWSNETG_H
#define _KWSNETG_H

#include <stdio.h>
#include <string>
#include "phntrans.h"

// error codes
#define KWSNGE_OK           0
#define KWSNGE_INPUTFILE    1
#define KWSNGE_OUTPUTFILE   2
#define KWSNGE_SYNTAX       3
#define KWSNGE_MISSINGWORD  4
#define KWSNGE_TRANSC       5
#define KWSNGE_ALREDYEXISTS 6
#define KWSNGE_NOTINIT      7
#define KWSNGE_UNKGRAPHEME  8 

#define KWSNG_ALLDICTS      INT_MAX

#define KWSNG_KEY           7452
#define KWSNG_XOR           'n'

class KWSNetGenerator
{
	protected:
                PhnTrans *PT;
                std::set<std::string> phnList;
		bool phnListLoaded;
		
		std::set<std::string> *WordList2Set(char *wordList);
		void WriteNode(FILE *fp, int node_id, char *type, char *word, char *flag);
		void WriteArcs(FILE *fp, std::map<int, float> *arcs);

                char *strtok_buff;
                char *StrTok(char *str, char *delims);

                bool GetTransInfo(std::list<PhnTrans::pt_entry> *list, int *nTransc, int *nPhonemes);
                
	public:
		KWSNetGenerator();
		~KWSNetGenerator();

                void SetPhnTrans(PhnTrans *pt) {PT = pt;}
                int LoadPhnList(char *file);
                
		int GenerateFromFile(char *wordListFile, char *file, char *unkWord = 0);
		int GenerateFromMem(char *wordList, char *file, char *unkWord = 0);
		int GenerateFromSet(std::set<std::string> *wordSet, char *file, char *unkWord = 0);

                int GetTranscLen(char *transc);
};

#endif





