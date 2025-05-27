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

#ifndef _GTKMENUPLUS_H
#define _GTKMENUPLUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include "entry.h"
#include "input.h"

extern gchar gl_on_exit[];
extern gboolean gl_opt_json_serialize;
extern gchar *gl_progname;
extern gint  gl_argc;
extern gchar **gl_argv;

enum Result
main_loop (struct InputSource *isrc,
           const gint argc,
           const gchar *argv[],
           const guint menu_depth_base,
           const struct Entry *overrides);

void
menu_position_cb (GtkMenu *menu,
                  gint *x,
                  gint *y,
                  gboolean *push_in,
                  gpointer data);

void
entry_activate (struct Entry *entry);

enum Result
main_finale (const struct Entry *entry);

#ifdef __cplusplus
}
#endif
#endif /* _GTKMENUPLUS_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
