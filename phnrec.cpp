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
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string>
//#include <getopt.h>
#include <map> 

#include "config.h"
#include "srec.h"
#include "getopt.h"

void help()
{
	puts("\nUSAGE: phnrec [options]\n");
	puts(" -c dir             configuration directory");
	puts(" -l file            list of files");
	puts(" -i file            input file");
	puts(" -o file            output file");
	puts(" -m file            output MLF");
	puts(" -a                 live audio input");
	puts(" -s fmt [waveform]  source format (wf-waveform, par-parameters, post-posteriors)");
	puts(" -t fmt [strings]   target format (par-parameters, post-posteriors, str-strings)"); 
	puts(" -w fmt [lin16]     waveform format (lin16, alaw)");
	puts(" -f fmt [str]       live output format (str, strlen, lab)");
	puts(" -p num [-3.8]      phoneme insertion penalty");
	puts(" -v                 verbose\n");
}

typedef enum {ofUnknown, ofLab, ofStr, ofStrLen} live_oformat;
live_oformat str2oform(char *str)
{
	if(strcmp(str, "lab") == 0)
		return ofLab;
	else if(strcmp(str, "str") == 0)
		return ofStr;
	else if(strcmp(str, "strlen") == 0)
		return ofStrLen;
	else
	{
		fprintf(stderr, "ERROR: Invalid output format: %s. (can be 'lab', 'str', 'strlen')\n", str);
		exit(1);
	}
	return ofUnknown;
}


typedef struct 
{
	live_oformat fmt;
	char prevWord[256];
	long_long prevStart;
	bool dumped;
	SpeechRec *SR;
	bool kws;
} live_conf;

void live_callback(unsigned int message, char *word, long_long start, long_long stop, float score, void *tmp)
{
	live_conf *cfg = (live_conf *)tmp;
	live_oformat fmt = cfg->fmt;
	
	if(message == DECMSG_NEWESTIM && strcmp(word, cfg->prevWord) == 0 && 
	   start == cfg->prevStart && cfg->dumped)
		return;

        #ifndef PHNREC_ONLY
           if(cfg->kws && score < cfg->SR->THR.GetThreshold(word))
		  return;
        #endif  // PHNREC_ONLY

	strcpy(cfg->prevWord, word);
	cfg->prevStart = start;
	cfg->dumped = true;

	int len = (int)((stop - start)/100000 + 1);

	switch(fmt)
	{
		case ofLab:
			#ifdef _MSC_VER
				fprintf(stdout, "%I64d %I64d", start, stop);
			#else
				fprintf(stdout, "%lli %lli", start, stop);
			#endif
			fprintf(stdout, " %s %f\n", word, score);
			break;
		case ofStr:
			fprintf(stdout, " %s\n", word);
			break;
		case ofStrLen:
			fprintf(stdout, " %s(%d)\n", word, len);
			break;
                case ofUnknown:
                        break;
	}
}


int main(int argc, char *argv[])
{
	char *config_dir = 0;
	char *file_list = 0;
	char *input_file = 0;
	char *output_file = 0;
	bool live_input = false;
	char *output_mlf = 0;
	char *wpenalty = 0;
	bool verbose = false;
	live_oformat format = ofStr;

	SpeechRec::data_format iformat = SpeechRec::dfWaveform;
	SpeechRec::data_format oformat = SpeechRec::dfStrings;
	SpeechRec::wave_format wformat = SpeechRec::wfUnknown;
	SpeechRec SR;

	// command line parsing
	if(argc == 1)
	{
		help();
		exit(1);
	}

	optind = 0;
	while (1)
	{
		int c = getopt(argc, argv, "-c:l:i:o:m:as:t:w:f:p:v");
		if(c == -1)
			break;

		switch(c)
		{
			case 'c':
				config_dir = optarg;
				break;
			case 'l':
				file_list = optarg;
				break;
			case 'i':
				input_file = optarg;
				break;
			case 'o':
				output_file = optarg;
				break;
			case 'm':
				output_mlf = optarg;
				break;
			case 'a':
				live_input = true;
				break;
			case 's':
				iformat = SR.Str2DataFormat(optarg);
				if(iformat == SpeechRec::dfUnknown)
					exit(1);
				break;
			case 't':
				oformat = SR.Str2DataFormat(optarg);
				if(oformat == SpeechRec::dfUnknown)
					exit(1);
				break;
			case 'w':
				wformat = SR.Str2WaveFormat(optarg);
				if(wformat == SpeechRec::wfUnknown)
					exit(1);
				break;
			case 'p':
				wpenalty = optarg;
				break;
			case 'f':
				format = str2oform(optarg);
				break;
			case 'v':
				verbose = true;
				break;
                        case '?':
                                fprintf(stderr, "ERROR: Error during command line parsing\n");
                                exit(1);
		}
	}

	// set log handler according verbosity
	if(!verbose)
		SR.SetLogHandler(&SpeechRec::MDummy, 0);

	// construct speech recognizer
	if(!config_dir)
	{
		fprintf(stderr, "ERROR: Configuration directory is not set (-c)\n");
		exit(1);
	}
	char config_file[1024];
	strcpy(config_file, config_dir);
	strcat(config_file, dirSep);
	strcat(config_file, "config");
	if(!SR.Init(config_file))
		exit(1);

	// set word insertion penalty
	if(wpenalty)
	{
		float value;
		if(sscanf(wpenalty, "%f", &value) != 1)
		{
			fprintf(stderr, "ERROR: Invalid argument for -p switch at command line: %s\n", wpenalty);
			exit(1);
		}
		SR.DE->SetWPenalty(value);
	}

	// set waveform format
	if(wformat != SpeechRec::wfUnknown)
		SR.SetWaveFormat(wformat);

	// check the input file name if an output is asked	
	if(output_file && !input_file)
	{
		fprintf(stderr, "ERROR: The input file is not specified (-i)\n");
		exit(1);
	}

	if(!SR.CheckDataFormatConv(iformat, oformat))
	{
		fprintf(stderr, "ERROR: Unsupported data conversion (-s, -t)\n");
		exit(1);
	}

	// process input file
	if(input_file)
	{
		char line[1024];
		strcpy(line, input_file);
		if(output_file)
		{
			strcat(line, " ");
			strcat(line, output_file);
		}
		if(!SR.ProcessFileListLine(iformat, oformat, line))
			exit(1);
	}

	// process file list
	if(file_list)
	{
		if(!SR.ProcessFileList(iformat, oformat, file_list, output_mlf))
			exit(1);
	}

	// run the live audio
	if(live_input)
	{
		live_conf cfg;
		cfg.fmt = format;
		const char *mode = SR.C.GetString("decoder", "mode");
		if(strcmp(mode, "kws") == 0)
		{
                        #ifndef PHNREC_ONLY
                          if(strcmp(SR.C.GetString("decoder", "type"), "stkint") == 0)
			  {
		  		  ((StkInterface *)SR.DE)->SetImproveKwdEstim(false);
			  }
                        #endif  // PHNREC_ONLY

			cfg.prevWord[0] = '\0';
			cfg.prevStart = -1;
			cfg.dumped = false;
			cfg.SR = &SR;
			cfg.kws = true; 
		}
		else
		{
			cfg.kws = false;
		}

		SR.DE->SetCallbackFunc(&live_callback, (void *)&cfg);
		
                if(SR.C.GetInt("onlinenorm", "estim_interval") != 0)
                {
			printf("Estimation of normalization parameters, please speak ...\n");			
                }

                if(!SR.RunLive())
			exit(1);
	}

	return 0;
}
