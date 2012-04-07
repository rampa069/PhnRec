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

#ifndef _PHNTRANSCHECK_H
#define _PHNTRANSCHECK_H

#include <string>
#include <set>

#define PTCE_OK        0
#define PTCE_INPUTFILE 1

class PhnTransChecker
{
        protected:
       		std::set<std::string> phnList;
                char *strtok_buff;
                char *StrTok(char *str, char *delims);

        public:
                PhnTransChecker();
                ~PhnTransChecker();
		int LoadPhnList(char *file);
		int GetTranscLen(char *transc);
                bool Check(char *transc, char *unknownPhns = 0);
		bool PhonemeExists(char *phn);
};
#endif
