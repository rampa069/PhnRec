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
#include <assert.h>
#include <string.h>

#include "gptrans.h"

GPTrans::GPTrans() :
	maxProns(-1),
	scalePronProb(false),
	lowestPronProb(-1.0f),
        initialized(false)
{
}

GPTrans::~GPTrans()
{
}

int GPTrans::LoadRules(char *file)
{
	R.LoadBinAtt(file);
        initialized = true;
	return GPT_OK;   // this should be fixed in the FSM class
}

int GPTrans::LoadSymbols(char *file)
{
	symbols.clear();

	FILE *fp = fopen(file, "r");
	if(!fp)
		return GTP_INPUTFILE;

	while(!feof(fp))
	{
		char buff[1024];
		int idx;
		fscanf(fp, "%[^ \t] %d\n", buff, &idx);
		symbols[buff] = idx;
		symbolsBack[idx] = buff;
	}

	fclose(fp);
	return GPT_OK;
}

int GPTrans::Generate(char *word, std::list<trans_entry> *trans_list)
{
	char word1[2048];
	strcpy(word1, word);

	// insert spaces between graphemes
	InsertSpaces(word1);

	// convert graphemes to indexes
	int idxs[2048];
	if(!Str2Idxs(word1, idxs))
		return GTP_UNKGRAPHEME;

	// insert first transcription variant to the list
	trans_list->clear();
	trans_entry e;
	e.trans = "";
	e.prob = 1.0f;
	trans_list->push_back(e);

	// count number of graphemes
	int len = 0;
	while(idxs[len] != 0)
		len++;

	// generate pronunciation variants
	int key_idxs[2048];
	std::list<rule> rules_list;
	std::list<trans_entry>::iterator it;

	int i;
	for(i = 0; i < len; i++)
	{
		CreateKeyIdxs(idxs, i, key_idxs);
		FindRules(key_idxs, &rules_list);

		std::list<trans_entry> help_list;
		help_list.clear();

		for(it = trans_list->begin(); it != trans_list->end(); it++)
		{
			std::string trans = it->trans;
			float tran_prob = it->prob;

			std::list<rule>::iterator itr;
			for(itr = rules_list.begin(); itr != rules_list.end(); itr++)
			{
				std::string target = symbolsBack[itr->target];
				float prob = itr->prob;

				if(itr == rules_list.begin())
				{
					// append a phoneme to the pronunciation
					it->trans = (trans == "" ? target : trans + " " + target);
					it->prob = tran_prob * prob;
				}
				else
				{
					// save new pronunciation variants
					e.trans = (trans == "" ? target : trans + " " + target);
					e.prob = tran_prob * prob;
					help_list.push_back(e);
				}
			}
		}

		// append new pronunciation variants to the list of pronunciation variants
		for(it = help_list.begin(); it != help_list.end(); it++)
		{
				e.trans = it->trans;
				e.prob = it->prob;
				trans_list->push_back(e);
		}
	}

	// filter pronunciation variants, remove '-', '*' and multiple spaces
	for(it = trans_list->begin(); it != trans_list->end(); it++)
	{
		char buff[1028];
		strcpy(buff, it->trans.c_str());
		FilterPron(buff);
		it->trans = buff;
	}

	// sort trans list
	trans_list->sort();
	trans_list->reverse();

	// scale pronunciation variance probability to the maximal value be one
	if(scalePronProb)
		ScalePronVarsProb(trans_list);

	// cut the list
	CutTransList(trans_list);

	return GPT_OK;
}

int GPTrans::GenerateBest(char *word, char *trans)
{
	std::list<trans_entry> trans_list;

	int ret = Generate(word, &trans_list);

	if(ret != GPT_OK)
		return ret;

	std::list<trans_entry>::iterator it = trans_list.begin();
	strcpy(trans, it->trans.c_str());

	return GPT_OK;
}

void GPTrans::InsertSpaces(char *str)
{
	char tmp[2048];
	strcpy(tmp, str);

	int i;
	for(i = 0; i < (int)strlen(str); i++)
	{
		str[2 * i] = tmp[i];
		str[2 * i + 1] = ' ';
	}
	str[2 * i - 2] = '\0';   // replace the last space
}

bool GPTrans::Str2Idxs(char *str, int *idxs)
{
	std::map<std::string, int>::iterator it;

	char tmp[1024];
	strcpy(tmp, str);
	char *tok = strtok(tmp, " \t");
	int i = 0;
	while(tok)
	{
		if(strlen(tok) != 0)
		{
			it = symbols.find(tok);
			if(it == symbols.end())
				return false;
			idxs[i] = symbols[tok];
			i++;
		}
		tok = strtok(0, " \t");
	}
	idxs[i] = 0;
	return true;
}

void GPTrans::CreateKeyIdxs(int *word_idxs, int i, int *key_idxs)
{
	int j = 0;
	bool left_out = false;
	bool right_out = false;
	int sign = 1;

	int len = 0;
	while(word_idxs[len] != 0)
		len++;

	int idx_boundary = symbols["+"];

	while(!left_out || !right_out)
	{
		if(i < 0)
		{
				left_out = true;
				key_idxs[j] = idx_boundary;
		}
		else if(i >= len)
		{
				right_out = true;
				key_idxs[j] = idx_boundary;
		}
		else
		{
			key_idxs[j] = word_idxs[i];
		}
		i += (sign * (j + 1));
		sign *= -1;
		j++;
	}
	key_idxs[j] = 0;
}

void GPTrans::FindRules(int *key_idxs, std::list<rule> *rules_list)
{
	NODE *node = R.GetStartNode();
	NODE *prevNode;
	NODE *lastEmitNode = 0;
	int i = 0;
	int last_idx = 0;

	while(node)
	{
		prevNode = node;
		node = R.GetNextNodeIS(node, key_idxs[i]);
		if(node)
		{
			lastEmitNode = prevNode;
			last_idx = key_idxs[i];
		}
		else
		{
			break;
		}
		i++;
	}

	rules_list->clear();
	rule r;
	if(lastEmitNode)
	{
		ARC *arc = lastEmitNode->fwArcsFirst;
		while(arc)
		{
			if((int)arc->labelFrom == last_idx)
			{
				assert(arc->labelTo != 0);
				r.target = arc->labelTo;
				r.prob = arc->weight;
				rules_list->push_back(r);
			}
			arc = arc->nextFW;
		}
	}
	else
	{
		r.target = 0;
		r.prob = 1.0f;
		rules_list->push_back(r);
	}
}

void GPTrans::FilterPron(char *pron)
{
	char pron1[1024];
	strcpy(pron1, pron);

	int i;
	for(i = 0; i < (int)strlen(pron1); i++)
	{
		if(pron1[i] == '-' || pron1[i] == '*' || pron1[i] == '+')
			pron1[i] = ' ';
	}

	int j = 0;
	char prevCh = ' ';
	for(i = 0; i < (int)strlen(pron1); i++)
	{
		if(pron1[i] != ' ' || prevCh != ' ')
		{
			pron[j] = pron1[i];
			j++;
		}
		prevCh = pron1[i];
	}
	pron[j] = '\0';

	j--;
	while(j >= 0 && pron[j] == ' ')
	{
		pron[j] = '\0';
		j--;
	}
}

void GPTrans::ScalePronVarsProb(std::list<trans_entry> *trans_list)
{
	std::list<trans_entry>::iterator it;
	float max = 1.0e-10f;
	for(it = trans_list->begin(); it != trans_list->end(); it++)
	{
		if(it->prob > max)
			max = it->prob;
	}
	float scale = 1.0f / max;

	for(it = trans_list->begin(); it != trans_list->end(); it++)
		it->prob *= scale;
}

void GPTrans::CutTransList(std::list<trans_entry> *trans_list)
{
	std::list<trans_entry> tmp(*trans_list);

	trans_list->clear();

	std::list<trans_entry>::iterator it;
	int n = 0;
	for(it = tmp.begin(); it != tmp.end(); it++)
	{
		if(lowestPronProb == -1.0f || it->prob > lowestPronProb)
		{
			trans_list->push_back(*it);
			n++;
			if(maxProns != -1 && n >= maxProns)
				break;
		}
	}
}

void GPTrans::CreateListOfAcceptedLetters()
{
        acceptedLetters.clear();
       	NODE *node = R.GetStartNode();
        ARC *arc = node->fwArcsFirst;
        while(arc)
        {
                acceptedLetters.insert(symbolsBack[arc->labelFrom]);
                arc = arc->nextFW;
        }
}

bool GPTrans::FilterInputString(char *str)
{
        CreateListOfAcceptedLetters();

        char tmp[1024];
        strcpy(tmp, str);

        unsigned int len =  strlen(tmp);

        int i, j = 0;
        for(i = 0; i < (int)len; i++)
        {
                char key[2];
                key[0] = tmp[i];
                key[1] = '\0';
                std::set<std::string>::iterator it = acceptedLetters.find(key);
                if(it != acceptedLetters.end())
                {
                        str[j] = tmp[i];
                        j++;
                }
        }
        return strlen(str) != len;
}


