#include "stdlib.h"
#include "norm.h"
#include "sxmlparser.h"
#include <assert.h>
#include <math.h>
#include <string>
#include <string.h>
#include <climits>


ChannelNormParams::ChannelNormParams() :
   mFrameLength(0),
   mpX(0),
   mpX2(0),
   mpMeans(0),
   mpInvStds(0),
   mpGlobStds(0),
   mNFrames(0),
   mNormKind(NK_NONE)  
{
}

ChannelNormParams::~ChannelNormParams()
{
   if(mpMeans)
   {
      Free();
   }
}

void ChannelNormParams::Alloc(int FrameLength)
{
   mpMeans = new float [FrameLength];
   mpInvStds = new float [FrameLength];
   mpGlobStds = new float [FrameLength];
   mpX = new float [FrameLength];
   mpX2 = new float [FrameLength];
   mNFrames = 0;

   mFrameLength = FrameLength;
}

void ChannelNormParams::Free()
{
   if(mpMeans)
   {
      delete [] mpMeans;
      delete [] mpInvStds;
      delete [] mpGlobStds;
      delete [] mpX;
      delete [] mpX2;
      mpMeans = 0;
      mpInvStds = 0;
      mpX = 0;
      mpX2 = 0;
   }
}

void ChannelNormParams::Null()
{
   int i;
   for(i = 0; i < mFrameLength; i++)
   {
      mpMeans[i] = 0.0f;
      mpInvStds[i] = 1.0f;
      mpGlobStds[i] = 1.0f;
      mpX[i] = 0.0f;
      mpX2[i] = 0.0f;      
   }
}

int ChannelNormParams::GetFrameLength()
{
   return mFrameLength;
}

float *ChannelNormParams::GetMeans()
{
   return mpMeans;
}

float *ChannelNormParams::GetInvStds()
{
   return mpInvStds;
}

unsigned int ChannelNormParams::GetAccFramesNum()
{
   return mNFrames;
}

void ChannelNormParams::Accum(int Length, float *pFrame)
{
   // check frame length
   if(Length != mFrameLength)
   {
      Alloc(Length);
      Null();
      mNFrames = 0;
   }

   int i;
   for(i = 0; i < Length; i++)
   {
      mpX[i] += pFrame[i];
      mpX2[i] += pFrame[i] * pFrame[i];
   }
   mNFrames++;   
}

void ChannelNormParams::Norm(int Length, float *pFrame)
{
   // check frame length
   if(Length != mFrameLength)
   {
      Alloc(Length);
      Null();
      mNFrames = 0;
   }

   int i;
   for(i = 0; i < Length; i++)
   {
      if((mNormKind & NK_MEAN) != 0)
      {
         pFrame[i] -= mpMeans[i];
      }
      if((mNormKind & NK_VAR) != 0)
      {
         pFrame[i] *= mpInvStds[i];
         if((mNormKind & NK_SCALEGVAR) != 0)
         {
            pFrame[i] *= mpGlobStds[i];
         }
      }
   }
}

void ChannelNormParams::Update()
{
   int i;
   for(i = 0; i < mFrameLength; i++)
   {
      mpMeans[i] = mpX[i] / (float)mNFrames;
      mpInvStds[i] = 1.0f / sqrt(mpX2[i] / (float)mNFrames - mpMeans[i] * mpMeans[i]); 
   }
   mNFrames = UINT_MAX;
}

void ChannelNormParams::SetNorm(unsigned int Kind)
{
   assert(Kind == NK_NONE || (Kind & NK_MEAN) != 0 || 
         ((Kind & NK_VAR) != 0 && (Kind & NK_MEAN)));
   mNormKind = Kind;
}

void ChannelNormParams::SetMeans(float *pVct)
{
   int i;
   for(i = 0; i < mFrameLength; i++)
   {
      mpMeans[i] = pVct[i];
   }
}

void ChannelNormParams::SetInvStds(float *pVct)
{
   int i;
   for(i = 0; i < mFrameLength; i++)
   {
      mpInvStds[i] = pVct[i];
   }
}

void ChannelNormParams::SetGlobStds(float *pVct)
{
   int i;
   for(i = 0; i < mFrameLength; i++)
   {
      mpGlobStds[i] = pVct[i];
   }
}

// ----------------------------------------------------------------------------

Normalization::Normalization() :
   mActualChannel(0),
   mpActualParams(0),
   mNFramesForEstim(0),
   mMeanNorm(false),
   mVarNorm(false),
   mScaleToGVar(false)
{
   strcpy(mpFile, "none");
   SetChannel(0);
}

Normalization::~Normalization()
{
}

void Normalization::SetChannel(int Channel)
{
   mActualChannel = Channel;

   std::map<int, ChannelNormParams>::iterator it = mParams.find(mActualChannel);
   if(it == mParams.end())
   {
     ChannelNormParams params;
     mParams[mActualChannel] = params;
   }

   mpActualParams = &mParams[mActualChannel];
}

void Normalization::ProcessFrame(int Length, float *pFrame)
{
   if(mpActualParams->GetAccFramesNum() < mNFramesForEstim)
   {
      mpActualParams->Accum(Length, pFrame);
   }

   if(mNFramesForEstim != 0 && mpActualParams->GetAccFramesNum() == mNFramesForEstim )
   {
      mpActualParams->Update();
      if(mSignalEstimEnd)
      {
         printf("Estimation finished, channel %d\n", mActualChannel);
      }
      Save(mpFile);
   }

   mpActualParams->Norm(Length, pFrame);
}

void Normalization::StartEstimation(int NFrames)
{
   std::map<int, ChannelNormParams>::iterator it;

   for(it = mParams.begin(); it != mParams.end(); it++)
   {
      it->second.Null();
   }
   
   mNFramesForEstim = NFrames;  
}

void Normalization::SetFile(char *pFile)
{
   strcpy(mpFile, pFile);
  
   FILE *pf;
   pf = fopen(pFile, "rb");
   if(pf != 0)
   {
      fclose(pf);
      Load(pFile);
   }    
}

void Normalization::SetSignalEstimEnd(bool Signal)
{
   mSignalEstimEnd = Signal;
}

void Normalization::SetMeanNorm(bool Norm)
{
   mMeanNorm = Norm;
   UpdateNormFlags();
}

void Normalization::SetVarNorm(bool Norm)
{
   mVarNorm = Norm;
   UpdateNormFlags();
}

void Normalization::SetScaleToGVar(bool Flag)
{
   mScaleToGVar = Flag;
   UpdateNormFlags();
}

void Normalization::UpdateNormFlags()
{
   unsigned int flag = NK_NONE;

   if(mMeanNorm)
   {
      flag |= NK_MEAN; 
   }
   if(mVarNorm)
   {
      flag |= NK_VAR;
   }
   if(mScaleToGVar)
   {
      flag |= NK_SCALEGVAR;
   } 

   std::map<int, ChannelNormParams>::iterator it;

   for(it = mParams.begin(); it != mParams.end(); it++)
   {
      it->second.SetNorm(flag);
   }
}

void Normalization::Save(char *pFile)
{
   if(strlen(pFile) == 0 && strcmp(pFile, "none") != 0)
   {
      return;
   }

   SXMLDocument doc;
   SXMLNode &root_node = doc.GetRootNode();

   std::map<int, ChannelNormParams>::iterator it;
   char pbuff[10];

   for(it = mParams.begin(); it != mParams.end(); it++)
   {
      // channel node
      SXMLNode *pchannel_node = new SXMLNode;
      root_node.AddChild(pchannel_node);       
      pchannel_node->SetName("channel");
      sprintf(pbuff, "%d", it->first);
      (*pchannel_node->GetProperties())["id"] = pbuff;
      
      // means
      SXMLNode *pmeans_node = new SXMLNode;
      pchannel_node->AddChild(pmeans_node);
      pmeans_node->SetName("mean");

      std::string means = "";
      float *pmeans = it->second.GetMeans();
      int i;
      for(i = 0; i < it->second.GetFrameLength(); i++)
      {
         sprintf(pbuff, " %e", pmeans[i]);
         means += pbuff;
      }
      pmeans_node->SetText(means);

      // variances      
      SXMLNode *pvars_node = new SXMLNode;
      pchannel_node->AddChild(pvars_node);
      pvars_node->SetName("variance");

      std::string vars = "";
      float *pinv_stds = it->second.GetInvStds();
      for(i = 0; i < it->second.GetFrameLength(); i++)
      {
         float v = 1.0f / pinv_stds[i];
         sprintf(pbuff, " %e", v * v);
         vars += pbuff;
      }
      pvars_node->SetText(vars);

   }   

   doc.Save(pFile, SXMLT_NONE);
}

void Normalization::Load(char *pFile)
{
   if(strlen(pFile) == 0 && strcmp(pFile, "none") != 0)
   {
      return;
   }

   mParams.clear();

   SXMLDocument doc;
   doc.Load(pFile);
   SXMLNode &root_node = doc.GetRootNode();

   root_node.FirstChild();

   SXMLNode *pchannel_node;
   while((pchannel_node = root_node.GetChild()) != 0)
   {
      if(pchannel_node->GetName() == "channel")
      {
         int id = 0;
         sscanf((char *)(*pchannel_node->GetProperties())["id"].c_str(), "%d", &id);
         SetChannel(id);

         pchannel_node->FirstChild();

         SXMLNode *pchild_node;
         while((pchild_node = pchannel_node->GetChild()) != 0)
         {
            std::string name = pchild_node->GetName();
            std::string text = pchild_node->GetText();
            if(name == "mean")
            {
               float *pvct;
               int len;

               ParseFloatVector((char *)text.c_str(), &pvct, &len);

               if(len != mpActualParams->GetFrameLength())
               {
                  mpActualParams->Alloc(len);
                  mpActualParams->Null();
               }

               mpActualParams->SetMeans(pvct);
               delete [] pvct;
            }
            else if(name == "variance")
            {
               float *pvct;
               int len;

               ParseFloatVector((char *)text.c_str(), &pvct, &len);

               int i;
               for(i = 0; i < len; i++)
               {
                  pvct[i] = 1.0f / sqrt(pvct[i]);
               } 

               if(len != mpActualParams->GetFrameLength())
               {
                  mpActualParams->Alloc(len);
                  mpActualParams->Null();
               }

               mpActualParams->SetInvStds(pvct);
               delete [] pvct;
            }
            else if(name == "gvariance")
            {
               float *pvct;
               int len;

               ParseFloatVector((char *)text.c_str(), &pvct, &len);

               int i;
               for(i = 0; i < len; i++)
               {
                  pvct[i] = sqrt(pvct[i]);
               } 

               if(len != mpActualParams->GetFrameLength())
               {
                  mpActualParams->Alloc(len);
                  mpActualParams->Null();
               }

               mpActualParams->SetGlobStds(pvct);
               delete [] pvct;
            }
         }
      }  
   }
  
   SetChannel(0);
}


void Normalization::ParseFloatVector(char *pText, float **ppVct, int *pLength)
{
   // get number of floating number points in the vector
   char *pt = pText;
   *pLength = 0;

   while(*pt != '\0')
   {
      while(*pt == ' ' || *pt == '\t')
      { 
         pt++;
      }
      if(*pt != '\0')
      {
         (*pLength)++;     
      }
      while(*pt != '\0' && *pt != ' ' && *pt != '\t')
      {
         pt++;
      }
   }

   // alloc thr vector
   float *pvct = new float [*pLength];
   *ppVct = pvct;

   // parse string
   pt = pText;
   int i;
   for(i = 0; i < *pLength; i++)
   {
      while(*pt != '\0' && (*pt == ' ' || *pt == '\t'))
      {
         pt++;
      }
      if(sscanf(pt, "%f", &pvct[i]) != 1)
      {
         fprintf(stderr, "ERROR: Invalid online normalization parameters\n");
         exit(1);
      }
      while(*pt != '\0' && *pt != ' ' && *pt != '\t')
      {
         pt++;
      }
   }
}
