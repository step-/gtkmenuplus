/*
Copyright (C) 2016-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2

Forked from version 1.00, 2013-04-24, by Alan Campbell
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
#include "autoconfig.h"
#include "common.h"
#include "conf.h"
#include "logmsg.h"
#include "path.h"

void
die (const gchar *message)
{
 logmsg_emit (RFATAL, message, NULL);
 exit (EXIT_FAILURE);
}

/**
compile_regex:
*/
int
compile_regex (regex_t *r,
               const char *pattern,
               int cflags)
{
 int status = regcomp (r, pattern, cflags);
 if (status != 0)
 {
  char msg[1024];
  regerror (status, r, msg, sizeof msg);
  fprintf (stderr, "can't compile regex '%s': %s\n", pattern, msg);
  return status;
 }
 return 0;
}

#if defined(FEATURE_CONDITIONAL) || defined(FEATURE_VARIABLE)
/**
popen_read:
@label: label for error messages, usually the calling directive's name.
@expr: string holding the shell command that will output the condition's
intermediate value.
@value: byte array to hold popen's nul-terminated output.
@max_size: size of @value in bytes.
@err_msg: pointer to return a dynamically-allocated error message.
The caller owns the memory, which must be freed with g_free().
*/
enum Result
popen_read (const gchar *label,
            const gchar *expr,
            gchar *value,
            const guint max_size,
            gchar **err_msg)
{
 enum Result result = RFAIL;
 gchar *exprx, *end;
 gint status;
 FILE *fp;

 exprx = path_expand_full (expr, gl_script_dir, (guint *) &status);
 if (status)
 {
  *err_msg = g_strdup_printf ("path expansion failed in `%s`", label);
  goto out;
 }
 if (exprx != NULL)
 {
  expr = exprx;
 }
 /* synchronous */
 fp = popen (expr, "r");
 if (fp == NULL)
 {
  *err_msg = g_strdup_printf ("`%s` can't popen", label);
  goto out;
 }
 fread (value, 1, max_size, fp);
 status = pclose(fp);
 if (status < 0)
 {
  *err_msg = g_strdup_printf ("`%s` can't pclose", label);
  goto out;
 }
 if (WIFEXITED (status))
 {
  status = WEXITSTATUS (status);
  if (status)
  {
   *err_msg = g_strdup_printf ("`%s` exit status %d: %s", label, status, expr);
   goto out;
  }
 }
 else
 {
  if (WIFSIGNALED (status))
  {
   status = WTERMSIG (status);
  }
  else
  {
   status = SIGSTOP;
  }
  *err_msg = g_strdup_printf ("`%s` signaled -%d: %s", label, status, expr);
  goto out;
 }

 result = ROK;
 end = strchr (value, '\0') - 1;
 if (end[0] == '\n')
 {
  end[0] = '\0';
 }

out:
 if (exprx != NULL)
 {
  free (exprx);
 }
 return result;
}
#endif

/**
is_path_to_dir
Test whether path leads to a directory that can be
scanned according to the current configuration state.

@path: the supposed directory path.
@db: pointer to struct dirent corresponding to @path.
@rpath: pointer to C string returning
realpath(@path) if @path is a symlink. NULLABLE.
@quiet: whether to print an error message.

Return: TRUE/FALSE and newly-allocated *rpath. The caller owns the memory.
*/
gboolean
is_path_to_dir (const gchar *path,
                const struct dirent *de,
                gchar **rpath,
                gboolean quiet)
{
 gboolean is;
 struct stat sb;
 gchar *rp = NULL;

#ifdef _DIRENT_HAVE_D_TYPE
 is = ISDIR (de);
 if (!is && ISLNK (de)
     && (conf_get_followsymlink () & CONF_FOLLOWSYMLINK_DIR))
#else
 is = lstat (launcherpath, &sb) == 0 && ISDIR (&sb);
 if (!is && ISLNK (&sb)
     && (conf_get_followsymlink () & CONF_FOLLOWSYMLINK_DIR))
#endif
 {
  rp = realpath (path, NULL);
  if (rp == NULL)
  {
   if (!quiet && logmsg_can_emit (RWARN))
   {
    gchar *m = g_strdup_printf ("'%s': %s", path, strerror (errno));
    logmsg_emit (RWARN, m, NULL);
    g_free (m);
   }
  }
  else
  {
   is = stat (rp, &sb) == 0 && S_ISDIR (sb.st_mode);
  }
 }
 if (rpath)
 {
  *rpath = rp;
 }
 else
 {
  free (rp);
 }
 return is;
}

/**
is_path_to_reg
Test whether path leads to a regular file that can be
scanned according to the current configuration state.

@path: the supposed regular file path.
@db: pointer to struct dirent corresponding to @path.
@rpath: pointer to C string returning
realpath(@path) if @path is a symlink. NULLABLE.
@quiet: whether to print an error message.

Return: TRUE/FALSE and newly-allocated *rpath. The caller owns the memory.
*/
gboolean
is_path_to_reg (const gchar *path,
                const struct dirent *de,
                gchar **rpath,
                gboolean quiet)
{
 gboolean is;
 struct stat sb;
 gchar *rp = NULL;

#ifdef _DIRENT_HAVE_D_TYPE
 is = ISREG (de);
 if (!is && ISLNK (de)
     && (conf_get_followsymlink () & CONF_FOLLOWSYMLINK_FILE))
#else
 is = lstat (launcherpath, &sb) == 0 && ISREG (&sb);
 if (!is && ISLNK (&sb)
     && (conf_get_followsymlink () & CONF_FOLLOWSYMLINK_FILE))
#endif
 {
  rp = realpath (path, NULL);
  if (rp == NULL)
  {
   if (!quiet && logmsg_can_emit (RWARN))
   {
    gchar *m = g_strdup_printf ("'%s': %s", path, strerror (errno));
    logmsg_emit (RWARN, m, NULL);
    g_free (m);
   }
  }
  else
  {
   is = stat (rp, &sb) == 0 && S_ISREG (sb.st_mode);
  }
 }
 if (rpath)
 {
  *rpath = rp;
 }
 else
 {
  free (rp);
 }
 return is;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
