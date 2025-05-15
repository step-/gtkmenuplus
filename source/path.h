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

#ifndef _PATH_H
#define _PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include <assert.h>
#include <regex.h>
#include "entry.h"

extern gchar gl_script_dir[];

gchar *
path_expand_full (const gchar *path,
                  const gchar *dotpath,
                  guint *error);

enum Result
path_expand (gchar *path,
             gchar *basedir,
             const gchar *label,
             struct Entry *entry);

gboolean
path_get_base_dir (gchar *obuf,
                   const gchar *filename,
                   gboolean use_cwd,
                   struct Entry *entry);

int
path_module_init (void);

void
path_module_finalize (void);

#ifdef __cplusplus
}
#endif
#endif /* _PATH_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
