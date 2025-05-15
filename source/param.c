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

#include <gtk/gtk.h>
#include "common.h"
#include "entry.h"
#include "logmsg.h"
#include "param.h"
#include "param.decl.h"

/**
param_array_init:
Initialize a %Params variable holding command-line or `include=` parameters.

@param: %Params pointer.
@argc: length of parameter array.
@argv: parameter array.
@par0: pointer to script parameter `$0` ("-" or "$0" or pathname).
*/
gboolean
param_array_init (struct Params *param,
                  const guint argc,
                  const gchar **argv,
                  const gchar *par0)
{
 param->argc = argc;
 param->argv = argv;
 param->par0 = par0;
 param->in_use = NULL;
 if (argc)
 {
  param->in_use = (gboolean *) calloc (argc, sizeof (gboolean));
  if (!param->in_use)
  {
   perror ("calloc parameters");
   return FALSE;
  }
 }
 return TRUE;
}

/**
param_array_destroy:
Free the contents of a %Params variable.

@param: pointer to #Params.
*/
void
param_array_destroy (struct Params *param)
{
 if (param->in_use != NULL)
 {
  if (logmsg_can_emit (ROK))
  {
   for (gint i = 0; i < param->argc; i++)
   {
    if (!param->in_use[i])
    {
     gchar m[128];
     if (snprintf (m, sizeof m, "parameter #%d was not used", i + 1) < (int)
         sizeof m)
     {
      logmsg_emit (ROK, m, NULL);
     }
    }
   }
  }
  free (param->in_use);
 }
 param->argc = 0;
 param->argv = NULL;
 param->in_use = FALSE;
}

/**
expand_param:
Replace a parameter reference ($n) with the parameter's value.

@iptr: return location for a pointer to the first byte after #PV_REF_SYM.
@optr: return location for a pointer to the end of the output buffer.
@max_size: max size available for expansion.
@found: return location for "*@iptr is a valid reference" condition value.
@param: pointer to %Params variable holding existing parameter values.
@entry: pointer to return error messages.

Return: %Result. If the referenced parameter can be found,
expand its value in *@optr, advance @iptr and @optr,
and set *@found = TRUE; otherwise set *@found = FALSE.
An empty or undefined parameter yields an empty string value.
*/
enum Result
expand_param (const gchar **iptr,
              gchar **optr,
              const guint max_size,
              gboolean *found,
              const struct Params *param,
              struct Entry *entry)
{
 guint olen;
 const gchar *src;
 gchar *iptr_end; /* pointer to the first byte after the parameter reference */
 const gulong ref_num = strtoul (*iptr, &iptr_end, 10);
 const gint idx = ref_num - 1;

 *found = *iptr != iptr_end;

 if (!*found)
 {
  /* PV_REF_SYM singleton => parameter number not found. */
  return ROK;
 }
 if (ref_num == 0)
 {
  src = param->par0;
  olen = strlen (src);
 }
 else if (idx >= param->argc || param->argv == NULL)
 {
  /* Parameter is unassigned: nothing to copy. */
  src = NULL;
  olen = 0;
 }
 else
 {
  /* Copy the expanded parameter. */
  src = param->argv[idx];
  olen = strlen(src);
  if (param->in_use != NULL)
  {
   param->in_use[idx] = TRUE;
  }
 }

 if (olen >= max_size)
 {
  entry_push_error (entry, RFAIL, "line too long after expanding parameter $%d",
                      (gint) ref_num);
  return RFAIL;
 }
 else if (olen > 0)
 {
  strncpy (*optr, src, olen);
 }
 *iptr   = iptr_end;
 *optr += olen;

 return ROK;
}

/* vim:set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
