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

#include "autoconfig.h"
#include "entry.h"
#include "input.h"
#include "logmsg.h"

/* Menu entries that are allocated dynamically for whatever reason. */
static GPtrArray *gl_entries;

/**
entry_new:
Allocate a new #Entry structure. The caller owns the allocated memory.

Return: #Entry pointer to the new structure or NULL on error.
*/
inline struct Entry *
entry_new ()
{
 struct Entry *entry = calloc (1, sizeof (struct Entry));
 return entry;
}

/**
entry_new_tracked:
Allocate a new #Entry structure. The _module_ owns the allocated memory.

Return: #Entry pointer to the new structure or NULL on error.
*/
struct Entry *
entry_new_tracked ()
{
 struct Entry *entry = entry_new ();
 if (entry != NULL)
 {
  g_ptr_array_add (gl_entries, entry);
 }
 return entry;
}

/**
entry_get_tracked_count:
*/

guint
entry_get_tracked_count ()
{
 return gl_entries->len;
}

/**
entry_init:
Initialize simple or block directive.

@entry: target to be initialized.
@fn_leave: function called on leaving a block directive.
NULL for simple directives.
@container: the directive that "contains" @entry;
LINE_UNDEFINED if @entry is not contained.
@feat: #EntryFlags feature bit mask.
@depth: current menu depth.
*/
void
entry_init (struct Entry *entry,
            const fnEntry fn_leave,
            const enum LineType container,
            const enum EntryFlags feat,
            const guint depth)
{
 entry->leave = fn_leave;
 if (entry->leave)
 {
  if (entry->directive->type != LINE_EOF)
  {
   free (entry->errctx->block_line);
   entry->errctx->block_line =
   strdup (input_fetch_current_line
           (entry->isrc, &entry->errctx->block_lineno));
  }
 }
 entry->flags = 0;
#ifdef FEATURE_CONDITIONAL
 entry->is_isolated_reference = 0;
#endif
 entry->label[0] = entry->cmd[1] = entry->icon[1] = '\0';
 entry->icon[0] =
 (feat & ENTRY_FLAG_ALLOW_ICOTIP) ? '\0' : ENTRY_DISALLOW_DIRECTIVE;
 entry->cmd[0] =
 (feat & ENTRY_FLAG_ALLOW_CMD) ? '\0' : ENTRY_DISALLOW_DIRECTIVE;
#ifdef FEATURE_TOOLTIP
 entry->tooltip[1] = '\0';
 entry->tooltip[0] =
 (feat & ENTRY_FLAG_ALLOW_ICOTIP) ? '\0' : ENTRY_DISALLOW_DIRECTIVE;
#endif
 entry->container = container;
 entry->menu_depth = depth;
 if ((feat & ENTRY_FLAG_RESET_ERROR) && entry->error)
 {
  entry_free_error (entry);
 }
#ifdef FEATURE_LAUNCHER
 entry->categories[0] = '\0';
 entry->no_display = FALSE;
#endif
}

/**
entry_is_directive_allowed:
Error check if directive is allowed in the current parsing context.
@entry: pointer to struct #Entry. The parsing context.
@value: current value of the directive in @Entry.
*/
enum Result
entry_is_directive_allowed (struct Entry *entry,
                            const gchar *value)
{
 enum Result result = ROK;

 if (value[0] == ENTRY_DISALLOW_DIRECTIVE)
 {
  result = RFAIL;
  if (entry->container != LINE_UNDEFINED)
  {
   const gchar *container_keyword =
   (input_fetch_directive_from_linetype (entry->container))->keyword;
   entry_push_error (entry, RFAIL, "`%s=` not allowed after `%s=`",
                     entry->directive->keyword, container_keyword);
  }
  else if (entry->directive->type == LINE_CMD)
  {
   entry_push_error (entry, RFAIL,
                     "`%s=` is only allowed after `item=` "
                     "or `include=`", entry->directive->keyword);
   /* `icon=` is allowed after `include=`FILE, where it does nothing, and
   after `include=`DIR, to bulk set the icon of the scanned files. */
  }
  else
  {
   entry_push_error (entry, RFAIL,
                     "`%s=` is only allowed after `item=`, `submenu=` "
                     "or `include=`", entry->directive->keyword);
   /* ditto */
  }
 }
 else if (value[0])
 {
  const gchar *container_keyword =
  (input_fetch_directive_from_linetype (entry->container))->keyword;
  result = RFAIL;
  entry_push_error (entry, result, "found two `%s=` directives after `%s`",
                    entry->directive->keyword, container_keyword);
 }

 return result;
}

/**
error_list_free:
@list: pointer to list pointer. Nulled on return from this function.
*/
void
error_list_free (ErrorList **list)
{
 for (GSList *el = *list; el; el = el->next)
 {
  struct EntryError *error = el->data;
  g_free (error->message);
  error->message = NULL;
  if (error->context && !error->context->block_line_locked)
  {
   g_clear_pointer (&error->context->block_line, g_free);
  }
 }
 g_slist_free_full (*list, g_free);
 *list = NULL;
}

/**
entry_free_error:
*/
void
entry_free_error (struct Entry *entry)
{
 error_list_free (&entry->error);
}

/**
entry_free:
*/
void
entry_free (struct Entry *entry)
{
 entry_free_error (entry);
 free (entry);
}

/**
entry_push_error:
Format an error message and attach it to an entry's LIFO error stack.

@entry:
@level: error severity level.
@fmt ...: printf-like arguments.

A new stack entry is always created. However the error
message will not be copied within if the -v and -q options
do not allow emitting an error with this message @level.
*/
__attribute__((format(printf, 3, 4))) /* 3=format 4=params */
void
entry_push_error (struct Entry *entry,
                  const enum Result level,
                  const gchar *fmt,
                  ...)
{
 struct EntryError *error = g_malloc (sizeof *error);
 if (error != NULL)
 {
  error->message = NULL;
  if (logmsg_can_emit (level))
  {
   if (logmsg_get_verbosity () < 3)
   {
    if (entry->error && entry->error->next) /* #messages > 2 */
    {
     if (entry->error->next->next) /* #messages > 3 */
     {
      goto push;
     }
     error->message =
     g_strdup_printf ("too many errors; run with option -vvv to view all");
     goto push;
    }
   }
   va_list args;
   va_start (args, fmt);
   error->message = g_strdup_vprintf (fmt, args);
   va_end (args);
  }
push:
  error->level = level;
  error->context = entry->errctx;
  entry->error = g_slist_prepend (entry->error, error);
 }
}

/**
entry_pop_error:
*/
ErrorList *
entry_pop_error (struct Entry *entry)
{
 ErrorList *head = entry->error;
 if (head)
 {
  entry->error = g_slist_remove_link (entry->error, head);
 }
 return head;
}

/**
entry_prefix_error:
Prefix the error list head element unless the message is NULL.
*/
__attribute__((format(printf, 2, 3))) /* 2=format 3=params */
void
entry_prefix_error (struct Entry *entry,
                    const gchar *fmt,
                    ...)
{
 if (entry->error != NULL)
 {
  struct EntryError *error = entry->error->data;
  gchar *prefix, *old = error->message;

  if (old)
  {
   va_list args;
   va_start (args, fmt);
   prefix = g_strdup_vprintf (fmt, args);
   va_end (args);

   error->message = g_strconcat (prefix, old, NULL);
   g_free (old);
   g_free (prefix);
  }
 }
}

/**
entry_get_max_error_level:
Get the maximum level over the error list of a struct #Entry.

@entry:
@base: return no less than the @base value.
@stop: if NULL iterate over list element range [head, tail] (whole list);
over the range [head, stop) otherwise.

Return: the maximum m
*/
enum Result
entry_get_max_error_level (const struct Entry *entry,
                           enum Result base,
                           const ErrorList *stop)
{
 for (GSList *el = entry->error; el && el != stop; el = el->next)
 {
  struct EntryError *error = el->data;
  if (error->level > base)
  {
   base = error->level;
  }
 }
 return base;
}

/**
entry_set_error_level:
Change the error list head's level.
*/
void
entry_set_error_level (struct Entry *entry,
                       const enum Result level)
{
 if (entry->error)
 {
  struct EntryError *error = entry->error->data;
  error->level = level;
 }
}

/**
entry_print_error:
Pretty print an error.

@data: pointer to struct #EntryError.
@user_data: pointer to struct #Entry.
*/
static void
entry_print_error (gpointer data,
                   gpointer user_data)
{
 struct EntryError *error = data;
 struct Entry *entry = user_data;

 if (error->message != NULL)
 {
  logmsg_emit (error->level, error->message, entry);
 }
}

/**
entry_emit_error:
Pretty print the error list.
*/
void
entry_emit_error (struct Entry *entry)
{
 if (entry->error)
 {
  static const struct InputSource isrc = {0};
  if (entry->isrc == NULL)
  {
   entry->isrc = &isrc;
  }
  entry_print_error (entry->error->data, entry);
  if (entry->error->next)
  {
   entry->errctx->block_lineno = 0; /* reduce noise */
   ErrorList *revtail = g_slist_reverse (g_slist_copy (entry->error->next));
   g_slist_foreach (revtail, (GFunc) entry_print_error, entry);
   g_slist_free (revtail);
  }
  if (entry->isrc == &isrc)
  {
   entry->isrc = NULL;
  }
 }
}

/**
entry_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
entry_module_init (void)
{
 gl_entries = g_ptr_array_new_full (32, (GDestroyNotify) entry_free);
 return 0;
}

/**
entry_module_finalize:
Clean up this module. Call before exit(2).
*/
void
entry_module_finalize (void)
{
 (void) g_ptr_array_free (gl_entries, TRUE);
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
