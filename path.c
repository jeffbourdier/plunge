/* path.c - path functions for Plunge
 *
 * Copyright (c) 2016-20 Jeffrey Paul Bourdier
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

/* printf */
#include <stdio.h>

/* string.h:
 *   memcpy, strlen
 *
 * JB_DIRECTORY_SEPARATOR
 */
#include "jb.h"

/* MAX_LINE_LENGTH, MAX_PATH_LENGTH */
#include "path.h"


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Build an absolute pathname from a directory pathname and a relative pathname.
 *   abs:  receives absolute pathname
 *   dir:  directory pathname
 *   rel:  relative pathname
 */
void path_build(char * abs, const char * dir, const char * rel)
{
  size_t i = strlen(dir), n = strlen(rel) + 1;
  char s[MAX_PATH_LENGTH];

  memcpy(s, dir, i);
  if (s[i - 1] != JB_DIRECTORY_SEPARATOR) s[i++] = JB_DIRECTORY_SEPARATOR;
  memcpy(s + i, rel, n);
  memcpy(abs, s, i + n);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Output a relative pathname within a given field width (shortened as necessary),
 * left-justified, with a trailing newline or right-padded with double-spaced dots.
 *   path:  relative pathname
 *   width:  field width
 */
void path_output(const char * path, int width)
{
  static const int v = 3, w = 6;

  int n, m, i, j, k;
  char s[MAX_LINE_LENGTH];

  /* If the width is less than the maximum line length, output with padding (instead of a trailing newline). */
  m = (width < MAX_LINE_LENGTH) ? width - v : width;

  /* If necessary, shorten the pathname to fit within the field. */
  if ((n = strlen(path)) > m)
  {
    for (i = n - 1; i > w && path[i] != JB_DIRECTORY_SEPARATOR; --i);
    while ((k = m - (j = n - i)) < w) ++i;
    memcpy(s, path, n = k - v);
    while (n < k) s[n++] = '.';
    memcpy(s + n, path + i, j);
    n = m;
  }
  else memcpy(s, path, n);

  /* If appropriate, right-pad the field with double-spaced dots.  (Otherwise, append a newline.) */
  if (m < width) while (n < width) { s[n] = ((width - n) % 2) ? ' ' : '.'; ++n; }
  else s[n++] = '\n';

  /* Finally, null-terminate and output the string buffer. */
  s[n] = '\0';
  printf("%s", s);
}
