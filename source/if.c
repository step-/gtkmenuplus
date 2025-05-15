/*
Copyright (C) 2019,2024-2025 step https://github.com/step-/gtkmenuplus

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "autoconfig.h"
#include "common.h"
#include "entry.h"
#include "if.h"
#include "if.decl.h"
#include "if_truth_value.decl.h"
#include "input.h"
#include "logmsg.h"

typedef struct _if_unit
{
 /* True while 'if=' is active on directives at the current nesting level. */
 gboolean is_active;
 /* True if directive 'else' was seen. */
 gboolean has_else;
 /* True while nested if/else directives are allocated on the heap. */
 /* True if the current if= level is evaluating contained directives. */
 /* Otherwise it's scanning lines to find a matching else[if]/endif. */
 gboolean evaluates_body;
 /* if= expression's truth value. */
 if_truth_value truth_value;
 /* if/else(if)/endif line number (debug). */
 guint lineno;
} if_unit;

/* If-context stack.
Each context holds pointers to the if-units of the current file. */
static GPtrArray *gl_if_ctx[NESTED_INCLUDE_LIMIT] = {0};
static gint gl_if_ctx_top = -1;

/**
if_context_push_new:
Push a newly-allocated stack element.

Return: FALSE if the allocation failed, TRUE otherwise.
The module owns the memory.
*/
gboolean
if_context_push_new ()
{
 if (gl_if_ctx_top < (gint) G_N_ELEMENTS (gl_if_ctx))
 {
  GPtrArray *ptr = g_ptr_array_new_full (NESTED_IF_LIMIT, g_free);
  if (ptr != NULL)
  {
   gl_if_ctx[++gl_if_ctx_top] = ptr;
   return TRUE;
  }
 }
 return FALSE;
}

/**
if_context_pop:
Pop the top stack element.
*/
gboolean
if_context_pop ()
{
 if (gl_if_ctx_top >= 0)
 {
  g_ptr_array_unref (gl_if_ctx[gl_if_ctx_top]);
  gl_if_ctx[gl_if_ctx_top--] = NULL;
  return TRUE;
 }
 return FALSE;
}

/**
if_unit_push_new:
Allocate a new #if-unit and add it the stack top element.

Return: FALSE if the allocation failed, TRUE otherwise.
The module owns the memory.
*/
gboolean
if_unit_push_new ()
{
 if (gl_if_ctx_top >= 0)
 {
  if_unit *unit = malloc (sizeof (if_unit));
  if (unit != NULL)
  {
   GPtrArray *top = gl_if_ctx[gl_if_ctx_top];

   /* Inherit the evaluates_body value from the outer if=. */
   /* The outermost if= always evaluates its body. */
   const gboolean evaluates_body =
   top->len ? ((if_unit *) g_ptr_array_index
               (top, top->len - 1))->evaluates_body : TRUE;

   g_ptr_array_add (top, unit);
   unit->evaluates_body = evaluates_body;
   unit->is_active = TRUE;
   unit->has_else = FALSE;
   unit->truth_value = TRUTH_FALSE;
   return TRUE;
  }
 }
 return FALSE;
}

/**
*/
void
if_unit_pop ()
{
 GPtrArray *top = gl_if_ctx[gl_if_ctx_top];
 if (top->len)
 {
  g_ptr_array_set_size (top, top->len - 1);
 }
}

/**
*/
static struct _if_unit *
if_fetch_top_unit ()
{
 if (gl_if_ctx_top >= 0)
 {
  GPtrArray *top = gl_if_ctx[gl_if_ctx_top];
  if (top->len > 0)
  {
   return (if_unit *) g_ptr_array_index (top, top->len - 1);
  }
 }
 return NULL;
}

/**
*/
gboolean
if_get_has_else ()
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  return unit->has_else;
 }
 return FALSE;
}

/**
*/
void
if_set_has_else (gboolean state)
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  unit->has_else = state;
 }
}

/**
*/
gboolean
if_get_is_active ()
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  return unit->is_active;
 }
 return FALSE;
}

/**
*/
void
if_set_is_active (gboolean state)
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  unit->is_active = state;
 }
}

/**
*/
gboolean
if_get_evaluates_body ()
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  return unit->evaluates_body;
 }
 return FALSE;
}

/**
*/
gboolean
if_get_container_evaluates_body ()
{
 if (gl_if_ctx_top >= 0)
 {
  GPtrArray *top = gl_if_ctx[gl_if_ctx_top];
  if (top->len > 1)
  {
   return ((if_unit *) g_ptr_array_index (top, top->len - 2))->evaluates_body;
  }
 }
 return FALSE;
}

/**
*/
gboolean
if_get_is_contained ()
{
 GPtrArray *top = gl_if_ctx[gl_if_ctx_top];
 return top->len > 1;
}

/**
*/
void
if_set_evaluates_body (gboolean state)
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  unit->evaluates_body = state;
 }
}

/**
*/
if_truth_value
if_get_truth_value ()
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  return unit->truth_value;
 }
 return TRUTH_INVALID;
}

/**
*/
void
if_set_truth_value (if_truth_value state)
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  unit->truth_value = state;
 }
}

/**
*/
void
if_set_lineno (guint lineno)
{
 if_unit *unit = if_fetch_top_unit ();
 if (unit)
 {
  unit->lineno = lineno;
 }
}

/**
if_evaluate_expression
Evaluate condition expression.

@entry: pointer to struct #Entry.
@label: a label for error messages.

Return: #Result ROK if evaluation succeeded and properties of
the stack top element's #if-unit could be modified accordingly;
#Result RFAIL and push an error message otherwise.
*/
enum Result
if_evaluate_expression (struct Entry *entry,
                        const gchar *label)
{
 gchar *_dat = entry->_dat;
 gchar caze[64] = {0};
 enum Result result = ROK;
 if_truth_value truth_value;

 /*****
 Compute "case" (see our ->man(5) page for the definition of "case").
 *****/

#if defined(FEATURE_PARAMETER) || defined(FEATURE_VARIABLE)
 if (entry->is_isolated_reference)
 {
  if (_dat[0] == '\0')
  {
   /* empty condition variable => "0" => FALSE */
   strcpy (caze, "0 (empty $ref)");
  }
  else if (entry->flags & ENTRY_FLAG_QEQ)
  {
   /* `if ?= non-empty` */
   strcpy (caze, "1 (non-empty $ref)");
  }
 }
 else
#endif
 {
  if (*_dat)
  {
   gchar *err_msg = NULL;
   result = popen_read (label, _dat, caze, sizeof (caze) - 1, &err_msg);
   if (err_msg)
   {
    entry_push_error (entry, result, "%s", err_msg);
    g_free (err_msg);
   }
  }
 }

 /*****
 Compute "truth value" from "case", and set if-unit's members consequently.
 *****/

 if (*caze == '\0')
 {
  /* empty condition constant string */
  truth_value = TRUTH_FALSE;
  if_set_evaluates_body (FALSE);
 }
 else
 {
  gchar *end;
  glong num;

  num = strtol (caze, &end, 0);
  if (caze < end)
  {
   truth_value = num != 0 ? TRUTH_TRUE : TRUTH_FALSE;
   if_set_evaluates_body (num != 0);
  }
  else
  {
   truth_value = if_match_truth_value (caze);
   if (truth_value != TRUTH_INVALID)
   {
    if_set_evaluates_body (truth_value);
   }
  }
 }

 if (logmsg_can_emit (RDEBUG))
 {
  static gchar m[512];
  if (snprintf (m, sizeof m,
                "`if` truth value is %d from \"%s\" on line %d: %s",
                truth_value, caze, entry->isrc->lineno, _dat) < (gint) sizeof m)
  {
   logmsg_emit (RDEBUG, m, NULL);
  }
 }

 if (truth_value != TRUTH_INVALID)
 {
  if (if_get_evaluates_body ())
  {
   if_set_truth_value (TRUTH_TRUE);
  }
 }
 else
 {
  result = RFAIL;
  entry_push_error (entry, result, "in `if` condition: invalid case \"%s\"",
                    caze);
 }

 if (result != ROK)
 {
  /* Suspend directive evaluation through `endif`. */
  if_set_evaluates_body (FALSE);
  if_set_truth_value (TRUTH_INVALID);
 }
 return result;
}

#ifdef DEBUG_SHOW_CONDITION
/**
*/
void
if_debug_states (const gchar *msg)
{
 GPtrArray *top = gl_if_ctx[gl_if_ctx_top];
 if (!logmsg_can_emit (RDEBUG))
 {
  return;
 }
 if (msg != NULL)
 {
  fprintf (stderr, "%s\n", msg);
 }
 for (int i = top->len - 1; i >= 0; i--)
 {
  if_unit *p = g_ptr_array_index (top, i);
  fprintf (stderr, "if[%d]"
           " is_active(%d)"
           " has_else(%d)"
           " evaluates_body(%d)"
           " truth_value(%d)"
           " lineno(%d)\n",
           i, p->is_active,
           p->has_else, p->evaluates_body, p->truth_value, p->lineno);
 }
}

#endif
/**
*/
enum Result
a_if (struct Entry *entry)
{
 enum Result result;
 result = if_unit_push_new () ? ROK : RFATAL;
 if (result == ROK)
 {
  if_set_lineno (entry->isrc->lineno);
  if (if_get_evaluates_body ())
  {
    result = if_evaluate_expression (entry, "if");
  }
 }
 DEBUG_IF_STATES (__FUNCTION__);
 return result;
}

/**
*/
enum Result
a_else (struct Entry *entry)
{
 if (!if_get_is_active ())
 {
  entry_push_error (entry, RFAIL, "%s", "else without if");
  return RFAIL;
 }
 if (if_get_has_else ())
 {
  entry_push_error (entry, RFAIL, "%s", "multiple elses after if");
  return RFAIL;
 }
 if_set_lineno (entry->isrc->lineno);
 if_set_has_else (TRUE);

 /*
 This `else` is allowed to evaluate its body if
 [1] the truth value of its matching `if=` is FALSE,    AND
 [2] it's contained in an `if`-`endif` that evaluates.
 */
 if_set_evaluates_body (if_get_truth_value () == TRUTH_FALSE);   /* [1] */
 if (if_get_is_contained () && if_get_evaluates_body ())
 {
  if_set_evaluates_body (if_get_container_evaluates_body ());    /* [2] */
 }
 DEBUG_IF_STATES (__FUNCTION__);
 return ROK;
}

/**
*/
enum Result
a_elseif (struct Entry *entry)
{
 enum Result result = ROK;
 if (!if_get_is_active ())
 {
  entry_push_error (entry, RFAIL, "%s", "elseif without if");
  return RFAIL;
 }
 if (if_get_has_else ())
 {
  entry_push_error (entry, RFAIL, "%s", "elseif after else");
  return RFAIL;
 }
 if_set_lineno (entry->isrc->lineno);
 if (if_get_truth_value () == TRUTH_FALSE)
 {
  if_set_evaluates_body (TRUE);
 }
 else
 {
  if_set_evaluates_body (FALSE);
 }
 if (if_get_evaluates_body () == TRUE)
 {
  result = if_evaluate_expression (entry, "elseif");
 }
 DEBUG_IF_STATES (__FUNCTION__);
 return result;
}

/**
*/
enum Result
a_endif (struct Entry *entry)
{
 DEBUG_IF_STATES (__FUNCTION__);
 if_set_lineno (entry->isrc->lineno);
 if (!if_get_is_active())
 {
  entry_push_error (entry, RFAIL, "%s", "endif without if");
  return RFAIL;
 }
 if_unit_pop ();
 return ROK;
}

/**
if_module_init:
Initialize this module.

Return: 0(OK) -1(Error).
*/
int
if_module_init ()
{
 while (if_context_pop ())
  ;
 return 0;
}

/**
if_module_finalize:
Clean up this module. Call before exit(2).
*/
void
if_module_finalize (void)
{
 while (if_context_pop ())
  ;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
