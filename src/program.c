/*==========================================================================

  kzgrep 
  program.c
  Copyright (c)2020 Kevin Boone
  Distributed under the terms of the GPL v3.0

  This file contains the main body of the program. By the time
  program_run() has been called, RC files will have been read and comand-
  line arguments parsed, so all the contextual information will be in the
  ProgramContext. Logging will have been initialized, so the log_xxx
  methods will work, and be filtered at the appopriate levels.
  The unparsed command-line arguments will be available
  in the context as nonswitch_argc and nonswitch_argv.

==========================================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include <errno.h>
#include <regex.h>
#include <pcre.h>
#include "feature.h" 
#include "program_context.h" 
#include "program.h" 
#include "file.h" 
#include "list.h" 
#include "path.h" 
#include "zipfile.h" 
#include "usage.h" 
#include "numberformat.h" 
#include "console.h" 
#include "wstring.h" 

// Forward declaration
int program_do_file_or_dir (const ProgramContext *context, 
       const pcre *, const char *arg, BOOL *did_something);

/*==========================================================================
  program_do_dir

  Expands the specified path, which must have been determined previously
    to be a directory. Process all files in the directory that match
    the inclusion criteria.
 
  Returns the total number of matches in all files.
==========================================================================*/
int program_do_dir (const ProgramContext *context, const pcre *preg,
    const Path *path, BOOL *did_something)
  {
  LOG_IN
  int matches = 0;

  BOOL all = program_context_get_boolean (context, "all", FALSE);

  List *list = NULL;
  int flags = FE_DEFAULT | FE_PREPEND_PATH;
  if (all) flags |= FE_HIDDEN;

  if (path_expand_directory (path, flags, &list))
    {
    int l = list_length (list);
    for (int i = 0; i < l; i++)
      {
      const String *s = list_get (list, i);
      Path *newpath = path_create (string_cstr(s));
      char *s_newpath = (char *)path_to_utf8 (newpath);
      matches += program_do_file_or_dir (context, preg, s_newpath, 
        did_something);
      free (s_newpath);
      path_destroy (newpath);
      }
    }
  else
   {
   char *s_path = (char *) path_to_utf8 (path);
   log_warning ("'%s': %s", s_path, strerror(errno)); 
   free (s_path);
   }
  if (list) list_destroy (list);

  LOG_OUT
  return matches;
  } 

/*==========================================================================
  program_zip_strerror

  Get a human-readable (English-only :/) message for a zip error
==========================================================================*/
const char *program_zip_strerror (ZipError error)
  {
  LOG_IN
  const char *ret = "Unknown error";
  switch (error)
    {
    case ZE_OK: ret = "OK"; break;
    case ZE_OPENREAD: ret = "Can't open file for reading"; break;
    case ZE_OPENWRITE: ret = "Can't open file for writing"; break;
    case ZE_BADZIP: ret = "Not a zipfile"; break;
    case ZE_CD: ret = "Not a zipfile"; break;
    case ZE_CORRUPT: ret = "Damaged or unsupported zipfile"; break;
    case ZE_UNSUPPORTED_COMP: ret = "Unsupported compression method"; break;
    case ZE_INTERNAL: ret = "Internal error"; break;
    }
  LOG_OUT
  return ret;
  }


/*==========================================================================
  program_is_utf8
  Attempt to determine whether a block of data _could_ be UTF-8. The
    length is constrained, to avoid reading vast files. However, the
    shorter the length, the likelier it is that a non-UTF8 file will 
    sneak through.
==========================================================================*/
BOOL program_is_utf8 (BYTE *data, int length)
  {
  LOG_IN
  if (length > 200) // We may need to tweak this later
    length=200;
  int bytes = 1;
  BYTE c;
  for (int i = 0; i < length; i++)
    {
    c = data[i];
    if (bytes == 1)
      {
      if (c >= 0x80)
        {
        while (((c <<= 1) & 0x80) != 0)
          {
          bytes++;
          }
        if (bytes == 1 || bytes > 6)
          {
          return FALSE;
          }
        }
      }
    else
      {
      if ((c & 0xC0) != 0x80)
        {
        return FALSE;
        }
      bytes--;
      }
    }
  if (bytes > 1)
    {
    return FALSE; 
    }
  LOG_OUT
  return TRUE;
  }
 

/*==========================================================================
  program_truncate_and_print_line

  Fit a line of text to the width specified in the context, and
    (if output is to a console) highlight the text between the 
    specified start and end points.

  Note that the function only provides the wherewithall to highlight 
    a single block of text, regardless of the nuber of matches
    there actually were. This is a limitation that might need attention
    later.
==========================================================================*/
void program_truncate_and_print_line (const ProgramContext *context, 
      const UTF8 *line, int hi_start, int hi_end) 
    {
    LOG_IN
   
    log_debug ("%s: %S", __PRETTY_FUNCTION__, line);

    int width = program_context_get_integer (context, "width", 0);

    int line_length = strlen ((char *)line); 
    if (line_length < width || width == 0)
      {
      for (int i = 0; i < line_length; i++)
        {
        if (i == hi_start) console_fg_colour (CC_RED, FALSE);
        char c = line[i];
        if (i == hi_end) console_fg_colour (CC_DEFAULT, FALSE);
        putchar (c);
        }
      printf ("\n");
      }
    else
      {
      if (hi_start < width / 2)
        {
        for (int i = 0; i < width; i++)
          {
          if (i == hi_start) console_fg_colour (CC_RED, FALSE);
          char c = line[i];
          if (i == hi_end) console_fg_colour (CC_DEFAULT, FALSE);
          putchar (c);
          }
        }
      else if (hi_start > line_length - width / 2)
        {
        for (int i = 0; i < width; i++)
          {
          int ps = line_length - width;
          if (i == hi_start - ps) console_fg_colour (CC_RED, FALSE);
          char c = line[i + ps];
          if (i == hi_end - ps) console_fg_colour (CC_DEFAULT, FALSE);
          putchar (c);
          }
        }
      else 
        {
        for (int i = 0; i < width; i++)
          {
          int ps = hi_start - width / 2;
          if (i == hi_start - ps) console_fg_colour (CC_RED, FALSE);
          char c = line[i + ps];
          if (i == hi_end - ps) console_fg_colour (CC_DEFAULT, FALSE);
          putchar (c);
          }
        }
      printf ("\n");
      }

  console_fg_colour (CC_DEFAULT, FALSE); // TOD -- only if changed
  
  LOG_OUT
  }


/*==========================================================================
  program_grep_binary
 
  Search for the specified regex in the buffer. If found, display the
    match, and return TRUE
==========================================================================*/
BOOL program_grep_binary (const ProgramContext *context, 
       const char *zip_filename, const char *int_filename, 
       const pcre *preg, const BYTE *buff, int length)
  {
  LOG_IN
  BOOL ret = FALSE;
  char *buff2 = malloc (length + 1);
  memcpy (buff2, buff, length);
  buff2[length] = 0;
  BOOL quiet = program_context_get_boolean (context, "quiet", FALSE);

  BOOL no_entries = program_context_get_boolean 
          (context, "no-entryname", FALSE);

  for (int i = 0; i < length; i++)
    if (buff2[i] == 0) buff2[i] = ' ';

  int pmatch[30];

  int m = pcre_exec (preg, NULL, (const char *)buff2, 
           length, 0, 0, pmatch, 30);
  if (m > 0)
    {
    if (!quiet)
      {
      console_write_attribute (CA_BRIGHT, FALSE);
      printf ("%s:", zip_filename);
      if (!no_entries)
        printf ("%s:", int_filename);
      console_write_attribute (CA_NORMAL, FALSE);
      printf ("binary file matches\n");
      }
    ret = TRUE;
    }
  free (buff2);
  LOG_OUT
  return ret;
  }


/*==========================================================================
  program_grep_utf8_line
  
  Search for the regular expression in the specified line. If 
    found, display the result and return TRUE
==========================================================================*/
BOOL program_grep_utf8_line (const ProgramContext *context, 
       const char *zip_filename, const char *int_filename, 
       const pcre *preg, const UTF8 *line, int line_number)
  {
  LOG_IN
  BOOL ret = FALSE;
  BOOL quiet = program_context_get_boolean (context, "quiet", FALSE);
  BOOL line_numbers = program_context_get_boolean 
          (context, "line-number", FALSE);
  BOOL no_entries = program_context_get_boolean 
          (context, "no-entryname", FALSE);

  int pmatch[30];
  int m = pcre_exec (preg, NULL, (const char *)line, 
           strlen ((char *)line), 0, 0, pmatch, 30);
  if (m > 0)
    {
    ret = TRUE;
    if (!quiet)
      {
      if (!program_context_get_boolean (context, "no-filename", FALSE))
        {
        console_write_attribute (CA_BRIGHT, FALSE);
        printf ("%s:", zip_filename);
        if (!no_entries)
          printf ("%s:", int_filename);
        if (line_numbers && !no_entries)
          printf ("%d:", line_number);
        console_write_attribute (CA_NORMAL, FALSE);
        }
    
      program_truncate_and_print_line (context, line, 
         pmatch[0], pmatch[1]);
      }
    }

  LOG_OUT
  return ret;
  }

/*==========================================================================
  program_grep_utf8
  Split the data buffer into lines, and scan each one. This whole thing
    needs to be tidied up so as to avoid possibly mistaking part of a 
    multi-byte character for a end-of-line. The easiest way to do this
    would be to convert the entire buffer into UTF32 and do the processing
    with integers rather than bytes; but this wil be very memory 
    intensive. Moreover, the regex library only works with byte-size
    characters, so we would have to convert repeatedly. 
  
  This funnction returns the number of lines that match.
==========================================================================*/
int program_grep_utf8 (const ProgramContext *context, 
       const char *zip_filename, const char *int_filename, 
       const pcre *preg, const UTF8 *buff, int length)
  {
  LOG_IN
  int matches = 0;
  BOOL stop = FALSE;
  BOOL quiet = program_context_get_boolean (context, "quiet", FALSE);
  BOOL first = program_context_get_boolean (context, "first", FALSE);

  char *b = (char *)buff;
  char *lastb = b;
  int lines = 0;
  int i = 0;

  do 
    {
    char c = 0;
    if (i != length) c = *b;
    if (c == (char) '\n' || i == length)
      {
      lines++;
      int linelen = b - lastb;
      if (linelen > 0)
        {
        char *line = malloc ((linelen + 1) * sizeof (char));
        memcpy (line, lastb, linelen * sizeof (char));
        line[linelen] = 0;
        if (line[0] != (char)'\n')
          {
          if (program_grep_utf8_line (context, zip_filename, int_filename, 
                preg, (UTF8 *)line, lines))
            matches++;
          }
        free (line);
        }
      lastb = b + 1; // Skip over the \n so it is not included
      }
    b++;
    i++;
    if (matches > 0 && (quiet || first)) stop = TRUE;
    } while (i <= length && !stop);
  LOG_OUT
  return matches;
  }


/*==========================================================================
  program_do_entry
  
  Process a specific entry from the zipfile, which may be text or non-text,
    but at this point is assumed to be a viable target (entry filename
    matches, etc); 
 
  Returns the number of matching lines for a text entry, and either 0
    or 1 for a non-text entry
==========================================================================*/
int program_do_entry (const ProgramContext *context, const ZipFile *z,
       const pcre *preg, int n)
  {
  LOG_IN
  int matches = 0;
  uint64_t size;
  char int_filename[PATH_MAX];
  const char *zip_filename = zipfile_get_filename (z);

  zipfile_get_entry_details (z, n, int_filename, 
        sizeof (int_filename), &size); 
  
  BOOL force_text = program_context_get_boolean (context, "text", 
         FALSE);

  BYTE *buff = NULL;
  uint64_t length;
  ZipError error = zipfile_extract_to_memory (z, n, &buff, &length);
  if (!error)
    {
    if (program_is_utf8 (buff, length) || force_text) 
      {
      log_debug ("Assuming %s is UTF8", int_filename);
      matches += program_grep_utf8 (context, zip_filename, int_filename, 
                   preg, buff, length);
      }
    else
      {
      BOOL do_binary = TRUE;
      if (program_context_get_boolean (context, "no-binary", FALSE)) 
        do_binary = FALSE;
      if (do_binary)
        {
        matches += program_grep_binary (context, zip_filename, 
                     int_filename, preg, buff, length);
        }
      else
        log_debug ("Skipping binary file %s", int_filename);
      }
    }
  else log_warning ("%s!%s: %s", zip_filename, int_filename, 
        program_zip_strerror (error));

  if (buff) free (buff);

  LOG_OUT
  return matches;
  }


/*==========================================================================
  program_match_filename

  Returns true if the filename matches the pattern stored in the 
    context. This function is used both for filesystem filenames and
    entry filenames, according to the value of zip_entries. These
    different kinds of filename have different properties in the
    context, and different case-sensitivity.
==========================================================================*/
BOOL program_match_filename (const ProgramContext *context, 
      const char *filename, BOOL zip_entries)
  {
  LOG_IN
  BOOL ret = FALSE;

  const char *include; 
  if (zip_entries)
    include = program_context_get (context, "entries");
  else
    include = program_context_get (context, "files");
 
  if (include)
    {
    String *s = string_create (include);
    List *list = string_split (s, ",");
    int l = list_length (list);
    for (int i = 0; i < l && !ret; i++) 
      {
      const String *pattern = list_get (list, i);
      const char *s_pattern = string_cstr (pattern);
      
      // Skip the path part, if there is one. We only want to 
      //  check filenames
      const char *fnstart = filename;
      char *sep = strrchr (filename, '/');
      if (sep)
        fnstart = sep + 1; 

      if (file_name_matches_pattern_case (fnstart, s_pattern, zip_entries))
        ret = TRUE;
      }
    list_destroy (list);
    string_destroy (s);
    }
  else
    ret = TRUE;

  LOG_OUT
  return ret;
  }


/*==========================================================================
  program_consider_entry
  
  Consider unpacking the n'th entry in ZipFile z and, if the entry 
    filename matches the inclusion criteria, send it for searching

  By the time this method is called, we have already established that the
    zipfile is valid, and the entry is of non-zero size.

  Returns the number of matches found in those files that were actually
    searched.
==========================================================================*/
int program_consider_entry (const ProgramContext *context, const ZipFile *z,
       const pcre *preg, int n, BOOL *did_something)
  {
  LOG_IN
  int matches = 0;
  uint64_t size;
  char int_filename[PATH_MAX];
  const char *zip_filename = zipfile_get_filename (z);

  zipfile_get_entry_details (z, n, int_filename, 
        sizeof (int_filename), &size); 

  if (program_match_filename (context, int_filename, TRUE))
    {
    uint64_t max_size = program_context_get_int64 (context, 
      "max-size", 1024*1024); 
    if (size <= max_size)
      {
      // We can't put it off any longer -- we have to unpack
      //   and grep this entry
      *did_something = TRUE;
      matches += program_do_entry (context, z, preg, n);
      }
    else
      {
      char *ss = numberformat_size_64 (size, ",", TRUE);
      log_warning ("%s!%s is too large (%s)", zip_filename, 
          int_filename, ss);
      free (ss);
      }
    }
  else
    {
    log_debug ("Skipping non-matching entry: %s: %s\n", 
      zip_filename, int_filename);
    }

  LOG_OUT
  return matches;
  }


/*==========================================================================
  program_do_file

  Process a specific zipfile, examining each entry and, if it meets 
   certain criteria, sending it for further examination.

  Returns the total number of matches
==========================================================================*/
int program_do_file (const ProgramContext *context, const pcre *preg,
    const Path *path, BOOL *did_something)
  {
  LOG_IN

  log_debug ("%s: path=%s", __PRETTY_FUNCTION__, path);

  int matches = 0;
  char *s_path = (char *)path_to_utf8 (path);
  ZipFile *z = zipfile_create (s_path);
  int error = zipfile_read_contents (z);
  if (!error)
    {
    // It's a zipfile, and we can probably read it...

    log_debug ("zipfile_read_contents OK");

    BOOL stop = FALSE;
    BOOL first = program_context_get_boolean (context, 
          "first", FALSE);
    int l = zipfile_get_num_entries (z); 
    for (int i = 0; i < l && !stop; i++)
      {
      uint64_t size;
      char int_filename[PATH_MAX];
      zipfile_get_entry_details (z, i, int_filename, 
        sizeof (int_filename), &size); 
      if (size != 0)
        {
        log_debug ("Consider entry %d", i);
        matches += program_consider_entry (context, z, preg, i, did_something);
        if (matches && first) 
          {
          log_debug 
               ("Stopping now because first match only is set");
          stop = TRUE;
          }
        }
      else
        {
        log_debug ("Skipping zero-length entry %s", int_filename);
        }
      }
    }
  else log_warning ("%s: %s", s_path, program_zip_strerror (error));
  zipfile_destroy (z);
  free (s_path);

  LOG_OUT
  return matches;
  }

/*==========================================================================
  program_consider_file

  Check whether a filename matches the inclusion criteria and, if
    so, send it for checking

  Returns the total number of matches 
==========================================================================*/
int program_consider_file (const ProgramContext *context, 
     const pcre *preg, const Path *path, BOOL *did_something)
  {
  LOG_IN
  int matches = 0;
  char *s_path = (char *)path_to_utf8 (path);
  log_debug ("%s arg=%s", __PRETTY_FUNCTION__, s_path);

  char *filename = (char *)path_get_filename_utf8 (path);
  if (filename)
    {
    if (program_match_filename (context, filename, FALSE))
      matches += program_do_file (context, preg, path, did_something);
    free (filename);
    }

  free (s_path);
  LOG_OUT
  return matches;
  } 

/*==========================================================================
  program_do_file_or_dir
==========================================================================*/
int program_do_file_or_dir (const ProgramContext *context, 
       const pcre *preg, const char *arg, BOOL *did_something)
  {
  LOG_IN
  int matches = 0;
  log_debug ("%s arg=%s", __PRETTY_FUNCTION__, arg);
  Path *path = path_create (arg);
  struct stat sb;
  if (path_stat (path, &sb))
    {
    if (path_is_regular (path))
      {
      matches += program_consider_file (context, preg, path, did_something);
      }
    else if (path_is_directory (path))
      {
      if (program_context_get_boolean (context, "recurse", FALSE))
        {
        matches += program_do_dir (context, preg, path, did_something);
        }
      else
        {
        log_warning ("'%s' is a directory, and 'recurse' was not set",
             arg);
        }
      }
    else
      {
      log_debug ("'%s' is neither a regular file nor a directory",
           arg);
      }
    }
  else
    {
    log_warning ("'%s': %s", arg, strerror(errno)); 
    }
  
  path_destroy (path);
  LOG_OUT
  return matches;
  }


/*==========================================================================
  program_run

  Start of the program-specific logic. 

  The return value will eventually become the exit value from the program.
  The conventional exit values for grep are 0 if there is a match, 1
    if there is not match, and 2 if there is an error. However, it is
    not easy to determine what an 'error' amounts to when processing
    multiple entries in multiple files. Consequently, this function
    only returns 2 in cases where the errors are so fatal as to prevent 
    searching any files at all.
==========================================================================*/
int program_run (ProgramContext *context)
  {
  LOG_IN
  int ret = 0;
  int matches = 0;
  
  char ** const argv = program_context_get_nonswitch_argv (context);
  int argc = program_context_get_nonswitch_argc (context);

  if (argc >= 3)
    {
    const char *_pattern = argv[1];
    int flags = 0;
    if (program_context_get_boolean (context, "ignore-case", FALSE))
      flags |= PCRE_CASELESS;

    BOOL word_regexp = program_context_get_boolean 
      (context, "word-regexp", FALSE);

    char *pattern;
    if (word_regexp)
      asprintf (&pattern, "\\b(%s)\\b", _pattern);
    else
      asprintf (&pattern, "%s", _pattern);

    const char *pcre_error = NULL;
    int error_pos = 0-1;
    
    pcre *re = pcre_compile (pattern, flags, 
       &pcre_error, &error_pos, NULL);
    if (re)
      {
      for (int i = 2; i < argc; i++)
        {
        BOOL did_something = FALSE;
        matches += program_do_file_or_dir (context, re, argv[i], 
          &did_something);
        if (!did_something)
          {
          log_warning ("%s: No zipfile entries were processed", argv[i]);
          }
        }
      pcre_free (re);
      }
    else
      {
      log_error ("Bad regular expression: %s, position %d", 
          pcre_error, error_pos);
      ret = 2;
      }
    free (pattern);
    }
  else
    {
    usage_show (stderr, argv[0]);
    ret = 2;
    }

  if (ret != 2)
    {
    if (matches > 0) ret = 0; else ret = 1;
    }

  LOG_OUT
  return ret;
  }

