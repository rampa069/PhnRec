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

#include "myrand.h"

static unsigned int myrand_next = 1;

int myrand()
{
        myrand_next = myrand_next * 1103515245 + 12345;
        return ((myrand_next >> 16 ) & MYRAND_MAX);
}

void mysrand(unsigned int seed)
{
        myrand_next = seed;
}
