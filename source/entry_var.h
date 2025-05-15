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

#ifndef _ENTRY_VAR_H
#define _ENTRY_VAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "entry.h"

struct Variable
{
 gboolean          used;
 gboolean          eval;
 guint             value_len;
 gchar             name_value[MAX_LINE_LENGTH + 1];
 gchar            *value;
 struct Variable  *next;
};

enum Result
a_variable (struct Entry *entry);

enum Result
entry_var (struct Entry *entry);

enum Result
entry_var_expand (struct Entry *entry,
                  const gchar **iptr,
                  gchar **optr,
                  const guint max_size,
                  gboolean *found);

gboolean
var_name_test (const gchar *p);

int
entry_var_module_init (void);

void
entry_var_module_finalize (void);

#ifdef __cplusplus
}
#endif

#endif /* _ENTRY_VAR_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
