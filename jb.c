/* jb.c - Jeff Bourdier (JB) library
 *
 * Copyright (c) 2017-21 Jeffrey Paul Bourdier
 *
 * Licensed under the MIT License.  This file may be used only in compliance with this License.
 * Software distributed under this License is provided "AS IS", WITHOUT WARRANTY OF ANY KIND.
 * For more information, see the accompanying License file or the following URL:
 *
 *   https://opensource.org/licenses/MIT
 */


/*****************
 * Include Files *
 *****************/

#include <sys/stat.h>  /* mkdir, stat, (struct) stat */
#include <ctype.h>     /* isspace */
#include <errno.h>     /* ENOENT, errno */
#ifdef _WIN32
#  include <direct.h>  /* _mkdir */
#else
#  include <libgen.h>  /* basename, dirname */
#endif
#include <limits.h>    /* INT_MIN */
#include <stdio.h>     /* fclose, FILE, fprintf, fwrite, putchar, puts, stderr */
#include <stdlib.h>    /* free, malloc */
#include <string.h>    /* strcmp, strdup, strlen, strncmp, strrchr */
#include "jb.h"        /* (struct) jb_command_option, JB_PATH_SEPARATOR, jb_exe_strip, jb_file_open */


/*********************************
 * Private Function Declarations *
 *********************************/

int validate_option(struct jb_command_option * options, int option_count, const char * text, int which);
int make_directory(const char * path);


/*************
 * Functions *
 *************/


#ifdef _WIN32

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Return the last component of a pathname.  For more information, see:
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/basename.html
 * (On Linux, this function is defined in libgen.h.  There is no Win32 equivalent.)
 */
char * basename(char * path)
{
  char * p = strrchr(path, JB_PATH_SEPARATOR);
  return p ? p + 1 : path;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Report the parent directory name of a file pathname.  For more information, see:
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/dirname.html
 * (On Linux, this function is defined in libgen.h.  There is no Win32 equivalent.)
 */
char * dirname(char * path)
{
  char * p = strrchr(path, JB_PATH_SEPARATOR);
  if (!p) return path + strlen(path);
  *p = '\0';
  return path;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Strip the extension off of an executable filename.
 *   filename:  name of executable file (without path)
 */
void jb_exe_strip(char * filename)
{
  static const char * exe = ".exe";

  size_t n = strlen(filename), m = strlen(exe);
  char * p;

  if (n > m)
  {
    p = filename + n - m;
    if (!strcmp(p, exe)) *p = '\0';
  }
}

#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Check command line arguments.
 *   argc:  number of command line arguments (same as argc passed to main)
 *   argv:  array of command line arguments (same as argv passed to main)
 *   usage:  latter part of usage message (following the options)
 *   help:  help message, including application description and list of
 *     command line options (including --help, which all apps should implement)
 *   options:  array of jb_command_option structures
 *   option_count:  number of items in options
 *   arg_count:  number of arguments that should follow command line options
 * Return Value:
 *   If INT_MIN, the "help" option was specified on the command line.  Program should terminate with success.
 *   Otherwise, if less than zero, the command line is invalid.  Program should terminate with failure.
 *   Otherwise, the number of arguments (following the options) that were specified on the command line.
 */
int jb_command_parse(int argc, char * argv[], const char * usage, const char * help,
                     struct jb_command_option * options, int option_count, int arg_count)
{
  static const char * usage_format = "Usage: %s [OPTION]... %s\n";
  static const char * help_format = "Try '%s --help' for more information.\n";
  static const char * home_page_format = "Home page: <https://jeffbourdier.github.io/%s>\n";

  int i, j, n;
  const char * s;
  char * p = basename(argv[0]);

#ifdef _WIN32
  /* Since Windows outputs an empty line before each command prompt, do
   * so here (after the command line) as well, to improve readability.
   */
  putchar('\n');
#endif

  /* If options are present, they must come first.  Iterate through
   * each argument, validating each, until we're done with the options.
   */
  for (i = 1; i < argc; ++i)
  {
    s = argv[i];

    /* As soon as we encounter an argument that does not begin with a hyphen, we're done with the options. */
    if (s[0] != '-') break;

    /* If another hyphen follows, it's a long-format option, in which case only one is allowed (per argument). */
    if (s[1] == '-') n = validate_option(options, option_count, &s[2], 0);

    /* Otherwise, any number of short-format options are allowed (per argument). */
    else for (j = 1; s[j]; ++j) if (n = validate_option(options, option_count, &s[j], 1)) break;

    /* If the last option required an argument, we're done with it, so move on to the next. */
    if (n > 0) continue;

    if (n < 0)
    {
      /* If the "help" option is present, print a full usage/help message (and exit). */
      if (n == INT_MIN)
      {
        printf(usage_format, p, usage); puts(help);
#ifdef _WIN32
        /* It is assumed that on Win32, the filename ends in ".exe" (whereas on Linux, the filename has no extension). */
        jb_exe_strip(p);
#endif
        printf(home_page_format, p);
      }
      /* Otherwise, the option is not valid.  Print a brief usage message (and exit). */
      else
      {
        fprintf(stderr, usage_format, p, usage);
        fprintf(stderr, help_format, p);
      }
      return n;
    }
  }

  /* Determine if the correct number of arguments follows the options. */
  if ((n = argc - i) == arg_count) return n;
  fprintf(stderr, usage_format, p, usage);
  fprintf(stderr, help_format, p);
  return -1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Read the contents of a file.
 *   path:  file pathname
 *   size:  number of bytes to read
 * Return Value:  On success, a pointer to the data read from the file.  (Memory for this buffer is obtained
 *   with malloc, and should be freed with free.)  Otherwise, NULL (and errno is set appropriately).
 */
void * jb_file_read(const char * path, size_t size)
{
  void * p;
  FILE * f;
  int n;

  /* Allocate memory for a buffer to store the data read from the file. */
  if (!(p = malloc(size + 1))) { perror("malloc"); return NULL; }

  /* Open the file for reading.
   * Note that on Win32, by default, a file is opened in text mode, which means that "\r\n" is translated to
   * "\n" on input.  Open the file in binary mode, so that no such translation occurs.  (Linux, being (mostly)
   * POSIX-compliant, does not suffer from this problem.  See also the comment on jb_file_open in jb_file_write.)
   */
  if (jb_file_open(&f, path, "rb") || !f) { perror("fopen"); free(p); return NULL; }

  /* Read the contents of the file into our buffer. */
  if (fread(p, 1, size, f) < size) { perror("fread"); n = errno; free(p); fclose(f); errno = n; return NULL; }
  if (fclose(f)) { perror("fclose"); free(p); return NULL; }
  return p;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Write the contents of a buffer to a file.
 *   path:  file pathname
 *   buffer:  data written to file at path
 *   size:  number of bytes to write
 * Return Value:  Zero on success; otherwise, nonzero (and errno is set appropriately).
 */
int jb_file_write(const char * path, const void * buffer, size_t size)
{
  FILE * f;
  int n;

  /* Before attempting to open (and possibly create) the file, make sure that its parent directory exists. */
  if (make_directory(path)) return -1;

  /* Open the file for writing.
   * Note that on Win32, by default, a file is opened in text mode, which means that "\n" is translated to
   * "\r\n" on output.  Open the file in binary mode, so that no such translation occurs.  (Linux, being (mostly)
   * POSIX-compliant, does not suffer from this problem.  See also the comment on jb_file_open in jb_file_read.)
   */
  if (jb_file_open(&f, path, "wb") || !f) { perror("fopen"); return -1; }

  /* Write the buffer contents to the path. */
  if (fwrite(buffer, 1, size, f) < size) { perror("fwrite"); n = errno; fclose(f); errno = n; return -1; }
  if (fclose(f)) { perror("fclose"); return -1; }
  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Remove leading and trailing white-space characters from a string.
 *   s:  string to remove white-space characters from
 * Return Value:  String with white-space characters removed.
 */
char * jb_trim(char * s)
{
  int i, n = strlen(s);

  /* Reposition null terminator so that there is no trailing white-space. */
  for (i = n; --i;) { if (!isspace(s[i])) break; }
  if (++i < n) s[n = i] = '\0';

  /* Return a pointer to the first non-white-space character. */
  for (i = 0; i < n; ++i) { if (!isspace(s[i])) break; }
  return &s[i];
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Determine if a string is a valid command line option.
 *   options:  array of jb_command_option structures
 *   option_count:  number of items in options
 *   text:  string representing long or short form of option
 *   which:  index into jb_command_option.text indicating long (0) or short (1) form of option
 * Return Value:
 *   If INT_MIN, the "help" option was specified.
 *   Otherwise, if less than zero, the string is not a valid option.
 *   Otherwise, the number of arguments required by the option (1 or 0).
 */
int validate_option(struct jb_command_option * options, int option_count, const char * text, int which)
{
  int i, n;
  const char * s;

  /* A null/empty string is invalid. */
  if (!text || !strlen(text)) return -1;

  /* Check to see if this is the universal "help" option (which all apps should implement). */
  if (which ? (*text == 'h') : !strcmp(text, "help")) return INT_MIN;

  /* Compare the string to each command line option.  If a match is found, return indicating whether or not it is valid. */
  for (i = 0; i < option_count; ++i)
  {
    /* Compare the string to the appropriate text (short or long).  If no match, continue. */
    s = options[i].text[which];
    n = which ? 1 : strlen(s);
    if (strncmp(text, s, n)) continue;

    /* If the option requires an argument, set a pointer to the argument
     * and return whether the argument is valid (non-null) or not (null).
     */
    s = options[i].text[0];
    if (s[strlen(s) - 1] == '=') return (*(options[i].argument = text + n)) ? 1 : -1;

    /* Indicate that the option is present and return (valid). */
    options[i].is_present = 1; return 0;
  }

  /* If the string does not match any option, the command line is invalid. */
  return -1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Make sure that a parent directory exists.
 *   path:  file pathname
 * Return Value:  Zero on success; otherwise, nonzero (and errno is set appropriately).
 */
int make_directory(const char * path)
{
#ifndef _WIN32
  static const mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH;
#endif

  char * s, * p;
  struct stat st;
  int r;

  /* Get the directory component of the pathname.
   * Note that on Win32, strdup is considered "deprecated" and results in error C4996.
   * The ISO C++ conformant function, _strdup (unavailable on Linux), is used instead.
   */
#ifdef _WIN32
  s = _strdup(path);
#else
  s = strdup(path);
#endif
  if (!s) { perror("strdup"); return -1; }
  p = dirname(s);

  /* If an empty string is returned, the original pathname was most likely a nonexistent drive.
   * (And by the way, failure to catch this results in infinite recursion.)
   */
  if (!strlen(p)) { free(s); errno = ENOENT; perror("make_directory"); return -1; }

  /* If the directory already exists, we're golden. */
  if (!(r = stat(p, &st)) || errno != ENOENT)
  {
    free(s);
    if (r) { perror("stat"); return r; }
    return 0;
  }

  /* The directory does not already exist, so it will need to be created.
   * In order to do that, however, its parent directory must also exist.
   */
  if (make_directory(p)) { free(s); return -1; }

  /* The parent directory exists, so create the directory of interest. */
#ifdef _WIN32
  r = _mkdir(p);
#else
  r = mkdir(p, mode);
#endif
  free(s);
  if (r) { perror("mkdir"); return r; }
  return 0;
}
