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

// Modifications: 26. 02. 2004 - add suport for HTK files (saveHTK, loadHTK)
//                             - solved problems with compilation under CBuilder
//                06. 05. 2004 - removed operator static for swap2b(void*), swap4b(void*)
//                               GCC does not like it
//                06. 04. 2005 - sqrt function added
//                             - lowerLimit, upperLimit functions added
//                07. 04. 2005 - fixed loadHTK function for files > 32000 frames 
//

#ifndef Matrix_H
#define Matrix_H


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#undef write

#ifndef WIN32
        #include <unistd.h>
#else
        #include <io.h>
	#ifdef __CYGWIN__
		#include <unistd.h>
	#endif
#endif

const int rall = -1;

// selection range
class rg
{
	protected:
		int start1, step1, end1;
		int start2, step2, end2;
	public:
		rg() { start1 = -1; step1 = 1; end1 = -1; start2 = -1; step2 = 1; end2 = -1;};
		rg(int idx) {start1 = idx; end1 = idx; step1 = 1; start2 = -1; step2 = 1;
		       end2 = -1;};
		rg(int start, int end) {start1 = start; step1 = 1; end1 = end;
		       start2 = -1; step2 = 1; end2 = -1;};
		rg(int start, int step, int end) {start1 = start; step1 = step; end1 = end;
		       start2 = -1; step2 = 1; end2 = -1;};
		rg(const rg &rg1, const rg &rg2) {rg1.getDim1(start1, step1, end1);
			rg2.getDim1(start2, step2, end2);};
		void setDim1(const int start, const int step, const int end) {start1 = start; step1 = step;
		       end1 = end;};
		void setDim2(const int start, const int step, const int end) {start2 = start; step2 = step;
		       end2 = end;};
		void setDim1(const int start, const int end) {start1 = start; step1 = 1; end1 = end;};
		void setDim2(const int start, const int end) {start2 = start; step2 = 1; end2 = end;};
		void getDim1(int &start, int &step, int &end) const {start = start1; step = step1;
		       end = end1;};
		void getDim2(int &start, int &step, int &end) const {start = start2; step = step2;
		       end = end2;};
		rg & operator = (rg &rg1) { start1 = rg1.start1; step1 = rg1.step1; end1 = rg1.end1;
		       start2 = rg1.start2; step2 = rg1.step2; end2 = rg1.end2; return *this;};
};


// HTK parameter file header (see HTK manual)
typedef struct
{
        int nSamples;
        int sampPeriod;
        short sampSize;
        short paramKind;
} HTK_header;


// matrix template
template<class T>
class Mat
{
	protected:
		T *M;
		int nrows;
		int ncolumns;
		bool created_by_op;
		void initVars();
	public:
		// constructors/destructors
		Mat();
		Mat(const int nr, const int nc);
		Mat(const int nr, const int nc, const T v);
		~Mat();

		// HTK header (is used by readHTK, saveHTK)
		HTK_header htk_hdr;

		// pointer to memory
		const T *getMem() const {return M;};

		// matrix size
		int rows() const {return nrows;};
		int columns() const {return ncolumns;};

		// matrix initialisation
		void init(const int nr, const int nc);

		// operations with single cell
		inline void set(const int r, const int c, const T v);
		inline T get(const int r, const int c) const;
		void add(const int r, const int c, const T v) {set(r, c, get(r, c) + v);};
		void sub(const int r, const int c, const T v) {set(r, c, get(r, c) - v);};
		void mul(const int r, const int c, const T v) {set(r, c, get(r, c) * v);};
		void div(const int r, const int c, const T v) {set(r, c, get(r, c) / v);};

		// copy matrix selections
		void copy(const Mat<T> &from, const int s_start_r, const int s_step_r, const int s_end_r,
			  const int s_start_c, const int s_step_c, const int s_end_c,
			  const int d_start_r, const int d_step_r, const int d_end_r,
			  const int d_start_c, const int d_step_c, const int d_end_c);
		void copy(const Mat<T> &from, const int s_start_r, const int s_end_r, const int s_start_c, const int s_end_c,
			  const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c);
		void copy(const Mat<T> &from, const rg &rg_source, const rg &rg_target);

		// sets matrix selections
		void set(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const T v);
		void set(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const T *v);
		void set(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);

		void set(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, const T v);
		void set(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, const T *v);
		void set(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, const Mat<T> &from);

		void set(const rg &range, const T v);
		void set(const rg &range, const T *v);
		void set(const rg &range, const Mat<T> &from);

		void set(const T v);
		void set(const T *vals);
		void set(const Mat<T> &from);

		// extract functions
		void extr(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, T *v);
		void extr(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, Mat<T> &to);

		void extr(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, T *v);
		void extr(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, Mat<T> &to);

		void extr(const rg &range, T *v);
		void extr(const rg &range, Mat<T> &to);

		void extr(T *vals);
		void extr(Mat<T> &to);


		// add functions
		void add(const T v);
		void add(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const T v);
		void add(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, const T v);

		void add(const int s_start_r, const int s_step_r, const int s_end_r,
			 const int s_start_c, const int s_step_c, const int s_end_c,
			 const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void add(const int s_start_r, const int s_end_r,
			 const int s_start_c, const int s_end_c,
			 const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);
		void add(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void add(const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);

		void add(const Mat<T> &from);

		void add(const rg &range, const T v);
		void add(const rg &tg_range, const Mat<T> &from, const rg &sr_range);
		void add(const rg &tg_range, const Mat<T> &from);

		// sub functions
		void sub(const T v) {add(-v);};
		void sub(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const T v)
			 { add(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, -v);};
		void sub(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, const T v)
			 { add(d_start_r, d_end_r, d_start_c, d_end_c, -v); };
		void sub(const int s_start_r, const int s_step_r, const int s_end_r,
			 const int s_start_c, const int s_step_c, const int s_end_c,
			 const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void sub(const int s_start_r, const int s_end_r,
			 const int s_start_c, const int s_end_c,
			 const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);
		void sub(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void sub(const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);

		void sub(const Mat<T> &from);

		void sub(const rg &range, const T v) {add(range, -v);};
		void sub(const rg &tg_range, const Mat<T> &from, const rg &sr_range);
		void sub(const rg &tg_range, const Mat<T> &from);

		// mul function
		void mul(const T v);
		void mul(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const T v);
		void mul(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, const T v);

		void mul(const int s_start_r, const int s_step_r, const int s_end_r,
			 const int s_start_c, const int s_step_c, const int s_end_c,
			 const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void mul(const int s_start_r, const int s_end_r,
			 const int s_start_c, const int s_end_c,
			 const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);
		void mul(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void mul(const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);

		void mul(const Mat<T> &from);

		void mul(const rg &range, const T v);
		void mul(const rg &tg_range, const Mat<T> &from, const rg &sr_range);
		void mul(const rg &tg_range, const Mat<T> &from);

		// div
		void div(const T v) {mul((T)1.0f/v);};
		void div(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const T v)
			 { mul(d_start_r, d_step_r,  d_end_r,
			  d_start_c,  d_step_c,  d_end_c, (T)1.0f/v);};
		void div(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, const T v)
			 { mul(d_start_r, d_end_r, d_start_c,  d_end_c, (T)1.0f/v);};

		void div(const int s_start_r, const int s_step_r, const int s_end_r,
			 const int s_start_c, const int s_step_c, const int s_end_c,
			 const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void div(const int s_start_r, const int s_end_r,
			 const int s_start_c, const int s_end_c,
			 const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);
		void div(const int d_start_r, const int d_step_r, const int d_end_r,
			 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from);
		void div(const int d_start_r, const int d_end_r,
			 const int d_start_c, const int d_end_c, const Mat<T> &from);

		void div(const Mat<T> &from);

		void div(const rg &range, const T v) {mul(range, (T)1.0f/v);};
		void div(const rg &tg_range, const Mat<T> &from, const rg &sr_range);
		void div(const rg &tg_range, const Mat<T> &from);

		// functions applied on each cell
		typedef T (*mfunc1)(const T);
		typedef T (*mfunc2)(T, T);

		void func(const mfunc1 f);
		void func(const mfunc2 f, const T v);
		Mat<T> & funcOp(const mfunc1 f);
		Mat<T> & funcOp(const mfunc2 f, const T v);

		static T _pow(const T v1, const T v2) {return (T) ::pow((double)v1, (double)v2);};
		static T _log(const T v)    {return (T) ::log((double)v);};
		static T _log10(const T v)  {return (T) ::log10((double)v);};
		static T _exp(const T v)    {return (T) ::exp((double)v);};
		static T _abs(const T v)    {return (T) ::fabs((double)v);};
		static T _sqrt(const T v)   {return (T) ::sqrt((double)v);};

		void pow(T v) {func(&_pow, v);};
		void log() {func(&_log);};
		void log10() {func(&_log10);};
		void exp() {func(&_exp);};
		//void abs() {func(&_abs);};
                void sqrt() {func(&_sqrt);};

		Mat<T> & powOp(T v) {return funcOp(&_pow, v);};
		Mat<T> & logOp() {return funcOp(&_log);};
		Mat<T> & log10Op() {return funcOp(&_log10);};
		Mat<T> & expOp() {return funcOp(&_exp);};
		//Mat<T> & absOp() {return funcOp(&_abs);};

		// set/get diagonal (trace)
		void setDiag(const T v);
		void setDiag(T *vals);
		void setDiag(Mat<T> &from) {setDiag((T *)from.getMem());};
		void getDiag(T *vals);
		void getDiag(Mat<T> &to);
		Mat<T> & getDiagOp();

		// zoro matrix with 'v' values in diagonal
		void eye(T v) {set((T)0.0f); setDiag(v);};
		void eye() {eye((T)1.0f);};

		// fill in the matrix with linear sequence
		void linSpace(const T start, const T end);

		// swap rors/columns
		void swapRows(int r1, int r2);
		void swapColumns(int c1, int c2);

		// sets values outside the upper/lover triangular part to zeros
		void upperTriangular();
		void lowerTriangular();
		
		// dump matrix to standard output
		void dump();

		// lower-upper decomposition A = L * U
		void lud_this(Mat<T> & idxs, T &row_exs);
		Mat<T> & lud(Mat<T> &L, Mat<T> &U, Mat<T> & idxs, T &row_exs);

		// solve linear system of equations A * x = b
		// - actual matrix must be the LU decomposition result
		// - x is returned in b
		void luSolvLin(const Mat<T> &idxs, Mat<T> &b);
		// A * x = b
		// - the A matrix is actual
		// - x is returned
		Mat<T> &solvLin(const Mat<T> &b);

		// determinant of matrix
		T det();

		// transpose matrix B = A'
		Mat & transpose();
		Mat & t() {return transpose();};

		// inverse matrix B = A^(-1)
		Mat<T> & inv();
		
		// balance matrix to the coresponding row and column norms be quite equal
		void balance();
		
		// reduce matrix to upper hessenberg form
		void hessenberg();

		// operators
		Mat<T> & operator = (const Mat &from);
		Mat<T> & operator = (const T v) {set(v); return *this;};
		Mat<T> & operator = (const T *vals);
		Mat<T> & operator += (const Mat &from) {add(from); return *this;};
		Mat<T> & operator += (const T v) {add(v); return *this;};
		Mat<T> & operator -= (const Mat &from) {sub(from); return *this;};
		Mat<T> & operator -= (const T v) {sub(v); return *this;};
		Mat<T> & operator *= (const T v) {mul(v); return *this;};
		Mat<T> & operator *= (const Mat &from);
		Mat<T> & operator /= (const T v) {div(v); return *this;};
		Mat<T> & operator /= (const Mat &from) {div(from); return *this;};

		Mat<T> & operator / (const T v);
		Mat<T> & operator * (const T v);
		Mat<T> & operator + (const T v);
		Mat<T> & operator - (const T v);

		Mat<T> & operator * (const Mat<T> &m2);
		Mat<T> & mulOp(const Mat<T> &m2);
		Mat<T> & operator + (const Mat &from);
		Mat<T> & operator - (const Mat &from);

		// sums
		void sumRows(Mat &to);
		void sumColumns(Mat &to);
		Mat<T> & sumRowsOp();
		Mat<T> & sumColumnsOp();

		// limits
		void upperLimit(const T thr);
		void lowerLimit(const T thr);

		// submatrixes
		Mat<T> & submatrix(const int s_start_r, const int s_step_r, const int s_end_r,
			           const int s_start_c, const int s_step_c, const int s_end_c);
		Mat<T> & submatrix(const int s_start_r, const int s_end_r,
			           const int s_start_c, const int s_end_c);
		Mat<T> & submatrix(const rg &range);
		Mat<T> & operator ()(const rg &range) {return submatrix(range);};

		// load / write matrixes
		void loadAscii(const char *file);
		void loadBinary(const char *file);
                bool loadHTK(const char *file);                
		void saveAscii(const char *file);
		void saveBinary(const char *file);
                bool saveHTK(const char *file);

                // help functions
                inline void swap4b(void *a);
                inline void swap2b(void *a);

};

template<class T>
void Mat<T>::initVars()
{
	nrows = 0;
	ncolumns = 0;
	M = 0;
	created_by_op = false;

        htk_hdr.nSamples = 0;
        htk_hdr.sampPeriod = 100000;
        htk_hdr.sampSize = 0;
        htk_hdr.paramKind = 6;
}

template<class T>
Mat<T>::Mat()
{
	initVars();
}

template<class T>
Mat<T>::Mat(const int nr, const int nc)
{
	initVars();
	init(nr, nc);
}

template<class T>
Mat<T>::Mat(const int nr, const int nc, const T v)
{
	initVars();
	init(nr, nc);
	set(v);
}


template<class T>
Mat<T>::~Mat()
{
	if(M)
		delete [] M;
	nrows = 0;
	ncolumns = 0;
}

template<class T>
Mat<T> & Mat<T>::operator = (const T *vals)
{
	set(vals);
	return *this;
}

template<class T>
Mat<T> & Mat<T>::operator = (const Mat &from)
{
	if(from.rows() != nrows || from.columns() != ncolumns)
		init(from.rows(), from.columns());
	copy(from, 0, nrows - 1, 0, ncolumns - 1, 0, nrows - 1, 0, ncolumns - 1);

	if(from.created_by_op)
		delete &from;

	return *this;
};

template<class T>
Mat<T> & Mat<T>::operator * (const Mat<T> &m2)
{
	if(ncolumns != m2.rows())
	{
		fprintf(stderr, "Mat<T>::operator * - dimensions do not match (%d, %d)\n", ncolumns, m2.rows());
		exit(1);
	}
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	int nr = nrows;
	int nc = m2.columns();
	nM->init(nr, nc);
	int i, j, k;
	T v;
	for(i = 0; i < nr; ++i)
	{
		for(j = 0; j < nc; ++j)
		{
			v = 0.0f;
			for(k = 0; k < ncolumns; ++k)
				v += get(i, k) * m2.get(k, j);
			nM->set(i, j, v);
		}
	}
	if(m2.created_by_op)
		delete &m2;

	if(created_by_op)
		delete this;

	return *nM;
}

template<class T>
void Mat<T>::linSpace(const T start, const T end)
{
	T delta = (end - start) / (T)(nrows * ncolumns - 1);
	T v = start;
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
	{
		M[i] = v;
		v += delta;
	}
}

template<class T>
void Mat<T>::set(const T v)
{
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		M[i] = v;
}

template<class T>
void Mat<T>::mul(const T v)
{
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		M[i] *= v;
}

template<class T>
void Mat<T>::add(const T v)
{
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		M[i] += v;
}

template<class T>
void Mat<T>::init(const int nr, const int nc)
{
	if(nr == nrows && nc == ncolumns)
		return;

	if(M)
		delete [] M;
	nrows = nr;
	ncolumns = nc;
	M = new T[nr * nc];
}

template<class T>
void Mat<T>::set(const int r, const int c, const T v)
{
	if(r < 0 || r >= nrows)
	{
		fprintf(stderr, "Mat<T>::set - row is out boundary %d (0 -> %d)\n", r, nrows - 1);
		exit(1);
	}
	if(c < 0 || c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::set - column is out boundary %d (0 -> %d)\n", c, ncolumns - 1);
		exit(1);
	}
	M[r * ncolumns + c] = v;
}

template<class T>
T Mat<T>::get(const int r, const int c) const
{
	if(r < 0 || r >= nrows)
	{
		fprintf(stderr, "Mat<T>::get - row is out boundary %d (0 -> %d)\n", r, nrows - 1);
		exit(1);
	}
	if(c < 0 || c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::get - column is out boundary %d (0 -> %d)\n", c, ncolumns - 1);
		exit(1);
	}
	return M[r * ncolumns + c];
}

template<class T>
void Mat<T>::dump()
{
	int i, j;
	for(i = 0; i < nrows; ++i)
	{
		printf("%e", (T)get(i, 0));
		for(j = 1; j < ncolumns; ++j)
			printf(" %e", (T)get(i, j));
		printf("\n");
	}
}

template<class T>
void Mat<T>::copy(const Mat<T> &from, const int s_start_r, const int s_step_r, const int s_end_r,
		  const int s_start_c, const int s_step_c, const int s_end_c,
		  const int d_start_r, const int d_step_r, const int d_end_r,
		  const int d_start_c, const int d_step_c, const int d_end_c)
{
	if(s_start_r < 0 || s_start_r >= from.rows() || s_end_r < 0 || s_end_r >= from.rows() ||
	   s_start_c < 0 || s_start_c >= from.columns() || s_end_c < 0 || s_end_c >= from.columns())
	{
		fprintf(stderr, "Mat<T>::copy - source cut is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			s_start_r, s_end_r, s_start_c, s_end_c, 0, from.rows() - 1, 0, from.columns() - 1);
		exit(1);
	}

	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::copy - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int sr, sc;
	int dr, dc;
	dr = d_start_r;
	for(sr = s_start_r; sr <= s_end_r; sr += s_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(sc = sr * from.columns() + s_start_c; sc <= sr * from.columns() + s_end_c; sc += s_step_c)
		{
			M[dc] = from.M[sc];
			dc += d_step_c;
		}
		dr += d_step_r;
	}
}

template<class T>
void Mat<T>::copy(const Mat<T> &from, const int s_start_r, const int s_end_r,
		  const int s_start_c, const int s_end_c,
                  const int d_start_r, const int d_end_r,
		  const int d_start_c, const int d_end_c)
{
	copy(from, s_start_r, 1, s_end_r, s_start_c, 1, s_end_c,
	     d_start_r, 1, d_end_r, d_start_c, 1, d_end_c);
}

template<class T>
void Mat<T>::copy(const Mat<T> &from, const rg &rg_source, const rg &rg_target)
{
	int s_start_r, s_step_r, s_end_r;
	int s_start_c, s_step_c, s_end_c;
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	rg_source.getDim1(s_start_r, s_step_r, s_end_r);
	rg_source.getDim2(s_start_c, s_step_c, s_end_c);
	rg_target.getDim1(d_start_r, d_step_r, d_end_r);
	rg_target.getDim2(d_start_c, d_step_c, d_end_c);

	if(s_start_r == rall)
		s_start_r = 0;
	if(s_end_r == rall)
		s_end_r = from.rows() - 1;
	if(s_start_c == rall)
		s_start_c = 0;
	if(s_end_c == rall)
		s_end_c = from.columns() - 1;

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	copy(from, s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	     d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c);
}

template<class T>
void Mat<T>::set(const int d_start_r, const int d_step_r, const int d_end_r,
                const int d_start_c, const int d_step_c, const int d_end_c, const T v)
{
	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::set - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int dr, dc;
	for(dr = d_start_r; dr <= d_end_r; dr += d_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(dc = dr * ncolumns + d_start_c; dc <= dr * ncolumns + d_end_c; dc += d_step_c)
			M[dc] = v;
	}
}

template<class T>
void Mat<T>::set(const int d_start_r, const int d_step_r, const int d_end_r,
                const int d_start_c, const int d_step_c, const int d_end_c, const T *v)
{
	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::set - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int dr, dc, i;
	i = 0;
	for(dr = d_start_r; dr <= d_end_r; dr += d_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(dc = dr * ncolumns + d_start_c; dc <= dr * ncolumns + d_end_c; dc += d_step_c)
		{
			M[dc] = v[i];
			i++;
		}
	}
}

template<class T>
void Mat<T>::set(const int d_start_r, const int d_step_r, const int d_end_r,
                const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	set(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from.getMem());
}


template<class T>
void Mat<T>::set(const int d_start_r, const int d_end_r,
		    const int d_start_c, const int d_end_c, const T v)
{
	set(d_start_r, 1, d_end_r, d_start_c, 1, d_end_c, v);
}

template<class T>
void Mat<T>::set(const int d_start_r, const int d_end_r,
		    const int d_start_c, const int d_end_c, const T *v)
{
	set(d_start_r, 1, d_end_r, d_start_c, 1, d_end_c, v);
}

template<class T>
void Mat<T>::set(const int d_start_r, const int d_end_r,
		    const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	set(d_start_r, 1, d_end_r, d_start_c, 1, d_end_c, from);
}

template<class T>
void Mat<T>::set(const rg &range, const T v)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	set(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, v);
}

template<class T>
void Mat<T>::set(const rg &range, const T *v)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	set(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, v);
}

template<class T>
void Mat<T>::set(const rg &range, const Mat<T> &from)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	set(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::set(const T *vals)
{
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		M[i] = vals[i];
}

template <class T>
void Mat<T>::set(const Mat<T> &from)
{
	set(from.getMem());
}

template <class T>
void Mat<T>::extr(const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, T *v)
{
	int dr, dc, i;
	i = 0;
	for(dr = d_start_r; dr <= d_end_r; dr += d_step_r)
	{
		for(dc = dr * ncolumns + d_start_c; dc <= dr * ncolumns + d_end_c; dc += d_step_c)
		{
			v[i] = M[dc];
			i++;
		}
	}
}

template <class T>
void Mat<T>::extr(const int d_start_r, const int d_step_r, const int d_end_r,
	const int d_start_c, const int d_step_c, const int d_end_c, Mat<T> &to)
{
	int nr = (d_end_r - d_start_r) / d_step_r + 1;
	int nc = (d_end_c - d_start_c) / d_step_c + 1;
	to.init(1, nr * nc);
	extr(d_start_r,d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, (T *)to.getMem());
}

template <class T>
void Mat<T>::extr(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, T *v)
{
	extr(d_start_r, 1, d_end_r, d_start_c, 1, d_end_c, v);
}

template <class T>
void Mat<T>::extr(const int d_start_r, const int d_end_r, const int d_start_c, const int d_end_c, Mat<T> &to)
{
	extr(d_start_r, 1, d_end_r, d_start_c, 1, d_end_c, to);
}

template <class T>
void Mat<T>::extr(const rg &range, T *v)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	extr(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, v);
}

template <class T>
void Mat<T>::extr(const rg &range, Mat<T> &to)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	extr(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, to);
}

template <class T>
void Mat<T>::extr(T *vals)
{
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		vals[i] = M[i];
}

template <class T>
void Mat<T>::extr(Mat<T> &to)
{
	int n = nrows * ncolumns;
	to.init(1, n);
	extr((T *)to.getMem());
}

template<class T>
void Mat<T>::add(const int s_start_r, const int s_step_r, const int s_end_r,
		 const int s_start_c, const int s_step_c, const int s_end_c,
		 const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	if(s_start_r < 0 || s_start_r >= from.rows() || s_end_r < 0 || s_end_r >= from.rows() ||
	   s_start_c < 0 || s_start_c >= from.columns() || s_end_c < 0 || s_end_c >= from.columns())
	{
		fprintf(stderr, "Mat<T>::add - source cut is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			s_start_r, s_end_r, s_start_c, s_end_c, 0, from.rows() - 1, 0, from.columns() - 1);
		exit(1);
	}

	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::add - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int sr, sc;
	int dr, dc;
	dr = d_start_r;
	for(sr = s_start_r; sr <= s_end_r; sr += s_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(sc = sr * from.columns() + s_start_c; sc <= sr * from.columns() + s_end_c; sc += s_step_c)
		{
			M[dc] += from.M[sc];
			dc += d_step_c;
		}
		dr += d_step_r;
	}
}

template<class T>
void Mat<T>::add( const int s_start_r, const int s_end_r,
		  const int s_start_c, const int s_end_c,
                  const int d_start_r, const int d_end_r,
		  const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	add(from, s_start_r, 1, s_end_r, s_start_c, 1, s_end_c,
		d_start_r, 1, d_end_r, d_start_c, 1, d_end_c);
}

template<class T>
void Mat<T>::add(const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	add(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::add(const int d_start_r, const int d_end_r,
		 const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_step_r = 1;
	int d_step_c = 1;
	add(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::add(const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_start_r = 0;
	int d_end_r = nrows - 1;
	int d_step_r = 1;
	int d_start_c = 0;
	int d_step_c = 1;
	int d_end_c = ncolumns - 1;
	add(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::add(const int d_start_r, const int d_step_r, const int d_end_r,
                const int d_start_c, const int d_step_c, const int d_end_c, const T v)
{
	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::add - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int dr, dc;
	for(dr = d_start_r; dr <= d_end_r; dr += d_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(dc = dr * ncolumns + d_start_c; dc <= dr * ncolumns + d_end_c; dc += d_step_c)
			M[dc] += v;
	}
}

template<class T>
void Mat<T>::add(const int d_start_r, const int d_end_r,
	         const int d_start_c, const int d_end_c, const T v)
{
	add(d_start_r, 1, d_end_r, d_start_c, 1, d_end_c, v);
}

template<class T>
void Mat<T>::add(const rg &range, const Mat<T> &from)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	add(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::add(const rg &range, const T v)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	add(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, v);
}

template<class T>
void Mat<T>::add(const rg &rg_target, const Mat<T> &from, const rg &rg_source)
{
	int s_start_r, s_step_r, s_end_r;
	int s_start_c, s_step_c, s_end_c;
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	rg_source.getDim1(s_start_r, s_step_r, s_end_r);
	rg_source.getDim2(s_start_c, s_step_c, s_end_c);
	rg_target.getDim1(d_start_r, d_step_r, d_end_r);
	rg_target.getDim2(d_start_c, d_step_c, d_end_c);

	if(s_start_r == rall)
		s_start_r = 0;
	if(s_end_r == rall)
		s_end_r = from.rows() - 1;
	if(s_start_c == rall)
		s_start_c = 0;
	if(s_end_c == rall)
		s_end_c = from.columns() - 1;

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;


	add(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	     d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::sub(const int s_start_r, const int s_step_r, const int s_end_r,
		 const int s_start_c, const int s_step_c, const int s_end_c,
		 const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	if(s_start_r < 0 || s_start_r >= from.rows() || s_end_r < 0 || s_end_r >= from.rows() ||
	   s_start_c < 0 || s_start_c >= from.columns() || s_end_c < 0 || s_end_c >= from.columns())
	{
		fprintf(stderr, "Mat<T>::sub - source cut is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			s_start_r, s_end_r, s_start_c, s_end_c, 0, from.rows() - 1, 0, from.columns() - 1);
		exit(1);
	}

	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::sub - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int sr, sc;
	int dr, dc;
	dr = d_start_r;
	for(sr = s_start_r; sr <= s_end_r; sr += s_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(sc = sr * from.columns() + s_start_c; sc <= sr * from.columns() + s_end_c; sc += s_step_c)
		{
			M[dc] -= from.M[sc];
			dc += d_step_c;
		}
		dr += d_step_r;
	}
}

template<class T>
void Mat<T>::sub( const int s_start_r, const int s_end_r,
		  const int s_start_c, const int s_end_c,
                  const int d_start_r, const int d_end_r,
		  const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	sub(from, s_start_r, 1, s_end_r, s_start_c, 1, s_end_c,
		d_start_r, 1, d_end_r, d_start_c, 1, d_end_c);
}

template<class T>
void Mat<T>::sub(const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	sub(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::sub(const int d_start_r, const int d_end_r,
		 const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_step_r = 1;
	int d_step_c = 1;
	sub(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::sub(const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_start_r = 0;
	int d_end_r = nrows - 1;
	int d_step_r = 1;
	int d_start_c = 0;
	int d_step_c = 1;
	int d_end_c = ncolumns - 1;
	sub(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::sub(const rg &rg_target, const Mat<T> &from, const rg &rg_source)
{
	int s_start_r, s_step_r, s_end_r;
	int s_start_c, s_step_c, s_end_c;
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	rg_source.getDim1(s_start_r, s_step_r, s_end_r);
	rg_source.getDim2(s_start_c, s_step_c, s_end_c);
	rg_target.getDim1(d_start_r, d_step_r, d_end_r);
	rg_target.getDim2(d_start_c, d_step_c, d_end_c);

	if(s_start_r == rall)
		s_start_r = 0;
	if(s_end_r == rall)
		s_end_r = from.rows() - 1;
	if(s_start_c == rall)
		s_start_c = 0;
	if(s_end_c == rall)
		s_end_c = from.columns() - 1;

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;


	sub(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	     d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::sub(const rg &range, const Mat<T> &from)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	sub(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::mul(const int d_start_r, const int d_step_r, const int d_end_r,
                const int d_start_c, const int d_step_c, const int d_end_c, const T v)
{
	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::mul - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
	}

	int dr, dc;
	for(dr = d_start_r; dr <= d_end_r; dr += d_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(dc = dr * ncolumns + d_start_c; dc <= dr * ncolumns + d_end_c; dc += d_step_c)
			M[dc] *= v;
	}
}

template<class T>
void Mat<T>::mul(const int d_start_r, const int d_end_r,
		    const int d_start_c, const int d_end_c, const T v)
{
	mul(d_start_r, 1, d_end_r, d_start_c, 1, d_end_c, v);
}

template<class T>
void Mat<T>::mul(const int s_start_r, const int s_step_r, const int s_end_r,
		 const int s_start_c, const int s_step_c, const int s_end_c,
		 const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	if(s_start_r < 0 || s_start_r >= from.rows() || s_end_r < 0 || s_end_r >= from.rows() ||
	   s_start_c < 0 || s_start_c >= from.columns() || s_end_c < 0 || s_end_c >= from.columns())
	{
		fprintf(stderr, "Mat<T>::mul - source cut is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			s_start_r, s_end_r, s_start_c, s_end_c, 0, from.rows() - 1, 0, from.columns() - 1);
		exit(1);
	}

	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::mul - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int sr, sc;
	int dr, dc;
	dr = d_start_r;
	for(sr = s_start_r; sr <= s_end_r; sr += s_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(sc = sr * from.columns() + s_start_c; sc <= sr * from.columns() + s_end_c; sc += s_step_c)
		{
			M[dc] *= from.M[sc];
			dc += d_step_c;
		}
		dr += d_step_r;
	}
}

template<class T>
void Mat<T>::mul( const int s_start_r, const int s_end_r,
		  const int s_start_c, const int s_end_c,
                  const int d_start_r, const int d_end_r,
		  const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	mul(from, s_start_r, 1, s_end_r, s_start_c, 1, s_end_c,
		d_start_r, 1, d_end_r, d_start_c, 1, d_end_c);
}

template<class T>
void Mat<T>::mul(const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	mul(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::mul(const int d_start_r, const int d_end_r,
		 const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_step_r = 1;
	int d_step_c = 1;
	mul(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::mul(const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_start_r = 0;
	int d_end_r = nrows - 1;
	int d_step_r = 1;
	int d_start_c = 0;
	int d_step_c = 1;
	int d_end_c = ncolumns - 1;
	mul(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::mul(const rg &range, const Mat<T> &from)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	mul(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::mul(const rg &range, const T v)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	mul(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, v);
}

template<class T>
void Mat<T>::mul(const rg &rg_target, const Mat<T> &from, const rg &rg_source)
{
	int s_start_r, s_step_r, s_end_r;
	int s_start_c, s_step_c, s_end_c;
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	rg_source.getDim1(s_start_r, s_step_r, s_end_r);
	rg_source.getDim2(s_start_c, s_step_c, s_end_c);
	rg_target.getDim1(d_start_r, d_step_r, d_end_r);
	rg_target.getDim2(d_start_c, d_step_c, d_end_c);

	if(s_start_r == rall)
		s_start_r = 0;
	if(s_end_r == rall)
		s_end_r = from.rows() - 1;
	if(s_start_c == rall)
		s_start_c = 0;
	if(s_end_c == rall)
		s_end_c = from.columns() - 1;

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;


	mul(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	     d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::div(const int s_start_r, const int s_step_r, const int s_end_r,
		 const int s_start_c, const int s_step_c, const int s_end_c,
		 const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	if(s_start_r < 0 || s_start_r >= from.rows() || s_end_r < 0 || s_end_r >= from.rows() ||
	   s_start_c < 0 || s_start_c >= from.columns() || s_end_c < 0 || s_end_c >= from.columns())
	{
		fprintf(stderr, "Mat<T>::div - source cut is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			s_start_r, s_end_r, s_start_c, s_end_c, 0, from.rows() - 1, 0, from.columns() - 1);
		exit(1);
	}

	if(d_start_r < 0 || d_start_r >= nrows || d_end_r < 0 || d_end_r >= nrows ||
	   d_start_c < 0 || d_start_c >= ncolumns || d_end_c < 0 || d_end_c >= ncolumns)
	{
		fprintf(stderr, "Mat<T>::div - destination is out of matrix boundary: %d %d %d %d (%d %d %d %d)\n",
			d_start_r, d_end_r, d_start_c, d_end_c, 0, nrows - 1, 0, ncolumns - 1);
		exit(1);
	}

	int sr, sc;
	int dr, dc;
	dr = d_start_r;
	for(sr = s_start_r; sr <= s_end_r; sr += s_step_r)
	{
		dc = dr * ncolumns + d_start_c;
		for(sc = sr * from.columns() + s_start_c; sc <= sr * from.columns() + s_end_c; sc += s_step_c)
		{
			M[dc] /= from.M[sc];
			dc += d_step_c;
		}
		dr += d_step_r;
	}
}

template<class T>
void Mat<T>::div( const int s_start_r, const int s_end_r,
		  const int s_start_c, const int s_end_c,
                  const int d_start_r, const int d_end_r,
		  const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	div(from, s_start_r, 1, s_end_r, s_start_c, 1, s_end_c,
		d_start_r, 1, d_end_r, d_start_c, 1, d_end_c);
}

template<class T>
void Mat<T>::div(const int d_start_r, const int d_step_r, const int d_end_r,
		 const int d_start_c, const int d_step_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	div(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::div(const int d_start_r, const int d_end_r,
		 const int d_start_c, const int d_end_c, const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_step_r = 1;
	int d_step_c = 1;
	div(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::div(const Mat<T> &from)
{
	int s_start_r = 0;
	int s_step_r = 1;
	int s_end_r = from.rows() - 1;
	int s_start_c = 0;
	int s_step_c = 1;
	int s_end_c = from.columns() - 1;
	int d_start_r = 0;
	int d_end_r = nrows - 1;
	int d_step_r = 1;
	int d_start_c = 0;
	int d_step_c = 1;
	int d_end_c = ncolumns - 1;
	div(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	    d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::div(const rg &range, const Mat<T> &from)
{
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	range.getDim1(d_start_r, d_step_r, d_end_r);
	range.getDim2(d_start_c, d_step_c, d_end_c);

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;

	div(d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::div(const rg &rg_target, const Mat<T> &from, const rg &rg_source)
{
	int s_start_r, s_step_r, s_end_r;
	int s_start_c, s_step_c, s_end_c;
	int d_start_r, d_step_r, d_end_r;
	int d_start_c, d_step_c, d_end_c;

	rg_source.getDim1(s_start_r, s_step_r, s_end_r);
	rg_source.getDim2(s_start_c, s_step_c, s_end_c);
	rg_target.getDim1(d_start_r, d_step_r, d_end_r);
	rg_target.getDim2(d_start_c, d_step_c, d_end_c);

	if(s_start_r == rall)
		s_start_r = 0;
	if(s_end_r == rall)
		s_end_r = from.rows() - 1;
	if(s_start_c == rall)
		s_start_c = 0;
	if(s_end_c == rall)
		s_end_c = from.columns() - 1;

	if(d_start_r == rall)
		d_start_r = 0;
	if(d_end_r == rall)
		d_end_r = nrows - 1;
	if(d_start_c == rall)
		d_start_c = 0;
	if(d_end_c == rall)
		d_end_c = ncolumns - 1;


	div(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c,
	     d_start_r, d_step_r, d_end_r, d_start_c, d_step_c, d_end_c, from);
}

template<class T>
void Mat<T>::func(const mfunc1 f)
{
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		M[i] = (*f)(M[i]);
}

template<class T>
void Mat<T>::func(const mfunc2 f, const T v)
{
	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		M[i] = (*f)(M[i], v);
}

template<class T>
Mat<T> &Mat<T>::funcOp(const mfunc1 f)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	nM->func(f);
	return *nM;
}

template<class T>
Mat<T> &Mat<T>::funcOp(const mfunc2 f, const T v)
{
	Mat<T> *nM = new Mat<T>;
	*nM = *this;
	nM->created_by_op = true;
	nM->func(f, v);
	nM->dump();
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::transpose()
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	nM->init(ncolumns, nrows);
	int i, j;
	for(i = 0; i < nrows; ++i)
	{
		for(j = 0; j < ncolumns; ++j)
			nM->set(j, i, get(i, j));
	}
	return *nM;
}

/* This function perform lower-upper matrix decomposition. Because of stability rows can be exchanged and
permutations are saved to the 'idx' Mat. If the 'row_exs' variable is 1, the number of row
exchanges is even, if it is -1, the number of exchanges is odd. This variable is used mainly for determinant
calculation.

L * U = A

result:
l11 u12 u13   l11  0   0      0  u12 u13
l21 l22 u23 = l21 l22  0   +  0   0  u23
l31 l32 l33   l31 l32 l33     0   0   0
*/

template<class T>
void Mat<T>::lud_this(Mat<T> & idxs, T &row_exs)
{
	if(nrows != ncolumns)
	{
		fprintf(stderr, "Mat<T>::lud_this - the matrix must be squere one: %d x %d", nrows, ncolumns);
		exit(1);
	}
	idxs.init(1, nrows);
	row_exs = (T)1.0f;

	Mat<T> scale(1, nrows);

	// Get scaling factors
	T max, sum, v;
	int i, j, k, maxi;
	for(i = 0; i < nrows; ++i)
	{
		max = (T)0.0f;
		for(j = 0; j < nrows; ++j)
		{
			v = (T)(double) fabs((float)get(i, j));
			if(v > max)
				max =  v;
			if(max == (T)0.0f)
			{
				fprintf(stderr, "Mat<T>::lud_this - the matrix is singular.");
				exit(1);
			}
			scale.set(0, i, (T)1.0f/max);
		}
	}

	// Crout's method
	for(j = 0; j < nrows; ++j)
	{
		for(i = 0; i < j; ++i)
		{
			sum = get(i, j);
			for(k = 0; k < i; ++k)
				sum -= get(i, k) * get(k, j);
			set(i, j, sum);
		}
		// look for maximum to find out the row that will be exchanged. We need to bypass
		// division by zero
		max = (T)0.0f;
		for(i = j; i < nrows; ++i)
		{
			sum = get(i, j);
			for(k = 0; k < j; ++k)
				sum -= get(i, k) * get(k, j);
			set(i, j, sum);
			v = scale.get(0, i) * (T)fabs((float)sum);
			if(v > max)
			{
				max = v;
				maxi = i;
			}
		}
		// exchange the row
		if(j != maxi)
		{
			for(k = 0; k < nrows; ++k)
			{
				v = get(maxi, k);
				set(maxi, k, get(j, k));
				set(j, k, v);
			}
			row_exs *= (T)-1.0f;
			// exchange scale values
			scale.set(0, maxi, scale.get(0, j));
		}
		idxs.set(0, j, (T)maxi);
		if(get(j, j) == (T)0.0f)
			set(j, j, (T)1.0e-20);	// Division by zero is still there => the matrix is singular
		if(j != nrows - 1)
		{
			v = (T)1.0f / get(j, j);
			for(i = j + 1; i < nrows; ++i)
				set(i, j, v * get(i, j));
		}
	}
}

template<class T>
Mat<T> & Mat<T>::lud(Mat<T> &L, Mat<T> &U, Mat<T> & idxs, T &row_exs)
{
	L = *this;
	L.lud_this(idxs, row_exs);
	U = L;
	U.upperTriangular();
	L.lowerTriangular();
	L.setDiag((T)1.0f);
	int i;
	for(i = nrows - 1; i >= 0; --i)
		L.swapRows((int)idxs.get(0, i), i);
}

// Sets elements outside lower triangular part to zeros
template<class T>
void Mat<T>::lowerTriangular()
{
	int i, j;
	for(i = 0; i < nrows; ++i)
	{
		for(j = i + 1; j < ncolumns; ++j)
			set(i, j, (T)0.0f);
	}
}

// Sets elements outside upper triangular part to zeros
template<class T>
void Mat<T>::upperTriangular()
{
	int i, j;
	for(i = 0; i < nrows; ++i)
	{
		for(j = 0; j < i; ++j)
			set(i, j, (T)0.0f);
	}
}

template<class T>
void Mat<T>::setDiag(const T v)
{
	int i;
	for(i = 0; i < nrows; ++i)
		set(i, i, v);
}

template<class T>
void Mat<T>::setDiag(T *vals)
{
	int i;
	for(i = 0; i < nrows; ++i)
		set(i, i, vals[i]);
}

template<class T>
void Mat<T>::getDiag(T *vals)
{
	int i;
	for(i = 0; i < nrows; ++i)
		vals[i] = get(i, i);
}

template<class T>
void Mat<T>::getDiag(Mat<T> &to)
{
	if(to.rows() * to.columns() != nrows)
		to.init(1, nrows);
	getDiag((T *)to.getMem());
}

template<class T>
Mat<T> & Mat<T>::getDiagOp()
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	getDiag(*nM);
	return *nM;
}


template<class T>
void Mat<T>::swapRows(int r1, int r2)
{
	T v;
	int i;
	for(i = 0; i < ncolumns; ++i)
	{
		v = get(r1, i);
		set(r1, i, get(r2, i));
		set(r2, i, v);
	}
}

template<class T>
void Mat<T>::swapColumns(int c1, int c2)
{
	T v;
	int i;
	for(i = 0; i < nrows; ++i)
	{
		v = get(i, c1);
		set(i, c1, get(i, c2));
		set(i, c2, v);
	}
}

template<class T>
T Mat<T>::det()
{
	Mat<T> M;
	M = *this;
	Mat<T> idxs;
	T row_exs;
	M.lud_this(idxs, row_exs);
	int i;
	for(i = 0; i < nrows; ++i)
		row_exs *= M.get(i, i);

	return row_exs;
}

/* solves a linear system of equation A * x = b . This function suppose the matrix is a LU decompostion
 of matrix 'A' obtained by the 'lud_this' function. This function can be run repeatedly with diferent 'b'
 and without to much computation. 'x' is returned in 'b' due memmory allocation bypass.

 A * x = b
 L * U * x = b
 L * (U * x) = b
 L * y = b
 U * x = y
*/

template<class T>
void Mat<T>::luSolvLin(const Mat<T> &idxs, Mat<T> &b)
{
	int i, j, idx;
	T sum, v;

	// reorder 'b' acording 'idxs'
	for(i = 0; i < nrows; ++i)
	{
		idx = (int)idxs.get(0, i);
		v = b.get(i, 0);
		b.set(i, 0, b.get(idx, 0));
		b.set(idx, 0, v);

	}

	// forward substitutions
	for(i = 0; i < nrows; ++i)
	{
		sum = b.get(i, 0);
		for(j = 0; j < i; ++j)
			sum -= get(i, j) * b.get(j, 0);
		b.set(i, 0, sum);
	}

	// backward substitutions
	for(i = nrows - 1; i >= 0; --i)
	{
		sum = b.get(i, 0);
		for(j = i + 1; j < nrows; ++j)
			sum -= get(i, j) * b.get(j, 0);
		b.set(i, 0, sum / get(i, i));
	}
}

template<class T>
Mat<T> & Mat<T>::solvLin(const Mat<T> &b)
{
	Mat<T> A;
	Mat<T> idxs;
	Mat<T> *x = new Mat<T>;
	x->created_by_op = true;
	*x = b;
	T row_exs;
	A = *this;
	A.lud_this(idxs, row_exs);
	A.luSolvLin(idxs, *x);
	return *x;
}

template<class T>
Mat<T> & Mat<T>::inv()
{
	if(nrows != ncolumns)
	{
		fprintf(stderr, "Mat<T>::inv - the matrix must be squere one: %d x %d", nrows, ncolumns);
		exit(1);
	}
	Mat<T> A;
	Mat<T> idxs;
	Mat<T> b(nrows, 1);
	Mat<T> *iM = new Mat<T>;
	iM->created_by_op = true;
	iM->init(nrows, nrows);
	A = *this;
	T row_exs;
	A.lud_this(idxs, row_exs);

	int i;
	for(i = 0; i < nrows; ++i)
	{
		b.set((T)0.0f);
		b.set(i, 0, (T)1.0f);
		A.luSolvLin(idxs, b);
		iM->set(0, nrows - 1, i, i, b);
	}
	return *iM;
}

template<class T>
Mat<T> & Mat<T>::operator *= (const Mat &from)
{
	Mat<T> *nM = new Mat<T>;
	*nM = *this * from;
	*this = *nM;
	delete nM;
	return *this;
}

template<class T>
Mat<T> & Mat<T>::operator / (const T v)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	*nM = *this;
	*nM /= v;
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::operator * (const T v)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	*nM = *this;
	*nM *= v;
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::operator + (const T v)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	*nM = *this;
	*nM += v;
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::operator - (const T v)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	*nM = *this;
	*nM -= v;
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::mulOp(const Mat<T> &from)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	*nM = *this;
	nM->mul(from);
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::operator + (const Mat &from)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	*nM = *this;
	*nM += from;
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::operator - (const Mat &from)
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	*nM = *this;
	*nM -= from;
	return *nM;
}

template<class T>
void Mat<T>::sumRows(Mat<T> &to)
{
	if(to.rows() * to.columns() != nrows)
		to.init(nrows, 1);
	int i, j;
	T sum;
	T *mem = (T *)to.getMem();
	for(i = 0; i < nrows; ++i)
	{
		sum = (T)0.0f;
		for(j = i * ncolumns; j < (i + 1) * ncolumns; ++j)
			sum += M[j];
		mem[i] = sum;
	}
}

template<class T>
void Mat<T>::sumColumns(Mat<T> &to)
{
	if(to.rows() * to.columns() != ncolumns)
		to.init(1, ncolumns);
	int i, j;
	T sum;
	T *mem = (T *)to.getMem();
	for(i = 0; i < ncolumns; ++i)
	{
		sum = (T)0.0f;
		for(j = 0; j < nrows; ++j)
			sum += get(j, i);
		mem[i] = sum;
	}
}

template<class T>
Mat<T> & Mat<T>::sumRowsOp()
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	sumRows(*nM);
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::sumColumnsOp()
{
	Mat<T> *nM = new Mat<T>;
	nM->created_by_op = true;
	sumColumns(*nM);
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::submatrix(const int s_start_r, const int s_step_r, const int s_end_r,
	           const int s_start_c, const int s_step_c, const int s_end_c)
{
	int nr = (s_end_r - s_start_r) / s_step_r + 1;
	int nc = (s_end_c - s_start_c) / s_step_c + 1;
	Mat<T> *nM = new Mat<T>;
	nM->init(nr, nc);
	nM->created_by_op = true;
	nM->copy(*this, s_start_r, s_step_r, s_end_r,
	               s_start_c, s_step_c, s_end_c,
		       0, 1, nr - 1,
		       0, 1, nc - 1);
	return *nM;
}

template<class T>
Mat<T> & Mat<T>::submatrix(const int s_start_r, const int s_end_r,
		   const int s_start_c, const int s_end_c)
{
	int s_step_r = 1;
	int s_step_c = 1;
	return submatrix(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c);
}

template<class T>
Mat<T> & Mat<T>::submatrix(const rg &range)
{
	int s_start_r, s_step_r, s_end_r;
	int s_start_c, s_step_c, s_end_c;

	range.getDim1(s_start_r, s_step_r, s_end_r);
	range.getDim2(s_start_c, s_step_c, s_end_c);

	if(s_start_r == rall)
		s_start_r = 0;
	if(s_end_r == rall)
		s_end_r = nrows - 1;
	if(s_start_c == rall)
		s_start_c = 0;
	if(s_end_c == rall)
		s_end_c = ncolumns - 1;
	return submatrix(s_start_r, s_step_r, s_end_r, s_start_c, s_step_c, s_end_c);
}


template<class T>
void Mat<T>::saveAscii(const char *file)
{
	FILE *fp;
	fp = fopen(file, "w");
	if(!fp)
	{
		fprintf(stderr, "Mat<T>::saveAscii - can not create the file: %s\n", file);
		exit(1);
	}

	int i, j;
	for(i = 0; i < nrows; ++i)
	{
		fprintf(fp, "%e", (T)get(i, 0));
		for(j = 1; j < ncolumns; ++j)
			fprintf(fp, " %e", (T)get(i, j));
		fprintf(fp, "\n");
	}
	fclose(fp);
}

template<class T>
void Mat<T>::saveBinary(const char *file)
{
	FILE *fp;
	fp = fopen(file, "wb");
	if(!fp)
	{
		fprintf(stderr, "Mat<T>::saveBinary - can not create the file: %s\n", file);
		exit(1);
	}

	float nr = (float)nrows;
	float nc = (float)ncolumns;

	write(fileno(fp), &nr, sizeof(nr));
	write(fileno(fp), &nc, sizeof(nr));

	float *mem = new float[nrows * ncolumns];

	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		mem[i] = (float)M[i];

	write(fileno(fp), mem, nrows * ncolumns * sizeof(float));

	delete [] mem;

	fclose(fp);
}

template<class T>
void Mat<T>::loadAscii(const char *file)
{
	FILE *fp;
	fp = fopen(file, "r");
	if(!fp)
	{
		fprintf(stderr, "Mat<T>::loadAscii - can not open the file: %s\n", file);
		exit(1);
	}

	struct CHAIN
	{
		float vals[5000];
		struct CHAIN *next;
	} *first, *chank;

	first = new struct CHAIN;
	chank = first;

	int nr = 0;
	int nc = -1;
	int nc1 = 0;
	int idx = 0;
	int ret;

	char buff[256];

	while(!feof(fp))
	{
		ret = fscanf(fp, "%f%256[^-+.0123456789\n\r]", &chank->vals[idx], buff);
		if(ret != 0)
		{
			idx++;
			nc1++;
			if(idx == 5000)
			{
				idx = 0;
				struct CHAIN *nch = new struct CHAIN;
				chank->next = nch;
				chank = nch;
			}
		}
		ret = fscanf(fp, "%256[\n\r]", buff);
		if(ret != 0)
		{
			if(nc != nc1 && nc != -1 && nc1 != 0)
			{
				fprintf(stderr, "Mat<T>::loadAscii - rows must have the equal number of values: %s\n", file);
				exit(1);
			}
			if(nc1 != 0)
			{
				nc = nc1;
				nr++;
				nc1 = 0;
			}
		}
	}

	if(nc1 != 0)  // last row does not contain the newline character
	{
		if(nc != nc1 && nc != -1)
		{
			fprintf(stderr, "Mat<T>::loadAscii - rows must have the equal number of values: %s\n", file);
			exit(1);
		}
		nr++;
		nc = nc1;
		nc1 = 0;
	}

	init(nr, nc);
	idx = 0;
	chank = first;
	int i;
	for(i = 0; i < nr * nc; ++i)
	{
		M[i] = (T)chank->vals[idx];
		idx++;
		if(idx == 5000)
		{
			first = chank;
			chank = chank->next;
			delete first;
			idx = 0;
		}

	}
	delete chank;

	fclose(fp);
}

template<class T>
void Mat<T>::loadBinary(const char *file)
{
	FILE *fp;
	fp = fopen(file, "rb");
	if(!fp)
	{
		fprintf(stderr, "Mat<T>::loadBinary - can not open the file: %s\n", file);
		exit(1);
	}

	float nr, nc;

	read(fileno(fp), &nr, sizeof(nr));
	read(fileno(fp), &nc, sizeof(nr));

	nrows = (int)nr;
	ncolumns = (int)nc;

	init(nrows, ncolumns);

	float *mem = new float[nrows * ncolumns];

	read(fileno(fp), mem, nrows * ncolumns * sizeof(float));

	int i;
	for(i = 0; i < nrows * ncolumns; ++i)
		M[i] = (T)mem[i];

	delete [] mem;

	fclose(fp);
}

template<class T>
void Mat<T>::balance()
{
	int last, i, j;
	T s, g, f, sqrdx;
	T rn, cn;

	sqrdx = (T)4.0f;
	last = 0;
	while(last == 0)
	{
		last = 1;
		for(i = 0; i < nrows; ++i)
		{
			cn = (T)0.0f;
			rn = (T)0.0f;
			for(j = 0; j < ncolumns; ++j)
			{
				if(j != i)
				{
					cn += (T)fabs((float)get(j, i));
					rn += (T)fabs((float)get(i, j));
				}
			}
			if(cn != (T)0.0f && rn != (T)0.0f)
			{
				g = rn / (T)2.0f;
				f = (T)1.0f;
				s = cn + rn;
				while(cn < g)
				{
					f *= (T)2.0f;
					cn *= sqrdx;
				}
				g = rn * (T)2.0f;
				while(cn > g)
				{
					f /= (T)2.0f;
					cn /= sqrdx;
				}
				if((cn + rn) / f < (T)0.95f * s)
				{
					last = 0;
					g = (T)1.0f / f;
					for(j = 0; j < ncolumns; ++j)
						set(i, j, get(i, j) * g);
					for(j = 0; j < ncolumns; ++j)
						set(j, i, get(j, i) * f);
				}
			}

		}
	}
}

// Reduct the matrix to upper Hessenberg form (upper diagonal matrix + one subdiagonal)
// This procedure is used for eigen vaues calculations
template<class T>
void Mat<T>::hessenberg()
{
	int i, j, k;
	int maxi;
	T max, v;

	for(i = 0; i < nrows - 1; ++i)
	{
		max = (T)0.0f;
		for(j = i + 1; j < nrows; ++j)
		{
			if(get(i, j) > max)
			{
				max = get(i, j);
				maxi = j;
			}
		}
		if(max != (T)0.0f)
		{
			if(maxi != i)
			{
				swapRows(maxi, i + 1);
				swapColumns(maxi, i + 1);
			}
			for(j = i + 2; j < nrows; ++j)
			{
				v = get(j, i) / get(i + 1, i);
				for(k = 0; k < nrows; ++k)
					set(j, k, (get(j, k) - v * get(i + 1, k)));
				for(k = 0; k < nrows; ++k)
					set(k, i + 1, get(k, i + 1) + v * get(k, j));
			}
		}
	}
}

/*
template<class T>
void Mat<T>::eig(Mat<T> &real, Mat<T> &imag)
{
	int nn, m, l, k, j, its, i, mmin;
	T z, y, x, w, v, u, t, s, r. q, p, anorm;
	
	anorm = (T)0.0f;
	for(i = 0; i < nrows; ++i)
	{
		for(j = (i - 1 > 0 ? i - 1 : 0); j < nrows; ++j)
			anorm += (T)fabs(get(i, j));

		nn = nrows - 1;
		t = (T)0.0f;
		while(nn >= 0)
		{
			its = 0;
			do
			{
				for(l = nn; l >= 2; l--)
				{
					s = (T)fabs((float)get(l - 1, l - 1)) + (T)fabs((float)get(l, l));
					if(s == (T)0.0f)
						s = anorm;
					if((T)fabs((float)(get(l, l - 1) + s)) == s)
						break;
				}
				x = get(nn, nn);
				if(l == nn)		// one root
				{
					wr.M[nn] = x + t;
					wi.M[nn--] = (T)0.0f;
				}
				else			// two roots
				{
					y = get(nn - 1, nn - 1);
					w = get(nn, nn - 1) * get(nn - 1, nn);
					if(l == (nn - 1))
					{
						
					}
				}

			}
		}
	}
}
*/

template<class T>
bool Mat<T>::saveHTK(const char *file)
{
        // open file
        FILE *fp = fopen(file, "wb");
        if(!fp)
        {
                //fprintf(stderr, "Mat<T>::saveBinary - can not create the file: file\n");
                return false;
        }

        // fill in header
        htk_hdr.sampSize = columns() * sizeof(float);
        htk_hdr.nSamples = rows();

        // swap byte order
        int i;
        for(i = 0; i < rows() * columns(); i++)
                swap4b(&M[i]);

        swap4b(&htk_hdr.nSamples);
        swap4b(&htk_hdr.sampPeriod);
        swap2b(&htk_hdr.sampSize);
        swap2b(&htk_hdr.paramKind);

        // write the file
        write(fileno(fp), &htk_hdr, sizeof(htk_hdr));
        write(fileno(fp), M, rows() * columns() * sizeof(float));
        fclose(fp);

        // swap byte order back
        for(i = 0; i < rows() * columns(); i++)
                swap4b(&M[i]);

        swap4b(&htk_hdr.nSamples);
        swap4b(&htk_hdr.sampPeriod);
        swap2b(&htk_hdr.sampSize);
        swap2b(&htk_hdr.paramKind);
	return true;
}

template<class T>
bool Mat<T>::loadHTK(const char *file)
{
        FILE *fp = fopen(file, "rb");
        if(!fp)
        {
                //fprintf(stderr, "Mat<T>::loadHTK - can not open the file: file\n", file);
                //exit(1);
		return false;
	}
	
        read(fileno(fp), &htk_hdr, sizeof(htk_hdr));

        swap4b(&htk_hdr.nSamples);
        swap4b(&htk_hdr.sampPeriod);
        swap2b(&htk_hdr.sampSize);
        swap2b(&htk_hdr.paramKind);

        init(htk_hdr.nSamples, htk_hdr.sampSize / sizeof(float));

        read(fileno(fp), M, rows() * columns() * sizeof(float));
        fclose(fp);

        int i;
        for(i = 0; i < rows() * columns(); i++)
                swap4b(&M[i]);
	return true;
}

template<class T>
inline void Mat<T>::swap4b(void *a)
{
        char *b = (char *)a;
        char c;
        c = b[0]; b[0] = b[3]; b[3] = c;
        c = b[1]; b[1] = b[2]; b[2] = c;
}

template<class T>
inline void Mat<T>::swap2b(void* a)
{
        char *b = (char *)a;
        char c;
        c = b[0]; b[0] = b[1]; b[1] = c;
}

template<class T>
inline void Mat<T>::upperLimit(const T thr)
{
	int i, j;
	for(i = 0; i < nrows; i++)
	{
		for(j = 0; j < ncolumns; j++)
		{
			float v = get(i, j);
			if(v > thr)
				v = thr;
			set(i, j, v);
		}
	}
}
		
template<class T>
inline void Mat<T>::lowerLimit(const T thr)
{
	int i, j;
	for(i = 0; i < nrows; i++)
	{
		for(j = 0; j < ncolumns; j++)
		{
			float v = get(i, j);
			if(v < thr)
				v = thr;
			set(i, j, v);
		}
	}
}
#endif
