/**************************************************************************
 *  copyright           : (C) 2004-2006 by Lukas Burget UPGM,FIT,VUT,Brno *
 *  email               : burget@fit.vutbr.cz                             *
 **************************************************************************
 *                                                                        *
 *  This program is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 **************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "getopt.h"


char *optarg;
int optind=0;

int getopt(int argc, char **argv, char *options)
{
  if(++optind == argc) return -1;

  if(argv[optind][0] == '-' && isalpha(argv[optind][1])) {
    char *opt = strchr(options, argv[optind][1]);
    if(opt == NULL) return '?';
    if(opt[1] == ':') {
      if(argv[optind][2]) {
        optarg = &argv[optind][2];
      } else {
        if(++optind == argc) return '?';
        optarg = argv[optind];
      }
    }
    return opt[0];
  }
  optarg = argv[optind];
  return 1;
}
