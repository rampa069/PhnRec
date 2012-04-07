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
#include <assert.h>
#include "dspc.h"
#include "srec.h"
#include "alaw.h"
#include "filename.h"

#ifdef __CYGWIN__
	#include <sys/stat.h>
#endif

#ifdef WIN32
	extern const char *dirSep = "\\";
#else
	extern const char *dirSep = "/";
#endif


// variable table - these variables can be set in a configuration file
config_variable cfg_entry[] =
{
	{"source",           "format",            CE_STRING, "lin16"},
	{"source",           "sample_freq",       CE_INT,    "8000"}, 
	{"source",           "scale",             CE_FLOAT,  "1.0f"},
	{"source",           "dc_shift",          CE_FLOAT,  "0.0f"},
        {"source",           "noise_level",       CE_FLOAT,  "0.0f"},
        {"params",           "kind",              CE_STRING, "fbanks"},
	{"params",           "suffix",            CE_STRING, "mel"},
	{"melbanks",         "nbanks",            CE_INT,    "15"},
        {"melbanks",         "nbanks_full",       CE_INT,    "-1"},
	{"melbanks",         "lower_freq",        CE_FLOAT,  "0"},
	{"melbanks",         "higher_freq",       CE_FLOAT,  "4000"},
	{"melbanks",         "vector_size",       CE_INT,    "200"},
	{"melbanks",         "vector_step",       CE_INT,    "80"},
	{"melbanks",         "preem_coef",        CE_FLOAT,  "0.0"},
	{"melbanks",         "z_mean_source",     CE_BOOL,   "false"},
	{"plp",              "order",             CE_INT,    "12"},
        {"plp",              "compress_fact",     CE_FLOAT,  "0.3333333"},
        {"plp",              "cep_lifter",        CE_FLOAT,  "22"},
	{"plp",              "cep_scale",         CE_FLOAT,  "10"},
        {"plp",              "add_c0",            CE_BOOL,   "false"},
        {"onlinenorm",       "estim_interval",    CE_INT,    "0"},
        {"onlinenorm",       "signal_est_end",    CE_BOOL,   "false"},
        {"onlinenorm",       "file",              CE_STRING, "none"},
        {"onlinenorm",       "mean_norm",         CE_BOOL,   "false"},
        {"onlinenorm",       "var_norm",          CE_BOOL,   "false"},
        {"onlinenorm",       "scale_to_gvar",     CE_BOOL,   "false"},
	{"offlinenorm",      "sent_mean_norm",    CE_BOOL,   "false"},
	{"offlinenorm",      "sent_var_norm",     CE_BOOL,   "false"},
	{"offlinenorm",      "sent_std_thr",      CE_FLOAT,  "0.01"},
	{"offlinenorm",      "sent_max_norm",     CE_BOOL,   "false"},
	{"offlinenorm",      "sent_chmax_norm",   CE_BOOL,   "false"},
	{"framenorm",        "min_floor",         CE_FLOAT,  "-9999.9"},
	{"framenorm",        "shift",             CE_FLOAT,  "0"},
	{"posteriors",       "system",            CE_STRING, "1BT_DCT"},
	{"posteriors",       "length",            CE_INT,    "31"},
	{"posteriors",       "add_c0",            CE_BOOL,   "true"},
	{"posteriors",       "hamming",           CE_BOOL,   "false"},
	{"posteriors",       "suffix",            CE_STRING, "lop"},
	{"posteriors",       "bunch_size",        CE_STRING, "1"},
	{"posteriors",       "enabled",           CE_BOOL,   "true"},
        {"posteriors",       "softening_func",    CE_STRING, "none 0 0 0"},
        {"decoder",          "type",              CE_STRING, "stkint"},               
	{"decoder",          "wpenalty",          CE_FLOAT,  "-2.0"},
	{"decoder",          "lm_scale",          CE_FLOAT,  "1.0"},
	{"decoder",          "time_pruning",      CE_INT,    "40"},
	{"decoder",          "mode",              CE_STRING, "decode"},
	{"decoder",          "softening_func",    CE_STRING, "log 0 0 0"},
        {"decoder",          "num_states_per_phn",CE_INT,    "1"},
	{"dirs",             "tmp",               CE_STRING, "$C/tmp"},
	{"models",           "hmm_defs",          CE_STRING, "$T/models"},
	{"models",           "nstates",           CE_INT,    "3"},
	{"models",           "gen_from_phn_list", CE_BOOL,   "false"},
	{"dicts",            "phoneme_list",      CE_STRING, ""},
	{"dicts",            "lexicon1",          CE_STRING, ""},
	{"dicts",            "lexicon2",          CE_STRING, ""},
	{"dicts",            "lexicon1_save_bin", CE_BOOL,   "false"},
	{"dicts",            "lexicon2_save_bin", CE_BOOL,   "false"},
	{"dicts",            "keyword_list",      CE_STRING, "none"},
	{"dicts",            "charset",           CE_STRING, "eastevrope"},
	{"networks",         "default",           CE_STRING, "$C/nets/network"},
	{"networks",         "gen_phn_loop",      CE_BOOL,   "false"},
	{"networks",         "gen_kws_net",       CE_BOOL,   "false"},
	{"networks",         "omit_phn",          CE_STRING, "oth"},
	{"labels",           "suffix",            CE_STRING, "rec"},
	{"labels",           "remove_path",       CE_BOOL,   "true"},
	{"kws",              "default_thr",       CE_FLOAT,  "-10.0"},
	{"kws",              "thresholds_file",   CE_STRING, "none"},
	{"gptransc",         "rules",             CE_STRING, "none"},
	{"gptransc",         "symbols",           CE_STRING, "none"},
	{"gptransc",         "max_variants",      CE_INT,    "-1"},
	{"gptransc",         "scale_prob",        CE_BOOL,   "false"},
	{"gptransc",         "prob_thr",          CE_FLOAT,  "-1.0"},
	{"phntransc",        "mode",              CE_STRING, "lexgpt"},
	{0       ,           0,                   0,          0}
};

// source-format: lin8, lin16, alaw
// system-type:   3BT, 1BT, 1BT_DCT, LCRC
// decoder-mode:  decode, kws
//-----------------------------------------------------------------------------------

// default error handler
void SpeechRec::MError(char *msg, void *arg)
{
	fprintf(stderr, "ERROR: %s", msg);
	exit(1);
}

// default logging handler
void SpeechRec::MLog(char *msg, void *arg)
{
	fprintf(stdout, "%s", msg);
}

// default handler for the decoder output 
void SpeechRec::OnWord(unsigned int message, char *word, long_long start, long_long stop, float score, void *tmp)
{
	OnWordMLF(message, word, start, stop, score, (void *)stdout);
}

// handler used to dump labels to MLF 
void SpeechRec::OnWordMLF(unsigned int message, char *word, long_long start, long_long stop, float score, void *tmp)
{
	start /= 100000;
	stop /= 100000;

        if(start == 0)
        {
           fprintf((FILE *)tmp, "0");
        }
        else
        {
           fprintf((FILE *)tmp, "%u00000", (unsigned int)start);
        }

        if(stop == 0)
        {
           fprintf((FILE *)tmp, " 0");
        }
        else
        {
           fprintf((FILE *)tmp, " %u00000", (unsigned int)stop);
        }

	fprintf((FILE *)tmp, " %s %f\n", word, score);
}

//-----------------------------------------------------------------------------------
// softening function

float SpeechRec::SoftIgor(float v, float middPoint, float rightLog, float leftLog)
{
        if(v < middPoint)
                return logf(v * (1.0f / middPoint)) / logf(leftLog);
        return -1.0f * logf(( 1.0f + ( -1.0f * v )) * (1.0f / (1.0f - middPoint))) / logf(rightLog);
}

float SpeechRec::SoftGMMBypass(float v, float middPoint, float rightLog, float leftLog)
{
        return sqrtf(-2.0f * logf(v));
}

//-----------------------------------------------------------------------------------

SpeechRec::SpeechRec() :
		errFunc(&MError),
		logFunc(&MLog),
		errArg(0),
		logArg(0),
		waveScale(1.0f),
		waveDCShift(0.0f),
                waveNoiseLevel(0.0f),
                actualParams(&MB),
                mTrapsEnabled(true),
		waveform(0),
		waveformLen(0),
		params(0),
		posteriors(0),
    		bunchSize(1),
		bunchIdx(0),
		lastParamVector(0),
		postSoftFunc(&SoftNone),
		postSoftArg1(0),
		postSoftArg2(0),
		postSoftArg3(0),
		decSoftFunc(&SoftLog),
		decSoftArg1(0),
		decSoftArg2(0),
		decSoftArg3(0),
                DE(0)
{
}

SpeechRec::~SpeechRec()
{
	if(params)
		delete [] params;
	if(posteriors)
		delete [] posteriors;
	if(DE)
		delete DE;
}

void SpeechRec::SubstVars(char *dir_name, char *cfg_dir)
{
	// replace dir separators /\ with the one used by system
	CorrectDirSeparator(dir_name, dirSep);

	if(strlen(dir_name) > 1 && (strncmp(dir_name, "$C", 2) == 0 || strncmp(dir_name, "$T", 2) == 0))
	{
		char buff[1024];
		strcpy(buff, dir_name);
		if(buff[1] == 'C')
			sprintf(dir_name, "%s%s", cfg_dir, buff + 2);
		else
			sprintf(dir_name, "%s%s", C.GetString("dirs", "tmp"), buff + 2);						
	}
}

bool SpeechRec::Init(char *config_file)
{
	// configuration	
	char config_dir[1024];
	GetFilePath(config_file, config_dir);

	C.SetVariableTable(cfg_entry);
	C.SetCheckUnknownVariables(true);

	int line;
	int ret = C.Load(config_file, &line);
	if(ret != EN_OK)
	{
		char msg[1024];
		switch(ret)
		{
                case EN_UNKVAR:
                        sprintf(msg, "Unknown variable in configuration file '%s', line %d\n", config_file, line);
						break;
				case EN_BADVAL:
                        sprintf(msg, "Invalid argument for a vatiable in configuration file '%s', line %d\n", config_file, line);
						break;
				case EN_FILEERR:
                        sprintf(msg, "Can not open configuration file '%s'\n", config_file);
						break;
				case EN_INVVAR:
                        sprintf(msg, "Invalid notation of variable in configuration file '%s', line %d\n", config_file, line);
						break;
		}
		MERROR(msg);
		return false;
	}

	// substitute variables in directory and file names
	// - temp dir
	char tmpDir[1024];
	char *pdir = C.GetString("dirs", "tmp");
	strcpy(tmpDir, pdir);
	SubstVars(tmpDir, config_dir);
	C.SetString("dirs", "tmp", tmpDir);
	#if defined WIN32 && !defined __CYGWIN__
    	    mkdir(tmpDir);
	#else
	    mkdir(tmpDir, 0xFFFF);
	#endif

	// - model file
	char file[1024];
	char *pfile;
	pfile = C.GetString("models", "hmm_defs");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("models", "hmm_defs", file);
	// - phoneme list
	pfile = C.GetString("dicts", "phoneme_list");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("dicts", "phoneme_list", file);
	// - network file
	pfile = C.GetString("networks", "default");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("networks", "default", file);
	// - lexicon
	pfile = C.GetString("dicts", "lexicon1");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("dicts", "lexicon1", file);
        //
	pfile = C.GetString("dicts", "lexicon2");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("dicts", "lexicon2", file);
	// - keyword_list
	pfile = C.GetString("dicts", "keyword_list");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("dicts", "keyword_list", file);
	// - thresholds file
	pfile = C.GetString("kws", "thresholds_file");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("kws", "thresholds_file", file);
	// - grapheme to phoneme transcription rules
	pfile = C.GetString("gptransc", "rules");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("gptransc", "rules", file);
	// - symbols for GP transcription rules
	pfile = C.GetString("gptransc", "symbols");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("gptransc", "symbols", file);
	// - norm file
	pfile = C.GetString("onlinenorm", "file");
	strcpy(file, pfile);
	SubstVars(file, config_dir);
	C.SetString("onlinenorm", "file", file);

        #ifndef PHNREC_ONLY

        	// automatic generation of hmm file
	 	if(C.GetBool("models", "gen_from_phn_list"))
		{
	 		int err = PhnList2HMMDef(C.GetString("dicts", "phoneme_list"),
				                 C.GetString("models", "hmm_defs"),
	                        	         C.GetInt("models", "nstates"));
			char msg[1024];
			if(err != NGEN_OK)
			{
				switch(err)
				{

					case NGEN_INPUTFILE:
						sprintf(msg, "Can not open the phoneme list: %s\n", C.GetString("dicts", "phoneme_list"));
						break;
					case NGEN_OUTPUTFILE:
						sprintf(msg, "Can not create the HMM file: %s\n", C.GetString("models", "hmm_defs"));
						break;
				}
				MERROR(msg);
				return false;
			}
		}

		// automatic generation of phoneme loop (word network)
		if(C.GetBool("networks", "gen_phn_loop"))
		{
			char net_dir[1024];
			GetFilePath(C.GetString("networks", "default"), net_dir);
	    		#ifdef __WIN32
        		    mkdir(net_dir);
			#else
			    mkdir(net_dir, 0xFFFF);
			#endif
			int err = PhnList2PhnLoop(C.GetString("dicts", "phoneme_list"),
		        	                  C.GetString("networks", "default"),
		                	          C.GetString("networks", "omit_phn"));
			if(err != NGEN_OK)
			{
				char msg[1024];
				switch(err)
				{
					case NGEN_INPUTFILE:
						sprintf(msg, "Can not open the dictionary file: %s\n", C.GetString("dicts", "phoneme_list"));
						break;
					case NGEN_OUTPUTFILE:
						sprintf(msg, "Can not create the network file: %s\n", C.GetString("networks", "default"));
						break;
				}
				MERROR(msg);
				return false;
			}
		}

		// grapheme to phoneme transcription
	        if(strcmp(C.GetString("gptransc", "rules"), "none") != 0)
        	{
                	int ret = GPT.LoadRules(C.GetString("gptransc", "rules"));
	                if(ret != GPT_OK)
        	        {
                	        char msg[1024];
                        	sprintf(msg, "Can not open grapheme to phoneme conversion rules: %s\n", C.GetString("gptransc", "rules"));
	                        MERROR(msg);
        	                return false;
                	}
	                if(strcmp(C.GetString("gptransc", "symbols"), "none") == 0)
        	        {
                	        char msg[1024];
                        	sprintf(msg, "Symbols for grapheme to phoneme conversion rules have to be used (gptransc/symbols)\n");
	                        MERROR(msg);
        	                return false;
                	}
	        }
        	else
	        {
        	        GPT.UnInitialize();
	        }
	        if(strcmp(C.GetString("gptransc", "symbols"), "none") != 0)
	        {
        	        int ret = GPT.LoadSymbols(C.GetString("gptransc", "symbols"));
                	if(ret != GPT_OK)
	                {
        	                char msg[1024];
                	        sprintf(msg, "Can not open symbols for grapheme to phoneme conversion rules: %s\n", C.GetString("gptransc", "symbols"));
                        	MERROR(msg);
	                        return false;
        	        }
	        }
	        GPT.SetMaxProns(C.GetInt("gptransc", "max_variants"));
	        GPT.SetScalePronProb(C.GetBool("gptransc", "scale_prob"));
	        GPT.SetLowestPronProb(C.GetFloat("gptransc", "prob_thr"));

	        // configure phoneme transcription (grapheme to phoneme transc and lexicon merger)
	        PT.SetLexicon(&Lex);
        	PT.SetGPTrans(&GPT);

	        KNG.SetPhnTrans(&PT);

		// - load lexicons
		Lex.Clear();
		int i;
		for(i = 1; i <= 2; i++)
		{
			char lexicon_tag[256];
			sprintf(lexicon_tag, "lexicon%d", i);
			char save_bin_tag[256];
			sprintf(save_bin_tag, "lexicon%d_save_bin", i); 
			if(strcmp(C.GetString("dicts", lexicon_tag), "none") != 0)
        	        {
        		        char errWord[256];
                        	int ret = Lex.Load(C.GetString("dicts", lexicon_tag), i - 1, C.GetBool("dicts", save_bin_tag), errWord);
		                if(ret != LEX_OK)
        		        {
					char msg[1024];
	                		switch(ret)
	               			{
                				case LEX_INPUTFILE:
				                	sprintf(msg, "Can not open lexicon %s\n", C.GetString("dicts", lexicon_tag));
					               	break;
        	                                case LEX_TRANSC:
	        	        			sprintf(msg, "Invalid word transcription '%s', file %s\n", errWord, C.GetString("dicts", lexicon_tag));
	       	        				break;
                				case LEX_ALREDYEXISTS:
				        	        sprintf(msg, "Word transctiption alredy exists '%s', file %s\n", errWord, C.GetString("dicts", lexicon_tag));
				               		break;
				               	case LEX_SYNTAX:
			                		sprintf(msg, "Missing transcription for word '%s', file %s\n", errWord, C.GetString("dicts", lexicon_tag));
	        	        			break;
	                		}
				        MERROR(msg);
                	                return false;
                        	}
	                }
		}

		// kws network generator and phoneme transcription checker
		// - load phoneme list
		if(strcmp(C.GetString("dicts", "phoneme_list"), "none") != 0)
		{
			int ret1 = KNG.LoadPhnList(C.GetString("dicts", "phoneme_list"));
	                int ret2 = PTCheck.LoadPhnList(C.GetString("dicts", "phoneme_list"));
			if(ret1 == KWSNGE_INPUTFILE || ret2 == PTCE_INPUTFILE)
			{
				char msg[1024];
				sprintf(msg, "Can not open phoneme list %s\n", C.GetString("dicts", "phoneme_list"));
				MERROR(msg);
				return false;
			}

		}

		if(C.GetBool("networks", "gen_kws_net"))
		{
			char unkWord[256];
			ret = KNG.GenerateFromFile(C.GetString("dicts", "keyword_list"), C.GetString("networks", "default"), unkWord);
			if(ret != KWSNGE_OK)
			{
				char msg[1024];
				switch(ret)
				{
					case KWSNGE_INPUTFILE:
						sprintf(msg, "Can not open keyword list %s\n", C.GetString("dicts", "keyword_list"));
						break;
					case KWSNGE_OUTPUTFILE:
						sprintf(msg, "Can not create network file %s\n", C.GetString("networks", "default"));
						break;
					case KWSNGE_MISSINGWORD:
						sprintf(msg, "Lexicon does not contain transcription for: %s\n", unkWord);
						break;
					case KWSNGE_NOTINIT:
						sprintf(msg, "Keyword network generator is not fully initialized. It needs phoneme list and lexicon\n");
						break;
				}
				MERROR(msg);
				return false;
			}
		}

		// load threshold file
		THR.SetDefaultThr(C.GetFloat("kws", "default_thr"));
		char *thr_file = C.GetString("kws", "thresholds_file");
		if(strcmp(thr_file, "none") != 0)
		{
			bool ok = THR.LoadThresholds(thr_file);
			if(!ok)
			{
				char msg[1024];
				sprintf(msg, "Can not open thresholds file %s\n", thr_file);
				MERROR(msg);
				return false;
			}
		}
	        else
	        {
        	        THR.Clear();
	        }
	
	#endif  // PHNREC_ONLY


	// source
	waveFormat = Str2WaveFormat(C.GetString("source", "format"));
	waveScale = C.GetFloat("source", "scale");
	waveDCShift = C.GetFloat("source", "dc_shift");
	waveNoiseLevel = C.GetFloat("source", "noise_level");

	MLOG("\nSystem initialization\n");

        // param kind - melbanks, PLP
        paramKind = Str2ParamKind(C.GetString("params", "kind"));
	switch(paramKind)
	{
			
        	case pkFBanks:	// mel-banks
			MLOG("  - mel-banks ...\n");
			MB.SetBanksNum(C.GetInt("melbanks", "nbanks"));
			MB.SetBanksFullNum(C.GetInt("melbanks", "nbanks_full"));
		        MB.SetSampleFreq(C.GetInt("source", "sample_freq"));
        		MB.SetVectorSize(C.GetInt("melbanks", "vector_size"));
	        	MB.SetStep(C.GetInt("melbanks", "vector_step"));
		        MB.SetPreemCoef(C.GetFloat("melbanks", "preem_coef"));
		        MB.SetZMeanSource(C.GetBool("melbanks", "z_mean_source"));	
	        	MB.SetLowFreq(C.GetFloat("melbanks", "lower_freq"));
	        	MB.SetHighFreq(C.GetFloat("melbanks", "higher_freq"));
			actualParams = &MB;
			break;

		#ifndef PHNREC_ONLY
			case pkPLP:	// plp	
				MLOG("  - plp ...\n");
				PLP.SetBanksNum(C.GetInt("melbanks", "nbanks"));
				PLP.SetBanksFullNum(C.GetInt("melbanks", "nbanks_full"));
			        PLP.SetSampleFreq(C.GetInt("source", "sample_freq"));
		        	PLP.SetVectorSize(C.GetInt("melbanks", "vector_size"));
			        PLP.SetStep(C.GetInt("melbanks", "vector_step"));
			        PLP.SetPreemCoef(C.GetFloat("melbanks", "preem_coef"));
			        PLP.SetZMeanSource(C.GetBool("melbanks", "z_mean_source"));
		        	PLP.SetLowFreq(C.GetFloat("melbanks", "lower_freq"));
			        PLP.SetHighFreq(C.GetFloat("melbanks", "higher_freq"));
				PLP.SetLPCOrder(C.GetInt("plp", "order"));
			        PLP.SetCompressFactor(C.GetFloat("plp", "compress_fact"));
		        	PLP.SetCepstralLifter(C.GetFloat("plp", "cep_lifter"));
				PLP.SetCepstralScale(C.GetFloat("plp", "cep_scale"));
			        PLP.SetAddC0(C.GetBool("plp", "add_c0"));

			actualParams = &PLP;
			break;
		#endif  // PHNREC_ONLY

		default:
			char msg[1024];
			sprintf(msg, "Unknown parameterization (parameters/kind): '%s'\n", C.GetString("parameters", "kind"));
			MERROR(msg);
			return false;
	}

        // on-line normalization
	MLOG("  - online normalization ...\n");
        int interval = C.GetInt("onlinenorm", "estim_interval");
        if(interval != 0)
           Norm.StartEstimation(interval);
        Norm.SetSignalEstimEnd(C.GetBool("onlinenorm", "signal_est_end"));
        Norm.SetFile(C.GetString("onlinenorm", "file"));
        Norm.SetMeanNorm(C.GetBool("onlinenorm", "mean_norm"));
        Norm.SetVarNorm(C.GetBool("onlinenorm", "var_norm"));
        Norm.SetScaleToGVar(C.GetBool("onlinenorm", "scale_to_gvar"));

	// traps
	MLOG("  - posteriors (loading NNs) ...\n");
	if(!TR.SetSystem(C.GetString("posteriors", "system")))
	{
		char msg[1024];
		sprintf(msg, "Unknown system, check configuration: %s", C.GetString("system", "type"));
		MERROR(msg);
		return false;
	}

	mTrapsEnabled = C.GetBool("posteriors", "enabled");
	if(mTrapsEnabled)
        {
   		TR.SetTrapLen(C.GetInt("posteriors", "length"));
		TR.SetHamming(C.GetBool("posteriors", "hamming"));
		TR.SetNBanks(C.GetInt("melbanks", "nbanks"));
		TR.SetAddC0(C.GetBool("posteriors", "add_c0"));
	        bunchSize = C.GetInt("posteriors", "bunch_size");
	        TR.SetBunchSize(bunchSize);
		TR.Init(config_dir);  // loads nets and allocates memory
		C.SetInt("posteriors", "noutputs", TR.GetNumOuts());
	}

	// decoder
	MLOG("  - decoder ...\n\n");

	DE = 0;

	#ifndef PHNREC_ONLY
	        if(strcmp(C.GetString("decoder", "type"), "stkint") == 0)
        	{
	        	        DE = (Decoder *)new StkInterface;
				((StkInterface *)DE)->ReadHMMSet(C.GetString("models", "hmm_defs"));
				if(strcmp(C.GetString("networks", "default"), "none") != 0)
        				((StkInterface *)DE)->LoadNetwork(C.GetString("networks", "default"));
				((StkInterface *)DE)->SetTimePruning(C.GetInt("decoder", "time_pruning"));
				((StkInterface *)DE)->SetLMScale(C.GetFloat("decoder", "lm_scale"));
	        }
	#endif  // PHNREC_ONLY

        if(strcmp(C.GetString("decoder", "type"), "phndec") == 0)
        {
                DE = (PhnDec *)new PhnDec;

		((PhnDec *)DE)->SetNStPerPhn(C.GetInt("decoder", "num_states_per_phn"));
		((PhnDec *)DE)->SetTimePruning(C.GetInt("decoder", "time_pruning"));

                if(((PhnDec *)DE)->LoadPhnList(C.GetString("dicts", "phoneme_list"))  != DECERR_NONE)
                {
			char msg[1024];
			sprintf(msg, "Can not load phoneme list: %s", C.GetString("dicts", "phoneme_list"));
			MERROR(msg);
                	return false;
                }
        }
        
	if(DE == 0)
        {
		char msg[1024];
		sprintf(msg, "Unknown dekoder, check configuration: %s", C.GetString("decoder", "type"));
		MERROR(msg);
                return false;
        }

	DE->SetWPenalty(C.GetFloat("decoder", "wpenalty"));
	if(strcmp(C.GetString("decoder", "mode"), "kws") == 0)
		DE->SetMode(DECMODE_KWS);
	else
		DE->SetMode(DECMODE_DECODE);

        // softening functions
	float a1, a2, a3;
	soft_func func = Str2SoftFunc(C.GetString("posteriors", "softening_func"), &a1, &a2, &a3);
	SetPostSoftFunc(func, a1, a2, a3);

	func = Str2SoftFunc(C.GetString("decoder", "softening_func"), &a1, &a2, &a3);
	SetDecSoftFunc(func, a1, a2, a3);

	//MLOG("Initialization done.\n");
	//MLOG("\n");

	char msg[1024];
	sprintf(msg, "------------------- SUMMARY -------------------\n"            ); MLOG(msg);
	sprintf(msg, "Dictionary:   %s\n", C.GetString("dicts",    "phoneme_list"  )); MLOG(msg);
	sprintf(msg, "Network file: %s\n", C.GetString("networks", "default"       )); MLOG(msg);
	sprintf(msg, "HMM file:     %s\n", C.GetString("models",   "hmm_defs"      )); MLOG(msg);
	sprintf(msg, "#States/Phn:  %d\n", C.GetInt(   "models",   "nstates"       )); MLOG(msg);
	sprintf(msg, "Time pruning: %d\n", C.GetInt(   "decoder",  "time_pruning"  )); MLOG(msg);
	sprintf(msg, "Word penalty: %f\n", C.GetFloat( "decoder",  "wpenalty"      )); MLOG(msg);
	sprintf(msg, "Soft func:    %s\n", C.GetString("decoder",  "softening_func")); MLOG(msg);
	sprintf(msg, "-----------------------------------------------\n\n"          ); MLOG(msg);

	// memory buffers
	if(params)
		delete [] params;
	if(posteriors)
		delete [] posteriors;


	int n = (TR.GetTrapShift() > bunchSize ? TR.GetTrapShift() : bunchSize);
	params = new float [n * C.GetInt("melbanks", "nbanks")];
	posteriors = new float [n * TR.GetNumOuts()];
	
	return true;
}

bool SpeechRec::ConvertWaveformFormat(wave_format format, void *sig, int nBytes, float **buff, int *buffLen)
{
	assert(format == wfLin16 || format == wfALaw);

	int sigLen;
	int i;
	float *out;
	
	if(buff == 0 || *buff == 0)
		out = 0;
	else
		out = *buff;

	switch(format)
	{
		case wfLin16:
			sigLen = nBytes / sizeof(short);
			
			if(!out || *buffLen != sigLen)
			{
				if(out)
					delete [] out;
				out = new float [sigLen > MB_VECTORSIZE ? sigLen : MB_VECTORSIZE];
				if(!out)
				{
					MERROR("Insufficient memory\n");
					return false;
				}
			}

			for(i = 0; i < MB_VECTORSIZE; ++i)  // this ensures at least one frame if the signal is too short
				out[i] = 0.0f;
			
			for(i = 0; i < sigLen; ++i)
				out[i] = (float)((short *)sig)[i];
			
			*buffLen = sigLen;
			*buff = out;

			break;

		case wfALaw:
			sigLen = nBytes;
			
			if(!out || *buffLen != sigLen)
			{
				if(buff)
					delete [] out;
				out = new float [sigLen > MB_VECTORSIZE ? sigLen : MB_VECTORSIZE];
				if(!out)
				{
					MERROR("Insufficient memory\n");
					return false;
				}
			}
			
			for(i = 0; i < MB_VECTORSIZE; ++i)
				out[i] = 0.0f;
			
			for(i = 0; i < sigLen; ++i)
				out[i] =  8.0f * (float)ALawTableD5[((unsigned char *)sig)[i]];
			
			*buffLen = sigLen;
			*buff = out;

			break;
		case wfUnknown: 
			assert(0);
	}

	// shift DC of the waveform
	if(waveDCShift != 0.0f)
		sAddition(*buffLen, *buff, waveDCShift);

	// scale the waweform
	if(waveScale != 1.0f)
		sMultiplication(*buffLen, *buff, waveScale);		

        if(waveNoiseLevel != 0.0f)
                sAddNoise(*buffLen, *buff, waveNoiseLevel);

	return true;
}

bool SpeechRec::ProcessOnline(void *inpSig, int sigNBytes, bool lastFrame)
{	
	if(!ConvertWaveformFormat(waveFormat, inpSig, sigNBytes, &waveform, &waveformLen))
		return false;

	actualParams->AddWaveform(waveform, waveformLen);

	while(actualParams->GetFeatures(params + bunchIdx * actualParams->GetNParams()))
	{     
                // minima thresholds etc.
		FrameBasedNormalization(params + bunchIdx * actualParams->GetNParams(), actualParams->GetNParams());	

                // initial interval based mean and variance normalization
                Norm.ProcessFrame(actualParams->GetNParams(), params + bunchIdx * actualParams->GetNParams());  

		lastParamVector = bunchIdx;
		bunchIdx++;

		if(bunchIdx == bunchSize)
		{
		        if(mTrapsEnabled)
			{
				TR.CalcFeaturesBunched(params, posteriors, bunchSize);

                                // this function is split into two parts just becasuse of saving in off-line mode
				int j;
				for(j = 0; j < bunchSize * TR.GetNumOuts(); j++)
					posteriors[j] = (*postSoftFunc)(posteriors[j], postSoftArg1, postSoftArg2, postSoftArg3);

				for(j = 0; j < bunchSize * TR.GetNumOuts(); j++)
					posteriors[j] = (*decSoftFunc)(posteriors[j], decSoftArg1, decSoftArg2, decSoftArg3);
			

				int i;
				for(i = 0; i < bunchSize; i++)
				{
					if(TR.GetDelay() >= TR.GetTrapShift())     // Initialization of TRAPS - skip half of the trap length
						DE->ProcessFrame(posteriors + i * TR.GetNumOuts());
				}
			}
			else
			{
				int i;
				for(i = 0; i < bunchSize; i++)
				{
					DE->ProcessFrame(params + i * actualParams->GetNParams());
				}
			}
			bunchIdx = 0;
		}
	}

	if(lastFrame)
		ProcessTail();	

	return true;
}

bool SpeechRec::ProcessLastBunch()
{	
	if(bunchIdx == 0)
		return false;

	TR.CalcFeaturesBunched(params, posteriors, bunchIdx);

	int j;
	for(j = 0; j < bunchIdx * TR.GetNumOuts(); j++)
		posteriors[j] = (*postSoftFunc)(posteriors[j], postSoftArg1, postSoftArg2, postSoftArg3);

	for(j = 0; j < bunchIdx * TR.GetNumOuts(); j++)
		posteriors[j] = (*decSoftFunc)(posteriors[j], decSoftArg1, decSoftArg2, decSoftArg3);

	int i;
	for(i = 0; i < bunchIdx; i++)
	{
		if(TR.GetDelay() >= TR.GetTrapShift())     
			DE->ProcessFrame(posteriors + i * TR.GetNumOuts());
	}
	
	bunchIdx = 0;

	return true;
}

bool SpeechRec::ProcessTail()
{
        if(mTrapsEnabled)
        {
		ProcessLastBunch();

		if(lastParamVector < 0)
			return false;
	
		int nparams = actualParams->GetNParams();
		int tshift = TR.GetTrapShift();
	
		// repeate last vector tshift times	
		int i;	
		for(i = 0; i < tshift; i++)
		{
	
			if(i != lastParamVector)
				sCopy(nparams, params + i * nparams, params + lastParamVector * nparams);
		}

		TR.CalcFeaturesBunched(params, posteriors, tshift);

		int j;
		for(j = 0; j < tshift * TR.GetNumOuts(); j++)
			posteriors[j] = (*postSoftFunc)(posteriors[j], postSoftArg1, postSoftArg2, postSoftArg3);

		for(j = 0; j < tshift * TR.GetNumOuts(); j++)
			posteriors[j] = (*decSoftFunc)(posteriors[j], decSoftArg1, decSoftArg2, decSoftArg3);

		for(i = 0; i < tshift; i++)
		{
			if(TR.GetDelay() >= TR.GetTrapShift())    
				DE->ProcessFrame(posteriors + i * TR.GetNumOuts());
		}
	
        }
	else
	{
		int i;
		for(i = 0; i < bunchIdx; i++)
		{
			DE->ProcessFrame(params + i * actualParams->GetNParams());
		}
	}

	bunchIdx = 0;
	lastParamVector = -1;

	return true;
}

bool SpeechRec::ProcessOffline(data_format inpf, data_format outpf, void *inpSig, int sigNBytes, Mat<float> *inpMat, Mat<float> *outMat)
{
	assert((int)inpf < (int)outpf);
	assert(outMat || outpf == dfStrings);
	assert(inpMat || inpf == dfWaveform);

	Mat<float> *paramMat = 0;
	Mat<float> *posteriorsMat = 0;

	// waveform -> parameters
	int nFrames;
	if(inpf == dfWaveform)
	{
		if(!ConvertWaveformFormat(waveFormat, inpSig, sigNBytes, &waveform, &waveformLen))
			return false;

		nFrames = (waveformLen > actualParams->GetVectorSize() ? (waveformLen - actualParams->GetVectorSize()) / actualParams->GetStep() + 1 : 1);
				
		actualParams->AddWaveform(waveform, waveformLen);
			
		if(outpf == dfParams)
		{
			paramMat = outMat;
		}
		else
		{
			paramMat = new Mat<float>; 
			if(!paramMat)
			{
				MERROR("Insufficient memory\n");
				return false;
			}
		}
		if(actualParams->GetNParams() != paramMat->columns() || nFrames != paramMat->rows())
		   paramMat->init(nFrames, actualParams->GetNParams());

		int fr = 0;
		while(actualParams->GetFeatures(params))
		{				
			FrameBasedNormalization(params, actualParams->GetNParams());
			paramMat->set(fr, fr, 0, actualParams->GetNParams() - 1, params);
			fr++;
		}

		if(outpf == dfParams)
			return true;
	}

	// sentence based normalization
	if(inpf == dfWaveform || inpf == dfParams)
	{
		if(inpf == dfParams)
			paramMat = inpMat;

                if(paramMat->columns() < actualParams->GetNParams())
                {
			MERROR("Invalid dimensionality of parameter vectors\n");
			return false;
                }
		else if(paramMat->columns() > actualParams->GetNParams())
		{
			Mat<float> *tmpMat = new Mat<float>;
			tmpMat->init(paramMat->rows(), actualParams->GetNParams());
			tmpMat->copy(*paramMat, 0, paramMat->rows() - 1, 0, actualParams->GetNParams() - 1, 
                                                   0, paramMat->rows() - 1, 0, actualParams->GetNParams() - 1);
			delete paramMat;
			paramMat = tmpMat;
			inpMat = paramMat;
		}

		SentenceBasedNormalization(paramMat);
	}

	// parameters -> posteriors
	if(outpf == dfPosteriors && !mTrapsEnabled)
        {
		MERROR("The 'traps' module have to be enabled for generating posteriors\n");
		return false;
        }

	if((inpf == dfWaveform || inpf == dfParams) && mTrapsEnabled)
	{
		if(inpf == dfParams)
			paramMat = inpMat;

		if(outpf == dfPosteriors)
		{
			posteriorsMat = outMat;
		}
		else
		{
			posteriorsMat = new Mat<float>;
			if(!posteriorsMat)
			{
				if(inpf != dfParams)
						delete paramMat;
				MERROR("Insufficient memory\n");
				return false;
			}
		}

		nFrames = paramMat->rows();

		if(TR.GetNumOuts() != posteriorsMat->columns() || nFrames != posteriorsMat->rows())
			posteriorsMat->init(nFrames, TR.GetNumOuts());

		// first part - initialization
		int i;
		int trapShift = TR.GetTrapShift();
		int nparams = actualParams->GetNParams();
		if(nFrames >= trapShift)
		{
			TR.CalcFeaturesBunched((float *)paramMat->getMem(), posteriors, trapShift, false);
		}
		else
		{
			sCopy(nFrames * paramMat->columns(), params, (float *)paramMat->getMem());
			for(i = nFrames; i < TR.GetTrapShift(); i++)
				paramMat->extr(nFrames - 1, nFrames - 1, 0, nparams - 1, params + i * nparams);
			TR.CalcFeaturesBunched(params, posteriors, trapShift, false);
		}

		// second part - main block
		if(nFrames > trapShift)
			TR.CalcFeaturesBunched((float *)paramMat->getMem() + trapShift * nparams, (float *)posteriorsMat->getMem(), nFrames - trapShift);

		// last part - termination
		int n = (nFrames > trapShift ? trapShift : nFrames);
		for(i = 0; i < n; i++)
			paramMat->extr(nFrames - 1, nFrames - 1, 0, nparams - 1, params + i * nparams);
		TR.CalcFeaturesBunched(params, (float *)posteriorsMat->getMem() + (nFrames - n) * posteriorsMat->columns(), n);

		// softening function: posteriors -> posteriors/log. posteriors
                int nPost = posteriorsMat->columns();
		for(i = 0; i < nFrames; i++)
		{
			posteriorsMat->extr(i, i, 0, nPost - 1, posteriors);
			int j;
			for(j = 0; j < nPost; j++)
				posteriors[j] = (*postSoftFunc)(posteriors[j], postSoftArg1, postSoftArg2, postSoftArg3);
			posteriorsMat->set(i, i, 0, nPost - 1, posteriors);
		}

		if(inpf != dfParams)
			delete paramMat;

		if(outpf == dfPosteriors)
			return true;
	}

	// posteriors -> strings
	if(inpf == dfWaveform || inpf == dfParams || inpf == dfPosteriors)
	{
		if(inpf == dfPosteriors || (inpf == dfParams && !mTrapsEnabled))
			posteriorsMat = inpMat;

		nFrames = posteriorsMat->rows();
                int nPost = posteriorsMat->columns(); // TR.GetNumOuts()

		// softening function: posteriors -> log. posteriors
		int i;
		for(i = 0; i < nFrames; i++)
		{
			posteriorsMat->extr(i, i, 0, nPost - 1, posteriors);
			int j;
			for(j = 0; j < nPost; j++)
				posteriors[j] = (*decSoftFunc)(posteriors[j], decSoftArg1, decSoftArg2, decSoftArg3);
			posteriorsMat->set(i, i, 0, nPost - 1, posteriors);
		}

		// log posteriors -> strings
		for(i = 0; i <  nFrames; i++)
		{
			posteriorsMat->extr(i, i, 0, nPost - 1, posteriors);
			DE->ProcessFrame(posteriors);
		}

		if(inpf != dfPosteriors)
			delete posteriorsMat;
	}

	return true;
}

bool SpeechRec::ProcessFile(data_format inpf, data_format outpf, char *source, char *target, FILE *out_MLF)
{
	assert(outpf == dfStrings || target);

	char msg[1024];
	if(target)
		sprintf(msg, "%s -> %s\n", source, target); 
	else
		sprintf(msg, "%s\n", source);
	MLOG(msg);

	void *inpSig;
	int sigNBytes;
	Mat<float> inpMat;
	Mat<float> outMat;

	// Load data
	if(inpf == dfWaveform)
	{
		if(!LoadWaveform(source, &inpSig, &sigNBytes))
			return false;
	}

	if(inpf == dfParams || inpf == dfPosteriors)
	{
		if(!inpMat.loadHTK(source))
		{
			char msg[1024];
			sprintf(msg, "Can not open file: %s\n", source);
			MERROR(msg);
			return false;
		}
	}

	// Process data
	if(inpf == dfWaveform)                       
		actualParams->Reset();
	if((inpf == dfWaveform || inpf == dfParams) && (outpf == dfPosteriors || outpf == dfStrings)) 
		TR.Reset();
	if(outpf == dfStrings)
	{
        	if(out_MLF)
		{
			fprintf(out_MLF, "\"%s\"\n", target);
			DE->SetCallbackFunc(OnWordMLF, (void *)out_MLF);
			DE->Init();
		}
		else
		{
			if(target)
				DE->Init(target);
			else
				DE->Init();
		}
	}

	if(!ProcessOffline(inpf, outpf, inpSig, sigNBytes, &inpMat, &outMat))
	{
		if(inpf == dfWaveform)
			delete [] (char *)inpSig;
		return false;
	}

	if(outpf == dfStrings)
		DE->Done();

	if(out_MLF)
		fprintf(out_MLF, ".\n");


	// Save data and deallocate memory
	if(inpf == dfWaveform)
		delete [] (char *)inpSig;

	if(outpf ==  dfParams || outpf == dfPosteriors)
	{
		if(!outMat.saveHTK(target))
		{
			char msg[1024];
			sprintf(msg, "Can not create file: %s\n", target);
			MERROR(msg);
			return false;
		}
	}
	
	return true;
}

bool SpeechRec::ProcessFileListLine(data_format inpf, data_format outpf, char *line, FILE *fp_mlf)
{
	char file1[1024];
	char file2[1024];
	char sep[256];

	if(sscanf(line, "%[^ \n\r\t]%[ \t]%[^ \n\r\t]", file1, sep, file2) != 3)
	{
		if(sscanf(line, "%s", file1) != 1)
		{
			char msg[1024];
			sprintf(msg, "Invalid line in file list: %s\n", line);
			MERROR(msg);
			return false;
		}
		// compose target file name
		strcpy(file2, file1);
		switch(outpf)
		{
			case dfParams:
				ChangeFileSuffix(file2, C.GetString("params", "suffix"));
				break;
			case dfPosteriors:
				ChangeFileSuffix(file2, C.GetString("traps", "suffix"));
				break;
			case dfStrings:
				if(fp_mlf)
					CreateLabelFileNameForMLF(file1, file2);
				else
					ChangeFileSuffix(file2, C.GetString("labels", "suffix"));
				break;
			case dfWaveform:
				break;
			case dfUnknown:
				assert(0);
		}
	}

	// process one file
	if(!ProcessFile(inpf, outpf, file1, file2, fp_mlf))
		return false;

	return true;
}

bool SpeechRec::ProcessFileList(data_format inpf, data_format outpf, char *list, char *outMLFFile)
{
	assert((int)outpf > (int)inpf);

	// open file list
	FILE *fp_list = fopen(list, "r");
	if(!fp_list)
	{
		char msg[1024];
		sprintf(msg, "Can not open the file list: %s\n", list);
		MERROR(msg);
		return false;
	}

	// open MLF
	FILE *fp_mlf = 0;
	if(outMLFFile)
	{
		fp_mlf = fopen(outMLFFile, "w");
		if(!fp_mlf)
		{
			fclose(fp_list);
			char msg[1024];
			sprintf(msg, "Can not create the MLF: %s\n", outMLFFile);
			MERROR(msg);
			return false;
		}
		fprintf(fp_mlf, "#!MLF!#\n");
	}
	
	char line[1024];

	// process the list
	while(fgets(line, 1023, fp_list))
	{
		if(!ProcessFileListLine(inpf, outpf, line, fp_mlf))
			break;	
	}

	// close files
	if(fp_mlf)
		fclose(fp_mlf);

	fclose(fp_list);
	return true;
}

SpeechRec::wave_format SpeechRec::Str2WaveFormat(char *str)
{
	wave_format format = wfUnknown;

	if(strcmp(str, "lin16") == 0)
		format = wfLin16;
	else if(strcmp(str, "alaw") == 0)
		format = wfALaw;
	else
	{
		char msg[1024];
		sprintf(msg, "Invalid waveform format '%s'. Supported data formats are 'lin16' and 'alaw'.\n", str);
	    MERROR(msg);
	}
	return format;
}

SpeechRec::data_format SpeechRec::Str2DataFormat(char *str)
{
	data_format format = dfUnknown;
	
	if(strcmp(str, "wf") == 0)
		format = dfWaveform;
	else if(strcmp(str, "par") == 0)
		format = dfParams;
	else if(strcmp(str, "post") == 0)
		format = dfPosteriors;
	else if(strcmp(str, "str") == 0)
		format = dfStrings;
	else
	{
		char msg[1024];
		sprintf(msg, "Invalid data format '%s'. Supported data formats are 'wf', 'mb', 'post' and 'str'.\n", str);
		MERROR(msg);
	}
	return format;
}

SpeechRec::soft_func SpeechRec::Str2SoftFunc(char *str, float *a1, float *a2, float *a3)
{
        char func[256];

        *a1 = 0.0f;
        *a2 = 0.0f;
        *a3 = 0.0f;
        if(sscanf(str, "%255s %f %f %f", func, a1, a2, a3) !=  4)
        {
                char msg[1024];
                sprintf(msg, "Invalid softening function format '%s'. The format should be function identificator and three floating point arguments.\n", str);
                MERROR(msg);
                return &SoftLog;
        }

        soft_func pfunc = 0;
        if(strcmp(func, "none") == 0)
                pfunc = &SpeechRec::SoftNone;
        else if(strcmp(func, "log") == 0)
                pfunc = &SpeechRec::SoftLog;
        else if(strcmp(func, "igor") == 0)
                pfunc = &SpeechRec::SoftIgor;
        else if(strcmp(func, "gmm_bypass") == 0)
		pfunc = &SpeechRec::SoftGMMBypass;
        else
        {
                char msg[1024];
                sprintf(msg, "Unknow softening function '%s'. Supported softening functions are 'none', 'log', 'igor'.\n", str);
                MERROR(msg);
                return &SoftLog;
        }
        return pfunc;
}

int SpeechRec::WaveFormat2NBytesPerSample(wave_format format)
{
	switch(format)
	{
		case wfLin16:
			return sizeof(short);
		case wfALaw:
			return sizeof(char);
		case wfUnknown:
			assert(0);
	}
	return 0;
}

bool SpeechRec::CheckDataFormatConv(data_format from, data_format to)
{
	return ((int)to > (int)from ? true : false); 
}

bool SpeechRec::LoadWaveform(char *file, void **outWave, int *sigNBytes)
{
	assert(outWave && sigNBytes);

	FILE *fp;
	fp = fopen(file, "rb");
	if(!fp)
	{
		char msg[1024];
		sprintf(msg, "Can not open waveform file: %s\n", file);
		MERROR(msg);
		return false;
	}

	int len = FileLen(fp);
	char *buff = new char [len];
	if(!buff)
	{
		fclose(fp);
		MERROR("Insufficient memory\n");
		return false;
	}

	int r = read(fileno(fp), buff, len);
	if(r != len)
	{
		fclose(fp);
		char msg[1024];
		sprintf(msg, "Can not read file: %s\n", file);
		MERROR(msg);
		return false;
	}
	fclose(fp);

	*sigNBytes = len;
	*outWave = buff;

	return true;
}

void SpeechRec::CreateLabelFileNameForMLF(char *fileName, char *retLableFileName)
{
	char *suffix = C.GetString("labels", "suffix");
	bool remove_path = C.GetBool("labels", "remove_path");

	strcpy(retLableFileName, fileName);
	
	// replace dir separators /\ with the one used by system
	CorrectDirSeparator(retLableFileName, dirSep);
	ChangeFileSuffix(retLableFileName, suffix);
	if(remove_path)
		ChangeFilePath(retLableFileName, "*");
}

bool SpeechRec::RunLive()
{
	terminated = false;

	// allocate buffer
	char *formatStr = C.GetString("source", "format");
	wave_format iwformat = Str2WaveFormat(formatStr);
	if(iwformat == wfUnknown)
		return false;
	int NBytesPerSample = WaveFormat2NBytesPerSample(iwformat);
	assert(NBytesPerSample != 0);

	int len = C.GetInt("source", "sample_freq") / 8 * NBytesPerSample;
	char *buff = new char [len];
	if(!buff)
	{
		MERROR("Insufficient memory\n");
		return false;
	}

	// open waveform source
	try
	{
		WFS.setFormat(C.GetInt("source", "sample_freq"), 1, 8 * NBytesPerSample);
		WFS.open();
	}
	catch(wf_error &er)
	{
		delete [] buff;
		MERROR("Can not open waveform input device");
		return false;
	}

    // prepare classes
	actualParams->Reset();
	TR.Reset();
	DE->Init();
	ResetBunchBuff();

	// read data and process it
	while(!terminated)
	{
		WFS.read(buff, len);
		if(!ProcessOnline(buff, len, false))
			break;
	}

	DE->Done();

	// free buffer
	delete [] buff;
	return true;
}

void SpeechRec::SentenceBasedNormalization(Mat<float> *mat)
{
//        mat->saveAscii("c:\\before");

	// sentence mean and variance normalization
	bool mean_norm = C.GetBool("offlinenorm", "sent_mean_norm");
	bool var_norm = C.GetBool("offlinenorm", "sent_var_norm");

	if(mean_norm || var_norm)
	{
		Mat<float> mean;

		// mean calcualtion
		mat->sumColumns(mean);
		mean.div((float)mat->rows());

		// mean norm
		int i, j;
		for(i = 0; i < mat->columns(); i++)
			mat->sub(0, mat->rows() - 1, i, i, mean.get(0, i));

		if(var_norm)
		{
			// variance calculation
			Mat<float> var;
			var.init(mean.rows(), mean.columns());
			var.set(0.0f);
			for(i = 0; i < mat->columns(); i++)
			{
				for(j = 0; j < mat->rows(); j++)
				{
					float v = mat->get(j, i);
					var.add(0, i, v * v);
				}
			}
			var.div((float)mat->rows());
			var.sqrt();

			// lower threshold
			float lowerThr = C.GetFloat("melbanks", "sent_std_thr");
			var.lowerLimit(lowerThr);

			// variance norm
			for(i = 0; i < mat->columns(); i++)
				mat->mul(0, mat->rows() - 1, i, i, 1.0f / var.get(0, i));

			// add mean if not mean norm
			if(!mean_norm)
			{
				for(i = 0; i < mat->columns(); i++)
					mat->add(0, mat->rows() - 1, i, i, mean.get(0, i));
			}
		}
	}

	// sentence maximum normalization
	bool max_norm = C.GetBool("offlinenorm", "sent_max_norm");
	bool channel_max_norm = C.GetBool("offlinenorm", "sent_chmax_norm");

	if(max_norm || channel_max_norm)
	{
		Mat<float> max;
		max.init(1, mat->columns());
		max.set(-9999.9f);
		int i, j;

		for(i = 0; i < mat->columns(); i++)
		{
			for(j = 0; j < mat->rows(); j++)
			{
				float v = mat->get(j, i);
				if(v > max.get(0, i))
				{
				   max.set(0, i, v);
				}
			}
		}

		// global sentence maximum normalization
		if(max_norm)
		{
			float global_max = -9999.9f;
			for(i = 0; i < max.columns(); i++)
			{
				if(max.get(0, i) > global_max)
				{
				   global_max = max.get(0, i);
				}
				max.set(global_max);
			}
		}

		for(i = 0; i < mat->columns(); i++)
		{
			for(j = 0; j < mat->rows(); j++)
			{
	            		mat->set(j, i, mat->get(j, i) - max.get(0, i));
			}
		}
	}
}

void SpeechRec::FrameBasedNormalization(float *frame, int n)
{
	// shift of log-spectra
	float shift = C.GetFloat("framenorm", "shift");
	if(shift != 0.0f)
	{
		int i;
		for(i = 0; i < n; i++)
		{
			frame[i] += shift;
		}
	}

	// minimal floor of log-spectra
	float min_floor = C.GetFloat("framenorm", "min_floor");
	if(min_floor != -9999.9f)
	{
		int i;
		for(i = 0; i < n; i++)
		{
			if(frame[i] < min_floor)
			{
			   frame[i] = min_floor;
			}
		}
	}
}

SpeechRec::ParamKindType SpeechRec::Str2ParamKind(char *str)
{
   if(strcmp(str, "fbanks") == 0)
   {
      return pkFBanks;
   }
   else if(strcmp(str, "plp") == 0)
   {
      return pkPLP;
   }

   return pkUnknown;
}

