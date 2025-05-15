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

#ifndef _FMTG_H
#define _FMTG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autoconfig.h"
#include "common.h"

enum fmtg_markup_type
{
 FMTG_MARKUP_NONE = 0,
 FMTG_MARKUP_NORMAL,
 FMTG_MARKUP_MNEMONIC,
};

struct Formatting
{
 gchar  fmt_init[MAX_PATH_LEN + 1];     /* initial printf format for `format=` directive */
 gchar  fmt_next[MAX_PATH_LEN + 1];     /* printf formats for all `section`s and current printf format */
 gchar *fmt_next_end;                   /* end of the current printf format, see fmtg_next_formatting() */
 gchar  fmt_sep;                        /* `section` separator in `format=section;section...` */
 guint  menu_level;                     /* scope */
 gchar *nemo_chars;                     /* custom `mnemonic="chars"` */
 guint  nemo_byte_len;                  /* "chars" length in bytes */
 guint  nemo_chars_idx[MAX_MENU_DEPTH]; /* rotating pick index */
};

enum Result
a_format (struct Entry *entry);

#ifdef FEATURE_TOOLTIP
enum Result
a_tooltipformat (struct Entry *entry);
#endif

gint
fmtg_init_formatting (struct Formatting *fmtg,
                      const gchar *format,
                      const guint menu_level);

gint
fmtg_format_widget_label (GtkWidget *widget);

gint
fmtg_format_widget_tooltip (GtkWidget *widget);

gint
fmtg_module_init (void);

void
fmtg_module_finalize (void);

extern struct Formatting gl_fmtg_item[];
extern struct Formatting gl_fmtg_tooltip;

#ifdef __cplusplus
}
#endif
#endif

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
