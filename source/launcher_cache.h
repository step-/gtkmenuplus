/*
gtkmenuplus - Generate a GTK popup menu from text directives.

Copyright (C) 2016-2025 step https://github.com/step-/gtkmenuplus

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
#ifndef _LAUNCHER_CACHE_H
#define _LAUNCHER_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

struct Scandir
{
    guint n;
    struct dirent **namelist;
};

struct Entry *
launcher_cache_find_entry (const gchar *key);

void
launcher_cache_add_entry (const gchar *key,
                          struct Entry *value);

guint
launcher_cache_entry_count ();

struct Scandir *
launcher_cache_find_scandir (const gchar *key,
                             const guint signature);

void
launcher_cache_add_scandir (const gchar *key,
                            struct Scandir *value,
                            const guint signature);

guint
launcher_cache_scandir_count ();

#ifdef __cplusplus
}
#endif
#endif /* _LAUNCHER_CACHE_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
