#include <cstring>
#include "Buffer.h"

namespace STK
{
  //****************************************************************************
  //****************************************************************************
  void 
  Buffer::
  Realloc(size_t newSize)
  {
    if (newSize > mBuffSize)
    {
      // we remember the current position, so we can refer to it later
      size_t offset = mCurrentData - mBuff;
      
      // keep pointer to the original data
      BuffDataType *   tmp_data = mBuff;
      
      // allocate new memory space
      mBuff = new BuffDataType[newSize];
      
      // copy the memory area
      memcpy(mBuff, tmp_data, mBuffSize);
      mCurrentData = mBuff + offset;
      mBuffSize    = newSize;
      mBuffEnd     = mBuff + mBuffSize - 1;
      
      delete tmp_data;    
    }
  }
  
  //****************************************************************************
  //****************************************************************************
  
};

