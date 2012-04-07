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
#include <string.h>

#include "phntranscheck.h"

PhnTransChecker::PhnTransChecker() :
        strtok_buff(0)
{
}

PhnTransChecker::~PhnTransChecker()
{
}

char *PhnTransChecker::StrTok(char *str, char *delims)
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

int PhnTransChecker::LoadPhnList(char *file)
{
	FILE *fp;
	fp = fopen(file, "r");
	if(!fp)
		return PTCE_INPUTFILE;

	phnList.clear();

	char phoneme[256];
	while(fscanf(fp, "%255s", phoneme) == 1)
		phnList.insert(phoneme);

	fclose(fp);

	return PTCE_OK;
}

bool PhnTransChecker::Check(char *transc, char *unknownPhns)
{
	char tr[1024];
	strcpy(tr, transc);
	char *tok = StrTok(tr, " \t");

	if(unknownPhns)
		strcpy(unknownPhns, "");

	if(!tok)
		return false;

	bool ok = true;

	while(tok)
	{
		std::set<std::string>::iterator it = phnList.find(tok);
		if(it == phnList.end())
		{
			ok = false;
			if(unknownPhns)
			{
				if(strlen(unknownPhns) != 0)
					strcat(unknownPhns, ", ");
				strcat(unknownPhns, tok);
			}
		}
		tok = StrTok(0, " \t");
	}

	if(!ok)
		return false;

	return true;
}

int PhnTransChecker::GetTranscLen(char *transc)
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

