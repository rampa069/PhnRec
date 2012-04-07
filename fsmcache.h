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

#ifndef _FSMCACHE_H
#define _FSMCACHE_H

#include <assert.h>

class FSMCache
{
	protected:
		int blockSize;
		int nBlocks;
		int nFreeBlocks;
		int nPages;
		void *firstPage;
		void *firstFreeBlock;
	public:
		FSMCache();
		void SetPageSize(int _blockSize, int _nBlocks);
		void *Alloc();
		void Free(void *block);
};

#endif



