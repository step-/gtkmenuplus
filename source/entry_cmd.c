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
#include "entry.h"
#include "entry_cmd.h"

static regex_t gl_rgx_uri_schema;

/**
entry_cmd_info_new_for_type:
Return #GAppInfo and #GFile corresponding to @cmd.
@entry: pointer to struct #Entry, usually @cmd's.
@cmd: command string.
@app_info: pointer to return #GAppInfo. Nullable.
@file: pointer to return #GFile. Nullable.

@cmd can be a file:// or resource:// URI or the pathname of a
document whose content type will correspond to *@app_info. @cmd will
correspond to *@file.

Return: TRUE and allocate the #GAppInfo and #GFile
structure; FALSE and push an error message otherwise.
The caller owns the allocated memory. Use g_object_unref to free.
*/
gboolean
entry_cmd_info_new_for_type (struct Entry *entry,
                             const gchar *cmd,
                             GAppInfo **app_info,
                             GFile **file)
{
 regmatch_t     match[2];
 GFile          *filo  = NULL;
 GFileInfo      *info  = NULL;
 GError         *error = NULL;
 gboolean       ret    = FALSE;

 if (file == NULL && app_info == NULL)
 {
  return FALSE;
 }
 /* permissive regex */
 if (regexec (&gl_rgx_uri_schema, cmd, 1, match, 0) == 0)
 {
  /* g_file_new_for_uri only supports file:///path, resource:///path */
  filo = g_file_new_for_uri (cmd);
 }
 else
 {
  /* disk file */
  filo = g_file_new_for_path (cmd);
 }
 if (file != NULL)
 {
  ret = TRUE;
  *file = filo;
 }
 if (app_info != NULL)
 {
  ret = FALSE;
  if (filo != NULL)
  {
   info = g_file_query_info (filo, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0,
                             NULL, &error);
   if (file == NULL)
   {
    g_object_unref (G_OBJECT (filo));
   }
  }
  if (error != NULL)
  {
   entry_push_error (entry, RFAIL, "%s", error->message);
   g_error_free (error);
  }
  else if (info != NULL)
  {
   const gchar *type = g_file_info_get_content_type (info);
   if (type != NULL)
   {
    ret = TRUE;
    *app_info = g_app_info_get_default_for_type (type, FALSE);
#if 0
    g_printerr ("mime: %s\n", g_content_type_get_description (type));
    if (*app_info)
    {
     g_printerr ("exe:  %s\ndesc: %s\ncmd:  %s\n",
     g_app_info_get_executable (*app_info),
     g_app_info_get_description (*app_info),
     g_app_info_get_commandline (*app_info));
    }
#endif
   }
   g_object_unref (info);
  }
 }
 return ret;
}

/**
entry_icon_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
entry_cmd_module_init (void)
{
 return compile_regex (&gl_rgx_uri_schema, "^[a-z]+://",
                       REG_EXTENDED | REG_ICASE);
}

/**
entry_icon_module_finalize:
Clean up this module. Call before exit(2).
*/
void
entry_cmd_module_finalize (void)
{
 regfree (&gl_rgx_uri_schema);
}

/* vim:set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
