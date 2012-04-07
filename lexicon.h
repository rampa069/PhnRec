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

#ifndef _LEXICON_H
#define _LEXICON_H

#include <string>
#include <list>
#include <map>
#include <set>
#include <climits>

// error codes
#define LEX_OK             0
#define LEX_INPUTFILE    101
#define LEX_OUTPUTFILE   102
#define LEX_SYNTAX       103
#define LEX_MISSINGWORD  104
#define LEX_TRANSC       105
#define LEX_ALREDYEXISTS 106
#define LEX_NOTINIT      107

#define LEX_ALLPARTS     INT_MAX

#define LEX_KEY           1000
#define LEX_XOR           '0'

class Lexicon
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
		class LexEntry
		{
			public:
				std::string word;
				std::string trans;
				int pronVariant;
                                int part;
                                float prob;
				bool operator < (const LexEntry &de) const
				{
					if(word < de.word)
						return true;
					else if(word > de.word)
						return false;
					else if(pronVariant < de.pronVariant)
						return true;
					else
						return false;
				}
		};
		std::multiset<LexEntry> words;

                typedef struct
                {
                        std::string fileName;
                        bool saveBinary;
                } part_info;
                std::map<int, part_info> parts;

		char *strtok_buff;
		char *StrTok(char *str, char *delims);
		void Chomp(char *str);

                int actPart;

        public:
                Lexicon();
                ~Lexicon();

		void Clear() {words.clear(); parts.clear();};

		int AddWord(char *word, char *trans, float prob, int part = 0, char *retErrWord = 0);
                bool RemoveWord(char *word, int part = LEX_ALLPARTS);

		int Load(char *file, int part = 0, bool saveBin = false, char *retErrWord = 0);
		int Save(int part = 0, char *file = 0, bool saveBin = false);

                int LoadBin(char *file, int part = 0, char *retErrWord = 0);
                int SaveBin(int part, char *file);

		bool WordExists(char *word);
                bool GetTranscs(char *word, std::list<trans_entry> *list);

                void SetActPart(int v) {actPart = v;}
};

#endif
