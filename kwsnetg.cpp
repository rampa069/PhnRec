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

#include "kwsnetg.h"
#include "filename.h"
#include "encode.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

KWSNetGenerator::KWSNetGenerator() :
		PT(0),
		phnListLoaded(false),
                strtok_buff(0)
{
}

KWSNetGenerator::~KWSNetGenerator()
{
}

char *KWSNetGenerator::StrTok(char *str, char *delims)
{
	if(str)
		strtok_buff = str;

	// skip delims front of token
	while(*strtok_buff != '\0' && strchr(delims, *strtok_buff))
		strtok_buff++;

	if(*strtok_buff == '\0')
		return 0;

	char *ret = strtok_buff;

	// skip token
	while(*strtok_buff != '\0' && !strchr(delims, *strtok_buff))
		strtok_buff++;

	// terminate token and prepare next one
	if(*strtok_buff != '\0')
	{
		*strtok_buff = '\0';
		strtok_buff++;

		// skip delims front of token
		while(*strtok_buff != '\0' && strchr(delims, *strtok_buff))
			strtok_buff++;
	}
	return ret;
}

int KWSNetGenerator::GetTranscLen(char *transc)
{
	char tr[1024];
	strcpy(tr, transc);
	char *tok = StrTok(tr, " \t");

	int n = 0;

	while(tok)
	{
		n++;
		tok = StrTok(0, " \t");
	}
	return n;
}

int KWSNetGenerator::LoadPhnList(char *file)
{
	FILE *fp;
	fp = fopen(file, "r");
	if(!fp)
		return KWSNGE_INPUTFILE;

	phnList.clear();

	char phoneme[256];
	while(fscanf(fp, "%255s", phoneme) == 1)
		phnList.insert(phoneme);

	fclose(fp);

	phnListLoaded = true;

	return KWSNGE_OK;
}

std::set<std::string> *KWSNetGenerator::WordList2Set(char *wordList)
{
	std::set<std::string> *wordSet = new std::set<std::string>;
	char *buff = new char [strlen(wordList)  + 1];
	strcpy(buff, wordList);

	char *tok = StrTok(buff, " \n\r");
	while(tok)
	{
		wordSet->insert(tok);
		tok = StrTok(0, " \n\r");
	}

	delete [] buff;
	return wordSet;
}


int KWSNetGenerator::GenerateFromFile(char *wordListFile, char *file, char *unkWord)
{
	FILE *fp;
	fp = fopen(wordListFile, "rb");
	if(!fp)
		return KWSNGE_INPUTFILE;

	int len = FileLen(fp);
	char *buff = new char [len + 1];
	if((int)fread(buff, sizeof(char), len, fp) != len)
		return KWSNGE_INPUTFILE;
	fclose(fp);
	buff[len] = '\0';

	int ret = GenerateFromMem(buff, file, unkWord);
	delete [] buff;
	
	return ret;
}

int KWSNetGenerator::GenerateFromMem(char *wordList, char *file, char *unkWord)
{
	std::set<std::string> *wordSet = WordList2Set(wordList);
	int ret = GenerateFromSet(wordSet, file, unkWord);
	delete wordSet;
	return ret;
}

void KWSNetGenerator::WriteNode(FILE *fp, int node_id, char *type, char *word, char *flag)
{
// old format - fprintf(fp, "%d\t%s\t%-8s\t%s\t\t\t", node_id, type, word, out_sym);
	fprintf(fp, "%d\t%s=%-12s\t", node_id, type, word);
	if(strlen(flag) != 0)
	{
		fprintf(fp, "f=%s\t", flag);
	}
	else
	{
		fprintf(fp, "\t");
	}
//	fprintf(fp, "%d\t%s\t%-8s\t%s\t\t\t", node_id, type, word, out_sym);
}

void KWSNetGenerator::WriteArcs(FILE *fp, std::map<int, float> *arcs)
{
	//fprintf(fp, "%d\t", arcs->size());

	std::map<int, float>::iterator it;

	char *sep = ""; 
	for(it = arcs->begin(); it != arcs->end(); it++)
	{
		if(it->second == 0.0f)
			fprintf(fp, "%s%d", sep, it->first);
		else
			fprintf(fp, "%s%d l=%f", sep, it->first, it->second);
		sep = " ";
	}

	fprintf(fp, "\n");
	arcs->clear();
}

int KWSNetGenerator::GenerateFromSet(std::set<std::string> *wordSet, char *file, char *unkWord)
{
	if(!phnListLoaded || !PT)
		return KWSNGE_NOTINIT;

	// number of words, number of phonemes in phoneme loop
	int nWords = wordSet->size();
	int nPhonemes = phnList.size();

	// count total nuber of phonemes contained in all words
	int nPhonemesInWords = 0;
	std::set<std::string>::iterator it;
	for(it = wordSet->begin(); it != wordSet->end(); it++)
	{
		char *word = (char *)(*it).c_str();
		int nPronVars;
		int nWrdPhonemes;

                std::list<PhnTrans::pt_entry> list;
                int ret = PT->GetTranscs(word, &list);
                if(ret == PHNTE_UNKGRAPHEME)
                {
                        strcpy(unkWord, word);
                        return KWSNGE_UNKGRAPHEME;
                }

                if(list.size() == 0)
                {
                        strcpy(unkWord, word);
                        return KWSNGE_MISSINGWORD;
                }

        GetTransInfo(&list, &nPronVars, &nWrdPhonemes);
		nPhonemesInWords += nWrdPhonemes;
	}

	// open file
	FILE *fp = fopen(file, "w");
	if(!fp)
		return KWSNGE_OUTPUTFILE;


	// count number of nodes - the 'NumNodes' header tag 
        //
	// 5                ~ start node (N), end node (N), phoneme loop null node (N),
	//                    bankground sticky node (F) node and word start node (W)
	// nPhonemes        ~ other nodes in phoneme loop (M)
	// nPhonemesInWords ~ all phonemes in words including all pronunciation wariants (M)
    // nWords           ~ keyword end nodes (K) 
	int nNodes = 5 + nPhonemes + nPhonemesInWords + 2 * nWords;
	fprintf(fp, "N=%d\n\n", nNodes); 
	fprintf(fp, "#id     wrd/mdl         flag    link1 prob1 link2 prob ...\n");
	
	// generate the net
	int node_id = 0;
	std::map<int, float> arcs;

	// 0 - start node
	WriteNode(fp, node_id++, "W", "!NULL", "");
	arcs[3] = 0.0f;
	WriteArcs(fp, &arcs);

	// 1 ~ termianl node
	int lastNodeId = node_id;
	WriteNode(fp, node_id++, "W", "!NULL", "");
	WriteArcs(fp, &arcs);

	// 2 ~ background sticky node
	WriteNode(fp, node_id++, "W", "!NULL", "F");
	arcs[lastNodeId] = 0.0f;
	WriteArcs(fp, &arcs);
	
	// 3 ~ phone loop null node
	fprintf(fp, "\n#PhnLoop\n");
	WriteNode(fp, node_id++, "W", "!NULL", "");
	int i;
	for(i = 0; i < nPhonemes; i++)
	{
		arcs[i + node_id] = 0.0f;        // arcs to phonemes
	}
	arcs[node_id + nPhonemes] = 0.0f;    // arc to the word start node
	arcs[2] = 0.0f;                      // arc to the sticky background node 
	WriteArcs(fp, &arcs);

	// 4 to  4 + nPhonemes - 1 ~ phonemes in phoneme loop 
	std::set<std::string>::iterator itp;
	for(itp = phnList.begin(); itp != phnList.end(); itp++)
	{
		WriteNode(fp, node_id++, "M", (char *)itp->c_str(), "");
		arcs[3] = -1.0f;
		WriteArcs(fp, &arcs);
	}

	// links to word starts
	fprintf(fp, "\n#links to word start nodes\n");
	WriteNode(fp, node_id++, "W", "!NULL", "");
	for(i = 0; i < nWords; i++)
	{
		arcs[node_id + i] = 0.0f;
	}		
	WriteArcs(fp, &arcs);
	
/*	std::map<int, float> arcs_wstart;	
	// - links
	for(it = wordSet->begin(); it != wordSet->end(); it++)
	{
        std::list<PhnTrans::pt_entry> list;
        std::list<PhnTrans::pt_entry>::iterator itt;

		char *word = (char *)(*it).c_str();
        PT->GetTranscs(word, &list);

        for(itt = list.begin(); itt != list.end(); itt++)
        {
			arcs_wstart[idx] = 0.0f;
			int len = GetTranscLen((char *)itt->trans.c_str());
			idx += len;
		}
	}
*/
  
	//WriteArcs(fp, &arcs);

	// word start nodes
	fprintf(fp, "\n#word start nodes\n");
	int idx = node_id + 2 * nWords;
	for(it = wordSet->begin(); it != wordSet->end(); it++)
	{
		char pbuff[256];
		sprintf(pbuff, "%s_B", (char *)it->c_str());
		WriteNode(fp, node_id++, "W", pbuff, "");
        
		std::list<PhnTrans::pt_entry> list;
        std::list<PhnTrans::pt_entry>::iterator itt;

		char *word = (char *)(*it).c_str();
        PT->GetTranscs(word, &list);
        for(itt = list.begin(); itt != list.end(); itt++)
        {
			arcs[idx] = 0.0f;
			int len = GetTranscLen((char *)itt->trans.c_str());
			idx += len;
		}

		//arcs[1] = 0.0f;
		WriteArcs(fp, &arcs);
	}

	// word end nodes
	fprintf(fp, "\n#word end nodes\n");
	int wordEndIds = node_id;
	for(it = wordSet->begin(); it != wordSet->end(); it++)
	{
		WriteNode(fp, node_id++, "W", (char *)it->c_str(), "K");
		arcs[1] = 0.0f;
		WriteArcs(fp, &arcs);
	}
		
	// pronunciation variants
	fprintf(fp, "\n");
	i = 0;
	for(it = wordSet->begin(); it != wordSet->end(); it++)
	{
                std::list<PhnTrans::pt_entry> list;
                std::list<PhnTrans::pt_entry>::iterator itt;

       		char *word = (char *)(*it).c_str();
                PT->GetTranscs(word, &list);

                for(itt = list.begin(); itt != list.end(); itt++)
		{
			char tr[1024];
			strcpy(tr, (char *)itt->trans.c_str());
			int len = GetTranscLen(tr);

			fprintf(fp, "#wrd \"%s\"\n", word);

			int j = 0;
			char *tok = StrTok(tr, " \t");
			while(tok)
			{
				WriteNode(fp, node_id++, "M", tok, "");
				if(j != len - 1)
					arcs[node_id] = 0.0f;
				else
					arcs[wordEndIds + i] = 0.0f;
					WriteArcs(fp, &arcs);
				tok = StrTok(0, " \t");
				j++;
			}

			fprintf(fp, "\n");
		}
		i++;
	}

	fclose(fp);

	return  KWSNGE_OK;
}

bool KWSNetGenerator::GetTransInfo(std::list<PhnTrans::pt_entry> *list, int *nTransc, int *nPhonemes)
{
        std::list<PhnTrans::pt_entry>::iterator it;
        *nTransc = 0;
        *nPhonemes = 0;

        if(list->empty())
                return false;

	for(it = list->begin(); it != list->end(); it++)
	{
		(*nTransc)++;
		(*nPhonemes) += GetTranscLen((char *)it->trans.c_str());
	}

	return true;
}
