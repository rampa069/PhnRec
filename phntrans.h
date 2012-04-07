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

#ifndef _PHNTRANS_H
#define _PHNTRANS_H

#include "lexicon.h"
#include "gptrans.h"

#define PHNTE_OK          0
#define PHNTE_UNKGRAPHEME 1

#define PHNTM_LEXICON     1
#define PHNTM_GPT         2
#define PHNTM_UNION       3
#define PHNTM_LEXGPT      4

#define PHNTS_LEXICON     1
#define PHNTS_GPT         2

#include <list>
#include <string>

class PhnTrans
{
        public:
                typedef struct PTE
                {
                        std::string trans;
                        int source;
                        float prob;
                } pt_entry;

                static bool pte_less_trans(pt_entry &a, pt_entry &b);
                static bool pte_less_prob(pt_entry &a, pt_entry &b);

        protected:
                Lexicon *Lex;
                GPTrans *GPT;
                int mode;

                void RemoveEqualEntries(std::list<pt_entry> *list);
                
        public:
                PhnTrans();
                ~PhnTrans();
                void SetLexicon(Lexicon *lex) {Lex = lex;};
                void SetGPTrans(GPTrans *gpt) {GPT = gpt;};
                int GetTranscs(char *word, std::list<pt_entry> *list);
                void SetMode(int m) {mode = m;};
};

#endif
