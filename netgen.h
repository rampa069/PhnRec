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

#ifndef _NETGEN_H
#define _NETGEN_H

// Error codes
#define NGEN_OK          0
#define NGEN_INPUTFILE   1
#define NGEN_OUTPUTFILE  2

// Load phoneme list and generate HMM definition file for STK
int PhnList2HMMDef(char *phnList, char *hmmDefs, int nStates);

// Load phoneme list and generate STK network file with phoneme loop
int PhnList2PhnLoop(char *phnList, char *phnLoop, char *omitPhn = 0);

/* Example of phoneme loop network file - phonemes "a b c"
NumberOfNodes=9 NumberOfArcs=13
0       N              -                                3       3 5 7
1       N              -                                0
2       N              -                                4       3 5 7 1
3       M              c                                1       4
4       W              c                                1       2
5       M              a                                1       6
6       W              a                                1       2
7       M              b                                1       8
8       W              b                                1       2
*/

#endif
