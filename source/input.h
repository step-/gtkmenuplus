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

#ifndef _INPUT_H
#define _INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <stdio.h>
#include "common.h"
#include "entry.h"

struct InputSource
{
 const gchar *fname;             /* menu description filepath or "-" */
 FILE *fp;                       /* fname's or stdin or NULL         */
 guint lineno;                   /* fname's line number              */
 gchar *_buf;                    /* command-line directives scratch  */
 gchar *token;                   /* next directive                   */
 gchar raw[MAX_LINE_LENGTH + 1]; /* untouched line / token           */
 gchar cooked[MAX_LINE_LENGTH + 1]; /* cooked line / token           */
};

#define SIZEOF_RAW     member_size(struct InputSource, raw)
#define SIZEOF_COOKED  member_size(struct InputSource, cooked)

struct Directive
{
 const gchar     *keyword;
 const gint      length;
 const gboolean  enable_indent_rule;
 const gboolean  niladic;
 enum  LineType  type;
 const gboolean  block_check;
 fnEntry     analyze;
};

const struct Directive *
input_fetch_directive_from_linetype (const enum LineType type);

gboolean
input_source_open (struct InputSource *isrc,
                   struct Entry *entry);

const gchar *
input_fetch_current_line (const struct InputSource *src,
                          guint *lineno);

void
input_copy_first_word (gchar *out,
                       const gchar *in);

void
input_trim_from_end (gchar *sstart,
                     gchar *end);

const struct Directive *
input_fetch_next_directive (struct InputSource *isrc,
                            struct Entry *entry,
                            guint *menu_depth,
                            gboolean skip_if_body);

gchar input_get_directive_separator (void);
void input_set_directive_separator (const gchar);

#ifdef __cplusplus
}
#endif
#endif /* _INPUT_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
