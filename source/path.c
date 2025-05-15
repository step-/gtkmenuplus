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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <regex.h>
#include <libgen.h>
#include "common.h"
#include "input.h"
#include "path.h"

gchar gl_script_dir[MAX_PATH_LEN + 1];

struct path_expand_regex
{
 const gchar *pattern;
 const gchar *needle;
 const guint advance; /* bytes */
 regex_t     *regex;
};

#define ANCHOR            "(^|[ \"'\\t])"

static struct path_expand_regex gl_path_expand_regexes[] =
{
 /* G_DIR_SEPARATOR */
 {ANCHOR "(\\~/)",     "~/",   1, 0}, /* for $HOME */ /* keep first */
 {ANCHOR "(\\~\\\\/)", "~\\/", 3, 0}, /* escaped */
 {ANCHOR "(\\./)",     "./",   2, 0}, /* applicable basedir ends with '/' */
 {ANCHOR "(\\.\\\\/)", ".\\/", 3, 0}, /* escaped */
 {0},
};

/**
path_expand_abbrev:
Replace abbreviation anywhere in string.

@haystack: subject string.
@needle: abbreviation.
@regex: #regex_t matcher for the needle.
@advance: advance these many bytes through haystack after a replacement.
@replacement: string.
@size: maximum byte size of the returned string.
@error: pointer to unsigned.

Returns: NULL and *@error != 0 if an error occurred; NULL and *@error == 0 if
no match. Otherwise it returns a newly allocated string and *@error == 0.  The
caller owns the memory.
*/
static gchar *
path_expand_abbrev (const gchar *haystack,
                    const gchar *needle,
                    const regex_t *regex,
                    const guint advance,
                    const gchar *replacement,
                    const guint size,
                    guint *error)
{
 const gchar *src; /* source current start */
 gchar *dst;       /* destination current start */
 gchar *buf;       /* return buffer */
 const gchar *end; /* buffer end */
 gboolean matched, overrun = FALSE;
 guint err = *error = 0;

 if (strstr (haystack, needle) == NULL)
 {
  return NULL;
 }
 buf = dst = malloc (size);
 if (buf == NULL)
 {
  perror (__FUNCTION__);
  err = 1;
  goto out;
 }
 end = buf + size - 1;
 src = haystack;
 const guint repl_len = strlen (replacement);
 const gboolean repl_ends_with_slash =
 replacement[repl_len - 1] == G_DIR_SEPARATOR;

 do {
  regmatch_t match[3];
  guint offset;

  overrun = FALSE;
  matched = regexec (regex, src, 3, match, 0) == 0;
  if (matched)
  {
   offset = match[1].rm_eo; /* before match */
   if (offset)
   {
    overrun = dst + offset >= end;
    if (!overrun)
    {
     memcpy (dst, src, offset);
     dst += offset;
     src += offset;
    }
   }
   overrun = dst + repl_len >= end;
   if (!overrun)
   {
    memcpy (dst, replacement, repl_len);
    dst += repl_len;
    src += advance > 0 ? advance : 1;
    if (repl_ends_with_slash && *src == G_DIR_SEPARATOR)
    {
     dst--; /* squeeze "//" to "/" */
    }
    dst[0] = '\0';
   }
  }
 } while (matched && !overrun && dst < end && *src);

 if (overrun)
 {
  err = 1;
 }
 else if (src[0])
 {
  guint len = strlen (src);
  if (dst + len < end)
  {
   memcpy (dst, src, len);
   dst[len] = '\0';
  }
  else
  {
   err = 1;
  }
 }

 if (err)
 {
  free (buf);
  buf = NULL;
 }
out:
 *error = err;
 return buf;
}

/**
path_expand_full:
Expand "~" with $HOME and "./" with the supplied path.

@path: haystack.
@dotpath: replacement for abbreviation "./". Must end with '/'.
@error: pointer to unsigned.

Returns: newly allocated string if the path was expanded otherwise NULL and set
*@error != 0 if an error occurred.
The size of the returned string not to exceed the size of `cmd=`'s RHS.
Caller owns the memory.
*/
gchar *
path_expand_full (const gchar *path,
                  const gchar *dotpath,
                  guint *error)
{
 gchar *str = NULL, *strx;
 guint err = 0;
 static gchar *home = NULL;
 static const gchar empty[] = "";

 assert (strchr (dotpath, '\0')[-1] == G_DIR_SEPARATOR);
 if (home == NULL)
 {
  gchar *p = getenv ("HOME");
  home = p != NULL && strchr (p, '~') == NULL ? p : (gchar*) empty;
 }
 for (guint i = 0; err == 0 && gl_path_expand_regexes[i].pattern != NULL; i++)
 {
  const gchar *haystack = str != NULL ? str : path;

  if (i % 2 == 0)
  {
   strx =
   path_expand_abbrev (haystack,
                       gl_path_expand_regexes[i].needle,
                       gl_path_expand_regexes[i].regex,
                       gl_path_expand_regexes[i].advance,
                       i == 0 ? home : dotpath, SIZEOF_COOKED, &err);
  }
  else
  {
   /* Unescape current needle as previous needle. */
   strx =
   path_expand_abbrev (haystack,
                       gl_path_expand_regexes[i].needle,
                       gl_path_expand_regexes[i].regex,
                       gl_path_expand_regexes[i].advance,
                       gl_path_expand_regexes[i - 1].needle,
                       SIZEOF_COOKED, &err);
  }
  if (strx != NULL)
  {
   free (str);
   str = strx;
  }
 }
 *error = err;
 return str;
}

/**
path_expand:
Overwrite input buffer with expanded path.

@path: In/Out string buffer of size #SIZEOF_COOKED.
@basedir:
@label:
@entry: pointer to return errors.
*/
enum Result
path_expand (gchar *path,
             gchar *basedir,
             const gchar *label,
             struct Entry *entry)
{
 gchar *p = path;
 enum Result result = ROK;

 if (*p == '\'' || *p == '"' )
 {
  ++p;
 }
 if (*p != G_DIR_SEPARATOR)
 {
  if (basedir[0] && (*p != '.' || (*p == '.' && p[1] == '.')) && *p != '~')
  {
   guint bl = strlen (basedir);
   guint pl = strlen (path);

   if (bl + pl + 1 >= SIZEOF_COOKED)
   {
    result = RFAIL;
    entry_push_error (entry, result, "%s: rebased path too long", label);
   }
   else
   {
    gchar buf[SIZEOF_COOKED];

    memcpy (buf, path, pl);
    memcpy (path, basedir, bl);
    memcpy (path + bl, buf, pl);
    path[pl + bl] = '\0';
   }
  }
  else
  {
   guint error;

   if (p[0] == '.')
   {
    /* Append "/" to "." or "..", if missing. */

    if(p[1] == '\0')
    {
     p[1] = G_DIR_SEPARATOR;
     p[2] = '\0';
    }
    else if (p[1] == '.' && p[2] == '\0')
    {
     p[2] = G_DIR_SEPARATOR;
     p[3] = '\0';
    }
   }
   else if (p[0] == '~' && p[1] == '\0')
   {
    /* Append "/" to "~", if missing. */
     p[1] = G_DIR_SEPARATOR;
     p[2] = '\0';
   }

   p = path_expand_full (path, gl_script_dir, &error);
   if (error)
   {
    result = RFAIL;
    entry_push_error (entry, result, "%s: expanded path too long", label);
   }
   else
   {
    if (p != NULL)
    {
     strcpy (path, p);
     free (p);
    }
   }
  }
 }
 return result;
}

/**
path_get_base_dir:
Return the base path.

@obuf: return buffer.
@filename: script file name or '-' or list of directives.
@use_cwd: TRUE to use the current working directory.
@entry: pointer to return error message.

Return: TRUE and copy the null-terminated base path ending with
'/' to @obuf; FALSE and set errno otherwise. If @use_cwd is TRUE,
return the current work directory for stdin and list of directives.
*/
gboolean
path_get_base_dir (gchar *obuf,
                   const gchar *filename,
                   gboolean use_cwd,
                   struct Entry *entry)
{
 gsize len;
 gchar abs_path[MAX_PATH_LEN + 1] = {0};
 const gsize max_size = MAX_PATH_LEN + 1;
 const gchar *p;

 if (filename[0] == '-' && filename[1] == '\0')
 {
  use_cwd = TRUE;
 }
 if (!use_cwd)
 {
  strncpy (abs_path, filename, sizeof(abs_path) - 1);
 }
 if ((p = strrchr(abs_path, G_DIR_SEPARATOR)) != NULL)
 {
  len = p - abs_path + 1;
  if (len < max_size - 1)
  {
   strncpy (obuf, abs_path, len);
   obuf[len] = '\0';
  }
  else
  {
   entry_push_error (entry, RFAIL, "base directory: %s",
                     strerror (errno = ENAMETOOLONG));
   return FALSE;
  }
 }
 if (use_cwd || p == NULL || strlen (obuf) == 2)
 {
  if (getcwd (obuf, max_size) == NULL)
  {
   return FALSE;
  }
  len = strlen (obuf);
  if (len < max_size - 1 && obuf[len - 1] != G_DIR_SEPARATOR)
  {
    strcat (obuf, G_DIR_SEPARATOR_S);
  }
 }
 entry_push_error (entry, RINFO, "using '%s' as base directory", obuf);
 return TRUE;
}

/**
path_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
path_module_init (void)
{
 for (guint i = 0; gl_path_expand_regexes[i].pattern != NULL; i++)
 {
  regex_t *regex;

  regex = malloc (sizeof (regex_t));
  if (regex == NULL
      || compile_regex (regex, gl_path_expand_regexes[i].pattern,
                        REG_EXTENDED) != 0)
  {
   return -1;
  }
  gl_path_expand_regexes[i].regex = regex;
 }
 return 0;
}

/**
input_module_finalize:
Clean up this module. Call before exit(2).
*/
void
path_module_finalize (void)
{
 for (guint i = 0; gl_path_expand_regexes[i].pattern != NULL; i++)
 {
  if (gl_path_expand_regexes[i].regex != NULL)
  {
   regfree (gl_path_expand_regexes[i].regex);
   free (gl_path_expand_regexes[i].regex);
  }
 }
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
