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

#ifndef _SREC_H
#define _SREC_H

// configuration file name
#define SR_CONFIG_FILE "config"

// system dependent headers
#ifdef WIN32
//	#include <direct.h>
	#include "wfsource.h"
#else
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include "lwfsource.h"
#endif

#include "matrix.h"
#include "config.h"
#include "melbanks.h"
#include "plp.h"
#include "traps.h"
#include "norm.h"
#include "phndec.h"
#include "configz.h"

#ifndef PHNREC_ONLY
  #include "stkinterface.h"
  #include "kwsnetg.h"
  #include "thresholds.h"
  #include "lexicon.h"
  #include "gptrans.h"
  #include "phntrans.h"
  #include "phntranscheck.h"
  #include "netgen.h"
#endif  // PHNREC_ONLY

// directory separator
extern const char *dirSep;

// 64 bits integer type
#ifndef long_long
    #ifdef WIN32
        typedef __int64 long_long;
    #else
        typedef long long long_long;
    #endif
#endif

// error and log reporting macros
#define MERROR(m) (*errFunc)(m, errArg);
#define MLOG(m)   (*logFunc)(m, logArg);

class SpeechRec
{
	public:
		// error and log callback function
		typedef void (*handler)(char *msg, void *arg);

		// input/output formats
		typedef enum {dfUnknown, dfWaveform, dfParams, dfPosteriors, dfStrings} data_format;
		typedef enum {wfUnknown, wfLin16, wfALaw} wave_format;

		// softening function
		typedef float (*soft_func)(float v1, float v2, float v3, float v4);

                typedef enum {pkUnknown = 0, pkFBanks = 1, pkPLP = 2} ParamKindType;
	protected:
		// error and log handlers
		handler errFunc;
		handler logFunc;
		void *errArg;
		void *logArg;
		
		// waveform
		wave_format waveFormat;
		float waveScale;
		float waveDCShift;
                float waveNoiseLevel;

                ParamKindType paramKind;
		MelBanks *actualParams;
                bool mTrapsEnabled;

		// memory buffers
		float *waveform;
		int waveformLen;
		float *params;	
		float *posteriors;
		int bunchSize;
		int bunchIdx;
		int lastParamVector;

		// synchonization
		volatile bool terminated;

		// softening function and its arguments
		soft_func postSoftFunc;
		float postSoftArg1;
		float postSoftArg2;
		float postSoftArg3;

		soft_func decSoftFunc;
		float decSoftArg1;
		float decSoftArg2;
		float decSoftArg3;

		// functions
		void SubstVars(char *dir_name, char *cfg_dir);
		void CreateLabelFileNameForMLF(char *fileName, char *retLableFileName);
                ParamKindType Str2ParamKind(char *str);

	public:
		// default handlers
		static void MError(char *msg, void *arg);
		static void MLog(char *msg, void *arg);
		static void MDummy(char *msg, void *arg) {;};
		static void OnWord(unsigned int message, char *word, long_long start, long_long stop, float score, void *tmp);
		static void OnWordMLF(unsigned int message, char *word, long_long start, long_long stop, float score, void *tmp);

		// system components
		#ifdef WIN32
		  CWFSource WFS;
		#else
		  LWFSource WFS;	
		#endif
		Config C;
		MelBanks MB;
		Normalization Norm;
                Traps TR;
		Decoder *DE;

                #ifndef PHNREC_ONLY
                  PLPCoefs PLP;
                  KWSNetGenerator KNG;
		  Thresholds THR;
                  Lexicon Lex;
                  GPTrans GPT;
                  PhnTrans PT;
                  PhnTransChecker PTCheck;
                #endif  // PHNREC_ONLY

		// functions
		SpeechRec();
		~SpeechRec();

		bool Init(char *config_file);

		bool ProcessOnline(void *inpSig, int sigNBytes, bool lastFrame = false);
		bool ProcessLastBunch();
		bool ProcessTail();		
		bool ProcessOffline(data_format inpf, data_format outpf, void *inpSig, int sigNBytes, Mat<float> *inpMat = 0, Mat<float> *outMat = 0);
		bool ProcessFile(data_format inpf, data_format outpf, char *source, char *target = 0, FILE *out_MLF = 0);
		bool ProcessFileListLine(data_format inpf, data_format outpf, char *line, FILE *fp_mlf = 0);		
		bool ProcessFileList(data_format inpf, data_format outpf, char *list, char *outMLFFile = 0);
		
		bool RunLive();
		void Terminate() {terminated = true;};
		
		bool ConvertWaveformFormat(wave_format format, void *sig, int nBytes, float **buff = 0, int *buffLen = 0);
		bool LoadWaveform(char *file, void **outWave, int *sigNBytes);

		bool CheckDataFormatConv(data_format from, data_format to);
		wave_format Str2WaveFormat(char *str);
		data_format Str2DataFormat(char *str);
		static int WaveFormat2NBytesPerSample(wave_format format);

		void SetWaveFormat(wave_format v) {waveFormat = v;};
		void SetWaveScale(float v) {waveScale = v;};
		void SetWaveDCShift(float v) {waveDCShift = v;};
		void SetLogHandler(handler func, void *arg = 0) {logFunc = func, logArg = arg;};
		void SetErrorHandler(handler func, void *arg = 0) {errFunc = func, errArg = arg;};
		void SetPostSoftFunc(soft_func f, float arg1 = 0, float arg2 = 0, float arg3 = 0) {postSoftFunc = f; postSoftArg1 = arg1; postSoftArg2 = arg2; postSoftArg3 = arg3;};
		void SetDecSoftFunc(soft_func f, float arg1 = 0, float arg2 = 0, float arg3 = 0) {decSoftFunc = f; decSoftArg1 = arg1; decSoftArg2 = arg2; decSoftArg3 = arg3;};
		void ResetBunchBuff() {bunchIdx = 0; lastParamVector = -1;}

		// softening functions
		static float SoftLog(float v, float arg1, float arg2, float arg3) {return logf(v);};
		static float SoftIgor(float v, float middPoint, float rightLog, float leftLog);
		static float SoftNone(float v, float arg1, float arg2, float arg3) {return v;};
                static float SoftGMMBypass(float v, float middPoint, float rightLog, float leftLog);

		soft_func Str2SoftFunc(char *str, float *a1, float *a2, float *a3);

		void SentenceBasedNormalization(Mat<float> *mat);
		void FrameBasedNormalization(float *frame, int n);
};

#endif
