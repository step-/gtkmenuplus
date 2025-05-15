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

#ifndef _COMMENT_H
#define _COMMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

#if defined(TEST_ALL_SHARPS) || defined(TEST_LEFTMOST_SHARP)
const char *
comment_get_pattern_sharp_isnt_comment (void);
#endif

const gchar *
comment_find_suffix (const gchar *subject);

int
comment_module_init (const int cflags);

void
comment_module_finalize (void);

#ifdef __cplusplus
}
#endif

#endif /* _COMMENT_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
