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
#include "thresholds.h"


Thresholds::Thresholds() :
	defaultThr(0.0f) 
{
}

bool Thresholds::LoadThresholds(char *file)
{
	FILE *fp = fopen(file, "r");
	if(!fp)
		return false;

	char line[1024];
	while(fgets(line, 1023, fp))
	{
		line[1023] = '\0';
		char word[1024];
		float thr;
		if(strlen(line) != 0)
		{
			sscanf(line, "%[^ \t] %f", word, &thr);
			thrs[word] = thr;
		}
	}

	fclose(fp);
	return true;
}

float Thresholds::GetThreshold(char *word)
{
	std::map<std::string, float>::iterator it = thrs.find(word);
	if(it != thrs.end())
		return it->second;
	return defaultThr;
}
