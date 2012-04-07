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
#include "fsmcache.h"

FSMCache::FSMCache() :
	blockSize(0),
	nBlocks(0),
	nFreeBlocks(0),
	nPages(0),
	firstPage(0),
	firstFreeBlock(0)
{
	;
}

void FSMCache::SetPageSize(int _blockSize, int _nBlocks)
{
	assert(_blockSize >= (int)sizeof(void *) && _nBlocks >= 1);

	blockSize = _blockSize;
	nBlocks = _nBlocks;
}

void *FSMCache::Alloc()
{
	//puts("aaa");
	if(nFreeBlocks == 0)
	{
		void *page = (void *)new char [blockSize * nBlocks + sizeof(void *)];
		((void **)page)[0] = firstPage;
		firstPage = page;
		nPages++;
		char *block = (char *)page + sizeof(void *);
		int i;
		//printf("%d\n", nBlocks);
		for(i = 0; i < nBlocks; i++)
		{
			((void **)block)[0] = firstFreeBlock;
			assert(block);
			firstFreeBlock = block;
			block += blockSize;
			nFreeBlocks++;
		}
		//printf("%d\n", nFreeBlocks);
	}

	//printf("%d\n", nFreeBlocks);

	void *retBlock = firstFreeBlock;
	firstFreeBlock = ((void **)firstFreeBlock)[0];
	nFreeBlocks--;
	assert(retBlock);
	return retBlock;
}

void FSMCache::Free(void *block)
{
	//puts("bbb");
	((void **)block)[0] = firstFreeBlock;
	firstFreeBlock = block;
	nFreeBlocks++;
}

