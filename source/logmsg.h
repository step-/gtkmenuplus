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

#ifndef _TOUSER_H
#define _TOUSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "entry.h"

/**********************************************************************
*                              Message                               *
**********************************************************************/

/* Message store item for deferred messages. */
struct ToUserMessage
{
 enum Result severity;
 gchar *msg;
};

void
logmsg_str_shorten (gchar *str);

gboolean
logmsg_can_emit (const enum Result level);

void
logmsg_emit (enum Result result,
             const gchar * message,
             const struct Entry *entry);

guint
logmsg_get_verbosity (void);

void
logmsg_set_verbosity (const guint);

guint
logmsg_get_quiet ();

void
logmsg_set_quiet (const guint value);

int
logmsg_module_init (void);

#ifdef __cplusplus
}
#endif
#endif /* _TOUSER_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
