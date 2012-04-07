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

#include "encode.h"
#include "myrand.h"

void rand_encode(char *message, int len, int key, char xor_mask)
{
        mysrand(key);
        int i;
        for(i = 0; i < len; i++)
        {
                char mask = myrand() % 0xFF;
                message[i] = message[i] ^ mask ^ xor_mask;
        }
}
