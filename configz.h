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

#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdlib.h>   // size_t for search.h

#include <set>
#include <string>	

// variable types
#define CE_STRING   0
#define CE_BOOL     1
#define CE_INT      2
#define CE_FLOAT    3

// CheckEntry and Load return codes
#define EN_OK       0
#define EN_UNKVAR   1
#define EN_BADVAL   2
#define EN_FILEERR  3
#define EN_INVVAR   4

typedef struct
{
        char *section;
        char *variable;
        int type;
        char *def_val;
} config_variable;

typedef struct
{
        std::string section;
        std::string variable;
        std::string value;
} config_entry;

struct ltce
{
        bool operator()(config_entry *a, config_entry *b) const
        {
                if(!a || !b)
                {
                        if(!b) return true;
                        else   return false;
                }
                else if(a->section < b->section)
                        return true;
                else if(a->section > b->section)
                        return false;
                else if(a->variable < b->variable)
                        return true;
                return false;
        }
};

class Config
{
        protected:
                config_variable *variables;
                bool checkUnknowVariables;
                std::set<config_entry *, ltce> entries;

                int CheckEntry(char *section, char *variable, char *value);
                config_variable *GetVariable(char *section, char *variable);
				void RemoveEntries();
				void AddDefaultValues();

                //char lastFile[1024];

        public:
                Config();
                ~Config();
                
                void Clear() {RemoveEntries(); AddDefaultValues();};

                void SetVariableTable(config_variable *table) {variables = table; AddDefaultValues();};
                void SetCheckUnknownVariables(bool v) {checkUnknowVariables = v;};
                int Load(char *file, int *errLine = 0);
                int Save(char *file = 0);

                void SetString(char *section, char *variable, char *value);
                void SetBool(char *section, char *variable, bool value);
                void SetInt(char *section, char *variable, int value);
                void SetFloat(char *section, char *variable, float value);

                char *GetString(char *section, char *variable);
                bool GetBool(char *section, char *variable);
                int GetInt(char *section, char *variable);
                float GetFloat(char *section, char *variable);
};

#endif
