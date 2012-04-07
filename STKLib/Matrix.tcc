
/** @file Matrix.tcc
 *  This is an internal header file, included by other library headers.
 *  You should not attempt to use it directly.
 */


#ifndef STK_Matrix_tcc
#define STK_Matrix_tcc

//#pragma GCC system_header

#include "common.h"
#define _XOPEN_SOURCE 600
#include <cstdlib>
#include <cassert>
#include <math.h>
#include <string.h>

#ifdef USE_BLAS
extern "C"{
  #include <cblas.h>
}
#endif

#include<fstream>
#include<iomanip>

namespace STK
{
  
//******************************************************************************    
  template<typename _ElemT>
    void
    BasicMatrix<_ElemT>::
    Init(const size_t rows, const size_t cols)
    {
      size_t skip;
      size_t l_dim;      // leading dimension
      size_t size;
      void*  data;
      void*  free_data;
      
      // compute the size of skip and real cols
      skip  = ((16 / sizeof(_ElemT)) - cols % (16 / sizeof(_ElemT))) 
            %  (16 / sizeof(_ElemT));
      l_dim = cols + skip;
      size  = rows * l_dim * sizeof(_ElemT);
      
      if (NULL != (data = stk_memalign(16, size, &free_data)))
      {
        mpData        = static_cast<_ElemT*> (data);
#ifdef STK_MEMALIGN_MANUAL
        mpFreeData    = static_cast<_ElemT*> (free_data);
#endif
        mMRows = rows;
        mMCols = cols;
        mMRealCols = l_dim;
        
        // set all bytes to 0
        memset(mpData, 0, size);
      }
      else
      {
        throw std::bad_alloc();
      }
    }
  
  //****************************************************************************
  //****************************************************************************
  // The destructor
  template<typename _ElemT>
    // virtual
    BasicMatrix<_ElemT>::
    ~BasicMatrix()
    {
      // we need to free the data block if it was defined
#ifndef STK_MEMALIGN_MANUAL
      free(mpData);
#else
      free(mpFreeData);
#endif
    }
  
  
  //****************************************************************************
  // Basic constructor
  template<typename _ElemT>
    Matrix<_ElemT>::
    Matrix(const size_t       r,
          const size_t       c,
          const StorageType  st)
    {
      Init(r, c, st);
    }


//******************************************************************************    
  template<typename _ElemT>
  void
  Matrix<_ElemT>::
  Init(const size_t r,
       const size_t c,
       const StorageType st)
  {
    // initialize some helping vars
    size_t  rows;
    size_t  cols;
    size_t  skip;
    size_t  real_cols;
    size_t  size;
    void*   data;       // aligned memory block
    void*   free_data;  // memory block to be really freed

    // at this moment, nothing is clear
    mStorageType = STORAGE_UNDEFINED;

    // if we store transposed, we swap the rows and cols
    // we assume that the user does not specify STORAGE_UNDEFINED
    if (st == STORAGE_TRANSPOSED)
    { rows = c; cols = r; }
    else
    { rows = r; cols = c; }


    // compute the size of skip and real cols
    skip      = ((16 / sizeof(_ElemT)) - cols % (16 / sizeof(_ElemT))) % (16 / sizeof(_ElemT));
    real_cols = cols + skip;
    size      = rows * real_cols * sizeof(_ElemT);

    // allocate the memory and set the right dimensions and parameters

    if (NULL != (data = stk_memalign(16, size, &free_data)))
    {
      this->mpData        = static_cast<_ElemT *> (data);
#ifdef STK_MEMALIGN_MANUAL
      this->mpFreeData    = static_cast<_ElemT *> (free_data);
#endif
      this->mStorageType = st;
      this->mMRows      = rows;
      this->mMCols      = cols;
      this->mMRealCols  = real_cols;
      //this->mMSize      = size;

      this->mTRows      = r;
      this->mTCols      = c;

      // set all bytes to 0
      memset(this->mpData, 0, this->MSize());
    }
    else
    {
      throw std::bad_alloc();
    }
  } //
  

  //****************************************************************************
  //****************************************************************************
  // Copy constructor
  template<typename _ElemT>
    Matrix<_ElemT>::
    Matrix(const ThisType & t)
    {
      // we need to be sure that the storage type is defined
      if (t.mStorageType != STORAGE_UNDEFINED)
      {
        void* data;
        void* free_data;
  
        // first allocate the memory
        // this->mpData = new char[t.M_size];
        // allocate the memory and set the right dimensions and parameters
        if (NULL != (data = stk_memalign(16, t.MSize(), &free_data)))
        {
          this->mpData        = static_cast<_ElemT *> (data);
#ifdef STK_MEMALIGN_MANUAL
          this->mpFreeData    = static_cast<_ElemT *> (free_data);
#endif
          // copy the memory block
          memcpy(this->mpData, t.mpData, t.MSize());
  
          // set the parameters
          this->mStorageType = t.mStorageType;
          this->mMRows       = t.mMRows;
          this->mMCols       = t.mMCols;
          this->mMRealCols   = t.mMRealCols;
          //this->mMSkip       = t.mMSkip;
          //this->mMSize       = t.MSize();
        }
        else
        {
          // throw bad_alloc exception if failure
          throw std::bad_alloc();
        }
      }
    }


  //****************************************************************************
  //****************************************************************************
  // The destructor
  template<typename _ElemT>
  Matrix<_ElemT>::
  ~Matrix<_ElemT> ()
  {
    // we need to free the data block if it was defined
    if (mStorageType != STORAGE_UNDEFINED)
    {
#ifndef STK_MEMALIGN_MANUAL
      free(mpData);
#else
      free(mpFreeData);
#endif
    }
  }

  
//******************************************************************************
  template<typename _ElemT>
  _ElemT &
  Matrix<_ElemT>::
  operator ()(const size_t r, const size_t c)
  {
    assert(mStorageType == STORAGE_REGULAR || mStorageType == STORAGE_TRANSPOSED);
    if (mStorageType == STORAGE_REGULAR)
    {
      return *(mpData + r * mMRealCols + c);
    }
    else
    {
      return *(mpData + c * mMRealCols + r);      
    }
    
  }

    
//******************************************************************************
  template<typename _ElemT>
  Matrix<_ElemT> &
  Matrix<_ElemT>::
  AddMMMul(ThisType & a, ThisType & b)
  { 
    if(!(a.Cols() == b.Rows() && this->Rows() == a.Rows() &&  this->Cols() == b.Cols()))
      STK::Error("Matrix multiply: bad matrix sizes (%d %d)*(%d %d) -> (%d %d)", a.Rows(), a.Cols(), b.Rows(), b.Cols(), this->Rows(), this->Cols());
    
    // :KLUDGE: Dirty for :o)        
    assert(a.mStorageType == STORAGE_REGULAR);
    for(int r = 0; r < this->Rows() ; r++){
      for(int c = 0; c < this->Cols(); c++){
        // for every out matrix cell
        for(int index = 0; index < a.Cols(); index++){
          (*this)(r, c) += a(r, index) * b(index, c);
          //std::cout << a(r, index);
        }
      }
    }
    
    STK::Warning("Could be slow, not well implemented");
    
    return *this;
  }; // AddMatMult(const ThisType & a, const ThisType & b)
  
  //******************************************************************************
  template<typename _ElemT>
  Matrix<_ElemT> &
  Matrix<_ElemT>::
  RepMMTMul(ThisType & a, ThisType & b)
  { 

      STK::Error("Just only BLAS for float..."); 
    
    return *this;
  }; // AddMatMult(const ThisType & a, const ThisType & b)
  
//******************************************************************************
  template<typename _ElemT>
  Matrix<_ElemT> &
  Matrix<_ElemT>::
  AddMCMul(ThisType & a, _ElemT c) { 
    assert(this->Cols() == a.Cols());
    assert(this->Rows() == a.Rows());
    assert(this->Storage() == a.Storage());
    _ElemT* pA = NULL;
    _ElemT* pT = NULL;
    
    if(this->mStorageType == STORAGE_REGULAR){
      for(unsigned row = 0; row < this->Rows(); row++){
        pA = a.Row(row);
        pT = this->Row(row);
        for(unsigned col = 0; col < this->Cols(); col++){
          //(*this)(row, col) += a(row, col) * c;
	  (*pT) += (*pA) * c;
	  pT++;
	  pA++;
        }
      }
    }
    
    if(this->mStorageType == STORAGE_TRANSPOSED){
      for(unsigned row = 0; row < this->Cols(); row++){
        pA = a.Row(row);
        pT = this->Row(row);
        for(unsigned col = 0; col < this->Rows(); col++){
          //(*this)(row, col) += a(row, col) * c;
	  (*pT) += (*pA) * c;
	  pT++;
	  pA++;
        }
      }
    }
    
    return *this;
  };
  
  //******************************************************************************
  template<typename _ElemT>
  Matrix<_ElemT> &
  Matrix<_ElemT>::
  RepMMSub(ThisType & a, ThisType & b)
  { 
    assert(this->Cols() == a.Cols());
    assert(this->Rows() == a.Rows());
    assert(this->Cols() == b.Cols());
    assert(this->Rows() == b.Rows());      
    _ElemT* pA = NULL;
    _ElemT* pB = NULL;
    _ElemT* pT = NULL;
    for(unsigned row = 0; row < this->Rows(); row++){
      pA = a.Row(row);
      pB = b.Row(row);
      pT = this->Row(row);
      for(unsigned col = 0; col < this->Cols(); col++){
        //(*this)(row, col) = a(row, col) - b(row, col);
	(*pT) = (*pA) - (*pB);
	pT++;
	pA++;
	pB++;
      }
    }
    return *this;
  }; 
  
//******************************************************************************
/*
  template<>
  inline
  Matrix<float> &
  Matrix<float>::
  AddMatMult(Matrix<float> & a, Matrix<float> & b)
  { 
    assert(a.Cols() == b.Rows());
    assert(this->Rows() == a.Rows());
    assert(this->Cols() == b.Cols());
    if(b.Storage() == STORAGE_TRANSPOSED){
      cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, a.Rows(), b.Cols(), b.Rows(),
                  1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
    }
    else{
      cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, a.Rows(), b.Cols(), b.Rows(),
                  1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
    }
    return *this;
  }; // AddMatMult(const ThisType & a, const ThisType & b)
*/

//******************************************************************************
  template<typename _ElemT>
  Matrix<_ElemT> &
  Matrix<_ElemT>::
  FastRowSigmoid()
  {
    STK::Error("Sigmoid only implemented for float");
    return *this;
  }
  

//******************************************************************************
  /*
  template<>
  inline
  Matrix<float> &
  Matrix<float>::
  FastRowSigmoid()
  {
    for(size_t row = 0; row < this->Rows(); row++)
    {
      sigmoid_vec(this->Row(row), this->Row(row), this->Cols());
    }
    return *this;
  }
  */
//******************************************************************************
  template<typename _ElemT>
  Matrix<_ElemT> &
  Matrix<_ElemT>::
  FastRowSoftmax()
  {
    STK::Error("Softmax only implemented for float");
    return *this;
  }
  
  //***************************************************************************
  //***************************************************************************
  template<typename _ElemT>
    Matrix<_ElemT> &
    Matrix<_ElemT>::
    DiagScale(_ElemT* pDiagVector)
    {
      assert(mStorageType == STORAGE_REGULAR);
      
      // :TODO: 
      // optimize this
      _ElemT* data = mpData;
      int     skip = mMRealCols - mMCols;
      for (size_t i=0; i < Rows(); i++)
      {
        for (size_t j=0; j < Cols(); j++)
        {
          data[j] *= pDiagVector[j];
        }
        data += skip;
      }
      
      return *this;
    }
//******************************************************************************
/*
  template<>
  inline
  Matrix<float> &
  Matrix<float>::
  FastRowSoftmax()
  {
    for(size_t row = 0; row < this->Rows(); row++){
      softmax_vec(this->Row(row), this->Row(row), this->Cols());
    }
    return *this;
  }
*/
  template<typename _ElemT>
    Matrix<_ElemT> &
    Matrix<_ElemT>::
    Transpose()
    {
      if(this->mStorageType == STORAGE_REGULAR) 
        this->mStorageType = STORAGE_TRANSPOSED;
      else
        this->mStorageType = STORAGE_REGULAR;  
        
      int pom;
      
    /* pom = this->mMRows;
      this->mMRows = mMCols;
      this->mMCols = pom;*/
  
      pom = this->mTRows;
      this->mTRows = mTCols;
      this->mTCols = pom;  
      
      return *this;
    }
  
  
  //******************************************************************************
  //******************************************************************************
  template<typename _ElemT>
    Matrix<_ElemT> &
    Matrix<_ElemT>::
    Clear()
    {
      memset(mpData, 0, MSize());
      return *this;
    }
  
  
  //****************************************************************************
  //****************************************************************************
  template<typename _ElemT>
    void 
    Matrix<_ElemT>::
    PrintOut(char* file)
    {
      FILE* f = fopen(file, "w");
      unsigned i,j;
      fprintf(f, "%dx%d\n", this->mMRows, this->mMCols);
      
      for(i=0; i<this->mMRows; i++)
      {
        _ElemT *row = this->Row(i);
        for(j=0; j<this->mMRealCols; j++){
          fprintf(f, "%20.17f ",row[j]);
        }
        fprintf(f, "\n");
      }
      
      fclose(f);
    }

  
  //****************************************************************************
  //****************************************************************************
  template<typename _ElemT>
    Matrix<_ElemT>&
    Matrix<_ElemT>::
    ClearI()
    {
      Clear();
      size_t  skip = mMRealCols + 1;
      _ElemT* data = mpData;
      
      for (size_t i=0; i < Rows(); i++)
      {
        *data = 1.0;
        data += skip;
      }
      return *this;
    }
  
  //****************************************************************************
  // Copy constructor
  template<typename _ElemT>
    WindowMatrix<_ElemT>::
    WindowMatrix(const ThisType & t) :
      Matrix<_ElemT> (t)
    {
      // we need to be sure that the storage type is defined
      if (t.st != STORAGE_UNDEFINED)
      {
        // the data should be copied by the parent constructor
        void* data;
        void* free_data;

        // first allocate the memory
        // this->mpData = new char[t.M_size];
        // allocate the memory and set the right dimensions and parameters
        if (NULL != (data = stk_memalign(16, t.MSize(), &free_data)))
        {
          Matrix<_ElemT>::mpData        = static_cast<_ElemT *> (data);
#ifdef STK_MEMALIGN_MANUAL
          Matrix<_ElemT>::mpFreeData    = static_cast<_ElemT *> (free_data);
#endif
          // copy the memory block
          memcpy(this->mpData, t.mpData, t.MSize());

          // set the parameters
          this->mStorageType = t.mStorageType;
          this->mMRows       = t.mMRows;
          this->mMCols       = t.mMCols;
          this->mMRealCols   = t.mMRealCols;
          //this->mMSkip       = t.mMSkip;
          //this->mMSize       = t.mMSize;
        }
        else
        {
          // throw bad_alloc exception if failure
          throw std::bad_alloc();
        }
      }
    }


//******************************************************************************
  template<typename _ElemT>
    void
    WindowMatrix<_ElemT>::
    SetSize(const size_t rowOff,
            const size_t rows)
    {
      if (Matrix<_ElemT>::mStorageType == STORAGE_REGULAR)
      {
        // point to the begining of window
        Matrix<_ElemT>::mpData = mpOrigData + mOrigMRealCols * rowOff;
        Matrix<_ElemT>::mTRows = rows;
        Matrix<_ElemT>::mTCols = mOrigTCols;
        Matrix<_ElemT>::mMRows = rows;
        Matrix<_ElemT>::mMCols = mOrigMCols;
        Matrix<_ElemT>::mMRealCols = mOrigMRealCols;
        //Matrix<_ElemT>::mMSkip = mOrigMSkip;
      }
      else if (Matrix<_ElemT>::mStorageType == STORAGE_TRANSPOSED)
      {
        // point to the begining of window
        Matrix<_ElemT>::mpData = mpOrigData + rowOff;
        Matrix<_ElemT>::mTRows = rows;
        Matrix<_ElemT>::mTCols = mOrigTCols;
        Matrix<_ElemT>::mMRows = mOrigMCols;
        Matrix<_ElemT>::mMCols = rows;
        Matrix<_ElemT>::mMRealCols = mOrigMRealCols;
        //Matrix<_ElemT>::mMSkip = mOrigMRealCols - rows;
      }
    }


//******************************************************************************
  template<typename _ElemT>
    void
    WindowMatrix<_ElemT>::
    SetSize(const size_t rowOff,
            const size_t rows,
            const size_t colOff,
            const size_t cols)
    {
      if (Matrix<_ElemT>::mStorageType == STORAGE_REGULAR)
      {
        // point to the begining of window
        Matrix<_ElemT>::mpData = mpOrigData + mOrigMRealCols * rowOff + cols;
        Matrix<_ElemT>::mTRows = rows;
        Matrix<_ElemT>::mTCols = cols;
        Matrix<_ElemT>::mMRows = rows;
        Matrix<_ElemT>::mMCols = cols;
        Matrix<_ElemT>::mMRealCols = this->mOrigMRealCols;
        //Matrix<_ElemT>::mMSkip = this->mOrigMRealCols - cols;
      }
      else if (Matrix<_ElemT>::mStorageType == STORAGE_TRANSPOSED)
      {
        // point to the begining of window
        Matrix<_ElemT>::mpData = mpOrigData + rowOff;
        Matrix<_ElemT>::mTRows = rows;
        Matrix<_ElemT>::mTCols = mOrigTCols;
        Matrix<_ElemT>::mMRows = mOrigMCols;
        Matrix<_ElemT>::mMCols = rows;
        Matrix<_ElemT>::mMRealCols = mOrigMRealCols;
        //Matrix<_ElemT>::mMSkip = mOrigMRealCols - rows;
      }

    }


  template<typename _ElemT>
    std::ostream &
    operator << (std::ostream & rOut, Matrix<_ElemT> & rM)
    {
      /*rOut << "Rows: " << rM.Rows() << "\n";
      rOut << "Cols: " << rM.Cols() << "\n";
      if(rM.Storage() == STORAGE_TRANSPOSED) rOut << "Transposed \n ";
      else rOut << "Regular \n ";*/
      
      /////
      for(int i=0; i<10; i++){
        rOut << rM.mpData[i] << " ";
      }
      rOut << "\n";
      /////
      
      size_t    rcount;
      size_t    ccount;

      _ElemT  * data    = rM.mpData;

      // go through all rows
/*      for (rcount = 0; rcount < rM.Rows(); rcount++)
      {
        // go through all columns
        for (ccount = 0; ccount < rM.Cols(); ccount++)
        {
          rOut << static_cast<_ElemT>( rM(rcount, ccount)) << " ";

        }

        //data += rM.mMSkip;

        rOut << std::endl;
      }*/
      return rOut;
    }

}// namespace STK

// #define STK_Matrix_tcc
#endif 
