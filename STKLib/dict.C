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

#include "dict.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *dict_filter;


void ReadDictionary(
    const char *dictFileName,
    MyHSearchData *wordHash,
    MyHSearchData *phoneHash)
{
  char line[1024];
  int line_no = 0;
  FILE *fp;
  Word *word;
  char *word_name, *model_name, *chrptr;
  Pronun *new_pronun;

  if ((fp = my_fopen(dictFileName, "rt", dict_filter)) == NULL) {
      Error("Cannot open file: '%s'", dictFileName);
  }
  while (fgets(line, sizeof(line), fp)) {
    ENTRY e, *ep;
    
    line_no++;
    if (strlen(line) == sizeof(line)-1) {
      Error("Line is too long (%s:%d)", dictFileName, line_no);
    }
    if (getHTKstr(word_name = line, &chrptr)) {
      Error("%s (%s:%d)", chrptr, dictFileName, line_no);
    }
    e.key  = word_name;
    e.data = NULL;
    my_hsearch_r(e, FIND, &ep, wordHash);

    if (ep != NULL) {
      word = (Word *) ep->data;
    } else {
      e.key  = strdup(word_name);
      word = (Word *) malloc(sizeof(Word));
      if (e.key == NULL || word  == NULL) Error("Insufficient memory");
      word->mpName = e.key;
      word->npronuns = 0;
      word->pronuns  = NULL;
      e.data = word;

      if (!my_hsearch_r(e, ENTER, &ep, wordHash)) {
        Error("Insufficient memory");
      }
    }

    word->npronuns++;
    word->npronunsInDict = word->npronuns;

    word->pronuns = (Pronun **) realloc(word->pronuns, word->npronuns * sizeof(Pronun *));
    new_pronun = (Pronun *) malloc(sizeof(Pronun));
    if (word->pronuns == NULL || new_pronun == NULL) Error("Insufficient memory");

    word->pronuns[word->npronuns-1] = new_pronun;
    new_pronun->mpWord       = word;
    new_pronun->outSymbol  = word->mpName;
    new_pronun->nmodels    = 0;
    new_pronun->model      = NULL;
    new_pronun->variant_no = word->npronuns;

    if (*chrptr == '[') {

      model_name = ++chrptr;
      while (*chrptr && *chrptr != ']') chrptr++;

      if (*chrptr != ']') Error("Matching `]' is missing (%s:%d)",
                                model_name, dictFileName, line_no);
      *chrptr++ = '\0';
      new_pronun->outSymbol = *model_name != '\0' ?  strdup(model_name) : NULL;
    }

    new_pronun->prob = strtod(chrptr, &chrptr);

    while (*chrptr) {
      if (getHTKstr(model_name = chrptr, &chrptr)) {
        Error("%s (%s:%d)", chrptr, dictFileName, line_no);
      }

      e.key  = model_name;
      e.data = NULL;
      my_hsearch_r(e, FIND, &ep, phoneHash);

      if (ep == NULL) {
        e.key  = strdup(model_name);
        e.data = e.key;

        if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, phoneHash)) {
          Error("Insufficient memory");
        }
        ep->data = e.data;
      }
      new_pronun->nmodels++;
      new_pronun->model=(_Model*) realloc(new_pronun->model,
                                new_pronun->nmodels*sizeof(*new_pronun->model));
      if (new_pronun->model == NULL) Error("Insufficient memory");
      new_pronun->model[new_pronun->nmodels-1].mpName = static_cast <char*> (ep->data);
    }
  }
  if (ferror(fp) || my_fclose(fp)) {
    Error("Cannot read dictionary file %s", dictFileName);
  }
}

void FreeDictionary(MyHSearchData *wordHash) {
  size_t i;
  size_t j;
  
  for (i = 0; i < wordHash->mNEntries; i++) 
  {
    Word *word = (Word *) wordHash->mpEntry[i]->data;
    for (j = 0; j < static_cast<size_t>(word->npronuns); j++) {
      free(word->pronuns[j]);
    }
    free(word->pronuns);
    free(wordHash->mpEntry[i]->data);
    free(wordHash->mpEntry[i]->key);
  }
  my_hdestroy_r(wordHash, 0);
}
