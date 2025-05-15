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

#ifndef _IF_H
#define _IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include "common.h"
#include "entry.h"

typedef enum
{
   TRUTH_FALSE = 0, /* must be 0 */
   TRUTH_TRUE,
   TRUTH_INVALID,
} if_truth_value;

gboolean if_context_push_new ();
gboolean if_context_pop ();
gboolean if_unit_push_new ();
void if_unit_pop ();
gboolean if_get_is_active ();
void if_set_is_active (gboolean state);
gboolean if_get_has_else ();
void if_set_has_else (gboolean state);
gboolean if_get_evaluates_body ();
gboolean if_get_container_evaluates_body ();
gboolean if_get_is_contained ();
void if_set_evaluates_body (gboolean state);
if_truth_value if_get_truth_value ();
void if_set_truth_value (if_truth_value);
void if_set_lineno (guint lineno);

enum Result
if_evaluate_expression (struct Entry *entry,
                        const gchar *label);

int
if_module_init ();

void
if_module_finalize (void);

#ifdef DEBUG_SHOW_CONDITION
void
if_debug_states (const gchar *);
#define DEBUG_IF_STATES(MSG)     if_debug_states (MSG)
#else
#define DEBUG_IF_STATES(MSG)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _IF_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
