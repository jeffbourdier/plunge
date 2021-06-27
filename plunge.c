/* plunge.c - file synchronization utility
 *
 * Copyright (c) 2016-21 Jeffrey Paul Bourdier
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

#include <sys/stat.h>     /* S_IFMT, S_IFREG, stat, (struct) stat */
#ifdef _WIN32
#  include <sys/utime.h>  /* (struct) utimbuf, utime */
#  include <io.h>         /* _A_SUBDIR, _findclose, (struct) _finddata_t, _findfirst, _findnext, intptr_t */
#else
#  include <utime.h>      /* (struct) utimbuf, utime */
#  include <dirent.h>     /* closedir, DIR, (struct) dirent, DT_DIR, opendir, readdir */
#endif
#include <errno.h>        /* ENOENT, errno */
#include <stdlib.h>       /* EXIT_FAILURE, EXIT_SUCCESS, free, malloc, realloc */
#include <string.h>       /* memcpy, strcmp, strlen, strncmp */
#include <time.h>         /* time */
#include <limits.h>       /* INT_MIN */
#include <stdio.h>        /* fgets, perror, puts, stdin */
#include "jb.h"           /* (struct) jb_command_option, jb_command_parse, JB_PATH_SEPARATOR,
                             jb_file_read, jb_file_write, JB_PATH_MAX_LENGTH, jb_trim */
#include "path.h"         /* MAX_LINE_LENGTH, path_build, path_output */


/**************************
 * Enum Type Declarations *
 **************************/

enum compare_files_result
{
  COMPARE_FILES_ERROR,
  COMPARE_FILES_SRC_NO_EXIST,
  COMPARE_FILES_SRC_NOT_FILE,
  COMPARE_FILES_DST_NO_EXIST,
  COMPARE_FILES_DST_NOT_FILE,
  COMPARE_FILES_SAME_AGE,
  COMPARE_FILES_DST_NEWER,
  COMPARE_FILES_SRC_LARGER,
  COMPARE_FILES_SRC_NEWER
};


/*************
 * Constants *
 *************/

static const char * STR_USAGE = "SOURCE DEST";
static const char * STR_HELP =
  "Synchronize (copy) newer files of corresponding names from SOURCE into DEST.\n"
  "Options:\n"
  "  -h, --help     output this message and exit\n"
  "  -n, --dry-run  don't actually copy files; just output messages\n"
  "  -p, --purge    report files in destination directory to purge\n"
  "  -v, --verbose  output messages for all files, whether copied or skipped";
static const char * STR_ERROR = "Error";
static const char * STR_PURGE = "\nThe following files in DEST may need to be purged:";

/* Terse messages */
static const char * STR_TERSE_HEADING =
  "                         Pathname                                 Status\n"
  "----------------------------------------------------------  ------------------";
static const char * STR_NEW                                 = "New";
static const char * STR_LARGER                              = "Newer and larger";
static const char * STR_NEWER                               = "Newer (not larger)";

/* Verbose messages */
static const char * STR_VERBOSE_HEADING =
  "                     Pathname                             Status        Action\n"
  "--------------------------------------------------  ------------------  ------";
static const char * STR_SRC_NO_EXIST                = "Src not found. . . . Skip";
static const char * STR_SRC_NOT_FILE                = "Src not a file . . . Skip";
static const char * STR_DST_NO_EXIST                = "Dst not found. . . . Copy";
static const char * STR_DST_NOT_FILE                = "Dst not a file . . . Skip";
static const char * STR_SAME_AGE                    = "Same age . . . . . . Skip";
static const char * STR_DST_NEWER                   = "Dst newer! . . . . . Skip";
static const char * STR_SRC_LARGER                  = "Src newer & larger . Copy";
static const char * STR_SRC_NEWER                   = "Src newer. . . . . . Copy";


/*********************
 * Macro Definitions *
 *********************/

/* Flags for process_file */
#define PROCESS_FILE_VERBOSE  0x1
#define PROCESS_FILE_DRY_RUN  0x2


/*********************************
 * Private Function Declarations *
 *********************************/

void process_file(const char * path, const char * src, const char * dst, int flags);
enum compare_files_result compare_files(const char * src, const char * dst, size_t * size_ptr, time_t * mtime_ptr);
void copy_file(const char * src, const char * dst, size_t size, time_t mtime);
void purge_files(const char * src, const char * dst, int offset, char ** paths, int path_count);
void purge_file(const char * name, int dir, const char * src, const char * dst, int offset, char ** paths, int path_count);


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Synchronize files by copying newer files of corresponding names from a source directory into a destination directory.
 */
int main(int argc, char * argv[])
{
  static const size_t k = sizeof(char *);

  static struct jb_command_option options[] =
  {
    { { "verbose", "v" }, 0 },
    { { "dry-run", "n" }, 0 },
    { { "purge",   "p" }, 0 }
  };

  int n, i, b = 0;
  char s[JB_PATH_MAX_LENGTH], * p, * q, ** a = NULL;

  /* Verify usage. */
  n = sizeof(options) / sizeof(struct jb_command_option);
  n = jb_command_parse(argc, argv, STR_USAGE, STR_HELP, options, n, 2);
  if (n < 0) return (n == INT_MIN) ? EXIT_SUCCESS : EXIT_FAILURE;

  /* Input the relative pathname of each file to sync (one per line). */
  for (i = 0; fgets(s, JB_PATH_MAX_LENGTH, stdin); ++i)
  {
    /* Skip empty lines. */
    if ((n = strlen(p = jb_trim(s))) < 1) continue;

    /* Allocate memory for another character pointer at the end of our array. */
    a = (char **)realloc(a, (i + 1) * k);

    /* Allocate memory for a new string and copy the relative pathname of the file into it. */
    memcpy((a[i] = (char *)malloc(JB_PATH_MAX_LENGTH)), p, ++n);

#ifdef _WIN32
    /* Replace any slashes in the pathname with the platform-dependent directory separator. */
    for (p = a[i]; *p; ++p) if (*p == '/') *p = JB_PATH_SEPARATOR;
#endif
  }
  if (!a) return EXIT_SUCCESS;

#ifndef _WIN32
  /* Output an empty line before the heading, to improve readability.
   * (On Windows, this would have been done already, by jb_command_parse.)
   */
  putchar('\n');
#endif

  /* Output the appropriate heading. */
  puts(options[0].is_present ? STR_VERBOSE_HEADING : STR_TERSE_HEADING);

  /* Process each file that was entered. */
  n = i;
  p = argv[argc - 2];
  q = argv[argc - 1];
  if (options[0].is_present) b |= PROCESS_FILE_VERBOSE;
  if (options[1].is_present) b |= PROCESS_FILE_DRY_RUN;
  for (i = 0; i < n; ++i) process_file(a[i], p, q, b);

  /* If specified, report files in the destination directory that may need to be purged.
   * (This requires absolute pathnames of source files to determine whether or not to skip them.)
   */
  if (options[2].is_present)
  {
    puts(STR_PURGE);
    for (i = 0; i < n; ++i) path_build(a[i], p, a[i]);
    if (q[(i = strlen(q)) - 1] != JB_PATH_SEPARATOR) ++i;
    purge_files(p, q, i, a, n);
  }

#ifndef _WIN32
  /* Output an empty line before the command prompt, to improve readability.  (Windows does this automatically.) */
  putchar('\n');
#endif

  /* All done. */
  for (i = 0; i < n; ++i) free(a[i]);
  free(a);
  return EXIT_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Process (i.e., sync) a given file.
 *   path:  relative pathname of file
 *   src:  source directory pathname
 *   dst:  destination directory pathname
 *   flags:  bitwise-OR combination of process_file flags (PROCESS_FILE_VERBOSE/PROCESS_FILE_DRY_RUN)
 */
void process_file(const char * path, const char * src, const char * dst, int flags)
{
  static const int j = MAX_LINE_LENGTH - 18, k = MAX_LINE_LENGTH - 26;

  char r[JB_PATH_MAX_LENGTH], s[JB_PATH_MAX_LENGTH];
  size_t n;
  time_t t;
  enum compare_files_result result;
  const char * p = NULL;
  int b, v = flags & PROCESS_FILE_VERBOSE;

  /* Compare the source and destination files, by absolute pathnames. */
  path_build(r, src, path);
  path_build(s, dst, path);
  result = compare_files(r, s, &n, &t);

  /* Build the appropriate message and decide whether or not to copy
   * the source to the destination, based on the comparison result.
   */
  switch (result)
  {
  case COMPARE_FILES_ERROR:        if (v) p = STR_ERROR;                b = 0; break;
  case COMPARE_FILES_SRC_NO_EXIST: if (v) p = STR_SRC_NO_EXIST;         b = 0; break;
  case COMPARE_FILES_SRC_NOT_FILE: if (v) p = STR_SRC_NOT_FILE;         b = 0; break;
  case COMPARE_FILES_DST_NO_EXIST: p = v ? STR_DST_NO_EXIST : STR_NEW;  b = 1; break;
  case COMPARE_FILES_DST_NOT_FILE: if (v) p = STR_DST_NOT_FILE;         b = 0; break;
  case COMPARE_FILES_SAME_AGE:     if (v) p = STR_SAME_AGE;             b = 0; break;
  case COMPARE_FILES_DST_NEWER:    if (v) p = STR_DST_NEWER;            b = 0; break;
  case COMPARE_FILES_SRC_LARGER:   p = v ? STR_SRC_LARGER : STR_LARGER; b = 1; break;
  case COMPARE_FILES_SRC_NEWER:    p = v ? STR_SRC_NEWER : STR_NEWER;   b = 1; break;
  }

  /* If appropriate, output the message and/or copy the source to the destination. */
  if (p) { path_output(path, v ? k : j); puts(p); }
  if (b && !(flags & PROCESS_FILE_DRY_RUN)) copy_file(r, s, n, t);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Compare two files.
 *   src:  absolute pathname of source file
 *   dst:  absolute pathname of destination file
 *   size_ptr:  receives size (in bytes) of source file
 *   mtime_ptr:  receives modification time of source file
 * Return Value:  Result of comparison.
 */
enum compare_files_result compare_files(const char * src, const char * dst, size_t * size_ptr, time_t * mtime_ptr)
{
  struct stat src_stat, dst_stat;

  /* If the source file does not exist, return that result.  (If an error occurred, return that too.) */
  if (stat(src, &src_stat))
  {
    if (errno == ENOENT) return COMPARE_FILES_SRC_NO_EXIST;
    perror("stat");
    return COMPARE_FILES_ERROR;
  }

  /* The source file exists.  If it is not a regular file, return that result. */
  if ((src_stat.st_mode & S_IFMT) != S_IFREG) return COMPARE_FILES_SRC_NOT_FILE;

  /* The source file exists and is a regular file.  Retrieve its total size (in bytes) and time of last modification. */
  *size_ptr = src_stat.st_size;
  *mtime_ptr = src_stat.st_mtime;

  /* If the destination file does not exist, return that result.  (If an error occurred, return that too.) */
  if (stat(dst, &dst_stat))
  {
    if (errno == ENOENT) return COMPARE_FILES_DST_NO_EXIST;
    perror("stat");
    return COMPARE_FILES_ERROR;
  }

  /* The destination file exists.  If it is not a regular file, return that result. */
  if ((dst_stat.st_mode & S_IFMT) != S_IFREG) return COMPARE_FILES_DST_NOT_FILE;

  /* The destination file exists and is a regular file.  Compare the two files' timestamps.
   * If they are the same age or the destination file is newer, return the respective result.
   */
  if (src_stat.st_mtime == dst_stat.st_mtime) return COMPARE_FILES_SAME_AGE;
  if (src_stat.st_mtime < dst_stat.st_mtime) return COMPARE_FILES_DST_NEWER;

  /* The source file is newer than the destination file.  Return the result based on how their sizes compare. */
  return (src_stat.st_size > dst_stat.st_size) ? COMPARE_FILES_SRC_LARGER : COMPARE_FILES_SRC_NEWER;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copy a file.
 * (While it might be tempting to have the shell/OS execute this command (say,
 * via the 'system' function), we choose not to, for the following reasons:
 *   - Portability.  The command would be different depending on the OS ('copy' on Win32 vs. 'cp' on Linux).
 *   - Efficiency.  We'd rather not spawn a child process if we don't have to.
 * Surprisingly, there's no cross-platform functionality to do this without invoking the
 * shell/OS.  (Win32 has CopyFile, but Linux has no equivalent.)  Thus, we write our own.)
 *   src:  absolute pathname of source file
 *   dst:  absolute pathname of destination file
 *   size:  size (in bytes) of source file
 *   mtime:   modification time of source file
 */
void copy_file(const char * src, const char * dst, size_t size, time_t mtime)
{
  void * p;
  int n;
  struct utimbuf t;

  /* Read the file into a buffer. */
  if (!(p = jb_file_read(src, size))) return;

  /* Write the contents of the buffer (source file) to the destination file. */
  n = jb_file_write(dst, p, size);
  free(p);
  if (n) return;

  /* Set the modification time of the destination file to that of the source file, so that the next time
   * this runs, we realize that the source and destination files are identical (size-wise and time-wise).
   */
  t.actime = time(NULL);
  t.modtime = mtime;
  if (utime(dst, &t)) perror("utime");
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Report files in the destination directory for which there are not corresponding files in the source directory.
 *   src:  source directory pathname
 *   dst:  destination directory pathname
 *   offset:  offset into absolute pathname of destination file at which to begin output
 *   paths:  pathnames of files to skip (because they are known to exist in the source directory)
 *   path_count:  number of pathnames in paths
 */
void purge_files(const char * src, const char * dst, int offset, char ** paths, int path_count)
{
  int i;
  char * q;
#ifdef _WIN32
  char s[JB_PATH_MAX_LENGTH];
  intptr_t p;
  struct _finddata_t d;
#else
  DIR * p;
  struct dirent * d;
#endif

  /* Iterate through each filename entry in the destination directory. */
#ifdef _WIN32
  ((char *)memcpy(s, dst, (i = strlen(dst))))[i] = JB_PATH_SEPARATOR;
  s[++i] = '*';
  s[++i] = '\0';
  if ((p = _findfirst(s, &d)) < 0) { perror("_findfirst"); return; }
  do
#else
  if (!(p = opendir(dst))) { perror("opendir"); return; }
  while (d = readdir(p))
#endif
  {
    /* Determine the name of the file, and whether or not it's actually a directory. */
#ifdef _WIN32
    q = d.name;
    i = (d.attrib & _A_SUBDIR);
#else
    q = d->d_name;
    i = (d->d_type == DT_DIR);
#endif

    /* Report the file if appropriate (i.e., if there is not a corresponding file in the source directory).
     * (If the file is actually a directory, its contents are purged recursively if needed.)
     */
    purge_file(q, i, src, dst, offset, paths, path_count);
#ifdef _WIN32
  } while (!_findnext(p, &d));
#else
  }
#endif

  /* All done. */
#ifdef _WIN32
  _findclose(p);
#else
  closedir(p);
#endif
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Report a file in the destination directory if there is not a corresponding file in the source directory.
 *   name:  filename (without path)
 *   dir:  nonzero if name is a directory; otherwise, zero
 *   src:  source directory pathname
 *   dst:  destination directory pathname
 *   offset:  offset into absolute pathname of destination file at which to begin output
 *   paths:  pathnames of files to skip (because they are known to exist in the source directory)
 *   path_count:  number of pathnames in paths
 */
void purge_file(const char * name, int dir, const char * src, const char * dst, int offset, char ** paths, int path_count)
{
  char * p, r[JB_PATH_MAX_LENGTH], s[JB_PATH_MAX_LENGTH];
  size_t n;
  int i, b = 0;
  struct stat st;

  /* Skip the current and parent directories. */
  if (dir && (!strcmp(name, ".") || !strcmp(name, ".."))) return;

  /* Build the absolute pathname of the source file. */
  path_build(r, src, name);
  if (dir)
  {
    r[n = strlen(r)] = JB_PATH_SEPARATOR;
    r[++n] = '\0';

    /* If the pathname appears in the list of files to skip, don't report it. */
    for (i = 0; i < path_count; ++i)
    {
      if (strlen(p = paths[i]) < n) continue;
      if (!strncmp(p, r, n)) { b = 1; break; }
    }
    r[--n] = '\0';
  } else for (i = 0; i < path_count; ++i) if (!strcmp(paths[i], r)) { b = 1; break; }

  /* If the file was not skipped, check for its existence in the source directory. */
  if (!b)
  {
    /* If the file exists in the source directory, don't report it. */
    if (!stat(r, &st)) b = 1;

    /* If an error occurred, report the error and be done. */
    else if (errno != ENOENT) { perror("stat"); return; }
  }

  /* If the file is now known to exist in the source directory (i.e., it
   * is not being reported) and it is not a directory, we can be done.
   */
  if (b && !dir) return;

  /* Build the absolute pathname of the destination file. */
  path_build(s, dst, name);

  /* If the file does not exist in the source directory, it probably
   * should not exist in the destination directory either, so report it.
   */
  if (!b) path_output(s + offset, MAX_LINE_LENGTH);

  /* Otherwise, the file must be a directory (and there is a corresponding
   * subdirectory in the source directory).  Recursively purge its contents.
   */
  else purge_files(r, s, offset, paths, path_count);
}
