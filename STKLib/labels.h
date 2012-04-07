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

#ifndef LABELS_H
#define LABELS_H

#include "common.h"
#include <cstdio>
#include <ctype.h>

//typedef struct _LabelFormat LabelFormat;
class LabelFormat 
{
public:
  unsigned  SCORE_NRM:1;
  unsigned  SCORE_OFF:1;
  unsigned  CENTRE_TM:1;
  unsigned  TIMES_OFF:1;
  unsigned  WORDS_OFF:1;
  unsigned  MODEL_OFF:1;
  unsigned  STRIP_TRI:1;
  unsigned  FRAME_SCR:1;
  long long left_extent;
  long long right_extent;
};

//typedef struct _Label Label;
class Label 
{
public:
  Label *     mpNext;
  long long   mStart;
  long long   mStop;
  char  *     mpName;
  void  *     mpData; // Set by ReadInputLabelFile to 'data' member of label_hash item
  int         mId;
  FLOAT       mScore;
  Label *     mpNextLevel;
};

extern const Label init_label;
extern const char *transc_filter;
extern const char *transc_ofilter;

//typedef struct _Transcription Transcription;
//struct _Transcription {
//  Transcription *next;
//  char *labelFileName;
//  Label *labels;
//  long  weight;
//};


FILE *OpenOutputMLF(const char *out_MLF);

FILE *OpenOutputLabelFile(
  char       *file_name,
  const char *out_lbl_dir,
  const char *out_lbl_ext,
  FILE       *MLFfp,
  const char *out_MLF);

void WriteLabels(
  FILE        *flp,
  Label       *labels,
  LabelFormat labelFormat,
  long        sampPeriod,
  const char  *label_file,
  const char  *out_MLF);

void CloseOutputLabelFile(FILE *lfp, const char *out_MLF);

MyHSearchData readLabelList(const char *labelListFileName);

void ReleaseLabels(Label *labels);

int IsMLF(FILE *MLFfp);
FILE *OpenInputMLF(const char *in_MLF);

enum UnknownLabelsAction {
  UL_ERROR,
  UL_WARN,
  UL_IGNORE,
  UL_READ,
  UL_INSERT
};

struct LabelStats {
  int nLabelsRead;
  int nLabelsTotal;
  long long endTime;
};

Label *ReadLabels(
  FILE *lfp,
  MyHSearchData *label_hash,
  enum UnknownLabelsAction unknownLabels,
  LabelFormat labelFormat,
  long sampPeriod,
  const char *file_name,
  const char *in_MLF,
  struct LabelStats *stats);

FILE *OpenInputLabelFile(
  char *file_name,
  const char *out_lbl_dir,
  const char *out_lbl_ext,
  FILE *MLFfp, const char *out_MLF);


void CloseInputMLF(FILE *lfp);
void CloseInputLabelFile(FILE *lfp, const char *out_MLF);


void AlingTranscriptions(Label **aligned_hyps, Label *new_hyp, long *weights);


extern int   substitution_cost;
extern int   insertion_cost;
extern int   deletion_cost;
extern float null_cost;
extern int   time_med_alg;

#endif // LABELS_H
