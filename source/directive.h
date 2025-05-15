/*
Copyright (C) 2016-2025 step https://github.com/step-/gtkmenuplus

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

#ifndef _DIRECTIVE_H
#define _DIRECTIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autoconfig.h"
#include "common.h"
#include "entry.h"

const struct Directive *
directive_get_table ();

enum Result leave_item (struct Entry *);
enum Result leave_submenu (struct Entry *);
enum Result leave_include (struct Entry *);

enum Result enter_item (struct Entry *);
enum Result enter_submenu_internal (struct Entry *entry,
                                    const enum EntryFlags feat);
enum Result enter_submenu (struct Entry *);
enum Result enter_include (struct Entry *);

enum Result a_cmd (struct Entry *);
enum Result a_icon (struct Entry *);
enum Result a_iconforsubmenu (struct Entry *);
enum Result a_separator (struct Entry *);
enum Result a_iconsize (struct Entry *);
enum Result a_menuposition (struct Entry *);
enum Result a_icondir (struct Entry *);
enum Result a_error (struct Entry *);
enum Result a_endsubmenu  (struct Entry *entry);
enum Result a_configure (struct Entry *);
enum Result a_onexit (struct Entry *);
enum Result a_absolutepath (struct Entry *);
#ifdef FEATURE_ACTIVATION_LOG
enum Result a_activationlogfile (struct Entry *);
enum Result a_activationlogexclude (struct Entry *);
#endif
#ifdef FEATURE_TOOLTIP
enum Result a_tooltip (struct Entry *);
#endif
#ifdef FEATURE_CONDITIONAL
enum Result a_if (struct Entry *);
enum Result a_else (struct Entry *);
enum Result a_elseif (struct Entry *);
enum Result a_endif (struct Entry *);
#endif
enum Result a_eof (struct Entry *);
#ifdef FEATURE_VARIABLE
enum Result a_variable (struct Entry *);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DIRECTIVE_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
