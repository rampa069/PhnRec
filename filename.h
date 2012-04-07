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

#ifndef _FILENAME_H
#define _FILENAME_H

#include <stdio.h>

#if defined WIN32 && !defined __CYGWIN__
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
#endif

// /dir/file.doc -> /dir/file.txt
void ChangeFileSuffix(char *fileName, char *newSuffix);

// /dir/file.txt -> /new_dir/file.txt
void ChangeFilePath(char *fileName, char *newPath);

// /dir/file.txt -> file.txt
void ExtractFileName(char *fileName);

// /dir/file.txt -> file
void ExtractBaseFileName(char *fileName);

// /dir/file.txt -> /dir/file
void CutOffFileSuffix(char *fileName);

// /dir/file.txt -> txt
void GetFileSuffix(char *fileName, char *retSuffix);

// /dir/file.txt -> /dir
void GetFilePath(char *fileName, char *retPath);

// *dSep = '\\' => /dir/file.txt -> \\dir\\file.txt
void CorrectDirSeparator(char *fileName, const char *dSep);

// check whether the file exists
bool FileExistence(char *fileName);

// check whether the file has write permition
bool FileWritePerm(char *file);

// check whether the file exists and can be opened with exclusive permission
bool FileExclPerm(char *fileName);

// returns file length
long FileLen(FILE *fp);

// copy file
bool FileCopy(char *from, char *to, bool overwrite = true);

// move file
bool FileMove(char *from, char *to);

// prevDir = /dir
// newDir = /dirx
// /dir1/dir2/file.txt -> /dirx/dir2/file.txt
bool SubstFileDir(char *fileName, char *prevDir, char *newDir);

// channelSuffix = _L
// /dir/file.txt  -> /dir/file_L.txt
void CreateChannelFileName(char *file, char *channelSuffix);

#endif
