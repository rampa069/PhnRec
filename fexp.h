#ifndef _FEXP_H
#define _FEXP_H

// Fast evaluation of exponentials (a hack from ICSI), fast sigmoid and fast softmax
// This code exists for compatibility with neural networks trained by Quicknet 3.0

#include <math.h>
#include <float.h>

#ifndef M_LN2
  #define M_LN2		0.69314718055994530942
#endif

#define FEXP_EXP_A (1048576 / M_LN2)
#define FEXP_EXP_C 60801

#ifdef  WORDS_BIGENDIAN
   #define FEXP_EXP(y) (fexp_d2i.n.j = (int) (FEXP_EXP_A*(y)) + (1072693248 - FEXP_EXP_C), fexp_d2i.d)
#else
   #define FEXP_EXP(y) (fexp_d2i.n.i = (int) (FEXP_EXP_A*(y)) + (1072693248 - FEXP_EXP_C), fexp_d2i.d)
#endif

#define FEXP_WORKSPACE union \
{                            \
   double d;                 \
   struct                    \
   {                         \
      int j;                 \
      int i;                 \
   } n;                      \
} fexp_d2i

inline float fexp_sigmoid(float x)
{
   FEXP_WORKSPACE;
    
   return 1.0f / (1.0f + FEXP_EXP(-x));
}

void fexp_sigmoid_v(int N, float *pData)
{
   int i;
   for(i = 0; i < N; i++)
   {
      pData[i] = fexp_sigmoid(pData[i]);
   }
}

void fexp_softmax_v(int N, float *pData)
{
    FEXP_WORKSPACE;

    // getting max
    int i;
    float max = -FLT_MAX;
    for (i = 0; i < N; i++)
    {
       if(pData[i] > max)
       {
          max = pData[i];
       }
    }    
    
    // exp of all values
    float sum = 0.0f;     
    for (i = 0; i < N; i++)
    {
        pData[i] = FEXP_EXP(pData[i] - max);
        sum += pData[i];
    }
    
    // scaling
    float scale = 1.0f / sum;
    for (i = 0; i < N; i++)
    {
       pData[i] *= scale;
    }
}

#endif
