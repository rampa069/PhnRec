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
#include <assert.h>
#include <string>
#include <string.h>
#include <list>

#include "netgen.h"

int PhnList2HMMDef(char *phnList, char *hmmDefs, int nStates)
{
	assert(nStates > 0);

	FILE *fpi;
	fpi = fopen(phnList, "r");
	if(!fpi)	
		return NGEN_INPUTFILE;

	FILE *fpo;
	fpo = fopen(hmmDefs, "w");
	if(!fpo)
	{
		fclose(fpi);
		return NGEN_OUTPUTFILE;
	}

	std::list<std::string> phonemes;
	char buff[256];
	while(fscanf(fpi, "%255s", buff) == 1)
		phonemes.push_back(buff);	

	int nPhonemes = phonemes.size();

	std::list<std::string>::iterator it;

	int st = 1;

	fprintf(fpo, "~o <VecSize> %d <PDFObsVec>\n\n", nPhonemes * nStates);

	for(it = phonemes.begin(); it != phonemes.end(); it++)
	{
		fprintf(fpo, "~h \"%s\"\n", (*it).c_str());
		fprintf(fpo, "<BEGINHMM>\n");
		fprintf(fpo, "<NUMSTATES> %d\n", nStates + 2);
		int i;
		for(i = 0; i < nStates; i++)
		{
			fprintf(fpo, "<STATE> %d <ObsCoef> %d\n", i + 2, st);
			st++;
		}
		fprintf(fpo, "<TRANSP> %d\n", nStates + 2);
		
		for(i = 0; i < nStates + 2; i++)
		{
			int j;
			for(j = 0; j < nStates + 2; j++)
			{
				if(i == 0 && j == 1)
					fprintf(fpo, " %e", 1.0f);
				else if(i != 0 && i != nStates + 1 && (j == i || j == i + 1))
					fprintf(fpo, " %e", 0.5f);
				else
					fprintf(fpo, " %e", 0.0f);
			}
			fprintf(fpo, "\n");	
			//fprintf(fpo, "<STATE> %d <ObsCoef> %d\n", i + 2, st);
		}

		fprintf(fpo, "<ENDHMM>\n\n");
	}
	
	fclose(fpi);
	fclose(fpo);
	
	return NGEN_OK;
}

int PhnList2PhnLoop(char *phnList, char *phnLoop, char *omitPhn)
{
	FILE *fpi;
	fpi = fopen(phnList, "r");
	if(!fpi)	
		return NGEN_INPUTFILE;

	FILE *fpo;
	fpo = fopen(phnLoop, "w");
	if(!fpo)
	{
		fclose(fpi);
		return NGEN_OUTPUTFILE;
	}

	std::list<std::string> phonemes;
	char buff[256];

	while(fscanf(fpi, "%255s", buff) == 1)
	{
		if(!omitPhn || (strcmp(omitPhn, buff) != 0))
			phonemes.push_back(buff);	
	}

	int nPhonemes = phonemes.size();

	std::list<std::string>::iterator it;

	int id = 0;

//	fprintf(fpo, "NumberOfNodes=%d NumberOfArcs=%d\n", nPhonemes * 2 + 3, nPhonemes *4 + 1);

	// arcs from the initial node to model nodes
	fprintf(fpo, "%d\t      \t\t\t\t\t", id);
	id++;

	int i;
	for(i = 0; i < nPhonemes; i++)
		fprintf(fpo, " %d", i * 2 + 3);
	
	fprintf(fpo, "\n");

	// terminal node
	//fprintf(fpo, "%d\tN\t-       \t\t\t\t0\n", id);
	id++;

	// links from the null node to model nodes
	// arcs from the initial node to model nodes and terminal node
	fprintf(fpo, "%d\t      \t\t\t\t\t", id);
	id++;

	for(i = 0; i < nPhonemes; i++)
		fprintf(fpo, " %d", i * 2 + 3);
	
	fprintf(fpo, " 1\n");

	// model and word nodes
	for(it = phonemes.begin(); it != phonemes.end(); it++)
	{
		fprintf(fpo, "%d\tM=%-8s\t\t\t\t%d\n", id, (*it).c_str(), id + 1);
		id++;
		fprintf(fpo, "%d\tW=%-8s\t\t\t\t2\n", id, (*it).c_str());
		id++;
	}
	
	fclose(fpi);
	fclose(fpo);

	return NGEN_OK;
}

/*
int main()
{
	PhnList2HMMDef("dict", "hmm", 3);	
	PhnList2PhnLoop("dict", "net", "oth");

	return 0;
}
*/
