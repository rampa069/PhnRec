#ifndef _NORM_H
#define _NORM_H

#include <map>

// normalization kinds
#define NK_NONE      0
#define NK_MEAN      1
#define NK_VAR       2
#define NK_SCALEGVAR 4

class ChannelNormParams
{
   public:
      ChannelNormParams();
      ~ChannelNormParams();
      void Alloc(int FrameLength);
      void Null();
      int GetFrameLength();
      unsigned int GetAccFramesNum();
      void Accum(int Length, float *pFrame);
      void Norm(int Length, float *pFrame); 
      void Update();
      void SetNorm(unsigned int Kind);
      float *GetMeans();
      float *GetInvStds();
      void SetMeans(float *pVct);
      void SetInvStds(float *pVct);
      void SetGlobStds(float *pVct);

   protected:
      int mFrameLength;
      float *mpX;
      float *mpX2;
      float *mpMeans;
      float *mpInvStds;
      float *mpGlobStds;
      unsigned int mNFrames;    
      unsigned int mNormKind; 
 
      void Free();          
};

class Normalization
{
   public:
      Normalization();
      ~Normalization();
      void SetChannel(int Channel);
      void ProcessFrame(int Length, float *pFrame);
      void StartEstimation(int NFrames);
      void SetFile(char *pFile);
      void SetSignalEstimEnd(bool Signal);
      void SetMeanNorm(bool Norm);
      void SetVarNorm(bool Norm);
      void SetScaleToGVar(bool Flag);
      void Save(char *pFile); 
      void Load(char *pFile);

   protected:
      int mActualChannel;
      ChannelNormParams *mpActualParams;
      std::map<int, ChannelNormParams> mParams;
      unsigned int mNFramesForEstim;
      char mpFile[1024];
      bool mSignalEstimEnd;
      bool mMeanNorm;
      bool mVarNorm;
      bool mScaleToGVar;

      void UpdateNormFlags();
      void ParseFloatVector(char *pText, float **ppVct, int *pLength);
};

#endif
