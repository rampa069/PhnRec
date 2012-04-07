#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <assert.h>
#include <list>
#include "phndec.h"


#define LOG_0_5 -0.69314718055994530941723212145818f

PhnDec::PhnDec() : 
  mPrevAlpha(0.0f),
  mTimePruning(50),
  mLogTrCurr(LOG_0_5),
  mLogTrNext(LOG_0_5),
  mNPhonemes(0),
  mpPhonemes(0),
  mNStPerPhn(1),
  mpLabFP(0)
{
  mode = DECMODE_DECODE;
}

PhnDec::~PhnDec()
{
  if(mpPhonemes != 0)
  {
    delete [] mpPhonemes;
  }
}

void PhnDec::SetMode(int m)
{
  assert(mode == DECMODE_DECODE);  // this decoder supports nothing more then decoding
  mode = m;
}

void PhnDec::SetCurrTrProb(float prob)
{
  mLogTrCurr = (float)log(prob);
  mLogTrNext = (float)log(1-prob);
}

int PhnDec::Init(char *pLabelFile)
{	
  assert(mpPhonemes);

  if(pLabelFile)
  {
	 if(mpLabFP)
	 {
	   fclose(mpLabFP);
	 }

     mpLabFP = fopen(pLabelFile, "w");
	 if(mpLabFP == 0)
	 {
	   return DECERR_OUTPUTFILE;
	 }
  }	

  mAlphas.init(mNPhonemes, mNStPerPhn + 1);
  mAlphas.set(-FLT_MAX);
  
  mPrevPhn.init(mNPhonemes, mNStPerPhn + 1);
  mPrevPhn.set(-1);

  mPhnLen.init(mNPhonemes, mNStPerPhn + 1);
  mPhnLen.set(0);

  mHistoryPhn.init(1, mTimePruning + 1);
  mHistoryPhn.set(-1);

  mHistoryLen.init(1, mTimePruning + 1);
  mHistoryLen.set(-1);

  mHistoryAlphas.init(1, mTimePruning + 1);
  mHistoryAlphas.set(-1);

  int i;
  for(i = 0; i < mNPhonemes; i++)
  {
    mAlphas.set(i, 0, wPenalty);  // word insertion penalty at the begin of phoneme string
	                              // this gives the same results as Lukas's decoder,
	                              // but I realy do not know why he use the penalty there
	mPrevPhn.set(i, 0, -1);
	mPhnLen.set(i, 0, 0);
  }

  mNFrames = 0;
  mPrevAlpha = 0.0f;

  return DECERR_NONE;
}

void PhnDec::PropagateInModels(float *pFrame)
{
  int i;
  for(i = 0; i < mNPhonemes; i++)
  {
	int j;
	for(j = mNStPerPhn; j > 0; j--)
	{
	  float tok_cur = mAlphas.get(i, j) + mLogTrCurr;
	  float tok_prev = mAlphas.get(i, j - 1) + mLogTrNext;
	  if(tok_cur > tok_prev)
	  {
	    mAlphas.set(i, j, tok_cur + pFrame[mPdfIndexes.get(i, j)]);
		mPhnLen.add(i, j, 1);
	  }
	  else
	  {
	    mAlphas.set(i, j, tok_prev + pFrame[mPdfIndexes.get(i, j)]);
		mPrevPhn.set(i, j, mPrevPhn.get(i, j - 1));
		mPhnLen.set(i, j, mPhnLen.get(i, j - 1) + 1);
	  }
	}
  }
}

void PhnDec::PropagateInNetwork()
{
  int i;
  float max = -FLT_MAX;
  int maxi = 0;
  for(i = 0; i < mNPhonemes; i++)
  {
    float tok = mAlphas.get(i, mNStPerPhn);
	if(tok > max)
	{
	  max = tok;
	  maxi = i;
	}
  }

  AddHistory(mPrevPhn.get(maxi, mNStPerPhn), mPhnLen.get(maxi, mNStPerPhn), max);

  for(i = 0; i < mNPhonemes; i++)
  {
	mAlphas.set(i, 0, max + wPenalty);
	mPrevPhn.set(i, 0, maxi);
	mPhnLen.set(i, 0, 0);
  }
}

void PhnDec::AddHistory(int phnIdx, int len, float alpha)
{
  int i;
  for(i = 1; i < mHistoryPhn.columns(); i++)
  {
    mHistoryPhn.set(0, i - 1, mHistoryPhn.get(0, i));
    mHistoryLen.set(0, i - 1, mHistoryLen.get(0, i));
	mHistoryAlphas.set(0, i - 1, mHistoryAlphas.get(0, i));
  }
  mHistoryPhn.set(0, mHistoryPhn.columns() - 1, phnIdx);
  mHistoryLen.set(0, mHistoryLen.columns() - 1, len);
  mHistoryAlphas.set(0, mHistoryAlphas.columns() - 1, alpha);
}

void PhnDec::ProcessFrame(float *pFrame)
{
  PropagateInModels(pFrame);
  PropagateInNetwork();
  mNFrames++;
  TimePruning();
  return;
}

void PhnDec::GetBestToken(int *pPrevPhn, int *pLen)
{
  float max = -FLT_MAX;
  *pLen = 1;
  *pPrevPhn = 0;
  int i;
  for(i = 0; i < mNPhonemes; i++)
  {
	int j;
	for(j = 1; j <= mNStPerPhn; j++)
	{
	  if(mAlphas.get(i, j) > max)
	  {
	    max = mAlphas.get(i, j);
		*pLen = mPhnLen.get(i, j);
		*pPrevPhn = mPrevPhn.get(i, j);
	  }
    }
  }
}


void PhnDec::TimePruning()
{
  int cols = mHistoryLen.columns();
  if(mNFrames >= cols)
  {
	int best_len;
	int prev_phn;
	
	GetBestToken(&prev_phn, &best_len);    
	int offs = cols - 1 - best_len;

	while(offs > 0)
	{
	  int l = mHistoryLen.get(0, offs);
	  prev_phn = mHistoryPhn.get(0, offs);
      offs -= l;
	}

	if(offs == 0)  // if there is a phoneme end at time of pruning boundary
	{
	  int end = mNFrames - cols + 1;
	  int start = end - mHistoryLen.get(0, 0);

      float like = mHistoryAlphas.get(0, 0) - mPrevAlpha;
	  mPrevAlpha = mHistoryAlphas.get(0, 0);

	  if(callbackFunc)
	  {
        (*callbackFunc)(DECMSG_WORD, 
                        (char *)mpPhonemes[prev_phn].c_str(),
                        (long_long)start * (long_long)100000l, 
                        (long_long)end * (long_long)100000l, 
                        like, 
                        callbackTmpParam);
	  }

	  float alizeStart,alizeEnd;
	  if(mpLabFP)
	  {
	    
	    if (mpPhonemes[prev_phn] != "pau"
	        && mpPhonemes[prev_phn] != "int"
	        && mpPhonemes[prev_phn] != "spk")
	    {
	    alizeStart=start;
	    alizeEnd=end;
	    fprintf(mpLabFP, "%.2f %.2f speech\n", alizeStart/100, alizeEnd/100);
	    }
          
	  }
	}
  }
}

float PhnDec::Done()
{

  // temporal list of LabelRec to revert phoneme history
  std::list<struct LabelRec> labels;

  int cols = mHistoryLen.columns();
  int offs = cols - 1;  
  int end = mNFrames;
  int phn = mPrevPhn.get(0, 0);  // conection node before phneme loop
    
  while(offs > 0 && phn != -1)
  {
    int len = mHistoryLen.get(0, offs);
    int start = end - len;
    float alpha = mHistoryAlphas.get(0, offs);
    int prev_phn = mHistoryPhn.get(0, offs);

    offs -= len;
    float like;
    if(offs > 0)
	{
      like = alpha - mHistoryAlphas.get(0, offs);
	}
	else
	{
	  like = alpha - mPrevAlpha;
	}

	LabelRec rec;
    rec.mPhn = phn;
	rec.mStart = start;
	rec.mEnd = end;
	rec.mLike = like;

	labels.push_front(rec);

    end = start;
    phn = prev_phn;
  }

  std::list<struct LabelRec>::iterator it;
  for(it = labels.begin(); it != labels.end(); it++)
  {
	if(callbackFunc)
	{
      (*callbackFunc)(DECMSG_WORD, 
                     (char *)mpPhonemes[it->mPhn].c_str(),
                     (long_long)it->mStart * (long_long)100000l, 
                     (long_long)it->mEnd * (long_long)100000l, 
                     it->mLike, 
                     callbackTmpParam);
	}


	  float alizeStart,alizeEnd;
	  if(mpLabFP)
	  {
	    
	    if (mpPhonemes[it->mPhn] != "pau"
	        && mpPhonemes[it->mPhn] != "int"
	        && mpPhonemes[it->mPhn] != "spk")
	    {
	    alizeStart=it->mStart;
	    alizeEnd=it->mEnd;
	    fprintf(mpLabFP, "%.2f %.2f speech\n", alizeStart/100, alizeEnd/100);
	    }
          
          }

  }

  if(mpLabFP)
  {
    fclose(mpLabFP);
	mpLabFP = 0;
  }
  return 0;
}


int PhnDec::LoadPhnList(char *pList)
{
  if(mpPhonemes != 0)
  {
    delete [] mpPhonemes;
  }

  FILE *pf;
  pf = fopen(pList, "r");
  if(pf == 0)
  {
    return DECERR_NETWORK;
  }

  // load phonemes to a list
  std::list<std::string> phonemes; 
  int i;

  char pbuff[256]; 
  while(fgets(pbuff, 255, pf) != 0)
  {
    pbuff[255] = '\0';
    for(i = 0; i < (int)strlen(pbuff); i++)
    {
      if(pbuff[i] == '\n' || pbuff[i] == '\r')
      {
        pbuff[i] = '\0';
      }
    }
    phonemes.push_back(pbuff);
  }  

  // copy phonemes to an array for faster access
  mNPhonemes = phonemes.size();
  mpPhonemes = new std::string[mNPhonemes];

  std::list<std::string>::iterator it;
  for(it = phonemes.begin(), i = 0; it != phonemes.end(); it++, i++)
  {
    mpPhonemes[i] = *it;
  }

  CreatePdfIndexes();

  return DECERR_NONE;
}

void PhnDec::CreatePdfIndexes()
{
  mPdfIndexes.init(mNPhonemes, mNStPerPhn + 1);
  
  int i;
  int j;
  int offs = 0;

  for(i = 0; i < mNPhonemes; i++)
  {
    for(j = 1; j <= mNStPerPhn; j++)
	{
	  mPdfIndexes.set(i, j, offs);
	  offs++;
	}
  }
}
