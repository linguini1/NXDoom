/*
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Dehacked string replacements
 */

#ifndef DEH_STR_H
#define DEH_STR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "doomtype.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if 0
/* Static macro versions of the functions below */

#define deh_string(x) (x)
#define deh_printf printf
#define deh_fprintf fprintf
#define deh_snprintf snprintf

#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Used to do dehacked text substitutions throughout the program */

const char *deh_string(const char *s) PRINTF_ARG_ATTR(1);
void deh_printf(const char *fmt, ...) PRINTF_ATTR(1, 2);
void deh_fprintf(FILE *fstream, const char *fmt, ...) PRINTF_ATTR(2, 3);
void deh_snprintf(char *buffer, size_t len, const char *fmt, ...)
    PRINTF_ATTR(3, 4);
void deh_add_string_replacement(const char *from_text, const char *to_text);

#endif /* DEH_STR_H */
