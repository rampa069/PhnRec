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

#ifndef CONFIG_H
#define CONFIG_H

// MEL BANKS
#define MB_SAMLEFREQ               8000
#define MB_INPUTSIZE               1000  // = 125 ms (1 ms = 8, 25 ms = 25 * 8 = 200)
#define MB_VECTORSIZE               200  // = 25 ms
#define MB_FFTVECTORSIZE            256
#define MB_FFTVECTORSIZE_2          128
#define MB_STEP                      80  // = 10 ms
#define MB_PREEMPHASIS                0
#define MB_MELBANKS_DEF              23
#define MB_LOFREQ                    64
#define MB_HIFREQ                  4000

// TRAPS + LCRC
#define BUNCH_SIZE                    5
#define BANDS_WEIGHTS_FILE_PREFIX     "/weights/band"
#define BANDS_WEIGHTS_FILE_SUFFIX     ".weights"
#define BANDS_NORMS_FILE_PREFIX       "/norms/band"
#define BANDS_NORMS_FILE_SUFFIX       ".norms"
#define BANDS_WINDOW_FILE_PREFIX      "/windows/band"
#define BANDS_WINDOW_FILE_SUFFIX      ".window"
#define BANDS_NBANDS                 21
#define MERGER_WEIGHTS_FILE           "/weights/merger.weights"
#define MERGER_NORMS_FILE             "/norms/merger.norms"

// LCRC
#define BANDS_WINDOW_FILE

// DECODER
#define DEC_CONTEXT                 23
#define DEC_WIP                     -3.7890625
#define DEC_REMOVE                  39

// MAIN
#define NPHONEMES                   40
#define TRAP_LEN                    31
#define RECBUFF_LEN                 1000

#endif
