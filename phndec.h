#ifndef PHNDEC_H
#define PHNDEC_H

#include "decoder.h"
#include <string>
#include "matrix.h"

class PhnDec : public Decoder
{
  protected:
    struct LabelRec
	{
      int mPhn;
	  int mStart;
	  int mEnd;
	  float mLike;
	};
	  
	Mat<float> mAlphas;
    Mat<int> mPrevPhn;
	Mat<int> mPhnLen;
	Mat<int> mHistoryPhn;
	Mat<int> mHistoryLen;
	Mat<float> mHistoryAlphas;
	Mat<int> mPdfIndexes;
	float mPrevAlpha;
	int mpHistory;
    int mTimePruning;
    int mNFrames;
	float mLogTrCurr;
	float mLogTrNext;
    int mNPhonemes;
    std::string *mpPhonemes;
    int mNStPerPhn;
	FILE *mpLabFP;
	void PropagateInModels(float *pFrame);
	void PropagateInNetwork();
    void AddHistory(int phnIdx, int len, float alpha);
    void GetBestToken(int *pPrevPhn, int *pLen);
    void TimePruning();
	void CreatePdfIndexes();

  public:
    PhnDec();
    ~PhnDec();
    int Init(char *pLabelFile = 0);
    void ProcessFrame(float *pFrame);
    float Done();
    int LoadPhnList(char *pList);
    void SetMode(int m);
    void SetNStPerPhn(int n) {mNStPerPhn = n;}
	void SetCurrTrProb(float prob);
	void SetTimePruning(int n){mTimePruning = n;}
};

#endif
