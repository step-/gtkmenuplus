/*
Copyright (C) 2016-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2

Forked from version 1.00, 2013-04-24, by Alan Campbell
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
#ifndef _LAUNCHER_H
#define _LAUNCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include <regex.h>
#include "entry.h"

struct LauncherProperty
{
 const gchar *property;
 gchar       *value;         /* g_malloc */
 gboolean    try_i18n;
 gboolean    required;
};

enum LauncherPropertyId
{
 LAUNCHER_PROP_CATEGORY,
 LAUNCHER_PROP_NAME,
 LAUNCHER_PROP_EXEC,
 LAUNCHER_PROP_COMMENT,
 LAUNCHER_PROP_ICON,
 LAUNCHER_PROP_TYPE,
 LAUNCHER_PROP_NODISPLAY,
};

enum Result a_launcher (struct Entry *entry);
enum Result a_launchersub (struct Entry *entry);
enum Result a_launcherargs (struct Entry *entry);
enum Result a_launcherdirfile (struct Entry *entry);
enum Result enter_launchersubmenu (struct Entry *entry);
enum Result leave_launchersubmenu (struct Entry *entry);
enum Result a_launcherdir (struct Entry *entry);

const gchar *launcher_get_launcherdirectory (void);
void launcher_set_launcherdirectory (const gchar *value);

int
launcher_module_init (const int cflags);

void
launcher_module_finalize (void);

#ifdef __cplusplus
}
#endif
#endif /* _LAUNCHER_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
