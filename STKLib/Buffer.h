#ifndef STK_Buffer_h
#define STK_Buffer_h

#include <iostream>

namespace STK
{  
  class Buffer
  {
  public:
    /// Default elementary buffer size
    static const size_t       DEFAULT_SIZE       = 1024;
    static const size_t       DEFAULT_DELTA_SIZE = 512;
    
    /// what is stored in the buffer
    typedef char              BuffDataType;
    
  
  private:
    size_t                    mDeltaBuffSize;
    
    /// Overal size of buffer
    size_t                    mBuffSize;
    
    /// The real buffer
    BuffDataType *            mBuff;
    
    /// Pointer to the current buffer element
    BuffDataType *            mCurrentData;
    
    /// Pointer to the end of data
    BuffDataType *            mBuffEnd;
    
  public:
    Buffer(size_t size      = DEFAULT_SIZE,
           size_t deltaSize = DEFAULT_DELTA_SIZE) 
    {
      // allocate extra character for eol
      mBuff          = new BuffDataType[size];
      mBuffSize      = size;
      mDeltaBuffSize = deltaSize;
      mCurrentData   = mBuff;
      mBuffEnd       = mBuff + mBuffSize - 1;
    }
    
    ~Buffer()
    {
      delete [] mBuff;
    }
    
    
    /**
     *  @brief Returns elementary buffer size
     */
    const size_t
    DeltaSize() const
    {
      return mDeltaBuffSize;
    }
    
    
    /**
     *  @brief Returns buffer size
     */
    const size_t
    Size() const
    {
      return mBuffSize;
    }
    
    
    /**
     *  @brief Reallocates buffer memory to the desired size and copy the original data
     *  @param newSize New buffer size
     *
     *  The procedure allocates new space for the buffer data and coppies 
     *  the original memory area to the new one. If new size is smaller,
     *  data are cropped.
     */
    void 
    Realloc(size_t newSize);
    
    
    /**
     *  @brief Expand buffer by DeltaSize
     *
     *  The procedure uses Realloc to expand the buffer
     */
    void
    Expand()
    {
      // reallocate
      Realloc(mBuffSize + mDeltaBuffSize);
    };
    
    
    /**
     *  @brief Sets current position to the begining of the buffer
     */
    void
    Rewind()
    {
      mCurrentData = mBuff;
    }    
    
    const BuffDataType *
    Data() const
    {
      return mCurrentData;
    }
    
    
    BuffDataType *
    pData()
    {
      return mCurrentData;
    }
    
    
    
    
    /**
     *  @brief Access operator
     *  @return Reference to the current data
     */
    BuffDataType &
    operator*() const
    {
      return *mCurrentData;
    }
    
    
    /**
     *  @brief Index operator to the data area
     *  @param index Index to the buffer
     *  @return Reference to the indexed element
     *
     *  The procedure returns the desired element
     */
    BuffDataType &
    operator[](size_t index)
    {
      if (index-1 >= mBuffSize)
      {
        Realloc(((index + 1) / mDeltaBuffSize + 1) * mDeltaBuffSize);
      }
      return mBuff[index];
    }
    
    
    /**
     *  @brief Infix ++ pointer operator
     *  @return Pointer to the data
     *
     */    
    BuffDataType *
    operator++()
    {
      // this will be static => performance
      static BuffDataType * ret;
      ret = mCurrentData;
      
      if (mCurrentData == mBuffEnd)
      {
        this->Expand();
      }
      
      ret = mCurrentData;
      mCurrentData++;
      
      return ret;
    }    
    
    
    /**
     *  @brief Postfix ++ pointer operator
     *  @return Pointer to the data
     *
     */    
    BuffDataType *
    operator++(int);

        
    /**
     *  @brief Infix -- pointer operator
     *  @return Pointer to the data
     *
     */    
    BuffDataType *
    operator--();
    
    
    
    /**
     *  @brief Postfix -- pointer operator
     *  @return Pointer to the data
     *
     */    
    BuffDataType *
    operator--(int);  
    
    
    Buffer &
    operator<<(char ch)
    {
      *(this->operator++()) = ch;
      return *this;
    }
      
  }; // template <typename ElemType> class Buffer
}; // namespace STK

// #define STK_Buffer_h
#endif 
