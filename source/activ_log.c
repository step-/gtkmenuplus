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

#include <regex.h>
#include "autoconfig.h"
#include "activ_log.h"
#include "activ_log.decl.h"
#include "entry.h"
#include "input.h"
#include "logmsg.h"
#include "main.h"

activation_log *activ_log = NULL;

/**
activation_log_fetch:
@path: new `activationlog=` file argument; NULLABLE.
@path must exist or be NULL; if not NULL; it is opened for writing.

Return: pointer to %activation_log structure owned by the
module: the current activation log if @path is NULL; a new
activation log otherwise, freeing the previous log if necessary.
Only one activation log can exist at the time.
*/
const activation_log *
activation_log_fetch (const gchar *path)
{
 activation_log *log = NULL;
 gchar *p = NULL;

 if (path == NULL)
 {
  return activ_log;
 }
 if (activ_log != NULL)
 {
   if (activation_log_module_finalize () != 0)
   {
    return NULL;
   }
 }
 if ((log = malloc (sizeof (activation_log))))
 {
  if ((p = malloc (SIZEOF_COOKED)))
  {
   gint r = snprintf (p, SIZEOF_COOKED, "%s", path);
   if (r > 0 && (gsize) r < SIZEOF_COOKED)
   {
    log->path = p;
    log->exclude = NULL;
    log->fd = -1;
   }
   else
   {
    free (p);
    free (log);
    log = NULL;
   }
  }
  else
  {
   free (log);
   log = NULL;
  }
 }
 return activ_log = log;
}

/**
activation_log_exclude:
Set/reset the exclusion pattern for the activation log.

@pattern: POSIX extended regular expression pattern string. NULLABLE.
Pass NULL to reset the pattern and disable the exclusion filter.

Return: 0(OK) -1(error)
Only one exclusion pattern can exist at the time.
*/
int
activation_log_exclude (const gchar *pattern)
{
 gsize len;

 if (activ_log->exclude != NULL)
 {
  free (activ_log->exclude);
  activ_log->exclude = NULL;
 }
 if (pattern == NULL || *pattern == '\0')
 {
  return 0;
 }
 len = strlen (pattern);
 if ((activ_log->exclude = malloc (len + 1)) == NULL)
 {
  return -1;
 }
 memcpy (activ_log->exclude, pattern, len);
 activ_log->exclude[len] = 0;
 return 0;
}

/**
entry_activationlog_write:
Add menu entry to the activation log file.

@entry: pointer to struct #Entry.
@overrides: pointer to struct #Entry holding possible overrides. NULLABLE.
The entry is NOT added if the activation log's exclusion pattern matches the
entry's command string.

Return: 0(OK) -1(error).

The goal of the activation log file to provide a file that can be included
by a main menu script to create a "Recently-used" submenu. Thus, the file is
formatted as a menu configuration file that contains just the Item, Cmd, Icon
(and Tooltip, if enabled) directives associated with the activated menu entry.

The activation log record concatenates a Meta head comment, followed
by the Item, Cmd, Icon (and Tooltip, only if built in) directives
with their values, and an EndMeta tail comment. Directive values are
variable- but not path-expanded. The Icon value is resolved to a text
icon resource (path, name/stock). Item and Tooltip style formatting
and Item auto-mnemonic formatting are not applied.

Meta :=
  '#{'
    <num_fields>
    ':' <conter_activated>
    ':' <time_created>
    ':' <time_activated>
  '}'
  [ <user_data> ]
  '\n'
EndMeta ::=
  '#{}'
  [ <user_data> ]
  '\n'
*/
int
activationlog_write_entry (struct Entry *entry)
{
 const gchar *exec, *name, *icon;
#ifdef FEATURE_TOOLTIP
 const gchar *comment;
#endif
 gchar *p, tmpf[MAX_PATH_LEN];
 int ret = 0, fd;
 FILE *copy = NULL, *heystack = NULL;

 if (activ_log == NULL)
 {
  return 0;
 }

 if (activ_log->exclude != NULL)
 {
  regex_t regex;
  int ret =
  regcomp (&regex, activ_log->exclude, REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
  if (ret != 0)
  {
   const gsize sz = regerror (ret, &regex, 0, 0);
   gchar *buf = malloc (sz);
   if (buf)
   {
    regerror (ret, &regex, buf, sz);
    entry_push_error (entry, RFAIL, "`activationlogexclude=`: %s", buf);
    free (buf);
   }
   return -1;
  }
  ret = regexec (&regex, entry->cmd, 0, 0, 0);
  regfree (&regex);
  if (ret == 0)
  {
   entry_push_error (entry, RINFO,
                     "`activationlogexclude=` pattern '%s' matched the command",
                     activ_log->exclude);
   return 0;
  }
 }

 snprintf (tmpf, sizeof (tmpf), "%s/.%sXXXXXX",
           getenv ("TMPDIR") ? getenv ("TMPDIR") : "/tmp", gl_progname);
 if (-1 == (fd = mkstemp (tmpf)))
 {
  entry_push_error (entry, RFAIL, "log mkstemp: %s", strerror (errno));
  return -1;
 }

 if (((copy = fdopen (fd, "w+")) == NULL)
     || ((heystack = fopen (activ_log->path, "r+")) == NULL))
 {
  entry_push_error (entry, RFAIL, "opening the activation: %s",
                    strerror (errno));
  ret = -1;
  goto out;
 }

 exec    = entry->cmd;
 name    = entry->label;
 icon    = entry->icon;
#ifdef FEATURE_TOOLTIP
 comment = entry->tooltip;
#endif

 /* Remember we are rewriting the menu widget as an `item=`
 directive. Therefore we map NULL to "" because `icon=NULL`
 would be interpreted as the icon name "NULL" otherwise. */
 if (*icon == ENTRY_DISALLOW_DIRECTIVE || *icon == ENTRY_NULL_ICON)
 {
  icon = "";
 }

 /*
 Append new or replace this log item (needle) to the log file (heystack).
 */
 gboolean found, error;
 gchar meta[MAX_PATH_LEN + 1];
 gchar buf[sizeof (meta) * 3/2];
 /* Meta's fields */
 int num_fields, cnt_activated, time_created, time_activated;
#define META_FMT "#{%u:%u:%u:%u}"
#define META_DIM 4
#define DATA_END "#{}"
#ifdef FEATURE_TOOLTIP
#define SZ   (sizeof("Item=\nCmd=\nIcon=\nTooltip=\n") + \
  member_size (struct Entry, label)                + \
  member_size (struct Entry, cmd)                  + \
  member_size (struct Entry, icon)                 + \
  member_size (struct Entry, tooltip))
 gchar needle[SZ];
 snprintf (needle, SZ, "Item=%s\nCmd=%s\nIcon=%s\nTooltip=%s\n",
           name, exec, icon, comment);
#else
#define SZ   (sizeof("Item=\nCmd=\nIcon=\n")           + \
  member_size (struct Entry, label)                + \
  member_size (struct Entry, cmd)                  + \
  member_size (struct Entry, icon))
 gchar needle[SZ];
 snprintf (needle, SZ, "Item=%s\nCmd=%s\nIcon=%s\n", name, exec, icon);
#endif
#undef SZ

 /*
 Is this log record already in the file?
 */

 /* Scan for line holding Meta and, optionally, <userdata>. */
 found = error = FALSE;
 if (isatty (fileno (heystack)))
 {
  ; /* Using a tty as activation log file is allowed. */
 }
 else while (fgets (meta, sizeof (meta), heystack) != NULL)
 {
  gint i, n;

  if (found)
  {
  /* Copy lines after a match, if any, is found. */
   fputs (meta, copy);
   continue;
  }

  /* Validate Meta line.  */
  num_fields = cnt_activated = time_created = time_activated = -1;
  if (sscanf (meta, META_FMT, &num_fields, &cnt_activated, &time_created,
               &time_activated) != META_DIM || num_fields < 0
      || cnt_activated < 0 || time_created < 0 || time_activated < 0)
  {
   error = TRUE;
   break;
  }
  /* Read the directives that are expected to come after Meta, */
  /* and read EndMeta and possibly <userdata>.                 */
  *(p = buf) = i = 0;
  while (1)
  {
    i++;
    p = fgets (p, sizeof (buf) - (ptrdiff_t) (p - buf), heystack);
    if (i > num_fields || p == NULL)
     break;
    p = strchr (p, '\0');
  }
  if (i != num_fields + 1 || p == NULL)
  {
   /* file too short or read error */
   error = TRUE;
   break;
  }
  n = 0;
  sscanf (p, DATA_END "%n", &n);
  if (n != sizeof (DATA_END) -1)
  {
   /* No EndMeta. */
   error = TRUE;
   break;
  }

  found = strncmp (buf, needle, (ptrdiff_t) (p - buf)) == 0;

  /* Write Meta. */
  if (found)
  {
   /* Update Meta's fields. */
   ++cnt_activated;
   time_activated = time (NULL);
   fprintf (copy, META_FMT, num_fields, cnt_activated, time_created,
            time_activated);

   p = strchr (meta, META_FMT[sizeof (META_FMT) - 2]) + 1;
   fputs (p, copy);         /* <userdata>? '\n'       */
  }
  else
  {
   /* Not found. Meta is unchanged. */
   fputs (meta, copy);
  }

  /* Write Directives and EndMeta. */
  fputs (buf, copy);       /* directives DATA_END <userdata>? */
 }

 if (found && error)
 {
  entry_push_error (entry, RFAIL, "while reading activation log %s: %s",
                    activ_log->path, strerror (errno = EINVAL));
  ret = -1;
  goto out;
 }
 else if (!found)
 {
  /* Write needle as a new menu entry block. */
  time_created = time_activated = time (NULL);
  fprintf (copy, (META_FMT "\n%s" DATA_END "\n"), META_DIM, 1, time_created,
           time_activated, needle);
 }
#undef META_FMT
#undef META_DIM
#undef DATA_END

 /* Copying instead of renaming the file supports a
 symlinked logfile located in an AUFS file system layer. */
 rewind (heystack);
 rewind (copy);
 while (fgets (buf, sizeof (buf), copy) != NULL)
  fputs (buf, heystack);

 ret = fclose (heystack);
 heystack = NULL;
 if (ret)
 {
  entry_push_error (entry, RFAIL, "log fclose: %s", strerror (errno));
 }

out:
 if (copy != NULL)
 {
  fclose (copy);
 }
 if (heystack != NULL)
 {
  fclose (heystack);
 }
 unlink (tmpf);

 return ret;
}

/**
activation_log_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
activation_log_module_init (void)
{
 return 0;
}

/**
activation_log_module_finalize:
Clean up this module. Call before exit(2).

Return: 0(OK) or -1(error) and sets errno.
*/
int
activation_log_module_finalize (void)
{
 int ret = 0;

 if (activ_log == NULL)
 {
  return 0;
 }
 if (activ_log->fd >= 0)
 {
  ret = close (activ_log->fd);
  if (ret)
  {
   if (logmsg_can_emit (RFAIL))
   {
    logmsg_emit (RFAIL, "`activationlogfile=` close", NULL);
   }
  }
 }
 if (activ_log->path != NULL)
 {
  free (activ_log->path);
 }
 if (activ_log->exclude != NULL)
 {
  free (activ_log->exclude);
 }
 free (activ_log);
 activ_log = NULL;
 return ret;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
