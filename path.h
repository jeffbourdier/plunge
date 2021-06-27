/* path.h - path functions for Plunge
 *
 * Copyright (c) 2016-21 Jeffrey Paul Bourdier
 *
 * Licensed under the MIT License.  This file may be used only in compliance with this License.
 * Software distributed under this License is provided "AS IS", WITHOUT WARRANTY OF ANY KIND.
 * For more information, see the accompanying License file or the following URL:
 *
 *   https://opensource.org/licenses/MIT
 */


/* Prevent multiple inclusion. */
#ifndef _PATH_H_
#define _PATH_H_


/*********************
 * Macro Definitions *
 *********************/

#define MAX_LINE_LENGTH 78


/*************************
 * Function Declarations *
 *************************/

void path_build(char * abs, const char * dir, const char * rel);
void path_output(const char * path, int width);


#endif  /* (prevent multiple inclusion) */
