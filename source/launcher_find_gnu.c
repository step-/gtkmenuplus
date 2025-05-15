/*
Copyright (C) 2025 step https://github.com/step-/gtkmenuplus

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

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "conf.h"
#include "launcher_find_gnu.h"

#define MAX_SCAN_DEPTH MAX_MENU_DEPTH

static int gl_msd[MAX_SCAN_DEPTH] = {0};
static int gl_msd_top = -1;
static void (*gl_on_found) (const char *path,
                            struct lfpod *pod) = NULL;
static struct lfpod *gl_pod = NULL;

/**
scan:
Recursive file scanner (preorder traversal).

@dirpath: directory to scan (symlink allowed).
@fn: path handler; for its argument list refer to nftw(2).
    The handler function implements a Scanning Policy.
@max_depth: scan up to max_depth levels, with @dirpath at level 0.
    This implementation further limits max_depth < MAX_SCAN_DEPTH.

Return: -2(internal error), -3(invalid @max_depth), and
see nftw(3) with _GNU_SOURCE defined for other return values.
*/
static int
scan (const char *dirpath,
      int (*fn) (const char *fpath,
                 const struct stat *sb,
                 int typeflag,
                 struct FTW *ftwbuf),
      int max_depth)
{
 const int nopenfd = 20;
 const int flags = FTW_PHYS | /*GNU*/ FTW_ACTIONRETVAL;
 int ret;

 if (gl_msd_top >= MAX_SCAN_DEPTH)
 {
  return -2;
 }
 if (max_depth < 0 || max_depth > MAX_SCAN_DEPTH)
 {
  return -3;
 }

 gl_msd[++gl_msd_top] = max_depth;
 ret = nftw (dirpath, fn, nopenfd, flags);
 --gl_msd_top;
 return ret;
}

#if 0
__attribute__((unused))
static void
file_info (const char *fpath,
           const struct stat *sb,
           int typeflag,
           struct FTW *ftwbuf)
{
 printf ("%-3s %2d %7jd   %-40s %d %s\n",
         (typeflag == FTW_D) ? "d" : (typeflag == FTW_DNR) ? "dnr" :
         (typeflag == FTW_DP) ? "dp" : (typeflag == FTW_F) ?
         (S_ISBLK (sb->st_mode) ? "fb" :
          S_ISCHR (sb->st_mode) ? "fc" :
          S_ISFIFO (sb->st_mode) ? "fp" :
          S_ISREG (sb->st_mode) ? "fr" :
          S_ISSOCK (sb->st_mode) ? "fs" : "f?") :
         (typeflag == FTW_NS) ? "ns" : (typeflag == FTW_SL) ? "sl" :
         (typeflag == FTW_SLN) ? "sln" : "?",
         ftwbuf->level, (intmax_t) sb->st_size,
         fpath, ftwbuf->base, fpath + ftwbuf->base);
}

#endif
/**
launcher_policy:
Implement scan() policy to match gtkmenuplus launcher files.

Due to the fixed nftw() callback signature, scan policy functions
must merge their Specific Policy code with the Common Policy code.

Common Policy:
The scanner follows symbolic links to files or directories according
to the conf_get_followsymlink() return value. The scanner considers hidden
files and directories when conf_get_ignorehidden() returns zero.

Specific Policy (for gtkmenuplus launcher files):
If a pathname satisfies the Common policy, it is selected if:
1. It is a regular file and its name ends with ".desktop"; or
2. It is a symbolic link ultimately pointing to a file per #1.

Return: see nftw(3) with _GNU_SOURCE defined.
*/
static int
launcher_policy (const char *fpath,
                 const struct stat *sb,
                 int typeflag,
                 struct FTW *ftwbuf)
{
 const char *found = NULL;
 char *rpath = NULL;
 /* file_info (fpath, sb, typeflag, ftwbuf); */

 gl_pod->nvisited++;

 /* Common Policy */

 if (typeflag == FTW_D || typeflag == FTW_DP)
 {
  if (ftwbuf->level >= gl_msd[gl_msd_top])
  {
   return FTW_SKIP_SUBTREE; /* GNU */
  }
  return FTW_CONTINUE;
 }
 if (ftwbuf->level > gl_msd[gl_msd_top])
 {
  return FTW_CONTINUE;
 }
 if (fpath[ftwbuf->base] == '.')
 {
  if (conf_get_ignorehidden ())
  {
   return FTW_CONTINUE;
  }
 }
 if (typeflag == FTW_SL)
 {
  enum conf_followsymlink_flags follow = conf_get_followsymlink ();
  if (follow != CONF_FOLLOWSYMLINK_NONE)
  {
   rpath = realpath (fpath, NULL);
   if (rpath == NULL)
   {
    entry_push_error (gl_pod->entry, RWARN, "%s: %s", fpath, strerror (errno));
   }
   else
   {
    struct stat rb;
    if (stat (rpath, &rb) == 0)
    {
     if (S_ISREG (rb.st_mode) && (follow & CONF_FOLLOWSYMLINK_FILE))
     {
      found = rpath;
     }
     else if (S_ISDIR (rb.st_mode) && (follow & CONF_FOLLOWSYMLINK_DIR))
     {
      free (rpath);
      int ret, len = strlen (fpath);
      char *dpath = malloc ((len + 3) * sizeof (char));

      memcpy (dpath, fpath, len);
      dpath[len++] = '/';
      dpath[len++] = '.';
      dpath[len] = '\0';
      ret = scan (dpath, launcher_policy, gl_msd[gl_msd_top] - ftwbuf->level);
      free (dpath);
      return ret != FTW_STOP ? ret : FTW_CONTINUE;
     }
    }
   }
  }
 }
 else if (typeflag == FTW_F && S_ISREG (sb->st_mode))
 {
  found = fpath;
 }

 /* Specific Policy */

 if (found)
 {
  int len = strlen (found);
  if (len > (int) sizeof ".desktop" - 1
      && strcmp (found + len - sizeof ".desktop" + 1, ".desktop") == 0)
  {
   /* file_info (fpath, sb, typeflag, ftwbuf); */
   if (ftwbuf->level > gl_pod->peek_level)
   {
     gl_pod->peek_level = ftwbuf->level;
   }
   gl_pod->nfound++;
   gl_on_found (fpath, gl_pod);
  }
 }
 free (rpath);
 return FTW_CONTINUE;
}

/**
launcher_find:
(Recursively) find all the launcher files within a directory.

@dirpath: directory to scan -- recursively if @max_depth > 1.
@max_depth: >=0 limits the scan depth -- 0 means not to scan at all.
@on_found: output callback for each path that satisfies `launcher_policy()`.
@pod: user data pointer passed to @on_found.

Return: 0(success), non-zero(error) with errno set.

Output path format:
A symbolic link path to a directory is never replaced by the target or real
path. The scanner keeps appending path components to the initial stem. However,
when it follows a symbolic link to a directory, `launcher_policy()` appends
"/./" to the current path. In this case, the caller may want to normalize
returned paths by collapsing "/./" to "/".

Error handling:
Errors are pushed via the @pod's #Entry pointer.
*/
int
launcher_find (const char *dirpath,
               const unsigned max_depth,
               void (*on_found) (const char *path,
                                 struct lfpod *pod),
               struct lfpod *pod)
{
 int ret;

 if (on_found != NULL && pod != NULL)
 {
  gl_on_found = on_found;
  gl_pod = pod;
  gint len = strlen (dirpath);
  gchar *dpath = malloc ((len + 3) * sizeof (gchar));

  memcpy (dpath, dirpath, len + 1);
  if (len > 1 && dpath[len - 1] != '.' && dpath[len -2] != '/')
  {
   if (dpath[len - 1] != '/')
   {
    dpath[len++] = '/';
   }
   dpath[len++] = '.';
   dpath[len] = '\0';
  }
  ret = scan (dpath, launcher_policy, max_depth);
  free (dpath);
 }
 else
 {
  errno = EINVAL;
  ret = -1;
 }
 switch (ret)
 {
  case  0:
  case FTW_STOP:
   ret = 0;
   break;
  case -1:
   entry_push_error (pod->entry, RFAIL, "%s", strerror (errno));
   break;
  default:
   entry_push_error (pod->entry, RFAIL, "launcher scanner error %d", ret);
   break;
 }
 gl_on_found = NULL;
 return ret;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
