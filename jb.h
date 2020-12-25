/* jb.h - Jeff Bourdier (JB) library
 *
 * Copyright (c) 2017-20 Jeffrey Paul Bourdier
 *
 * Licensed under the MIT License.  This file may be used only in compliance with this License.
 * Software distributed under this License is provided "AS IS", WITHOUT WARRANTY OF ANY KIND.
 * For more information, see the accompanying License file or the following URL:
 *
 *   https://opensource.org/licenses/MIT
 */


/* Prevent multiple inclusion. */
#ifndef _JB_H_
#define _JB_H_


/*****************
 * Include Files *
 *****************/

/* INT_MIN (returned by jb_command_parse) */
#include <limits.h>

/* strlen, strncmp */
#include <string.h>


/**************************
 * Structure Declarations *
 **************************/

struct jb_command_option
{
  const char * text[2];
  union { int is_present; const char * argument; };
};


/*********************
 * Macro Definitions *
 *********************/

#ifdef _WIN32
#  define JB_DIRECTORY_SEPARATOR '\\'
#else
#  define JB_DIRECTORY_SEPARATOR '/'
#endif

/* Inline functions */
#define jb_string_compare(s1, s2) strncmp(s1, s2, strlen(s2))


/*************************
 * Function Declarations *
 *************************/

int jb_command_parse(int argc, char * argv[], const char * usage, const char * help,
                     struct jb_command_option * options, int option_count, int arg_count);
void * jb_file_read(const char * path, size_t size);
int jb_file_write(const char * path, const void * buffer, size_t size);


#endif  /* (prevent multiple inclusion) */
