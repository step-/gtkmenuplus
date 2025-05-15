/*
Copyright (C) 2024-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2
-------------------------------------------------------------------------

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License, version 2, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 59
Temple Place, Suite 330, Boston, MA 02111-1307 USA
-------------------------------------------------------------------------
*/
/* *INDENT-OFF* */

#define _GNU_SOURCE

#include <fnmatch.h>
#include "common_gnu.h"

/**
match_wildcard:
Check whether @name matches pathname @pattern, which is a shell wildcard pattern
(see glob(7)).

@pattern: wildcard pattern string.
@name: name string.
*/
unsigned
match_wildcard (const char *pattern,
                const char *name)
{
 return fnmatch (pattern, name, FNM_PATHNAME | /*GNU*/FNM_EXTMATCH) == 0;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
