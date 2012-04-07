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
#include <assert.h>
#include <string.h>
#include "configz.h"

Config::Config() :
        variables(0),
        checkUnknowVariables(false)
{
        //strcpy(lastFile, "");
}

Config::~Config()
{
	RemoveEntries();
}

void Config::RemoveEntries()
{
	std::set<config_entry *, ltce>::iterator it = entries.begin();
	while(it != entries.end())
	{
		config_entry *entry = *it;
		entries.erase(it);
		delete entry;
		it = entries.begin();	
	}
}


void Config::AddDefaultValues()
{
        assert(variables);
        config_variable *var = variables;
        while(var->section)
        {
                SetString(var->section, var->variable, var->def_val);
                var++;
        }
}


config_variable *Config::GetVariable(char *section, char *variable)
{
        assert(variables);
        config_variable *var = variables;
        while(var->section)
        {
                if(strcmp(section, var->section) == 0 && strcmp(variable, var->variable) == 0)
                        return var;
                var++;
        }
        return 0;
}

int Config::CheckEntry(char *section, char *variable, char *value)
{
        config_variable *var = GetVariable(section, variable);
        int ival;
        float fval;
        
        if(!var)
                return EN_UNKVAR;

        assert(var->type == CE_STRING || var->type == CE_INT ||
               var->type == CE_FLOAT || var->type == CE_BOOL);

        switch(var->type)
        {
                case CE_INT:
                        if(sscanf(value, "%d", &ival) == 1)
                                return EN_OK;
                        else
                                return EN_BADVAL;
                case CE_FLOAT:
                        if(sscanf(value, "%f", &fval) == 1)
                                return EN_OK;
                        else
                                return EN_BADVAL;
                case CE_BOOL:
                        if(strcmp(value, "true") == 0 || strcmp(value, "false") == 0)
                                return EN_OK;
                        else
                                return EN_BADVAL;
        }
        return EN_OK;
}

int Config::Load(char *file, int *errLine)
{
        Clear();

        FILE *fp;
        fp = fopen(file, "rb");
        if(!fp)
                return EN_FILEERR;

        //strcpy(lastFile, file);

        char buff[1024];
        char section[1024];
        int line = 1;
        while(fgets(buff, 1023, fp))
        {
                int i;
                for(i = 0; i < (int)strlen(buff); i++)
                {
                        if(buff[i] == '\n' || buff[i] == '\r')
                        {
                                buff[i] = '\0';
                                break;
                        }
                }

                if(errLine)
                        *errLine = line;

                if(strlen(buff) > 1 && buff[0] == '[')        // at least []
                {
                        strcpy(section, buff + 1);
                        section[strlen(section) - 1] = '\0';  // cut off ]
                }
                else if(strlen(buff) > 0 && buff[0] == '#')   // comment     
                {
                }
                else if(strlen(buff) == 0)                    // empty line
                {
                }
                else
                {
                        char *var = strtok(buff, "=");
                        char *value = strtok(0, "#");
                        if(!var || !value)
                        {
                                fclose(fp);
                                return EN_INVVAR;
                        }

                        int ret = CheckEntry(section, var, value);
                        if((ret != EN_OK && ret != EN_UNKVAR) || (checkUnknowVariables && ret == EN_UNKVAR))
                        {
                                fclose(fp);
                                return ret;
                        }
						
						SetString(section, var, value);
                }
                line++;
        }

        fclose(fp);
        return EN_OK;
}

int Config::Save(char *file)
{
        //if(!file)
        //        file = lastFile;

        FILE *fp;
        fp = fopen(file, "w");
        if(!fp)
                return EN_FILEERR;

        char section[256];
        strcpy(section, "");
        std::set<config_entry *, ltce>::iterator it;
        for(it = entries.begin(); it != entries.end(); it++)
        {
                config_entry *entry = *it;
                if(strcmp(entry->section.c_str(), section) != 0)
                {
                        if(strlen(section) != 0)
                                fprintf(fp, "\n");
                        strcpy(section, entry->section.c_str());
                        fprintf(fp, "[%s]\n", section);
                }
                fprintf(fp, "%s=%s\n", entry->variable.c_str(), entry->value.c_str());
        }

        fclose(fp);
        return EN_OK;
}

char *Config::GetString(char *section, char *variable)
{
        config_entry *refEntry = new config_entry;
        refEntry->section = section;
        refEntry->variable = variable;
        std::set<config_entry *, ltce>::iterator it = entries.find(refEntry);
        assert(it != entries.end());
        delete refEntry;
        return (char *)(*it)->value.c_str();
}

bool Config::GetBool(char *section, char *variable)
{
        char *str = GetString(section, variable);
        if(strcmp(str, "true") == 0)
                return true;
        return false;
}

int Config::GetInt(char *section, char *variable)
{
        char *str = GetString(section, variable);
        int val;
        sscanf(str, "%d", &val);
        return val;
}

float Config::GetFloat(char *section, char *variable)
{
        char *str = GetString(section, variable);
        float val;
        sscanf(str, "%f", &val);
        return val;
}

void Config::SetString(char *section, char *variable, char *value)
{
        config_entry *refEntry = new config_entry;
        refEntry->section = section;
        refEntry->variable = variable;
        std::set<config_entry *, ltce>::iterator it = entries.find(refEntry);
        delete refEntry;

        if(it == entries.end())
        {
                config_entry *entry = new config_entry;
                entry->section = section;
                entry->variable = variable;
                entry->value = value;
                entries.insert(entry);
        }
        else
        {
                (*it)->value = value;
        }
}

void Config::SetBool(char *section, char *variable, bool value)
{
        if(value)
                SetString(section, variable, "true");
        else
                SetString(section, variable, "false");
}

void Config::SetInt(char *section, char *variable, int value)
{
        char buff[256];
        sprintf(buff, "%d", value);
        SetString(section, variable, buff);
}

void Config::SetFloat(char *section, char *variable, float value)
{
        char buff[256];
        sprintf(buff, "%f", value);
        SetString(section, variable, buff);
}







