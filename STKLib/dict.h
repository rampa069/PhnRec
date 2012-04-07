/***************************************************************************
 *   copyright            : (C) 2004 by Lukas Burget,UPGM,FIT,VUT,Brno     *
 *   email                : burget@fit.vutbr.cz                            *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DICT_H
#define DICT_H

#include "Models.h"
#include "common.h"

extern const char *dict_filter;

//typedef struct _Word Word;
//typedef struct _Pronun Pronun;
class Pronun;
class Word;

class Word 
{
public:
  char *mpName;
  Pronun **pronuns;
  int npronunsInDict;  // Number of word pronuns found in dictionary file
  int npronuns;        // Can be higher than npronunsInDict as new empty
                       // pronuns can be added while reading the network
};


union _Model 
{
//    Hmm  *hmm;
    char *mpName;
};

class Pronun 
{
public:
  Word*         mpWord;
  char*         outSymbol;
  int           variant_no;
  FLOAT         prob;
  int           nmodels;
  union _Model *model;
};

void ReadDictionary(
  const char *dictFileName,
  MyHSearchData *wordHash,
  MyHSearchData *phoneHash);

void FreeDictionary(MyHSearchData *wordHash);


#endif // DICT_H
