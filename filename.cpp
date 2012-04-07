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
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include "filename.h"

#if defined WIN32 && !defined __CYGWIN__
        #include <io.h>
	#include <share.h>
        #include <windows.h>
#else
        #include <sys/types.h>
        #include <sys/stat.h>
#endif

void ChangeFileSuffix(char *fileName, char *newSuffix)
{
	char *dot_pos = strrchr(fileName, '.');
	char *sep_pos1 = strrchr(fileName, '/');
	char *sep_pos2 = strrchr(fileName, '\\');
	char *sep_pos = (sep_pos1 > sep_pos2 ? sep_pos1 : sep_pos2);

	if(!dot_pos || (sep_pos && sep_pos > dot_pos))  // dot is not found or dot is in path 
	{
		strcat(fileName, ".");
		strcat(fileName, newSuffix);
	}
	else
	{
		strcpy(dot_pos + 1, newSuffix);
	}
}

void ExtractFileName(char *fileName)
{
	char buff[1024];
	strcpy(buff, fileName); 
	char *sep_pos1 = strrchr(buff, '/');
	char *sep_pos2 = strrchr(buff, '\\');
	char *sep_pos = (sep_pos1 > sep_pos2 ? sep_pos1 : sep_pos2);
	if(!sep_pos)
		return;
	strcpy(fileName, sep_pos + 1);
}

void CutOffFileSuffix(char *fileName)
{
	char *sep_pos1 = strrchr(fileName, '/');
	char *sep_pos2 = strrchr(fileName, '\\');
	char *dot_pos = strrchr(fileName, '.');
	char *sep_pos = (sep_pos1 > sep_pos2 ? sep_pos1 : sep_pos2);
	if(dot_pos && (!sep_pos || dot_pos > sep_pos))
		*dot_pos = '\0';
}

void GetFileSuffix(char *fileName, char *retSuffix)
{
	char *sep_pos1 = strrchr(fileName, '/');
	char *sep_pos2 = strrchr(fileName, '\\');
	char *dot_pos = strrchr(fileName, '.');
	char *sep_pos = (sep_pos1 > sep_pos2 ? sep_pos1 : sep_pos2);
	
	*retSuffix = '\0';
	if(dot_pos && (!sep_pos || dot_pos > sep_pos))
		strcpy(retSuffix, dot_pos + 1);
}

void ExtractBaseFileName(char *fileName)
{
	ExtractFileName(fileName); 
	CutOffFileSuffix(fileName);
}

// replace dir separators '/\' with the one used in system
void CorrectDirSeparator(char *fileName, const char *dSep)
{
	assert(strlen(dSep) == 1);
	int i;
	for(i = 0; i < (int)strlen(fileName); i++)
	{
		if(fileName[i] == '\\' || fileName[i] == '/')
			fileName[i] = dSep[0];
	}
}

void ChangeFilePath(char *fileName, char *newPath)
{
	char buff[1024];
	strcpy(buff, fileName);
	
	char *sep_pos1 = strrchr(buff, '/');
	char *sep_pos2 = strrchr(buff, '\\');
	char *sep_pos = (sep_pos1 > sep_pos2 ? sep_pos1 : sep_pos2);

	if(!sep_pos)
		return;

	strcpy(fileName, newPath);
	strcat(fileName, sep_pos);
}

void GetFilePath(char *fileName, char *retPath)
{
	strcpy(retPath, fileName);

	char *sep_pos1 = strrchr(retPath, '/');
	char *sep_pos2 = strrchr(retPath, '\\');
	char *sep_pos = (sep_pos1 > sep_pos2 ? sep_pos1 : sep_pos2);

	if(sep_pos)
		*sep_pos = '\0';
	else
		retPath[0] = '\0';
}

bool FileExistence(char *fileName)
{
        int fid = open(fileName, O_RDONLY);
        if(fid >= 0)
        {
                close(fid);
                return true;
        }
        if(errno == EACCES)
                return true;
        return false;
}

bool FileWritePerm(char *file)
{
	FILE *fp;
	fp = fopen(file, "ab");
	if(fp)
	{
		fclose(fp);
		return true;
	}
	return false;
}

bool FileExclPerm(char *fileName)
{
        if(!FileExistence(fileName))
                return false;

        #ifdef WIN32  
                int fid;
                fid = sopen(fileName, O_RDWR, SH_DENYRW);
                if (fid < 0)
                        return false;

                close(fid);
                return true;
        #else
                #warning "Function 'FileExclPerm' is not implemented under Linux"
		return false;
        #endif
}

long FileLen(FILE *fp)
{
        int f = fileno(fp);
        long oldpos = ftell(fp);
        long size = lseek(f, 0, SEEK_END);
        lseek(f, oldpos, SEEK_SET);
        return size;
}


bool FileCopy(char *from, char *to, bool overwrite)
{
        #if defined WIN32 && !defined __CYGWIN__
                return CopyFile(from, to, overwrite);
        #else
                #warning "Function 'FileCopy' is not implemented under Linux"
		return false;
        #endif
}

bool FileMove(char *from, char *to)
{
        #if defined WIN32 && !defined __CYGWIN__
                return MoveFile(from, to);
        #else
                #warning "Function 'FileMove' is not implemented under Linux"
		return false;
        #endif
}

bool SubstFileDir(char *fileName, char *prevDir, char *newDir)
{
        int len = strlen(prevDir);
        if(strncmp(fileName, prevDir, len) == 0)
        {
                char buff[1024];
                strcpy(buff, fileName);        
                strcpy(fileName, newDir);
                strcat(fileName, buff + len);
                return true;
        }
        return false;
}

void CreateChannelFileName(char *file, char *channelSuffix)
{
        char suffix[1024];
        GetFileSuffix(file, suffix);
        CutOffFileSuffix(file);
        strcat(file, channelSuffix);
        strcat(file, ".");
        strcat(file, suffix);
}

