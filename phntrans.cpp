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

#include "phntrans.h"


PhnTrans::PhnTrans() :
                Lex(0),
                GPT(0),
                mode(PHNTM_LEXGPT)
{
}

PhnTrans::~PhnTrans()
{
}

int PhnTrans::GetTranscs(char *word, std::list<pt_entry> *list)
{
        list->clear();
        pt_entry e;

        if(mode == PHNTM_LEXICON || mode == PHNTM_UNION || mode == PHNTM_LEXGPT)
        {
                assert(Lex);

                std::list<Lexicon::trans_entry> lex_list;
                std::list<Lexicon::trans_entry>::iterator it;

                Lex->GetTranscs(word, &lex_list);

                for(it = lex_list.begin(); it != lex_list.end(); it++)
                {
                        e.trans = it->trans;
                        e.prob = it->prob;
                        e.source = PHNTS_LEXICON;
                        list->push_back(e);
                }

        }

        if(GPT->GetInitialized() && (mode == PHNTM_GPT || mode == PHNTM_UNION || (mode == PHNTM_LEXGPT && list->size() == 0)))
        {
                assert(GPT);

                std::list<GPTrans::trans_entry> gtp_list;
                std::list<GPTrans::trans_entry>::iterator it;

                int ret = GPT->Generate(word, &gtp_list);
                if(ret != GPT_OK)
                        return  PHNTE_UNKGRAPHEME;

                for(it = gtp_list.begin(); it != gtp_list.end(); it++)
                {
                        e.trans = it->trans;
                        e.prob = it->prob;
                        e.source = PHNTS_GPT;
                        list->push_back(e);
                }
        }

        list->sort(pte_less_trans);
        RemoveEqualEntries(list);

        list->sort(pte_less_prob);

        return PHNTE_OK;
}

bool PhnTrans::pte_less_trans(pt_entry &a, pt_entry &b)
{
        if(a.trans < b.trans)
                return true;
        else if(a.trans > b.trans)
                return false;
        else if(a.source < b.source)
                return true;
        else if(a.source > b.source)
                return false;
        return a.prob > b.prob;
}

bool PhnTrans::pte_less_prob(pt_entry &a, pt_entry &b)
{
        if(a.prob > b.prob)
                return true;
        else if(a.prob < b.prob)
                return false;
        else if(a.trans < b.trans)
                return true;
        else if(a.trans > b.trans)
                return false;
        return a.source < b.source;
}

void PhnTrans::RemoveEqualEntries(std::list<pt_entry> *list)
{
        std::list<pt_entry> tmp(*list);
        list->clear();

        std::list<pt_entry>::iterator it;
        std::string prevTr = "";
        for(it = tmp.begin(); it != tmp.end(); it++)
        {
                if(it->trans != prevTr)
                {
                        pt_entry e;
                        e.trans = it->trans;
                        e.prob = it->prob;
                        e.source = it->source;
                        list->push_back(e);
                }
        }
}


