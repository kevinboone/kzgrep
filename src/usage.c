/*==========================================================================

  kzgrep
  usage.c
  Copyright (c)2020 Kevin Boone
  Distributed under the terms of the GPL v3.0

==========================================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "feature.h" 
#include "usage.h" 


/*==========================================================================
  usage_show
==========================================================================*/
void usage_show (FILE *fout, const char *argv0)
  {
  fprintf (fout, "Usage: %s [options] {pattern} {files}\n", argv0);
  fprintf (fout, "  -a,--all                include hiden paths\n");
  fprintf (fout, "  -?,--help               show this message\n");
  fprintf (fout, "     --entries=patterns   include entries with patterns\n");
  fprintf (fout, "     --files=patterns     include files wth patterns\n");
  fprintf (fout, "  -e,--no-entryname       don't show entry filenames\n");
  fprintf (fout, "  -f,--first              stop after first matching entry\n");
  fprintf (fout, "  -i,--ignore-case        ignore letter case\n");
  fprintf (fout, "  -h,--no-filename        suppress filename output\n");
  fprintf (fout, "  -I,--no-binary          ignore binary entries\n");
  fprintf (fout, "  -l,--log-level=N        log level, 0-5 (default 2)\n");
  fprintf (fout, "  -m,--max-size=N         max size of compressed entry\n");
  fprintf (fout, "  -n,--line-number        show matching line numbers\n");
  fprintf (fout, "  -o,--word-regexp        'word match' mode\n");
  fprintf (fout, "  -q,--quiet              produce no normal output\n");
  fprintf (fout, "  -r,--recurse            expand directories\n");
  fprintf (fout, "     --text               treat all entries as text\n");
  fprintf (fout, "  -v,--version            show version\n");
  fprintf (fout, "  -w,--width=N            set text output width; 0=all\n");
  }

 
