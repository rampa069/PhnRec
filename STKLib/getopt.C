#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "getopt.h"

char *optarg;
int optind=0;

int getopt(int argc, char **argv, char *options)
{
  if (++optind == argc) return -1;

  if (argv[optind][0] == '-' && isalpha(argv[optind][1])) {
    char *opt = strchr(options, argv[optind][1]);
    if (opt == NULL) return '?';
    if (opt[1] == ':') {
      if (argv[optind][2]) {
        optarg = &argv[optind][2];
      } else {
        if (++optind == argc) return '?';
        optarg = argv[optind];
      }
    }
    return opt[0];
  }
  optarg = argv[optind];
  return 1;
}
