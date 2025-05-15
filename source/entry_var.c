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

#include <ctype.h>
#include <gtk/gtk.h>
#include "common.h"
#include "entry.h"
#include "entry_var.h"
#include "entry_var.decl.h"
#include "input.h"
#include "logmsg.h"

static struct Variable *gl_var_table = NULL;

/**
entry_var_add:
Parse the `variable=[=]`directive, and add the variable's
name and literal value to the table of variables.

@entry: pointer to the current #Entry.
@varptr: pointer to a %Variable pointer, input and output argument.

Return: %Result. Set *@varptr to a newly-allocated %Variable structure,
if an evaluated (`==`) variable was parsed. Set *@varptr NULL for
the non-evaluated case. An existing variable is not added again.

Storage memory is freed automatically at program exit.
*/
static enum Result
entry_var_add (struct Entry *entry,
               struct Variable **varptr)
{
 gchar *_dat = (gchar *) entry->_dat;
 gchar *oeq = strchr (_dat, '=');
 gboolean eval = FALSE;
 enum Result result = RFAIL;
 const gchar *name;
 gchar *value, *p;
 gsize vlen, nlen;
 struct Variable *var = NULL;

 if (oeq == NULL)
 {
  entry_push_error (entry, result, "expected `=` after identifier \"%s\"",
                    _dat);
  return result;
 }
 if G_UNLIKELY (oeq == _dat)
 {
  /* Effectively NOT REACHED because the `absolute_path`
  directive took ownership of the /^\s*=/ line syntax. */
  entry_push_error (entry, result, "%s", "missing identifier before `=`");
  goto out;
 }

 *oeq = '\0'; /* reset before return */
 for (p = _dat; *p && isspace (*p); p++)
  ;
 input_trim_from_end ((gchar *) p, oeq - 1);
 name = p;

 if (getenv (name) != NULL)
 {
  /* Warn instead of failing for backward compatibility < 1.50.0. */
  result = RWARN;
  entry_push_error (entry, result,
                    "clobbering the %s environment variable is not allowed.",
                    name);
  goto out;
 }

 p = oeq + 1;
 eval = *p == '=';
 if (eval)
 {
  ++p;
 }
 for (; *p && isspace (*p); p++)
  ;
 if (*p == '\'' || *p == '"')
 {
  gchar *q = strrchr(p, *p);
  if (q)
  {
   *q = '\0';
  }
  p++;
 }
 value = p;

 if (!isalpha (*name) && *name != '_')
 {
  entry_push_error (entry, result, "invalid variable name \"%s\""
                    " (must start with letter or underscore)", name);
  goto out;
 }
 for (p = (gchar *) name; *p && (isalnum (*p) || *p == '_'); p++)
  ;
 if (*p)
 {
  entry_push_error (entry, result, "variable name '%s' contains characters "
                    "other than alpha, digit or underscore", name);
  goto out;
 }

 vlen = strlen (value);
 for (var = gl_var_table; var; var = var->next)
 {
  if (strcmp (var->name_value, name) == 0)
  {
   var->value_len = vlen;
   if (vlen > 0)
   {
    memcpy (var->value, value, vlen);
   }
   var->value[vlen] = '\0';
   result = ROK;
   goto out;
  }
 }

 /*
 Name not found; allocate a new Variable.
 */

 var = (struct Variable *) malloc (sizeof (struct Variable));
 if (var == NULL)
 {
  fprintf (stderr, "malloc ");
  perror (name);
  result = RFATAL;
  goto out;
 }

 /* Store name and value as contiguous strings in the same buffer. */
 nlen = strlen (name);
 var->value_len = vlen;
 memcpy (var->name_value, name, nlen);
 var->name_value[nlen] = '\0';
 var->value = var->name_value + nlen + 1;
 if (vlen > 0)
 {
  memcpy (var->value, value, vlen);
 }
 var->value[vlen] = '\0';
 var->used = var->eval = FALSE;
 var->next = NULL;
 var->next = gl_var_table;
gl_var_table = var;
 result = ROK;

out:
 if (eval)
 {
  *varptr = var;
 }
 *oeq = '=';
 return result;
}

/**
var_table_destroy:
Free the contents of the global table of script variables.
*/
static
void var_table_destroy ()
{
 struct Variable *pv, *var = gl_var_table;
 const gboolean can_emit = logmsg_can_emit (ROK);
 while (var != NULL)
 {
  if (can_emit && !var->used)
  {
   gchar *m = g_strdup_printf ("variable \"%s\" was not used", var->name_value);
   logmsg_emit (ROK, m, NULL);
   g_free (m);
  }
  pv = var;
  var = var->next;
  free (pv);
 }
}

/**
var_find:
Find *name in the global table of script variables.
@name: byte array - not necessarily a C string.
@length: @name length (bytes).

@name matches a name in the table when @length bytes match and the next byte
of the compared name is null.

Return: pointer to the %Variable record or NULL if not found.
*/
static struct Variable*
var_find (const gchar *name,
          const guint length)
{
 struct Variable* v = gl_var_table;
 while (v != NULL)
 {
  if (strncmp(v->name_value, name, length) == 0)
  {
   if (v->name_value[length] == '\0')
   {
    return v;
   }
  }
  v = v->next;
 }
 return NULL;
}

/**
entry_var_eval:
Run the RHS of a `var==` directive saving output as var's value.

@entry: pointer used to return an error message.
@var: %Variable pointer, which will be modified.
*/
static enum Result
entry_var_eval (struct Entry *entry,
                struct Variable *var)
{
 gchar buf[MAX_LINE_LENGTH + 1] = {0};
 gchar *err_msg = NULL;

 enum Result result =
 popen_read ("variable==", var->value, buf,
             MAX_LINE_LENGTH * sizeof (gchar), &err_msg);
 if (err_msg)
 {
  entry_push_error (entry, result, "%s", err_msg);
  g_free (err_msg);
 }
 if (result != ROK)
 {
  return result;
 }
 if (strlen (buf) + strlen (var->name_value) >= MAX_LINE_LENGTH)
 {
   entry_push_error (entry, RFAIL, "value of evaluated variable %s too long",
                      var->name_value);
   return RFAIL;
 }
 strcpy(var->value, buf);
 var->value_len = strlen(var->value);
 return ROK;
}

enum Result
entry_var (struct Entry *entry)
{
 struct Variable *var = NULL;
 enum Result result = entry_var_add (entry, &var);
 if (result == ROK)
 {
  if (var != NULL)
  {
   result = entry_var_eval (entry, var);
  }
 }
 return result;
}

/**
entry_var_expand
Replace a variable reference ($name) with the variable's value.

@entry: pointer to return error messages.
@iptr: input/output; as input, *@iptr is the first byte after #PV_REF_SYM;
as output, *@iptr points to the first byte after the variable reference.
@optr: return location for a pointer to the end of the output buffer.
@max_size: maximum size available for the expansion.
@found: return location for "*@iptr is a valid reference" condition value.

Return: %Result. If the referenced variable can be found, expand its value in
*@optr, advance @iptr and @optr (so that the expanded value starts at @iptr
and is @optr-@iptr bytes long), and set *@found = TRUE; otherwise set *@found
= FALSE.  An undefined variable yields its unexpanded name.
*/
enum Result
entry_var_expand (struct Entry *entry,
                  const gchar **iptr,
                  gchar **optr,
                  const guint max_size,
                  gboolean *found)
{
 gboolean brace = *iptr[0] == '{';
 guint nlen = 0, olen;
 gchar name[member_size (struct Variable, name_value)] = "";
 struct Variable *var;
 const gchar *p;
 gchar *q;

 *found = FALSE;

 for (p = brace ? *iptr + 1 : *iptr, q = name; nlen < max_size - 1 && *p &&
      (isalnum (*p) || *p == '_' || *p == '}'); nlen++, q++)
 {
  *q = *p++;
  if (*q == '}')
  {
   ++p;
   break;
  }
 }
 name[nlen] = '\0';

 if (nlen == 0 && !brace)
 {
  /* PV_REF_SYM singleton => variable name not found. */
  return ROK;
 }
 if (getenv (name) != NULL)
 {
  /* Environment variable name clash => variable name not found */
  return ROK;

  /* Rule out using internal names that clash with environment
  names to ensure that the `cmd=` and `var==` directives
  can freely mix internal and environment variables. */
 }
 var = var_find (name, nlen);
 if (var == NULL)
 {
  /* Unassigned variable; variable_name not found. */

  /* Preserve the original reference to allow setting and
  expanding shell variables in `cmd=` and `var==` directives. */
  return ROK;
 }

 *found = TRUE;
 olen = var->value_len;
 if (olen >= max_size)
 {
  entry_push_error (entry, RFAIL,
                    "line too long after expanding variable $%s", name);
  return RFAIL;
 }

 strcpy (*optr, var->value);
 var->used = TRUE;
 *iptr += brace ? nlen + 2 : nlen;
 *optr += olen;

 return ROK;
}

/**
var_name_test:
Quick test if a string could be a variable name.
The test is based on the fist byte only.
*/
gboolean
var_name_test (const gchar *p)
{
 return *p == '_' || *p == '{' || isalpha (*p);
}

/**
*/
enum Result
a_variable (struct Entry *entry)
{
 return entry_var (entry);
}

/**
entry_var_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
entry_var_module_init (void)
{
 return 0;
}

/**
entry_var_module_finalize:
Clean up this module. Call before exit(2).
*/
void
entry_var_module_finalize (void)
{
 var_table_destroy ();
}

/* vim:set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
