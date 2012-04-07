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
#include <string.h>
#include "lexicon.h"
#include "filename.h"
#include "encode.h"

Lexicon::Lexicon() :
        strtok_buff(0),
        actPart(LEX_ALLPARTS)
{
}

Lexicon::~Lexicon()
{
}

char *Lexicon::StrTok(char *str, char *delims)
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

void Lexicon::Chomp(char *str)
{
	int i;
	for(i = 0; i < (int)strlen(str); i++)
	{
		if(str[i] == '\n' || str[i] == '\r')
			str[i] = '\0';
	}
}

int Lexicon::Load(char *file, int part, bool saveBin, char *retErrWord)
{
        std::map<int, part_info>::iterator it = parts.find(part);
        assert(it == parts.end());   // dictionary with the dictNo is alredy loaded

        part_info fi;
        fi.fileName = file;
        fi.saveBinary = saveBin;
        parts[part] = fi;

        char binaryFile[1024];
        strcpy(binaryFile, file);
        ChangeFileSuffix(binaryFile, "bl");

        if(FileExistence(binaryFile))
                return LoadBin(binaryFile, part, retErrWord);

	FILE *fp;
	fp = fopen(file, "r");
	if(!fp)
		return LEX_INPUTFILE;

       	//ClearDict();

	char line[1024];
	char word[1024];
	char trans[1024];
	while(fgets(line, 1023, fp))
	{
		Chomp(line);
                char *pword = StrTok(line, " \t");
                if(pword)
                {
        		strcpy(word, pword);
                        char *ptrans = StrTok(0, "");
                        if(!ptrans)
                        {
                                if(retErrWord)
                                        strcpy(retErrWord, word);
                                return LEX_SYNTAX;
                        }
        		strcpy(trans, ptrans);
        		int ret = AddWord(word, trans, 1.0f, part, retErrWord);
        		if(ret != LEX_OK)
        		{
	        		fclose(fp);
        			return ret;
	        	}
                }
	}

	fclose(fp);

        if(saveBin && !FileExistence(binaryFile))
                SaveBin(part, binaryFile);

	return LEX_OK;
}

int Lexicon::Save(int part, char *file, bool saveBin)
{
	if(!file)
        {
                std::map<int, part_info>::iterator it = parts.find(part);
                assert(it != parts.end());  // unknown dictNo, loadDict was not caled with this dictNo
		file = (char *)it->second.fileName.c_str();
                saveBin = it->second.saveBinary;
        }

        if(saveBin)
                return SaveBin(part, file);

	FILE *fp;
	fp = fopen(file, "w");
	if(!fp)
		return LEX_OUTPUTFILE;

	std::multiset<LexEntry>::iterator it;
	for(it = words.begin(); it != words.end(); it++)
        {
                if(it->part == LEX_ALLPARTS || it->part == part)
        		fprintf(fp, "%s\t%s\n", it->word.c_str(), it->trans.c_str());
        }

	fclose(fp);
	return LEX_OK;
}

int Lexicon::AddWord(char *word, char *trans, float prob, int part, char *retErrWord)
{
	LexEntry entry;
	entry.word = word;
	entry.trans = trans;
	entry.pronVariant = 0;
        entry.prob = prob;
        entry.part = part;

	std::multiset<LexEntry>::iterator it = words.find(entry);
	while(it != words.end())
	{
		if(it->word == entry.word && it->trans == entry.trans && it->part == entry.part)
		{
                        words.erase(it);
                        break;
		}
                entry.pronVariant++;
		it = words.find(entry);
	}
	words.insert(entry);
	return LEX_OK;
}

bool Lexicon::WordExists(char *word)
{
	LexEntry entry;
	entry.word = word;
	entry.pronVariant = 0;

	std::multiset<LexEntry>::iterator it = words.find(entry);
	while(it != words.end())
	{
                if(actPart == LEX_ALLPARTS || it->part == actPart)
                        return true;
                entry.pronVariant++;
                it = words.find(entry);
        }

	return false;
}

int Lexicon::LoadBin(char *file, int part, char *retErrWord)
{
	FILE *fp;
	fp = fopen(file, "rb");
	if(!fp)
		return LEX_INPUTFILE;

        int len = FileLen(fp);
        char *data = new char [len + 1];
        data[len] = '\0';
        fread(data, len, sizeof(char), fp);
        rand_encode(data, len, LEX_KEY, LEX_XOR);

       	//ClearDict();

	char line[1024];
	char word[1024];
	char trans[1024];

        char *pdata = data;
        char *pnl;

	while((pnl = strchr(pdata, '\n')) != 0)
	{
                *pnl = '\0';
                strcpy(line, pdata);
                pdata = pnl + 1;
                char *pword = StrTok(line, " \t");
                if(pword)
                {
        		strcpy(word, pword);
                        char *ptrans = StrTok(0, "");
                        if(!ptrans)
                        {
                                if(retErrWord)
                                        strcpy(retErrWord, word);
                                return LEX_SYNTAX;
                        }
        		strcpy(trans, ptrans);
        		int ret = AddWord(word, trans, 1.0f, part, retErrWord);
        		if(ret != LEX_OK)
        		{
	        		fclose(fp);
        			return ret;
	        	}
                }
	}

	fclose(fp);
        return LEX_OK;
}

int Lexicon::SaveBin(int part, char *file)
{
	FILE *fp;
	fp = fopen(file, "wb");
	if(!fp)
		return LEX_OUTPUTFILE;

	std::multiset<LexEntry>::iterator it;
        int len = 0;
	for(it = words.begin(); it != words.end(); it++)
        {
                if(it->part == LEX_ALLPARTS || it->part == part)
                {
        		len += strlen(it->word.c_str());
                        len += strlen(it->trans.c_str());
                        len += 2;  // \t \n
                }
        }

        char *data = new char [len];
        char *pdata = data;

	for(it = words.begin(); it != words.end(); it++)
        {
                if(it->part == LEX_ALLPARTS || it->part == part)
                {
        		sprintf(pdata, "%s\t", it->word.c_str());
                        pdata += strlen(it->word.c_str());
                        pdata += 1;
                        sprintf(pdata, "%s\n", it->trans.c_str());
                        pdata += strlen(it->trans.c_str());
                        pdata += 1;
                }
        }

        rand_encode(data, len, LEX_KEY, LEX_XOR);
        fwrite(data, len, sizeof(char), fp);
        delete [] data;

	fclose(fp);
	return LEX_OK;

}

bool Lexicon::GetTranscs(char *word, std::list<trans_entry> *list)
{
        list->clear();

	LexEntry entry;
	entry.word = word;
	entry.pronVariant = 0;

	std::multiset<LexEntry>::iterator it = words.find(entry);

	if(it == words.end())
		return false;

	while(it != words.end() && it->word == entry.word)
	{
                trans_entry te;
                te.trans = it->trans;
                te.prob = it->prob;
                if(it->part == actPart || actPart == LEX_ALLPARTS)
                        list->push_back(te);
		it++;
	}
        return true;
}

bool Lexicon::RemoveWord(char *word, int part)
{
        // create temporal list
        std::multiset<LexEntry> tmp;
        tmp.clear();

        // key
	LexEntry entry;
	entry.word = word;
	entry.pronVariant = 0;

        bool removed = false;

        // move the word to the temporal list, remove entries from the selected part of lexicon
	std::multiset<LexEntry>::iterator it = words.find(entry);
        while(it != words.end())
        {
                if(part == LEX_ALLPARTS || it->part == part)
                        removed = true;
                else
                        tmp.insert(*it);

                words.erase(it);

                entry.pronVariant++;
                it = words.find(entry);
        }

        // add words from the temporal list
        for(it = tmp.begin(); it != tmp.end(); it++)
                AddWord(word, (char *)it->trans.c_str(), it->prob, it->part);

	return removed;        
}

