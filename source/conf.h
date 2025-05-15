/*
Copyright (C) 2024-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2

Forked from version 1.00, 2013-04-24, by Alan Campbell
Partially based on myGtkMenu, Copyright (C) 2004-2011 John Vorthman
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

#ifndef _CONF_H
#define _CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "entry.h"

enum conf_followsymlink_flags
{
 CONF_FOLLOWSYMLINK_NONE = 0,
 CONF_FOLLOWSYMLINK_FILE = (1 << 0),
 CONF_FOLLOWSYMLINK_DIR  = (1 << 1),
};

enum Result
conf_apply_keywords_from_entry (struct Entry *entry);

guint
conf_endsubmenu (const gchar *buf,
                 const guint state);

guint
conf_errorconsoleonly (const gchar *buf,
                       const guint state);

guint
conf_iconsize (const gchar *buf,
               const guint numarg);

guint
conf_menuposition (const gchar *buf,
                   const guint numarg);

guint    conf_get_abspathparts ();
guint    conf_get_endsubmenu ();
guint    conf_get_errorconsoleonly ();
guint    conf_get_followsymlink ();
guint    conf_get_formattinglocal ();
guint    conf_get_icons ();
guint    conf_get_iconsize_height ();
guint    conf_get_iconsize_width ();
guint    conf_get_ignorehidden ();
guint    conf_get_launchericons ();
guint    conf_get_launcherlistfirst ();
guint    conf_get_launchernodisplay ();
guint    conf_get_launchernullcategory ();
guint    conf_get_markup ();
guint    conf_get_menuposition_x ();
guint    conf_get_menuposition_y ();
gboolean conf_has_menuposition ();
guint    conf_get_mnemonic ();

#ifdef __cplusplus
}
#endif

#endif /* _CONF_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
