/*==========================================================================

  kzgrep
  zipfile.h
  Copyright (c)2020 Kevin Boone
  Distributed under the terms of the GPL v3.0

==========================================================================*/

#pragma once

#include <stdint.h>
#include "defs.h"
#include "buffer.h"
#include "feature.h"

#ifdef FEATURE_ZIPFILE


typedef enum
  {
  ZE_OK = 0,
  ZE_OPENREAD = 1,
  ZE_BADZIP = 2,
  // ZE_CD is an internal code, indicating that we hit the 'end central 
  //  directory' when building the index. It's only an error, and then
  //  only just, if it's the first entry in the file. No method should
  //  return this code to callers
  ZE_CD = 3, 

  // We only support DEFLATE (and uncompressed) entries
  ZE_UNSUPPORTED_COMP = 4,
  ZE_OPENWRITE = 5,
  // Zip structure OK, but compressed data defective in some way
  ZE_CORRUPT = 6,
  ZE_INTERNAL = -1
  } ZipError;

struct _ZipFile;
typedef struct _ZipFile ZipFile;

BEGIN_DECLS

ZipFile *zipfile_create (const char *filename);
void     zipfile_destroy (ZipFile *self);
ZipError zipfile_read_contents (ZipFile *self);
int      zipfile_get_num_entries (const ZipFile *self);
void     zipfile_get_entry_details (const ZipFile *self, 
           int n, char *filename, int max_filename, uint64_t *size);
ZipError zipfile_extract_to_file (const ZipFile *self, int entry, 
           const char *filename);
ZipError zipfile_extract_to_memory (const ZipFile *self, int n, 
           BYTE **out, uint64_t *length);
int      zipfile_extract_all (const ZipFile *self, 
           const char *extract_path, BOOL carry_on);
ZipError zipfile_extract_to_buffer (const ZipFile *self, int n, 
            Buffer **buffer);
const char *zipfile_get_filename (const ZipFile *self);

END_DECLS

#endif // FEATURE_ZIPFILE 

