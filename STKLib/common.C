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

#include "common.h"
#include "filmatch.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdexcept>
#include <iostream>

#include <string>


static union
{
        double d;
        struct
        {
                int j,i;
        } n;
} qn_d2i;

#define QN_EXP_A (1048576/M_LN2)
#define QN_EXP_C 60801
//#define EXP2(y) (qn_d2i.n.j = (int) (QN_EXP_A*(y)) + (1072693248 - QN_EXP_C), qn_d2i.d)
#define FAST_EXP(y) (qn_d2i.n.i = (int) (QN_EXP_A*(y)) + (1072693248 - QN_EXP_C), qn_d2i.d)

void fast_sigmoid_vec(FLOAT *in, FLOAT *out, int size)
{
  while(size--) *out++ = 1.0/(1.0 + FAST_EXP(-*in++));
}

FLOAT i_max_double (FLOAT *a, int len) {
	int i;
	FLOAT max;
	max = a[0];
	for (i=1; i<len; i++) {
		if (a[i] > max) {
			max = a[i];
		}
	}
	return max;
}

void fast_softmax_vec(FLOAT *in, FLOAT *out, int size)
{
	int i;
	FLOAT maxa,sum;
	// first find the max
	maxa = i_max_double (in, size);
	// normalize, exp and get the sum
	sum = 0.0;
	for (i=0; i<size; i++) {
		out[i] = FAST_EXP(in[i] - maxa);
		sum += out[i];
	}
	// now normalize bu the sum
	for (i=0; i<size; i++) {
		out[i] /= sum;
	}
}


//namespace STK
//{
  
  const char *    gpFilterWldcrd;
  // :WARNING: default HTK compatibility is set to false
  bool            gHtkCompatible = false;
  FLOAT           gMinLogDiff;
  
  const char *    gpScriptFilter;
  const char *    gpParmFilter;
  const char *    gpMmfFilter;
  const char *    gpHListOFilter;
  const char *    gpMmfOFilter;
  const char *    gpParmOFilter;

    
  static char *   gpParmKindNames[] = 
  {
    "WAVEFORM",
    "LPC",
    "LPREFC",
    "LPCEPSTRA",
    "LPDELCEP",
    "IREFC",
    "MFCC",
    "FBANK",
    "MELSPEC",
    "USER",
    "DISCRETE",
    "PLP",
    "ANON"
  };
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  ReadParmKind(const char *str, bool checkBrackets)
  {
    unsigned int  i;
    int           parmKind =0;
    int           slen     = strlen(str);
  
    if (checkBrackets) 
    {
      if (str[0] != '<' || str[slen-1] != '>')  return -1;
      str++; slen -= 2;
    }
    
    for (; slen >= 0 && str[slen-2] == '_'; slen -= 2) 
    {
      parmKind |= str[slen-1] == 'E' ? PARAMKIND_E :
                  str[slen-1] == 'N' ? PARAMKIND_N :
                  str[slen-1] == 'D' ? PARAMKIND_D :
                  str[slen-1] == 'A' ? PARAMKIND_A :
                  str[slen-1] == 'C' ? PARAMKIND_C :
                  str[slen-1] == 'Z' ? PARAMKIND_Z :
                  str[slen-1] == 'K' ? PARAMKIND_K :
                  str[slen-1] == '0' ? PARAMKIND_0 :
                  str[slen-1] == 'V' ? PARAMKIND_V :
                  str[slen-1] == 'T' ? PARAMKIND_T : -1;
  
      if (parmKind == -1) return -1;
    }
    
    for (i = 0; i < sizeof(gpParmKindNames) / sizeof(char*); i++) 
    {
      if (!strncmp(str, gpParmKindNames[i], slen))
        return parmKind | i;
    }
    return -1;
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  MakeFileName(char* pOutFileName,  const char* inFileName,
               const char* out_dir, const char* out_ext)
  {
    const char* base_name;
    const char* bname_end = NULL;
    const char* chrptr;
  
    //  if (*inFileName == '*' && *++inFileName == '/') ++inFileName;
  
    base_name = strrchr(inFileName, '/');
    base_name = base_name != NULL ? base_name + 1 : inFileName;
    
    if (out_ext) bname_end = strrchr(base_name, '.');
    if (!bname_end) bname_end = base_name + strlen(base_name);
  
  
    if ((chrptr = strstr(inFileName, "/./")) != NULL) 
    {
      // what is in path after /./ serve as base name
      base_name = chrptr + 3;
    }
    /* else if (*inFileName != '/') 
    {
      // if inFileName isn't absolut path, don't forget directory structure
      base_name = inFileName;
    }*/
  
    *pOutFileName = '\0';
    if (out_dir) 
    {
      if (*out_dir) 
      {
        strcat(pOutFileName, out_dir);
        strcat(pOutFileName, "/");
      }
      strncat(pOutFileName, base_name, bname_end-base_name);
    } 
    else 
    {
      strncat(pOutFileName, inFileName, bname_end-inFileName);
    }
  
    if (out_ext && *out_ext) 
    {
      strcat(pOutFileName, ".");
      strcat(pOutFileName, out_ext);
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  char *
  strtoupper(char *str)
  {
    char *chptr;
    for (chptr = str; *chptr; chptr++) {
      *chptr = toupper(*chptr);
    }
    return str;
  }
  
  //***************************************************************************
  //***************************************************************************
  int 
  qsstrcmp(const void *a, const void *b)
  {
    return strcmp(*(char **)a, *(char **)b);
  }
  
  //***************************************************************************
  //***************************************************************************
  int 
  qsptrcmp(const void *a, const void *b)
  {
    return *(char **)a - *(char **)b;
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  InitLogMath()
  {
    gMinLogDiff = log(DBL_EPSILON);
  }
  
  //***************************************************************************
  //***************************************************************************
  double 
  LogAdd(double x, double y)
  {
    double diff;
  
    if (x < y) {
        diff = x - y;
        x = y;
    } else {
      diff = y - x;
    }
  
    if (x < LOG_MIN) {
      return LOG_0;
    }
  
    if (diff < gMinLogDiff) {
        return  x;
    }
  
    return x + log(1.0 + exp(diff));
  }
  
  //***************************************************************************
  //***************************************************************************
  double 
  LogSub(double x, double y)
  {
    FLOAT diff = y - x;
    assert(diff <= 0.0);
  
    if (x < LOG_MIN || diff > -EPSILON) { //'diff > -EPSILON' means 'diff == 0.0'
      return LOG_0;
    }
  
    if (diff < gMinLogDiff) {
        return  x;
    }
  
    return x + log(1.0 - exp(diff));
  }
  
  //***************************************************************************
  //***************************************************************************
  FloatInLog 
  FIL_Add(FloatInLog a, FloatInLog b)
  {
    FloatInLog ret;
    if (a.negative == b.negative) {
      ret.negative = a.negative;
      ret.logvalue = LogAdd(a.logvalue, b.logvalue);
    } else if (a.logvalue > b.logvalue) {
      ret.negative = a.negative;
      ret.logvalue = LogSub(a.logvalue, b.logvalue);
    } else {
      ret.negative = b.negative;
      ret.logvalue = LogSub(b.logvalue, a.logvalue);
    }
    return ret;
  }
  
  //***************************************************************************
  //***************************************************************************
  FloatInLog 
  FIL_Sub(FloatInLog a, FloatInLog b)
  {
    b.negative = ~b.negative;
    return FIL_Add(a, b);
  }
  
  //***************************************************************************
  //***************************************************************************
  FloatInLog 
  FIL_Mul(FloatInLog a, FloatInLog b)
  {
    FloatInLog ret;
    ret.negative = a.negative ^ b.negative;
    ret.logvalue = a.logvalue + b.logvalue;
    return ret;
  }
  
  //***************************************************************************
  //***************************************************************************
  FloatInLog 
  FIL_Div(FloatInLog a, FloatInLog b)
  {
    b.logvalue = -b.logvalue;
    return FIL_Mul(a, b);
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  sigmoid_vec(FLOAT * pIn, FLOAT * pOut, int size)
  {
    while (size--) *pOut++ = 1.0/(1.0 + _EXP(-*pIn++));
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  exp_vec(FLOAT *pIn, FLOAT *pOut, int size)
  {
    while (size--) *pOut++ = _EXP(*pIn++);
  }
  
  //***************************************************************************
  //***************************************************************************
  void 
  log_vec(FLOAT *pIn, FLOAT *pOut, int size)
  {
    while (size--) *pOut++ = _LOG(*pIn++);
  }
  
  //***************************************************************************
  //***************************************************************************
  void sqrt_vec(FLOAT *pIn, FLOAT *pOut, int size)
  {
    while (size--) *pOut++ = _SQRT(*pIn++);
  }
  
  //***************************************************************************
  //***************************************************************************
  void softmax_vec(FLOAT *pIn, FLOAT *pOut, int size)
  {
    int i;
    FLOAT sum = 0.0;
    for (i = 0; i < size; i++) sum += pOut[i] = _EXP(pIn[i]);
    sum = 1.0 / sum;
    for (i = 0; i < size; i++) pOut[i] *= sum;
  }
  
  //***************************************************************************
  //***************************************************************************
  int ParmKind2Str(unsigned parmKind, char * pOutString) 
  {
    // :KLUDGE: Absolutely no idea what this is...
      if ((parmKind & 0x003F) >= sizeof(gpParmKindNames)/sizeof(gpParmKindNames[0])) 
      return 0;
  
    strcpy(pOutString, gpParmKindNames[parmKind & 0x003F]);
  
    if (parmKind & PARAMKIND_E) strcat(pOutString, "_E");
    if (parmKind & PARAMKIND_N) strcat(pOutString, "_N");
    if (parmKind & PARAMKIND_D) strcat(pOutString, "_D");
    if (parmKind & PARAMKIND_A) strcat(pOutString, "_A");
    if (parmKind & PARAMKIND_C) strcat(pOutString, "_C");
    if (parmKind & PARAMKIND_Z) strcat(pOutString, "_Z");
    if (parmKind & PARAMKIND_K) strcat(pOutString, "_K");
    if (parmKind & PARAMKIND_0) strcat(pOutString, "_0");
    if (parmKind & PARAMKIND_V) strcat(pOutString, "_V");
    if (parmKind & PARAMKIND_T) strcat(pOutString, "_T");
    
    return 1;
  }
  
  /*
  #define SQR(x) ((x) * (x))
  #define CUB(x) ((x) * (x) * (x))
  #define M_SQRT3 1.7320508075688771931766041234368
  
  int cubic_roots(FLOAT  a,  FLOAT  b, FLOAT  c,
                  FLOAT *r,  FLOAT *s, FLOAT *t)
  {
    FLOAT Q   = (a*a - 3*b) / 9;
    FLOAT R   = (2*a*a*a - 9*a*b + 27*c) / 54;
    FLOAT QQQ = Q*Q*Q;
    FLOAT D   = R*R - QQQ;
  
    if (D < 0.0) {
      FLOAT sqrtQ = sqrt(Q);
      FLOAT fi = acos(R/sqrt(QQQ));
      *r = -2 * sqrtQ * cos( fi             / 3) - a / 3;
      *s = -2 * sqrtQ * cos((fi + 2 * M_PI) / 3) - a / 3;
      *t = -2 * sqrtQ * cos((fi - 2 * M_PI) / 3) - a / 3;
      return 3;
    } else {
      FLOAT sig = R < 0.0 ? -1.0 : 1.0;
      FLOAT A = -sig * pow((abs(R) + sqrt(D)), 1.0 / 3.0);
      FLOAT B = A == 0 ? 0.0 : Q / A;
      *r =        (A+B) - a / 3;
      *s = -1/2 * (A+B) - a / 3;
      *t = M_SQRT3 / 2 * (A-B);
      return *t == 0 ? 2 : 1;
    }
  }
  */
  
  //***************************************************************************
  //***************************************************************************
  bool isBigEndian()
  {
    int a = 1;
    return (bool) ((char *) &a)[0] != 1;
  }
  
  //***************************************************************************
  //***************************************************************************
  int my_hcreate_r(size_t nel, MyHSearchData *tab)
  { 
    memset(&tab->mTab, 0, sizeof(tab->mTab));
    
    if ((tab->mpEntry = (ENTRY **) malloc(sizeof(ENTRY *)*nel)) == NULL) 
      return 0;
    
    if (!hcreate_r(nel, &tab->mTab)) 
    {
      free(tab->mpEntry);
      return 0;
    }
    
    tab->mNEntries = 0;
    tab->mTabSize  = nel;
    
    return 1;
  }
  
  //***************************************************************************
  //***************************************************************************
  int my_hsearch_r(ENTRY item, ACTION action, ENTRY ** ret,
                  MyHSearchData * tab)
  {
    unsigned int i;
    int          cc;
  
    if (action == ENTER && tab->mNEntries == tab->mTabSize) 
    {
      struct hsearch_data newtab;
      ENTRY **epp;
  
      epp = (ENTRY **) realloc(tab->mpEntry, sizeof(ENTRY *) * tab->mTabSize * 2);
      if (epp == NULL) 
      {
        *ret = NULL;
        return 0;
      }
  
      tab->mpEntry = epp;
      memset(&newtab, 0, sizeof(newtab));
      
      if (!hcreate_r(tab->mTabSize * 2, &newtab)) 
      {
        *ret = NULL;
        return 0;
      }
  
      tab->mTabSize *= 2;
  
      for (i = 0; i < tab->mNEntries; i++) 
      {
        cc = hsearch_r(*tab->mpEntry[i], ENTER, ret, &newtab); 
        assert(cc);
        tab->mpEntry[i] = *ret;
      }
  
      hdestroy_r(&tab->mTab);
      tab->mTab = newtab;
    }
  
    cc = hsearch_r(item, action, ret, &tab->mTab);
    assert(action == FIND || cc);
  
    if (action == ENTER && (*ret)->data == item.data)
      tab->mpEntry[tab->mNEntries++] = *ret;
      
    return 1;
  }
  
  //***************************************************************************
  //***************************************************************************
  void my_hdestroy_r(MyHSearchData *tab, int freeKeys)
  {
    unsigned int i;
    if (freeKeys) 
    {
      for (i = 0; i < tab->mNEntries; i++)
      {
        free(tab->mpEntry[i]->key);
      }
    }
    hdestroy_r(&tab->mTab);
    free(tab->mpEntry);
  }
  
  //***************************************************************************
  //***************************************************************************
  int fprintHTKstr(FILE *fp, const char *str)
  {
    return fprintf(fp, str    == NULL ? "!NULL"   :
                      str[0] == '"'  ? "'%s'"   :
                      str[0] == '\'' ? "\"%s\"" :
                                        "%s", str);
  }
  
  //***************************************************************************
  //***************************************************************************
  int getHTKstr(char *str, char **endPtrOrErrMsg)
  {
    char termChar = '\0';
    char *chrptr = str;
  
    while (isspace(*chrptr)) ++chrptr;
  
    if (*chrptr == '\'' || *chrptr == '"') {
      termChar = *chrptr;
      chrptr++;
    }
  
    for (; *chrptr; chrptr++) {
      if (*chrptr == '\'' || *chrptr == '"') {
        if (termChar == *chrptr) {
          termChar = '\0';
          chrptr++;
          break;
        }
      }
  
      if (isspace(*chrptr) && !termChar) {
        break;
      }
  
      if (*chrptr == '\\') {
        ++chrptr;
        if (*chrptr == '\0' || (*chrptr    >= '0' && *chrptr <= '7' &&
                              (*++chrptr  <  '0' || *chrptr >  '7' ||
                              *++chrptr  <  '0' || *chrptr >  '7'))) {
          *endPtrOrErrMsg = "Invalid escape sequence";
          return -1;
        }
  
        if (*chrptr  >= '0' && *chrptr  <= '7') {
          *chrptr = (*chrptr - '0') + (chrptr[-1] - '0') * 8 + (chrptr[-2] - '0') * 64;
        }
      }
      *str++ = *chrptr;
    }
  
    if (termChar) {
      *endPtrOrErrMsg = "Unterminated quoted string";
      return -2;
    }
  
  
    while (isspace(*chrptr)) ++chrptr;
    *endPtrOrErrMsg = chrptr;
    *str = '\0';
  
    return 0;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  char *expandFilterCommand(const char *command, const char *filename)
  {
    char *out, *outend;
    const char *chrptr = command;
    int ndollars = 0;
    int fnlen = strlen(filename);
  
    while (*chrptr++) ndollars += (*chrptr ==  *gpFilterWldcrd);
  
    out = (char*) malloc(strlen(command) - ndollars + ndollars * fnlen + 1);
    if (out == NULL) Error("Insufficient memory");
  
    outend = out;
  
    for (chrptr = command; *chrptr; chrptr++) {
      if (*chrptr ==  *gpFilterWldcrd) {
        strcpy(outend, filename);
        outend += fnlen;
      } else {
        *outend++ = *chrptr;
      }
    }
    *outend = '\0';
    return out;
  }
  
  #define LINEBUFF_INIT_SIZE 1024
  #define LINEBUFF_INCR_SIZE 1024
  //***************************************************************************
  //***************************************************************************
  char *readline(FILE *fp, struct ReadlineData *data)
  {
    char *chrptr;
    int   len;
  
    if (data->size == 0) {
      data->size = LINEBUFF_INIT_SIZE;
      data->buffer = (char *) malloc(data->size);
      if (data->buffer == NULL) Error("Insufficient memory");
    } else if (data->size == -1) { // EOF reached in previous call
      data->size = 0;
      free(data->buffer);
      return NULL;
    }
    chrptr = data->buffer;
    *chrptr = '\0';
  
    for (;;) {
      if (fgets(chrptr, data->size-(chrptr-data->buffer), fp) == NULL) {
        if (chrptr != data->buffer) {
          data->size = -1;     // EOF reached but buffer is not empty; next call return NULL
          return data->buffer;
        } else {               // EOF reached and buffer is empty; return NULL imediately
          data->size = 0;
          free(data->buffer);
          return NULL;
        }
      }
      len = strlen(chrptr);
      assert(len >= 0 && len < data->size-(chrptr-data->buffer));
  
      if (len > 0 && chrptr[len-1] == '\n') {
        chrptr[len-1] = '\0';
        return data->buffer;
      }
      chrptr += len;
      if (chrptr-data->buffer+1 == data->size) {
        data->size += LINEBUFF_INCR_SIZE;
        data->buffer = (char *) realloc(data->buffer, data->size);
        if (data->buffer == NULL) Error("Insufficient memory");
        chrptr = data->buffer+data->size-LINEBUFF_INCR_SIZE-1;
      }
    }
  }
  
  
  static char *HModules[] = {
  "HADAPT","HAUDIO","HFB",   "HLABEL","HMAP", "HMEM",  "HMODEL","HNET",  "HPARM",
  "HREC",  "HSHELL","HTRAIN","HWAVE", "LCMAP","LGBASE","LMODEL","LPCALC","LWMAP"};
  
  //***************************************************************************
  //***************************************************************************
  void InsertConfigParam(
    MyHSearchData *pConfigHash,
    const char *pParamName,
    const char *value,
    int optionChar) {
    ENTRY e, *ep;
    int i;
    char *bmod, *emod;
  
  
    e.key = (char*) malloc(strlen(pParamName)+1);
    for (i=0; *pParamName; pParamName++) {
      if (*pParamName != '-' && *pParamName != '_') {
        e.key[i++] = toupper(*pParamName);
      }
    }
    e.key[i] = '\0';
  
    // Find and remove any known module name from param name
    if ((emod = strrchr(e.key, ':')) != NULL && emod != e.key) {
      *emod = '\0';
      if ((bmod = strrchr(e.key, ':')) == NULL) bmod = e.key;
      else bmod++;
  
      if (bsearch(&bmod, HModules, sizeof(HModules)/sizeof(*HModules),
                sizeof(*HModules), qsstrcmp)) {
        memmove(bmod, emod+1, strlen(emod+1)+1);
      } else {
        *emod = ':';
      }
    }
  
    // Replace initial 'H' in application name by 'S'
    if (strrchr(e.key, ':') && e.key[0] == 'H') {
      e.key[0] = 'S';
    }
  
    my_hsearch_r(e, FIND, &ep, pConfigHash);
  
    if ((e.data = malloc(strlen(value) + 3)) == NULL) Error("Insufficient memory");
    strcpy(((char *) e.data) + 2, value);
  
    //1st byte of the value string is read/unread flag
    ((char *) e.data)[0] = '#';
    ((char *) e.data)[1] = optionChar;
  
  
    if (ep == NULL) {
      if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, pConfigHash)) {
        Error("Insufficient memory");
      }
    } else {
      free(ep->data);
      ep->data = e.data;
      free(e.key);
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  void ReadConfig(const char *file_name, MyHSearchData *pConfigHash)
  {
    char *parptr, *endptr, *chptr, *line;
    struct ReadlineData rld = {0};
    int line_no = 0;
    FILE *fp;
  
    if ((fp = fopen(file_name, "rb")) == NULL) {
      Error("Cannot open input config file %s", file_name);
    }
  
    while ((line = readline(fp, &rld)) != NULL) {
      line_no++;
  
      if ((chptr = strchr(line, '#')) != NULL) *chptr = '\0';
  
      for (parptr = line; isspace(*parptr); parptr++);
  
      if (*parptr == '\0') continue;
  
      chptr = parptr;
      for (;;) {
        // Replace speces by '_', which is removed in InsertConfigParam
        while (isalnum(*chptr) || *chptr == '_' || *chptr == '-') chptr++;
        while (isspace(*chptr)) *chptr++ = '_';
        if (*chptr != ':') break;
        chptr++;
        while (isspace(*chptr)) *chptr++ = '_';
      }
      if (*chptr != '=') Error("Character '=' expected (%s:%d, char %d)",
                              file_name, line_no, chptr-line+1);
      *chptr = '\0';
      chptr++;
      if (getHTKstr(chptr, &endptr)) {
        Error("%s (%s:%d)", endptr, file_name, line_no);
      }
      if (*endptr) Error("Extra characters at the end of line (%s:%d, char %d)",
                        file_name, line_no, endptr-line+1);
  
      InsertConfigParam(pConfigHash, parptr, chptr, 'C');
    }
  
    if (ferror(fp)) {
      Error("Cannot read input config file", file_name);
    }
    fclose(fp);
  }
  
  //***************************************************************************
  //***************************************************************************
  void CheckCommandLineParamUse(MyHSearchData * pConfigHash)
  {
    unsigned int i;
  
    for (i = 0; i < pConfigHash->mNEntries; i++) {
      char *value = (char *) pConfigHash->mpEntry[i]->data;
      if (value[0] != ' ' && value[1] != 'C') {
        assert(strchr(pConfigHash->mpEntry[i]->key, ':'));
        Error("Unexpected command line parameter %s",
              strchr(pConfigHash->mpEntry[i]->key, ':') + 1);
      }
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  void PrintConfig(MyHSearchData * pConfigHash)
  {
    unsigned int i;
    char         *par;
    char         *key;
    char         *val;
  
    printf("\nConfiguration Parameters[%d]\n", (int) pConfigHash->mNEntries);
    for (i = 0; i < pConfigHash->mNEntries; i++) {
      key = (char *) pConfigHash->mpEntry[i]->key;
      val = (char *) pConfigHash->mpEntry[i]->data;
      par = strrchr(key, ':');
      if (par) par++; else par = key;
      printf("%c %-15.*s %-20s = %-30s # -%c\n",
            val[0], (int) (par-key), key, par, val+2, val[1]);
    }
    putchar('\n');
  }
  
  //***************************************************************************
  //***************************************************************************
  ENTRY *GetParam(
    MyHSearchData *pConfigHash,
    const char *pParamName)
  {
    ENTRY e, *ep;
  
    e.key = (char *) (pParamName - 1);
    do {
      e.key++;
      my_hsearch_r(e, FIND, &ep, pConfigHash);
    } while (ep == NULL && (e.key = strchr(e.key, ':')) != NULL);
    if (ep != NULL) *(char *) ep->data = ' '; //Mark param as read
    return ep;
  }
  
  //***************************************************************************
  //***************************************************************************
  const char * 
  GetParamStr(
    MyHSearchData * pConfigHash,
    const char *    pParamName,
    const char *    default_value)
  {
    ENTRY *ep = GetParam(pConfigHash, pParamName);
    return ep != NULL ? 2 + (char *) ep->data : default_value;
  }
  
  //***************************************************************************
  //***************************************************************************
  static char *
  OptOrParStr(ENTRY *ep)
  {
    static char str[128];
    int optionChar = ((char *) ep->data)[1];
    if (optionChar == 'C') {
      sprintf(str, "parameter '%s'", ep->key);
    } else if (optionChar == '-') {
      sprintf(str, "option '--%s'", ep->key);
    } else{
      sprintf(str, "option '-%c'", optionChar);
    }
    return str;
  }
  
  //***************************************************************************
  //***************************************************************************
  long 
  GetParamInt(
    MyHSearchData *pConfigHash,
    const char *pParamName,
    long default_value)
  {
    char *chrptr;
    ENTRY *ep = GetParam(pConfigHash, pParamName);
    if (ep == NULL) return default_value;
  
    const char *val = 2 + (char *) ep->data;
    default_value = strtol(val, &chrptr, 0);
    if (!*val || *chrptr) {
      Error("Integer number expected for %s but found '%s'", OptOrParStr(ep),val);
    }
    return default_value;
  }
  
  //***************************************************************************
  //***************************************************************************
  FLOAT 
  GetParamFlt(
    MyHSearchData *   pConfigHash,
    const char *      pParamName,
    FLOAT             default_value)
  {
    char *chrptr;
    ENTRY *ep = GetParam(pConfigHash, pParamName);
    if (ep == NULL) return default_value;
  
    const char *val = 2 + (char *) ep->data;
    default_value = strtod(val, &chrptr);
    if (!*val || *chrptr) {
      Error("Decimal number expected for %s but found '%s'", OptOrParStr(ep),val);
    }
    return default_value;
  }
  
  //***************************************************************************
  //***************************************************************************
  bool 
  GetParamBool(
    MyHSearchData * pConfigHash,
    const char *    pParamName,
    bool            default_value)
  {
    ENTRY *ep = GetParam(pConfigHash, pParamName);
    if (ep == NULL) return default_value;
  
    const char *val = 2 + (char *) ep->data;
    if (!strcmp(val, "TRUE") || !strcmp(val, "T")) return 1;
    if (strcmp(val, "FALSE") && strcmp(val, "F")) {
      Error("TRUE or FALSE expected for %s but found '%s'", OptOrParStr(ep),val);
    }
    return false;
  }
  
  //***************************************************************************
  //***************************************************************************
  // '...' are pairs: string and corresponding integer value , terminated by NULL
  int 
  GetParamEnum(
    MyHSearchData * pConfigHash,
    const char *    pParamName,
    int             default_value, 
    ...)  
  {
    ENTRY *ep = GetParam(pConfigHash, pParamName);
    if (ep == NULL) return default_value;
  
    const char *val = 2 + (char *) ep->data;
    char *s;
    int i, cnt = 0, l = 0;
    va_list ap;
  
    va_start(ap, default_value);
    while ((s = va_arg(ap, char *)) != NULL) {
      l += strlen(s) + 2;
      ++cnt;
      i = va_arg(ap, int);
      if (!strcmp(val, s)) break;
    }
    va_end(ap);
    if (s) return i;
  
    //To report error, create string listing all possible values
    s = (char*) malloc(l + 1);
    s[0] = '\0';
    va_start(ap, default_value);
    for (i = 0; i < cnt; i++) {
      strcat(s, va_arg(ap, char *));
      va_arg(ap, int);
      if (i < cnt - 2) strcat(s, ", ");
      else if (i == cnt - 2) strcat(s, " or ");
    }
    va_end(ap);
    Error("%s expected for %s but found '%s'", s, OptOrParStr(ep),val);
    return 0;
  }
  
  //***************************************************************************
  //***************************************************************************
  int 
  GetDerivParams(
    MyHSearchData * pConfigHash,
    int *           derivOrder,
    int **          derivWinLens,
    int *           startFrmExt,
    int *           endFrmExt,
    char **         CMNPath,
    char **         CMNFile,
    const char **   CMNMask,
    char **         CVNPath,
    char **         CVNFile,
    const char **   CVNMask,
    const char **   CVGFile,
    const char *    pToolName,
    int             pseudoModeule)
  {
    const char *  str;
    int           targetKind;
    char *        chrptr;
    char          paramName[32];
    const char *  CMNDir;
    const char *  CVNDir;
    
    strcpy(paramName, pToolName);
    strcat(paramName, pseudoModeule == 1 ? "SPARM1:" :
                      pseudoModeule == 2 ? "SPARM2:" : "");
                      
    chrptr = paramName + strlen(paramName);
  
    strcpy(chrptr, "STARTFRMEXT");
    *startFrmExt = GetParamInt(pConfigHash, paramName, 0);
    strcpy(chrptr, "ENDFRMEXT");
    *endFrmExt   = GetParamInt(pConfigHash, paramName, 0);
  
    *CMNPath = *CVNPath = NULL;
    strcpy(chrptr, "CMEANDIR");
    CMNDir       = GetParamStr(pConfigHash, paramName, NULL);
    strcpy(chrptr, "CMEANMASK");
    *CMNMask     = GetParamStr(pConfigHash, paramName, NULL);
    if (*CMNMask != NULL) {
      *CMNPath = (char*) malloc((CMNDir ? strlen(CMNDir) : 0) + npercents(*CMNMask) + 2);
      if (*CMNPath == NULL) Error("Insufficient memory");
      if (CMNDir != NULL) strcat(strcpy(*CMNPath, CMNDir), "/");
      *CMNFile = *CMNPath + strlen(*CMNPath);
    }
    strcpy(chrptr, "VARSCALEDIR");
    CVNDir      = GetParamStr(pConfigHash, paramName, NULL);
    strcpy(chrptr, "VARSCALEMASK");
    *CVNMask     = GetParamStr(pConfigHash, paramName, NULL);
    if (*CVNMask != NULL) {
      *CVNPath = (char*) malloc((CVNDir ? strlen(CVNDir) : 0) + npercents(*CVNMask) + 2);
      if (*CVNPath == NULL) Error("Insufficient memory");
      if (CVNDir != NULL) strcat(strcpy(*CVNPath, CVNDir), "/");
      *CVNFile = *CVNPath + strlen(*CVNPath);
    }
    strcpy(chrptr, "VARSCALEFN");
    *CVGFile     = GetParamStr(pConfigHash, paramName, NULL);
    strcpy(chrptr, "TARGETKIND");
    str = GetParamStr(pConfigHash, paramName, "ANON");
    targetKind = ReadParmKind(str, false);
    if (targetKind == -1) Error("Invalid TARGETKIND = '%s'", str);
  
    strcpy(chrptr, "DERIVWINDOWS");
    if ((str = GetParamStr(pConfigHash, paramName, NULL)) != NULL) {
      long lval;
      *derivOrder      = 0;
      *derivWinLens = NULL;
      
      if (NULL != str)
      {
        while ((str = strtok((char *) str, " \t_")) != NULL) 
        {
          lval = strtol(str, &chrptr, 0);
          if (!*str || *chrptr) {
            Error("Integers separated by '_' expected for parameter DERIVWINDOWS");
          }
          *derivWinLens = (int *)realloc(*derivWinLens, ++*derivOrder*sizeof(int));
          if (*derivWinLens == NULL) Error("Insufficient memory");
          (*derivWinLens)[*derivOrder-1] = lval;
          str = NULL;
        }
      }
      
      return targetKind;
    }
    *derivOrder = targetKind & PARAMKIND_T ? 3 :
                  targetKind & PARAMKIND_A ? 2 :
                  targetKind & PARAMKIND_D ? 1 : 0;
  
    if (*derivOrder || targetKind != PARAMKIND_ANON) {
    *derivWinLens = (int *) malloc(3 * sizeof(int));
      if (*derivWinLens == NULL) Error("Insufficient memory");
  
      strcpy(chrptr, "DELTAWINDOW");
      (*derivWinLens)[0] = GetParamInt(pConfigHash, paramName, 2);
      strcpy(chrptr, "ACCWINDOW");
      (*derivWinLens)[1] = GetParamInt(pConfigHash, paramName, 2);
      strcpy(chrptr, "THIRDWINDOW");
      (*derivWinLens)[2] = GetParamInt(pConfigHash, paramName, 2);
      return targetKind;
    }
    *derivWinLens = NULL;
    *derivOrder   = -1;
    return targetKind;
  }
  
  //***************************************************************************
  //***************************************************************************
  FILE *
  my_fopen(
    const char *  file_name,
    const char *  type,
    const char *  filter)
  {
    FILE *fp;
  
    if (!strcmp(file_name, "-")) {
      fp = *type == 'r' ? stdin : stdout;
    } else if (*file_name == '|') {
      if ((fp = popen(++file_name, *type == 'r' ? "r" : "w")) == NULL) {
        Error("Cannot popen %s filter '%s'",
              *type == 'r' ? "input": "output", file_name);
      }
    } else if (filter) {
      char *f = expandFilterCommand(filter, file_name);
  
      if ((fp = popen(f, *type == 'r' ? "r" : "w")) == NULL) {
        Error("Cannot popen %s filter '%s'",
              *type == 'r' ? "input": "output", f);
      }
      free(f);
    } else if ((fp = fopen(file_name, type)) == NULL) {
      return NULL;
    }
    return fp;
  }
  
  //***************************************************************************
  //***************************************************************************
  int 
  my_fclose(FILE *fp)
  {
    struct stat sb = {0};
    if (fp == stdin || fp == stdout) return 0;
  
    if (fstat(fileno(fp), &sb)) {
//      return EOF;
    }
    if (S_ISFIFO(sb.st_mode)) return pclose(fp);
    else return fclose(fp);
  }
  
  
  //***************************************************************************
  //***************************************************************************
  int 
  ParseOptions(
    int             argc,
    char *          argv[],
    const char *    pOptionMapping,
    const char *    pToolName,
    MyHSearchData * cfgHash)
  {
    int          i;
    int          opt = '?';
    int          optind;
    bool         option_must_follow = false;
    char         param[1024];
    char *       value;
    char *       optfmt;
    const char * optarg;
    char *       chptr;
    char *       bptr;
    char         tstr[4] = " -?";
    unsigned long long option_mask = 0;
  
    #define MARK_OPTION(ch) {if (isalpha(ch)) option_mask |= 1ULL << ((ch) - 'A');}
    #define OPTION_MARK(ch) (isalpha(ch) && ((1ULL << ((ch) - 'A')) & option_mask))
    #define IS_OPTION(str) ((str)[0] == '-' && (isalpha((str)[1]) || (str)[1] == '-'))
  
    for (optind = 1; optind < argc; optind++) {
      if (!strcmp(argv[optind], "--")) break;
      if (argv[optind][0] != '-' || argv[optind][1] != 'A') continue;
      if (argv[optind][2] != '\0') {
        Error("Unexpected argument '%s' after option '-A'", argv[optind] + 2);
      }
      for (i=0; i < argc; i++) printf("%s ", argv[i]);
      putchar('\n');
      break;
    }
    for (optind = 1; optind < argc; optind++) {
      if (!strcmp(argv[optind], "--")) break;
      if (argv[optind][0] != '-' || argv[optind][1] != 'C') continue;
      if (argv[optind][2] != '\0') {
        ReadConfig(argv[optind] + 2, cfgHash);
      } else if (optind+1 < argc && !IS_OPTION(argv[optind+1])) {
        ReadConfig(argv[++optind], cfgHash);
      } else {
        Error("Config file name expected after option '-C'");
      }
    }
    for (optind = 1; optind < argc; optind++) {
      if (!strcmp(argv[optind], "--")) break;
      if (argv[optind][0] != '-' || argv[optind][1] != '-') continue;
      bptr = (char*) malloc(strlen(pToolName) + strlen(argv[optind]+2) + 2);
      if (bptr == NULL) Error("Insufficient memory");
      strcat(strcat(strcpy(bptr, pToolName), ":"), argv[optind]+2);
      value = strchr(bptr, '=');
      if (!value) Error("Character '=' expected after option '%s'", argv[optind]);
      *value++ = '\0';
      InsertConfigParam(cfgHash, bptr, value /*? value : "TRUE"*/, '-');
      free(bptr);
    }
    for (optind = 1; optind < argc && IS_OPTION(argv[optind]); optind++) {
      option_must_follow = false;
      tstr[2] = opt = argv[optind][1];
      optarg = argv[optind][2] != '\0' ? argv[optind] + 2 : NULL;
  
      if (opt == '-' && !optarg) {    // '--' terminates the option list
        return optind+1;
      }
      if (opt == 'C' || opt == '-') { // C, A and long options have been already processed
        if (!optarg) optind++;
        continue;
      }
      if (opt == 'A') continue;
  
      chptr = strstr(pOptionMapping, tstr); //rampa
      if (chptr == NULL) Error("Invalid command line option '-%c'", opt);
  
      chptr += 3;
      while (isspace(*chptr)) chptr++;
  
      if (!chptr || chptr[0] == '-') {// Option without format string will be ignored
        optfmt = " ";
      } else {
        optfmt = chptr;
        while (*chptr && !isspace(*chptr)) chptr++;
        if (!*chptr) Error("Fatal: Unexpected end of optionMap string");
      }
      for (i = 0; !isspace(*optfmt); optfmt++) {
        while (isspace(*chptr)) chptr++;
        value = chptr;
        while (*chptr && !isspace(*chptr)) chptr++;
        assert(static_cast<unsigned int>(chptr-value+1) < sizeof(param));
        strncat(strcat(strcpy(param, pToolName), ":"), value, chptr-value);
        param[chptr-value+strlen(pToolName)+1] = '\0';
        switch (*optfmt) {
          case 'n': value = strchr(param, '=');
                    if (value) *value = '\0';
                    InsertConfigParam(cfgHash, param,
                                      value ? value + 1: "TRUE", opt);
                    break;
          case 'l':
          case 'o':
          case 'r': i++;
                    if (!optarg && (optind+1==argc || IS_OPTION(argv[optind+1]))) {
                      if (*optfmt == 'r' || *optfmt == 'l') {
                        Error("Argument %d of option '-%c' expected", i, opt);
                      }
                      optfmt = "  "; // Stop reading option arguments
                      break;
                    }
                    if (!optarg) optarg = argv[++optind];
                    if (*optfmt == 'o') {
                      option_must_follow = (bool) 1;
                    }
                    bptr = NULL;
  
                    // For repeated use of option with 'l' (list) format, append
                    // ',' and argument string to existing config parameter value.
                    if (*optfmt == 'l' && OPTION_MARK(opt)) {
                      bptr = strdup(GetParamStr(cfgHash, param, ""));
                      if (bptr == NULL) Error("Insufficient memory");
                      bptr = (char*) realloc(bptr, strlen(bptr) + strlen(optarg) + 2);
                      if (bptr == NULL) Error("Insufficient memory");
                      strcat(strcat(bptr, ","), optarg);
                      optarg = bptr;
                    }
                    MARK_OPTION(opt);
                    InsertConfigParam(cfgHash, param, optarg, opt);
                    free(bptr);
                    optarg = NULL;
                    break;
          default : Error("Fatal: Invalid character '%c' in optionMap after %s",
                          *optfmt, tstr);
        }
      }
      if (optarg) Error("Unexpected argument '%s' after option '-%c'",optarg,opt);
    }
  
    for (i = optind; i < argc && !IS_OPTION(argv[i]); i++);
    if (i < argc) {
      Error("No option expected after first non-option argument '%s'", argv[optind]);
    }
    if (option_must_follow) {
      Error("Option '-%c' with optional argument must not be the last option",opt);
    }
    return optind;
  }
  
  //***************************************************************************
  //***************************************************************************
  FileListElem **
  AddFileElem(FileListElem **last, char *fileElem)
  {
    char *chrptr;
    *last = (FileListElem *) malloc(sizeof(FileListElem) + strlen(fileElem));
    if (!*last) Error("Insufficient memory");
    chrptr = strcpy((*last)->logical, fileElem);
    for (; *chrptr; chrptr++) if (*chrptr == '\\') *chrptr = '/';
    chrptr = strchr((*last)->logical, '=');
    if (chrptr) *chrptr = '\0';
    (*last)->mpPhysical = chrptr ? chrptr+1: (*last)->logical;
    last = &(*last)->mpNext;
    *last = NULL;
    return last;
  }
  
  //***************************************************************************
  //***************************************************************************
  FileListElem::
  FileListElem(const std::string & rFileName)
  {
    std::size_t  pos;
    
    mLogical = rFileName;
    
    // some slash-backslash replacement hack
    for (size_t i = 0; i < mLogical.size(); i++)
      if (mLogical[i] == '\\') 
        mLogical[i] = '/';
        
    // look for "=" symbol and if found, split it
    if ((pos = mLogical.find('=')) != std::string::npos)
    {
      // copy all from mLogical[pos+1] till the end to mPhysical
      mPhysical.assign(mLogical.begin() + pos + 1, mLogical.end());
      // erase all from pos + 1 till the end from mLogical
      mLogical.erase(pos + 1);
    }
    else
    {
      mPhysical = mLogical;
    }    
  }    
  
  //***************************************************************************
  //***************************************************************************
  static int 
  memcmpw(const char *nstr, const char *wstr, int len, char **substr)
  {
    int i, npercents = 0;
    
    for (i=0; i<len; i++) {
      if ((*wstr != *nstr) && (*wstr != '?') && (*wstr != '%')) return 1;
      if (*wstr == '%') (*substr)[npercents++] = *nstr;
      wstr++; nstr++;
    }
    *substr += npercents;
    **substr = '\0';
    return 0;
  }
  
  //***************************************************************************
  //***************************************************************************
  int 
  npercents(const char *str)
  {
    int ret = 0;
    while (*str) if (*str++ == '%') ret++;
    return ret;
  }
  
  //***************************************************************************
  //***************************************************************************
  int 
  process_mask(const char *normstr, const char *wildcard, char *substr)
  {
    char *hlpptr;
    const char  *endwc = wildcard + strlen(wildcard);
    const char  *endns = normstr + strlen(normstr);
  
    *substr = '\0';
  
    if ((hlpptr = (char *) memchr(wildcard, '*', endwc-wildcard)) == NULL) {//hvezdicka neni
      return !((endwc-wildcard != endns-normstr) ||
              memcmpw(normstr, wildcard, endns-normstr, &substr));
    }
  
    if ((hlpptr-wildcard > endns-normstr) ||
      memcmpw(normstr, wildcard, hlpptr-wildcard, &substr)) {// retezec je kratsi nez pozice prvni hvezdicky
      return 0;//je * o..x nebo 1..x
    }
    wildcard = hlpptr;
  
    for (;;) {
      while (*wildcard == '*') wildcard++; //najdi prvni znak za hvezdickou
      if (!*wildcard) return 1;//konec wildcard retezce (za serii hvezdicek jsou jen hvezdicky az do konce)
      if (!*normstr)  return 0;//konec porovnavaneho
      if ((hlpptr = (char *) memchr(wildcard, '*', endwc-wildcard)) == NULL) {
        return !((endwc-wildcard > endns-normstr) ||
                memcmpw(endns-(endwc-wildcard), wildcard, endwc-wildcard, &substr));
      }
  
      do {
        if ((endns-normstr) < (hlpptr-wildcard)) return 0;
        normstr = (char *) ((*wildcard == '?' || *wildcard == '%')
                  ? normstr
                  : memchr(normstr, *wildcard, endns-normstr-(hlpptr-wildcard)+1));
        if (!normstr || !*normstr) return 0;
      } while (memcmpw(normstr++, wildcard, hlpptr-wildcard, &substr));
  
      normstr += hlpptr - wildcard - 1;
      wildcard = hlpptr;
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void fprintf_ll(FILE* fp, long long n)
  {
    const char digit_map[]      = "0123456789abcdef";
    const int  base            = 10;
    const int  n_leading_zeros = 8;
    // the max number for long long is 9223372036854775807LL =>
    // reserve 19 places for number, 1 place for sign, 1 place for string 
    // zero termination, 8 places for leading zeros
    const int  buffer_size     = 19 + 1 + 1 + n_leading_zeros;
    char       buffer[buffer_size];
    int        buffer_i        = buffer_size - 1;
    
    assert(n >= 0);
    
    buffer[buffer_i--] = '\0';
    
    for (int i=32; n && i; i--, n /= base)
    {
      buffer[buffer_i--] = digit_map[n % base];
    }
    
    for (; buffer_i + n_leading_zeros + 2 > buffer_size; buffer_i--)
    {
      buffer[buffer_i] = '0';
    }
    
    fprintf(fp, "%s", buffer + buffer_i + 1);
  } // void fprintf_ll(FILE* fp, long long n)
        
  
  
  using std::string;
  
  /**
  *  @brief Returns true if rString matches rWildcard and fills substr with
  *         corresponding %%% matched pattern
  *  @param rString    String to be parsed
  *  @param rWildcard  String containing wildcard pattern
  *  @param Substr     The mathced %%% pattern is stored here
  *
  *  This is a C++ extension to the original process_mask function.
  *
  */
  bool
  ProcessMask(const std::string & rString,
              const std::string & rWildcard,
                    std::string & rSubstr)
  {
    char *  substr;
    int     percent_count        = 0;
    int     ret ;
    size_t  pos                  = 0;

    // let's find how many % to allocate enough space for the return substring
    while ((pos = rWildcard.find('%', pos)) != rWildcard.npos)
    {
      percent_count++;
      pos++;
    }

    // allocate space for the substring
    substr = new char[percent_count + 1];
    substr[percent_count] = 0;
    substr[0]             = '\0';

    // parse the string
    if (ret = match(rWildcard.c_str(), rString.c_str(), substr))
    {
      rSubstr = substr;
    }
    delete[] substr;
    return ret;
  } // ProcessMask


  //*****************************************************************************
  //*****************************************************************************
  void
  ParseHTKString(const std::string & rIn, std::string & rOut)
  {
    int ret_val;

    // the new string will be at most as long as the original, so we allocate
    // space
    char * new_str = new char[rIn.size() + 1];
    strcpy(new_str, rIn.c_str());

      // there might be some error message returned, so we reserve at least
    // 30 bytes
    char * tmp_str;

    // call the function
    if (!(ret_val = getHTKstr(new_str, &tmp_str)))
    {
      rOut = new_str;
    }

    else if ((ret_val == -1) || (ret_val == -2))
    {
      throw std::logic_error(tmp_str);
    }

    else
    {
      throw std::logic_error("Unexpected error parsing HTK string");
    }

  }

  
  /**
  * @brief Builds new filename based on the parameters given
  * @param rOutFileName reference to a string where new file name is stored
  * @param rInFileName the base name of the new file
  * @param rOutputDir output directory
  * @param rOutputExt new file's extension
  */
  /*
  void 
  MakeFileName(      std::string & rOutFileName, 
              const std::string & rInFileName,
              const std::string & rOutputDir, 
              const std::string & rOutputExt)
  {
    size_t       tmp_pos;
    const char * base_name;
    const char * bname_end = NULL;
    const char * chrptr;
  
    std::string::const_iterator   it_base_name_begin;
    std::string::const_iterator   it_base_name_end  = rInFileName.end() ;
    //std::string                   
    
    if ((tmp_pos = rInFileName.rfind('/')) != std::string::npos)
      it_base_name_begin = rInFileName.begin() + tmp_pos + 1;
    else
      it_base_name_begin = rInFileName.begin();
      
    //base_name = strrchr(rInFileName.c_str(), '/');
    //base_name = base_name != NULL ? base_name + 1 : rInFileName.c_str();
    
    // trim the trailing extension
    if (!rOutp
    utExt.empty())
    {
      //viif ((tmp_pos = 
    }
    
      bname_end = strrchr(base_name, '.');
      
    if (!bname_end) 
      bname_end = base_name + strlen(base_name);
  
  
    if ((chrptr = strstr(rInFileName.c_str(), "/./")) != NULL) 
    {
      // what is in path after /./ serve as base name
      base_name = chrptr + 3;
    }
  
    rOutFileName = "";
    
    if (!rOutputDir.empty()) 
    {
      rOutFileName = rOutputDir + "/";
      rOutFileName.append(base_name);
      
      if (*out_dir) 
      {
        strcat(pOutFileName, out_dir);
        strcat(pOutFileName, "/");
      }
      strncat(pOutFileName, base_name, bname_end - base_name);
    } 
    else 
    {
      strncat(outFileName, inFileName, bname_end - inFileName);
    }
  
    if (!rOutputExt.empty()) 
    {
      rOutFileName += "." + rOutputExt;
    }
  }
  */
//}; //namespace STK
    
