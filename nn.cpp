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
#include "nn.h"
#include "fexp.h"

#ifdef USE_BLAS
extern "C"
{ 
   #include "cblas.h"
}
#endif

#ifdef WIN32
   #define NN_SLASH '\\'
#else
   #define NN_SLASH '/' 
#endif

NeuralNet::NeuralNet() :
   mUseNorms(false),
   mAllocated(false),
   mBunchSize(0)
{
}

NeuralNet::~NeuralNet()
{
   if(mAllocated)
   {
      Free();
   }
}

char *NeuralNet::SkipSpaces(char *pText)
{
   char *pt = pText;
   while(pt != '\0' && strchr(" \t\n\r", *pt) != 0)
   {
      pt++;
   }
   return pt;
}

char *NeuralNet::SkipText(char *pText)
{
   char *pt = pText;
   while(pt != '\0' && strchr(" \t\n\r", *pt) == 0)
   {
      pt++;
   }
   return pt;
}

char *NeuralNet::SkipLines(char *pText, int n)
{
   char *pt = pText;
   int line = 0;
   while(pt != '\0' && line != n)
   {
      if(*pt == '\n')
      {
         line++;
      }
      pt++;
   }
   
   if(pt == '\0' && line != n)
   {
      return 0;
   }
   
   return pt;
}

char *NeuralNet::LoadText(char *pFile)
{
   FILE *pf;
   pf = fopen(pFile, "rb");
   if(pf == 0)
   {
      return 0;
   }
   
   int len = FileLen(pf); 
   char *ptext = new char[len + 1];
   if(!ptext)
   {
      fclose(pf);
      return 0;
   }
   
   if((int)fread(ptext, sizeof(char), len, pf) != len)
   {
      fclose(pf);
      delete [] ptext;
      return 0;
   }
   
   fclose(pf);   
   ptext[len] = '\0';
   return ptext;    
}

bool NeuralNet::GetInfo(char *pData, int *pNInp, int *pNHid, int *pNOut)
{
   // input to hidden matrix
   char *pt = pData;
   pt = SkipSpaces(pt);
   if(strncmp(pt, "weigvec", strlen("weigvec")) != 0)
   {
      return false;
   }
   pt += strlen("weigvec");
   pt = SkipSpaces(pt);
   int n_inp_hid;
   if(!GetIntValue(pt, &n_inp_hid))
   {
      return false;
   }    
   pt = SkipLines(pt, n_inp_hid + 1);  // goto next line and skip n_inp_hid lines
   if(pt == 0)
   {
      return false;
   }

   // hidden to output matrix
   if(strncmp(pt, "weigvec", strlen("weigvec")) != 0)
   {
      return false;
   }
   pt += strlen("weigvec");
   pt = SkipSpaces(pt);
   int n_hid_out;
   if(!GetIntValue(pt, &n_hid_out))
   {
      return false;
   }    
   pt = SkipLines(pt, n_hid_out + 1);  // goto next line and skip n_hid_out lines
   if(pt == 0)
   {
      return false;
   }
    
   // hidden biases
   if(strncmp(pt, "biasvec", strlen("biasvec")) != 0)
   {
      return false;
   }
   pt += strlen("biasvec");   
   pt = SkipSpaces(pt);
   int n_biases_hid;
   if(sscanf(pt, "%d", &n_biases_hid) != 1)
   {
      return false;
   }    
   pt = SkipLines(pt, n_biases_hid + 1);  // goto next line and skip n_biases_hid lines
   if(pt == 0)
   {
      return false;
   }

   // output biases 
   if(strncmp(pt, "biasvec", strlen("biasvec")) != 0)
   {
      return false;
   }
   pt += strlen("biasvec");   
   pt = SkipSpaces(pt);
   int n_biases_out;
   if(sscanf(pt, "%d", &n_biases_out) != 1)
   {
      return false;
   }    
   pt = SkipLines(pt, n_biases_out + 1);  // goto next line and skip n_biases_out lines
   if(pt == 0)
   {
      return false;
   }    

   *pNOut = n_biases_out;
   *pNHid = n_biases_hid;
   *pNInp = n_inp_hid / n_biases_hid;

   return true;
}

bool NeuralNet::ParseWeights(char *pData)
{
   // null matrixes
   memset((void *)mpInpHidMatrix, 0, mNHid16 * mNInp16 * sizeof(float));
   memset((void *)mpHidOutMatrix, 0, mNOut16 * mNHid16 * sizeof(float));    
   memset((void *)mpHiddenBiases, 0, mNHid16 * sizeof(float));
   memset((void *)mpOutputBiases, 0, mNOut16 * sizeof(float));    
    
   // input to hidden matrix
   char *pt = pData;
   pt = SkipSpaces(pt);
   if(strncmp(pt, "weigvec", strlen("weigvec")) != 0)
   {
      return false;
   }

   pt += strlen("weigvec");
   pt = SkipSpaces(pt);
   int n_inp_hid;
   if(!GetIntValue(pt, &n_inp_hid))
   {
      return false;
   }    
   pt = SkipText(pt);

   float *pmat = mpInpHidMatrix16;
   int i;
   for(i = 0; i < mNHid; i++)
   {
      int j;
      for(j = 0; j < mNInp; j++)
      {
         pt = SkipSpaces(pt);
         if(!GetFloatValue(pt, pmat))
         {
            return false;
         }
         pmat++;
         pt = SkipText(pt);
      }
      for(j = mNInp; j < mNInp16; j++)
      {
         *pmat = 0.0f;
         pmat++;
      }
   }
   
   // hidden to output matrix
   pt = SkipSpaces(pt);
   if(strncmp(pt, "weigvec", strlen("weigvec")) != 0)
   {
      return false;
   }

   pt += strlen("weigvec");
   pt = SkipSpaces(pt);
   int n_hid_out;
   if(!GetIntValue(pt, &n_hid_out))
   {
      return false;
   }    
   pt = SkipText(pt);

   pmat = mpHidOutMatrix16;
   for(i = 0; i < mNOut; i++)
   {
      int j;
      for(j = 0; j < mNHid; j++)
      {
         pt = SkipSpaces(pt);            
         if(!GetFloatValue(pt, pmat))
         {
            return false;
         }
         pmat++;
         pt = SkipText(pt);
      }
      for(j = mNHid; j < mNHid16; j++)
      {
         *pmat = 0.0f;
         pmat++;
      }
   }
  
   // hidden biases
   pt = SkipSpaces(pt);
   if(strncmp(pt, "biasvec", strlen("biasvec")) != 0)
   {
      return false;
   }
   pt += strlen("biasvec");   
   pt = SkipSpaces(pt);
   int n_biases_hid;
   if(!GetIntValue(pt, &n_biases_hid))
   {
      return false;
   }    
   pt = SkipText(pt);

   float *pvec = mpHiddenBiases16;
   for(i = 0; i < mNHid; i++)
   {
      pt = SkipSpaces(pt);      
      if(!GetFloatValue(pt, pvec))
      {
         return false;
      }
      pvec++;
      pt = SkipText(pt);
   }

   // output biases 
   pt = SkipSpaces(pt);
   if(strncmp(pt, "biasvec", strlen("biasvec")) != 0)
   {
      return false;
   }
   pt += strlen("biasvec");   
   pt = SkipSpaces(pt);
   int n_biases_out;
   if(!GetIntValue(pt, &n_biases_out))
   {
      return false;
   }    
   pt = SkipText(pt);

   pvec = mpOutputBiases16;
   for(i = 0; i < mNOut; i++)
   {
      pt = SkipSpaces(pt);      
      if(!GetFloatValue(pt, pvec))
      {
         return false;
      }
      pvec++;
      pt = SkipText(pt);
   }
   
   return true;
}

bool NeuralNet::ParseNorms(char *pData)
{
   // null vectors
   int i;
   for(i = 0; i < mNInp16; i++)
   {
     mpNormMeans16[i] = 0.0f;
     mpNormDevs16[i] = 1.0f;
   }
    
   // mean vector
   char *pt = pData;
   pt = SkipSpaces(pt);
   if(strncmp(pt, "vec", strlen("vec")) != 0)
   {
      return false;
   }
   pt += strlen("vec");
   pt = SkipSpaces(pt);
   int len;
   if(!GetIntValue(pt, &len))
   {
      return false;
   }    
   pt = SkipText(pt);

   float *pvct = mpNormMeans16;
   for(i = 0; i < mNInp; i++)
   {
      pt = SkipSpaces(pt);
      if(!GetFloatValue(pt, pvct))
      {
         return false;
      }
      pvct++;
      pt = SkipText(pt);
   }   

   // standard deviation vector
   pt = SkipSpaces(pt);
   if(strncmp(pt, "vec", strlen("vec")) != 0)
   {
      return false;
   }
   pt += strlen("vec");
   pt = SkipSpaces(pt);
   if(!GetIntValue(pt, &len))
   {
      return false;
   }    
   pt = SkipText(pt);

   pvct = mpNormDevs16;
   for(i = 0; i < mNInp; i++)
   {
      pt = SkipSpaces(pt);
      if(!GetFloatValue(pt, pvct))
      {
         return false;
      }
      pvct++;
      pt = SkipText(pt);
   }
   
//   puts("means");
//   DumpMatrix(mpNormMeans16, 1, mNInp16, mNInp16);

//   puts("devs");
//   DumpMatrix(mpNormDevs16, 1, mNInp16, mNInp16);
   
   
   return true;
}

int NeuralNet::LoadAscii(char *pWeightFile, char *pNormFile, int bunchSize)
{
   mBunchSize = bunchSize; 

   // Weights
   char *pweights = LoadText(pWeightFile);
   if(pweights == 0)
   {
      return NN_NOWEIGHTS;
   }
   
   if(!GetInfo(pweights, &mNInp, &mNHid, &mNOut))
   {
      delete [] pweights;
      return NN_BADWEIGHTS;
   }

   Alloc();
   
   if(!ParseWeights(pweights))
   {
      delete [] pweights;
      return NN_BADWEIGHTS;   
   }

   delete [] pweights;

   // Norms   
   if(pNormFile)
   {
      char *pnorms = LoadText(pNormFile);
      if(pnorms == 0)
      {
         delete [] pweights;
         return NN_NONORMS;
      }
      
      if(!ParseNorms(pnorms))
      {
         delete [] pweights;
         return NN_BADWEIGHTS;   
      }
      
      mUseNorms = true;
      delete [] pnorms;
   }
      
   return NN_OK;
}

int NeuralNet::LoadBinary(char *pFile, int bunchSize)
{
   mBunchSize = bunchSize;

   FILE *pf;
   pf = fopen(pFile, "rb");
   if(pf == 0)
   {
      return NN_NOWEIGHTS;
   }

   // number of layers (2 layers of neurons + 1 input layer)
   int nlayers = 2;
   if(fread(&nlayers, sizeof(int), 1, pf) != 1 || nlayers != 2)
   {
      return NN_BADWEIGHTS;
   }

   // sizes of layers
   int size[3];
   if((int)fread(size, sizeof(int), nlayers + 1, pf) != nlayers + 1)
   {
      return NN_WRITEERR;
   }
   mNInp = size[0];
   mNHid = size[1];
   mNOut = size[2];

   // allocate memory
   Alloc();

   // read weights
   if((int)fread(mpInpHidMatrix16, sizeof(float), mNInp16 * mNHid16, pf) != mNInp16 * mNHid16)
   {
      return NN_WRITEERR;
   }
   if((int)fread(mpHidOutMatrix16, sizeof(float), mNHid16 * mNOut16, pf) != mNHid16 * mNOut16)
   {
      return NN_WRITEERR;
   }

   // read biasses
   if((int)fread(mpHiddenBiases16, sizeof(float), mNHid16, pf) != mNHid16)
   {
      return NN_WRITEERR;
   }

   if((int)fread(mpOutputBiases16, sizeof(float), mNOut16, pf) != mNOut16)            
   {
      return NN_WRITEERR;
   }

   // read means and deviations
   if((int)fread(mpNormMeans16, sizeof(float), mNInp16, pf) != mNInp16)
   {
      return NN_WRITEERR;
   }
   if((int)fread(mpNormDevs16, sizeof(float), mNInp16, pf) != mNInp16)
   {
      return NN_WRITEERR;
   }

   fclose(pf);

   mUseNorms = true;

   return NN_OK;
}

int NeuralNet::SaveBinary(char *pFile)
{
   FILE *pf;
   pf = fopen(pFile, "wb");
   if(pf == 0)
   {
      return NN_CREATEERR;
   }

   // number of layers (2 layers of neurons + 1 input layer)
   int nlayers = 2;
   if((int)fwrite(&nlayers, sizeof(int), 1, pf) != 1)
   {
      return NN_WRITEERR;
   }

   // sizes of layers
   int size[3];
   size[0] = mNInp;
   size[1] = mNHid;
   size[2] = mNOut;
   if((int)fwrite(size, sizeof(int), nlayers + 1, pf) != nlayers + 1)
   {
      return NN_WRITEERR;
   }

   // save weights
   if((int)fwrite(mpInpHidMatrix16, sizeof(float), mNInp16 * mNHid16, pf) != mNInp16 * mNHid16)
   {
      return NN_WRITEERR;
   }
   if((int)fwrite(mpHidOutMatrix16, sizeof(float), mNHid16 * mNOut16, pf) != mNHid16 * mNOut16)
   {
      return NN_WRITEERR;
   }

   // save biasses
   if((int)fwrite(mpHiddenBiases16, sizeof(float), mNHid16, pf) != mNHid16)
   {
      return NN_WRITEERR;
   }

   if((int)fwrite(mpOutputBiases16, sizeof(float), mNOut16, pf) != mNOut16)            
   {
      return NN_WRITEERR;
   }

   // save means and deviations
   if((int)fwrite(mpNormMeans16, sizeof(float), mNInp16, pf) != mNInp16)
   {
      return NN_WRITEERR;
   }
   if((int)fwrite(mpNormDevs16, sizeof(float), mNInp16, pf) != mNInp16)
   {
      return NN_WRITEERR;
   }

   fclose(pf);
   return NN_OK;
}

int NeuralNet::Load(char *pWeightFile, char *pNormFile, int bunchSize)
{
   // cut off suffix and add '.nbin'
   char pbin_file[1024];
   strcpy(pbin_file, pWeightFile);
   char *pdot = strrchr(pbin_file, '.');
   char *pslash = strrchr(pbin_file, NN_SLASH);   
   if(pdot != 0 && (pslash == 0 || pdot > pslash))
   {
	   *pdot = '\0';
   }
   strcat(pbin_file, ".nbin");

   // load binary if the file exists
   if(LoadBinary(pbin_file, bunchSize) == NN_OK)
   {
      return NN_OK;
   }

   // load ascii otherwise and save it binary 
   int ret = LoadAscii(pWeightFile, pNormFile, bunchSize);
   if(ret == NN_OK)
   {
      SaveBinary(pbin_file);
   }

   return ret; 
}

float *NeuralNet::MemAlign16(float *pBuff)
{
   char *pbuff = (char *)pBuff;
   while(((unsigned int)pbuff & 0xF) != 0)
   {
      pbuff++;
   }
   return (float *)pbuff;
}

int NeuralNet::Align16(int n)
{
    while((n & 0xF) != 0)
    {
       n++;
    }
    return n;
}

void NeuralNet::Alloc()
{
   if(mAllocated)
   {
      Free();
   }
   
   mNInp16 = Align16(mNInp * sizeof(float)) / sizeof(float);
   mNHid16 = Align16(mNHid * sizeof(float)) / sizeof(float);
   mNOut16 = Align16(mNOut * sizeof(float)) / sizeof(float);

   mpInputPatterns = new float[mBunchSize * mNInp16 + 0xF];
   mpHiddenPatterns = new float[mBunchSize * mNHid16 + 0xF];
   mpOutputPatterns = new float[mBunchSize * mNOut16 + 0xF];

   mpInputPatterns16 = MemAlign16(mpInputPatterns);
   mpHiddenPatterns16 = MemAlign16(mpHiddenPatterns);
   mpOutputPatterns16 = MemAlign16(mpOutputPatterns); 

   // mNInp16 * mNHid16: calculate more (imaginar) neurons in the hidden 
   // layer to bypass copping of patterns between hidden and output layer
   mpInpHidMatrix = new float [mNHid16 * mNInp16 + 0xF]; 
   mpHidOutMatrix = new float [mNOut16 * mNHid16 + 0xF];

   mpInpHidMatrix16 = MemAlign16(mpInpHidMatrix);
   mpHidOutMatrix16 = MemAlign16(mpHidOutMatrix);            

   mpHiddenBiases = new float [mNHid16 + 0xF];
   mpOutputBiases = new float [mNOut16 + 0xF];            
   
   mpHiddenBiases16 = MemAlign16(mpHiddenBiases);
   mpOutputBiases16 = MemAlign16(mpOutputBiases);           

   mpNormMeans = new float [mNInp16 + 0xF];
   mpNormDevs = new float [mNInp16 + 0xF];

   mpNormMeans16 = MemAlign16(mpNormMeans);  
   mpNormDevs16 = MemAlign16(mpNormDevs);
      
   mAllocated = true;
}

void NeuralNet::Free()
{
   delete [] mpInputPatterns;
   delete [] mpHiddenPatterns;
   delete [] mpOutputPatterns;

   delete [] mpInpHidMatrix;
   delete [] mpHidOutMatrix;

   delete [] mpHiddenBiases;
   delete [] mpOutputBiases;            

   delete [] mpNormMeans;
   delete [] mpNormDevs;

   mAllocated = false;
}

void NeuralNet::Normalize(int bunchOccupation, float *pData, int len, float *pMeans, float *pDevs)
{
   float *pdata = pData;
   int i;
   for(i = 0; i < bunchOccupation; i++)
   {
      int j;
      for(j = 0; j < len; j++)
      {
         pdata[j] -= pMeans[j];
         pdata[j] *= pDevs[j];
      }         
      pdata += len;
   }
}

#ifdef USE_BLAS
// C = A * TR(B) + C
// BLAS version
void NeuralNet::MatrixMultiplyAndAdd(int nARows, int nACols, int nTRBRows, float *pA, float *pB, float *pC)
{
   if(nARows >= NN_BLAS_MINBUNCH)   // use Blas matrix rutine
   {
      // C = aplha * A + TR(B) + beta * C
      // args: matrix format (a11 a12 ... ; a21 a22 ... ; ...)
      //       A matrix preprocessing (NONE)
      //       B matrix preprocessing (TR)
      //       m, n, k - matrix dimensions
      //       alpha = 1.0
      //       A = (m x k)
      //       lda - length of one row (this values is added if the pointer is moved from
      //              the begin of one row to begin of next row)
      //       B = (k x n)
      //       ldb
      //       beta = 1.0
      //       C = (m x n)
      //       ldc
      cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, nARows, nTRBRows, nACols,
	              1.0f, pA, nACols, pB, nACols /* B is stored transposed */, 1.0f, pC, nTRBRows);
   }
   else          // calculate matrix multiplication vector by vector
                 // this should be faster for small amount of vectors according Standa
   {
      int i;
      for(i = 0; i < nARows; i++)
      {
         // y = alpha * A * x + beta * y
         // args: matrix format (a11 a12 ... ; a21 a22 ... ; ...)
         //       A matrix preprocessing (NONE)
         //       n, m  - matrix dimensions
         //       alpha = 1
         //       A = (m x n)
         //       lda
         //       x
         //       incx = 1
         //       beta
         //       y
         //       incy = 1
         cblas_sgemv(CblasRowMajor, CblasNoTrans, nTRBRows, nACols,
	                1.0f, pB, nACols, pA + (i * nACols), 1, 1.0f, pC + (i * nTRBRows), 1);
      }
   }
}
#else

#warning "Compilation without BLAS acceleration!"

// C = A * TR(B) + C
// unaccelerated version
void NeuralNet::MatrixMultiplyAndAdd(int nARows, int nACols, int nTRBRows, float *pA, float *pB, float *pC)
{
   int i;
   int j;
   int k;

   float *p_a = pA;
   float *p_c = pC;
   for(i = 0; i < nARows; i++)
   {
      float *p_b = pB;
	  for(j = 0; j < nTRBRows; j++)
      {
         for(k = 0; k < nACols; k++)
		 {
	        *p_c += p_a[k] * p_b[k];
		 }
		 p_c++;
		 p_b += nACols;
	  }
	  p_a += nACols;
   }
}
#endif

void NeuralNet::Sigmoid(int bunchOccupation, float *pData, int len, int lenAligned)
{
   float *pdata = pData;
   int i;
   for(i = 0; i < bunchOccupation; i++)
   {
      // Sigmoid
      int j;
      for(j = 0; j < len; j++)
      {
         #ifdef NN_FAST_EXP
            *pdata = fexp_sigmoid(*pdata);
         #else   
            *pdata = 1.0f / (1.0f + expf(-(*pdata)));
         #endif
         pdata++;
      }
      // null values for alligment
      for(j = len; j < lenAligned; j++)
      {
         *pdata = 0.0f;
         pdata++;      
      }
   }
}

void NeuralNet::SoftMax(int bunchOccupation, float *pData, int len, int lenAligned)
{
   float *pdata = pData;
   int i;
   for(i = 0; i < bunchOccupation; i++)
   {
      // Softmax
      int j;
      #ifdef NN_FAST_EXP
         fexp_softmax_v(len, pdata);
      #else
         float sum = 0.0f;
         for(j = 0; j < len; j++)
         {
            pdata[j] = expf(pdata[j]);    
            sum += pdata[j];
         }
         sum = 1.0f / sum;
         for(j = 0; j < len; j++)
         {
            pdata[j] *= sum;
         }
      #endif

      pdata += len;

      // null values for alligment      
      for(j = len; j < lenAligned; j++)
      {
         *pdata = 0.0f;
         pdata++;      
      }
   }
}

void NeuralNet::PrepareBiases(int bunchOccupation, int len, float *pBiases, float *pPatterns)
{
   float *ppatterns = pPatterns;
   int i;
   for(i = 0; i < bunchOccupation; i++)
   {
      int j;
      for(j = 0; j < len; j++)
      {
         *ppatterns = pBiases[j];
         ppatterns++;
      }
   }
}

void NeuralNet::ForwardPass1Bunch(int bunchOccupation, float *pInp, float *pOut)
{
//   DumpMatrix(mpInputPatterns16, bunchOccupation, mNInp16, mNInp16);

   // normalize features
   if(mUseNorms)
   {
      Normalize(bunchOccupation, pInp, mNInp16, mpNormMeans16, mpNormDevs16);
   }
   
   // forward pass to hidden layer
   PrepareBiases(bunchOccupation, mNHid16, mpHiddenBiases16, mpHiddenPatterns16);
   MatrixMultiplyAndAdd(bunchOccupation, mNInp16, mNHid16, 
                        mpInputPatterns16, mpInpHidMatrix16, mpHiddenPatterns16);  
      
   Sigmoid(bunchOccupation, mpHiddenPatterns16, mNHid, mNHid16);

   // forward pass to output
   PrepareBiases(bunchOccupation, mNOut16, mpOutputBiases16, mpOutputPatterns16);
   MatrixMultiplyAndAdd(bunchOccupation, mNHid16, mNOut16, 
                        mpHiddenPatterns16, mpHidOutMatrix16, mpOutputPatterns16);  

//   puts("xxx - out");
//   DumpMatrix(mpOutputPatterns16, bunchOccupation, mNOut, mNOut16);
   SoftMax(bunchOccupation, mpOutputPatterns16, mNOut, mNOut16);
//   puts("xxx - out - softmax");
//   DumpMatrix(mpOutputPatterns16, bunchOccupation, mNOut, mNOut16);
}

void NeuralNet::Forward(float *pInp, float *pOut, int nVct)
{
   int n_bunches = nVct / mBunchSize; // number of the whole bunches
   int n_rem = nVct % mBunchSize;     // number of vectors in the last bunch

   int inp_bunch_len = mNInp * mBunchSize;
   int out_bunch_len = mNOut * mBunchSize;

   // process vectors in bunches
   int i;
   int j;
   for(i = 0; i < n_bunches; ++i)
   {
      // - copy unaligned memory block to aligned memory block 
      for(j = 0; j < mBunchSize; j++)
      {
         memcpy(mpInputPatterns16 + j * mNInp16, pInp + i * inp_bunch_len + j * mNInp, mNInp * sizeof(float));
         memset(mpInputPatterns16 + j * mNInp16 + mNInp, 0, (mNInp16 - mNInp) * sizeof(float));
      }
      
      // - forward pass of one full bunch
      ForwardPass1Bunch(mBunchSize, mpInputPatterns16, mpOutputPatterns16);
      
      // - copy output data to unaligned memory block
      for(j = 0; j < mBunchSize; j++)
      {
         memcpy(pOut + i * out_bunch_len + j * mNOut, mpOutputPatterns16 + j * mNOut16, mNOut * sizeof(float));
      }       
   }

   // process remained vectors
   if(n_rem != 0)
   {
      // -copy unaligned memory block to aligned memory block 
      for(j = 0; j < n_rem; j++)
      {
         memcpy(mpInputPatterns16 + j * mNInp16, pInp + n_bunches * inp_bunch_len + j * mNInp, mNInp * sizeof(float));
         memset(mpInputPatterns16 + j * mNInp16 + mNInp, 0, (mNInp16 - mNInp) * sizeof(float));
      }

      // forward pass of one full bunch
      ForwardPass1Bunch(n_rem, mpInputPatterns16, mpOutputPatterns16);
      
      // copy output data to unaligned memory block
      for(j = 0; j < n_rem; j++)
      {
         memcpy(pOut + n_bunches * out_bunch_len + j * mNOut, mpOutputPatterns16 + j * mNOut16, mNOut * sizeof(float));
      }
   }    
}

bool NeuralNet::GetFloatValue(char *pText, float *pRet)
{
   char pbuff[100];
   char *pt = pText;
   int i = 0;
   pt = SkipSpaces(pt);
   while(pt != '\0' && i < 100 && strchr(" \t\n\r", pt[i]) == 0)
   {
      pbuff[i] = pt[i];
      i++;
   }
   pbuff[i] = '\0';
   return (sscanf(pbuff, "%e", pRet) == 1);
}

bool NeuralNet::GetIntValue(char *pText, int *pRet)
{
   char pbuff[100];
   char *pt = pText;
   int i = 0;
   pt = SkipSpaces(pt);
   while(pt != '\0' && i < 100 && strchr(" \t\n\r", pt[i]) == 0)
   {
      pbuff[i] = pt[i];
      i++;
   }
   pbuff[i] = '\0';
   return (sscanf(pbuff, "%d", pRet) == 1);
}

void NeuralNet::DumpMatrix(float *pMat, int nRows, int nColumns, int nColumnsAligned)
{
   int i;
   float *prow = pMat;
   for(i = 0; i < nRows; i++)
   {
      int j;
      for(j = 0; j < nColumns; j++)
      { 
         printf(" %f", prow[j]);
      }
      printf("\n");
      prow += nColumnsAligned;
   }
}

int NeuralNet::GetInputSize()
{
   return mNInp;
}

int NeuralNet::GetOutputSize()
{
   return mNOut;
}

int NeuralNet::GetHiddenSize()
{
   return mNHid;
}
