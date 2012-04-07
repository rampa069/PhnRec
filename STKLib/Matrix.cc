/** 
 * @file Matrix.cc 
 * 
 * Implementation of specialized Matrix template methods 
 */

#include "Matrix.h"


namespace STK
{
  //***************************************************************************
  //***************************************************************************
  template<>
    Matrix<float> &
    Matrix<float>::
    FastRowSigmoid()
    {
      for(size_t row = 0; row < this->Rows(); row++)
      {
        fast_sigmoid_vec(this->Row(row), this->Row(row), this->Cols());
      }
      return *this;
    }
  
  
  //***************************************************************************
  //***************************************************************************
  template<>
    Matrix<float> &
    Matrix<float>::
    AddMMMul(Matrix<float> & a, Matrix<float> & b)
    { 
      assert(a.Cols() == b.Rows());
      assert(this->Rows() == a.Rows());
      assert(this->Cols() == b.Cols());
      assert(a.mStorageType == STORAGE_REGULAR);
      if(b.Storage() == STORAGE_TRANSPOSED){
#ifdef USE_BLAS
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, a.Rows(), b.Cols(), b.Rows(),
                    1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
#else
        Error("Method not implemented without BLAS");
#endif        
      }
      else{
#ifdef USE_BLAS
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, a.Rows(), b.Cols(), b.Rows(),
                    1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
#else
        Error("Method not implemented without BLAS");
#endif
      }
      return *this;
    }; // AddMatMult(const ThisType & a, const ThisType & b)

  
  //***************************************************************************
  //***************************************************************************
  template<>
    Matrix<float> &
    Matrix<float>::
    DiagScale(float* pDiagVector)
    {
      assert(mStorageType == STORAGE_REGULAR);
      
      // :TODO: 
      // optimize this
      float*  data = mpData;
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
  
  
  //***************************************************************************
  //***************************************************************************
  template<>
    Matrix<float> &
    Matrix<float>::
    RepMMTMul(Matrix<float> & a, Matrix<float> & b)
    { 
      assert(a.Rows() == this->Rows());
      assert(b.Rows() == this->Cols());
      assert(a.Cols() == b.Cols());
      assert(b.mStorageType == STORAGE_TRANSPOSED);
      Clear();
      
      ///this->PrintOut("mojenuly.0");
      
      //cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, this->Rows(), this->Cols(), b.Rows(),
      //            1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
      
      //printf("***%d %d %d %d %d %d\n", a.Rows(), b.Rows(), b.Cols(), a.mMRealCols, b.mMRealCols, this->mMRealCols);
      
#ifdef USE_BLAS
      cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, a.Rows(), b.Rows(), b.Cols(),
                  1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
#else
      Error("Method not implemented without BLAS");
#endif
      
      //cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, C->rows, C->cols, B->rows,
      //            1.0f, A->arr, A->realCols, B->arr, B->realCols, 1.0f, C->arr, C->realCols);		  
  
      return *this;
    }; 

  //***************************************************************************
  //***************************************************************************
  template<>
    Matrix<float> &
    Matrix<float>::
    Invert()
    { 
      assert(Rows() == Cols());
    
/*  
#ifdef USE_BLAS
      int* pivot = new int[Rows()];
      if (STORAGE_REGULAR == Storage())
      {
        clapack_sgetrf(CblasColMajor, Rows(), Cols(), mpData, mMRealCols, pivot);
        clapack_sgetri(CblasColMajor, Rows(), mpData, mMRealCols, pivot);
      }
      else
      {
        clapack_sgetrf(CblasRowMajor, Rows(), Cols(), mpData, mMRealCols, pivot);
        clapack_sgetri(CblasRowMajor, Rows(), mpData, mMRealCols, pivot);
      }
      delete [] pivot;
#else
*/
      Error("Method is not implemented");
/*
#endif
*/
      return *this;
    }; 

/*  
    STK::Matrix<FLOAT> x(5, 5);
    x[0][0] = 0.9901;
    x[0][1] = 0.6435;
    x[0][2] = 0.7446;
    x[0][3] = 0.2126;
    x[0][4] = 0.6072;
    
    x[1][0] = 0.7889;
    x[1][1] = 0.3200;
    x[1][2] = 0.2679;
    x[1][3] = 0.8392;
    x[1][4] = 0.6299;
    
    x[2][0] = 0.4387;
    x[2][1] = 0.9601;
    x[2][2] = 0.4399;
    x[2][3] = 0.6288;
    x[2][4] = 0.3705;
    
    x[3][0] = 0.4983;
    x[3][1] = 0.7266;
    x[3][2] = 0.9334;
    x[3][3] = 0.1338;
    x[3][4] = 0.5751;
    
    x[4][0] = 0.2140;
    x[4][1] = 0.4120;
    x[4][2] = 0.6833;
    x[4][3] = 0.2071;
    x[4][4] = 0.4514;
    
    for (int i = 0; i < x.Rows(); i++)
    {
      for (int j = 0; j < x.Cols(); j++)
      {
        cout << x[i][j] << " ";
      }
      cout << endl;
    }
    
    x.Invert();
    
    for (int i = 0; i < x.Rows(); i++)
    {
      for (int j = 0; j < x.Cols(); j++)
      {
        cout << x[i][j] << " ";
      }
      cout << endl;
}*/
        
        
  //***************************************************************************
  //***************************************************************************
    
    template<>
    Matrix<float> &
    Matrix<float>::
    RepMTMMul(Matrix<float> & a, Matrix<float> & b)
    { 
      /*assert(a.Rows() == this->Rows());
      assert(b.Rows() == this->Cols());
      assert(a.Cols() == b.Cols());
      */
      assert(a.mStorageType == STORAGE_REGULAR);
      assert(this->mStorageType == STORAGE_TRANSPOSED);
      Clear();
            
      //cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans, this->Rows(), this->Cols(), b.Rows(),
      //          1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
      //printf("%d %d %d %d %d %d\n", a.Cols(), b.Cols(), b.Rows(), a.mMRealCols, b.mMRealCols, this->mMRealCols);
#ifdef USE_BLAS
      cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans, a.Cols(), b.Cols(), b.Rows(),
                  1.0f, a.mpData, a.mMRealCols, b.mpData, b.mMRealCols, 1.0f, this->mpData, this->mMRealCols);
#else
      Error("Method not implemented without BLAS");
#endif
      
      return *this;
    }; 
  
  //***************************************************************************
  //***************************************************************************
  template<>
    Matrix<float> &
    Matrix<float>::
    FastRowSoftmax()
    {
      for(size_t row = 0; row < this->Rows(); row++){
        fast_softmax_vec(this->Row(row), this->Row(row), this->Cols());
      }
      return *this;
    }
    
  
}; //namespace STK
