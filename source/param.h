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

#ifndef _PARAM_H
#define _PARAM_H

#ifdef __cplusplus
extern "C" {
#endif

struct Params
{
 gint           argc;
 const gchar   **argv;
 const gchar    *par0;
 gboolean       *in_use;
};

gboolean
param_array_init (struct Params *param,
                  const guint argc,
                  const gchar **argv,
                  const gchar *par0);

void
param_array_destroy (struct Params *param);

enum Result
expand_param (const gchar **iptr,
              gchar **optr,
              const guint max_size,
              gboolean *found,
              const struct Params *param,
              struct Entry *entry);

#ifdef __cplusplus
}
#endif

#endif /* _PARAM_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
