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

extern guint gl_menu_depth;
extern guint gl_menu_counter;
extern guint gl_menu_widgets;
extern guint gl_submenu_widgets;
extern guint gl_include_depth;

enum Result
entry_append_leaf_node (struct Entry *entry,
                        const gchar *label,
                        const gchar *cmd,
                        GtkWidget **widgetptr);

enum Result
entry_add_icon (struct Entry *entry,
                GtkWidget *menu_item);

enum Result
make_rooted_dirpath (struct Entry *entry);

GtkMenu *
core_get_menu (void);

int
core_module_init (void);

#ifdef __cplusplus
}
#endif
#endif /* _GTKMENUPLUS_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
