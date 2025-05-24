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

#ifndef _ENTRY_H
#define _ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include "autoconfig.h"
#include "common.h"

struct Entry;
#define ENTRY_ERROR(E)              ((struct EntryError *)(E)->data)

typedef enum Result (*fnEntry) (struct Entry *);

enum EntryFlags
{
 ENTRY_FLAG_NONE         = 0,
 ENTRY_FLAG_QEQ          = 1 << 0, /* `directive ?=` */
 ENTRY_FLAG_INCLUDE_DIR  = 1 << 1, /* in `include=dirpath` */
 ENTRY_FLAG_ARRAY        = 1 << 2, /* `directive[index]=` */
 ENTRY_FLAG_ALLOW_CMD    = 1 << 3, /* `cmd=` can follow block directive */
 ENTRY_FLAG_ALLOW_ICOTIP = 1 << 4, /* `icon=`/`tooltip=` can follow... */
 ENTRY_FLAG_RESET_ERROR  = 1 << 5, /* reset/preserve .error member value */
};

struct ErrorContext
{
 guint block_lineno;
 gchar *block_line;
 gboolean block_line_locked;
};

struct EntryError
{
 gchar *message;
 enum Result level;
 struct ErrorContext *context;
};

typedef GSList ErrorList;

/**
Struct Entry:

The `Entry` struct contains not only the display data necessary for creating
a GTK menu entry but also convenience pointers, states, and an error message
buffer. A single menu entry can be constructed using either a one-shot directive
(such as Launcher, absolute, or relative pathname directives) or a block of
directives (e.g., Item + Icon + Tooltip + Cmd).

The specific `enter` function, which parses an Item directive, initiates a new
GTK menu entry by initializing a `Entry` variable. This variable includes a
`leave` function and is passed around to gather data from the directives in the
block. Once the variable has collected sufficient data and can be converted into
an actual menu entry, the parser calls the `leave` function on the variable.
One-shot directives serve as their own block and thus do not require separate
enter/leave functions.

Directives can be classified as either internal or external to a block.
External directives, marked as `block_check`, are not part of a block
themselves but provide an opportunity to close an open block. The parser
calls a `leave` function only when it encounters a `block_check` directive,
allowing it to finalize any open block. Each directive is associated with
its own parsing function, which is declared in the `gl_directives` array
along with its `block_check` property.
*/
/*
In the comments below, backticks designate directives, curly braces
designate .desktop file property names, and "=" is added for clarity.
*/
struct Entry
{
 enum EntryFlags         flags;
 guint                   menu_depth;
 gint                    index;                         /* `directive[index]` */
 const struct Directive  *directive;
 enum LineType           allowed_requester;
 fnEntry                 leave;              /* block directives define leave */
#ifdef FEATURE_CONDITIONAL
 gboolean                is_isolated_reference;    /* true if *_dat =~ ^$ref$ */
#endif
#ifdef FEATURE_LAUNCHER
 gboolean                launcher_file_is_invalid;
 enum LauncherType       launcher_type;                             /* {Type} */
 gboolean                no_display;                           /* {NoDisplay} */
 gchar                   categories[MAX_LINE_LENGTH + 1];     /* {Categories} */
#endif
 const struct            InputSource *isrc;
 gchar                   *_dat;                                /* isrc.cooked */
 gchar                   label[MAX_LINE_LENGTH + 1]; /* `item/submenu` {Name} */
 gchar                   cmd[MAX_PATH_LEN + 1];               /* `cmd` {Exec} */
 gchar                   icon[MAX_PATH_LEN + 1];             /* `icon` {Icon} */
#ifdef FEATURE_TOOLTIP
 gchar                   tooltip[MAX_LINE_LENGTH + 1]; /* `tooltip` {Comment} */
#endif
 ErrorList               *error;
 struct ErrorContext     *errctx;
};

/*
To disallow the Cmd, Icon and Tooltip directives in the context of the
current menu entry, assign ENTRY_DISALLOW_DIRECTIVE to the first byte of
the .cmd, .icon, and .tooltip members of the current Entry.
*/
#define ENTRY_DISALLOW_DIRECTIVE '\001'
/*
If Icon=NULL follows a Directory Include directive, assign ENTRY_NULL_ICON
to the first byte of the .cmd member of the current Entry.
*/
#define ENTRY_NULL_ICON '\002'

struct Entry *
entry_new ();

struct Entry *
entry_new_tracked ();

guint
entry_get_tracked_count ();

void
entry_init (struct Entry *entry,
            const fnEntry fn_leave,
            const enum LineType requester,
            const enum EntryFlags feat,
            const guint depth);

enum Result
entry_is_directive_allowed (struct Entry *entry,
                            const gchar *value);

void
error_list_free (ErrorList **list);

void
entry_free_error (struct Entry *entry);

void
entry_free (struct Entry *entry);

void
entry_push_error (struct Entry *entry,
                  const enum Result level,
                  const gchar *fmt,
                  ...);

ErrorList *
entry_pop_error (struct Entry *entry);

void
entry_prefix_error (struct Entry *entry,
                    const gchar *fmt,
                    ...);

enum Result
entry_get_max_error_level (const struct Entry *entry,
                           enum Result base,
                           const ErrorList *stop);

void
entry_set_error_level (struct Entry *entry,
                       const enum Result level);

void
entry_emit_error (struct Entry *entry);

int
entry_module_init (void);

void
entry_module_finalize (void);

#ifdef __cplusplus
}
#endif
#endif /* _ENTRY_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
