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
#include <fnmatch.h>
#include "autoconfig.h"
#include "conf.h"
#include "core.h"
#include "directive.h"
#include "entry.h"
#include "entry_icon.h"
#include "input.h"
#include "launcher.h"
#include "launcher_find_gnu.h"
#include "launcher.decl.h"
#include "launcher_cache.h"
#include "logmsg.h"
#include "path.h"

struct la2de
{
 gchar label[SIZEOF_COOKED];
 struct dirent *de;
};

struct DirFile
{
 gchar  path[MAX_PATH_LEN + 1];
 struct Entry entry;
};

static const gchar*  gl_pat_launcher_exec_data =
"[[:space:]]+%[fFuUdDnNickvm][[:space:]]*";
static regex_t gl_rex_launcher_exec_data;
static struct DirFile gl_launcher_dirfile[MAX_MENU_DEPTH];
static guint gl_launcher_ctr = 0; /* displayed .desktop files */
static gchar gl_launcher_args[MAX_PATH_LEN + 1];
static gchar gl_launcher_dir[MAX_PATH_LEN + 1];
static GPtrArray *gl_launchersubmenu_stack = NULL;
static struct LauncherProperty gl_launcher_prop[] =
{
 /*keyword       value  try_i18n required */
 {"Categories",  NULL,  FALSE,   FALSE},
 {"Name",        NULL,  TRUE,    TRUE},
 {"Exec",        NULL,  FALSE,   TRUE},
 {"Comment",     NULL,  TRUE,    FALSE},
 {"Icon",        NULL,  FALSE,   FALSE},
 {"Type",        NULL,  FALSE,   FALSE},
 {"NoDisplay",   NULL,  FALSE,   FALSE},
 {0},
};

/**
launcher_prop_free_values:
g_free the .value members of gl_launcher_prop.
*/
static void
launcher_prop_free_values ()
{
 for (guint i = 0; gl_launcher_prop[i].property; i++)
 {
  if (gl_launcher_prop[i].value != NULL)
  {
   g_free (gl_launcher_prop[i].value);
   gl_launcher_prop[i].value = NULL;
  }
 }
}

/**
launcher_exec_data_whiteout_code:
Whiteout the first field code, e.g. %F, %u, etc., of a "Exec=" line[1].

@key: DATA string in "Exec="DATA to be modified.

[1] https://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
*/
static void
launcher_exec_data_whiteout_code (gchar *data)
{
 regmatch_t pmatch[2];

 if (regexec(&gl_rex_launcher_exec_data, data, 1, pmatch, 0) == 0)
 {
  gchar *p;
  for(p = data + pmatch[0].rm_so; p < data + pmatch[0].rm_eo; p++)
    *p = ' ';
 }
}

/**
launcher_copy_properties:
Copy launcher property values between two menu entries.

@dst: pointer to SIZEOF_COOKED-sized destination buffer.
@src: pointer to SIZEOF_COOKED-sized source buffer.
*/
static void
launcher_copy_properties (struct Entry *dst,
                          const struct Entry *src)
{
 strcpy (dst->label, src->label);
 strcpy (dst->icon, src->icon);
 strcpy (dst->cmd, src->cmd);
#ifdef FEATURE_TOOLTIP
 strcpy (dst->tooltip, src->tooltip);
#endif
 strcpy (dst->categories, src->categories);
 dst->launcher_type = src->launcher_type;
 dst->no_display = src->no_display;
}

/**
launcher_entry_fill_cached:
Initialize a struct #Entry from the cache. If the cache is empty,
read and cache the launcher properties from a launcher file.

@filepath: launcher pathname and also the cached entry key.
@entry: pointer to the #Entry to initialize.
@label: pass a string to override the {Name=} property. Nullable.
@required: if TRUE report a warning on each launcher property that
is required by gl_launcher_prop[] but missing in the launcher file.
@callerid: the directive that is calling this function.

Side effects: allocate gl_launcher_prop[i].value.

Returns: enum Result.
*/
static enum Result
launcher_entry_fill_cached (const gchar *filepath,
                            struct Entry *entry,
                            const gchar *label,
                            const gboolean required,
                            const enum LineType callerid)
{
 struct Entry *cached = launcher_cache_find_entry (filepath);
 if (cached == NULL)
 {
  return launcher_entry_cache_props (filepath, entry, label, required);
 }
 else if (cached->launcher_file_is_invalid)
 {
  return RWARN;
 }
 else
 {
  launcher_copy_properties (entry, cached);

  /* Set the remaining struct members that aren't linked to .desktop props. */
  entry->menu_depth = gl_menu_depth; /* see also {N1} */
  entry->leave = NULL;
  if (entry->directive != NULL && entry->directive->type != callerid)
  {
   entry->directive = input_fetch_directive_from_linetype (callerid);
  }
 }
 return ROK;
}

/**
launcher_entry_cache_props:
Initialize a struct #Entry from a launcher file.

@entry: pointer to the #Entry to initialize.
@label: pass a string to override the {Name=} property. Nullable.
@required: if TRUE report a warning on each launcher property that
is required by gl_launcher_prop[] but missing in the launcher file.
*/
static enum Result
launcher_entry_cache_props (const gchar *filepath,
                            struct Entry *entry,
                            const gchar *label,
                            const gboolean required)
{
 enum Result result = ROK;
 gchar *value = NULL;

 /* Parse .desktop or .directory file path. */
 launcher_prop_free_values ();
 GKeyFile* key_file = g_key_file_new ();
 GError* gerror = NULL;
 if (!g_key_file_load_from_file (key_file, filepath, 0, &gerror))
 {
  g_key_file_free (key_file);
  entry->launcher_file_is_invalid = TRUE;
  entry->launcher_type = LAUNCHER_TYPE_UNKNOWN;
  entry_push_error (entry, RFAIL, "'%s': %s", filepath, gerror->code ==
                    G_KEY_FILE_ERROR_PARSE ? "invalid key-value syntax" :
                    gerror->code == G_KEY_FILE_ERROR_UNKNOWN_ENCODING ?
                    "invalid UTF-8 key-value" : gerror->message);
  g_error_free (gerror);
  goto copy_and_cache;
 }

 for (enum LauncherPropertyId i = 0; gl_launcher_prop[i].property; i++)
 {
  /*
  If key can't be found then NULL is returned, and gerror is set to
  G_KEY_FILE_ERROR_KEY_NOT_FOUND. If the keyed value can't be interpreted
  or a translation can't be found, then the untranslated value is returned.
  */
  value = NULL;
  gerror = NULL;
  if (gl_launcher_prop[i].try_i18n)
  {
   value = g_key_file_get_locale_string (key_file, "Desktop Entry",
                                         gl_launcher_prop[i].property,
                                         NULL, NULL);
  }
  if (value == NULL)
  {
   gerror = NULL;
   value = g_key_file_get_string (key_file, "Desktop Entry",
                                  gl_launcher_prop[i].property, NULL);
  }
  /* launcher_element_free_values () will free .value */
  gl_launcher_prop[i].value = value;
  if (i == LAUNCHER_PROP_TYPE)
  {
   value = gl_launcher_prop[LAUNCHER_PROP_TYPE].value;
   if (value == NULL)
   {
    entry->launcher_type = LAUNCHER_TYPE_UNKNOWN;
   }
   else if (strcmp (value, "Application") == 0)
   {
    entry->launcher_type = LAUNCHER_TYPE_APPLICATION;
   }
   else if (strcmp (value, "Directory") == 0)
   {
    entry->launcher_type = LAUNCHER_TYPE_DIRECTORY;
   }
   else if (strcmp (value, "Link") == 0)
   {
    entry->launcher_type = LAUNCHER_TYPE_LINK;
   }
   else
   {
    entry->launcher_type = LAUNCHER_TYPE_UNKNOWN;
   }
  }
 }
 g_key_file_free (key_file);
 if (entry->launcher_type == LAUNCHER_TYPE_APPLICATION)
 {
  if (required)
  {
   const struct LauncherProperty *pr;
   for (pr = gl_launcher_prop; pr->property; pr++)
   {
    if (label && strcmp (pr->property,
                         gl_launcher_prop[LAUNCHER_PROP_NAME].property) == 0)
    {
     continue;
    }
    if (pr->required && pr->value == NULL)
    {
     result = RWARN;
     entry_push_error (entry, result, "Required key `%s=` not found in '%s'",
                       pr->property, filepath);
    }
   }
  }
  if (result >= RFAIL)
  {
   goto out;
  }
 }
 else if (entry->launcher_type == LAUNCHER_TYPE_UNKNOWN)
 {
  result = RWARN;
  entry_push_error (entry, result, "%s: invalid .desktop file: ?'Type='?",
                    filepath);
  goto out;
 }

#define STRCPY(A, B)   \
  if ((B) != NULL) {   \
    strcpy((A), (B));  \
  } else {             \
    *(A) = '\0';       \
  }

copy_and_cache:
 STRCPY (entry->label, gl_launcher_prop[LAUNCHER_PROP_NAME].value ?
         gl_launcher_prop[LAUNCHER_PROP_NAME].value : label);
 STRCPY (entry->cmd, gl_launcher_prop[LAUNCHER_PROP_EXEC].value);
 STRCPY (entry->icon, gl_launcher_prop[LAUNCHER_PROP_ICON].value);
#ifdef FEATURE_TOOLTIP
 STRCPY (entry->tooltip, gl_launcher_prop[LAUNCHER_PROP_COMMENT].value);
#endif
 STRCPY (entry->categories, gl_launcher_prop[LAUNCHER_PROP_CATEGORY].value);
 value = gl_launcher_prop[LAUNCHER_PROP_NODISPLAY].value;
 entry->no_display = value != NULL ? strcmp ("true", value) == 0 : FALSE;
#undef STRCPY

 launcher_cache_add_entry (filepath, entry);

out:
 return result;
}

/**
launcher_submenu_fill_entry:
Initialize the base entry of a launcher submenu.

@dirpath: `launchersub=` directive argument.
@entry: pointer to struct #Entry.
@callerid: the directive that is calling this function.

Side effects: cache the entry of the super dirfile, if the file exists.

Returns: enum #Result.
*/
static enum Result
launcher_submenu_fill_entry (const gchar *dirpath,
                             struct Entry *entry)
{
 assert (entry->directive->type == LINE_LAUNCHER_SUB);

 /* If `launcherdirfile[index]=` is active. */
 if (gl_launcher_dirfile[entry->menu_depth].path[0])
 {
  /* `launchersub=` renders directories as automatic submenu entries. By
  default the entry label is the directory name and the icon is empty
  unless a launcher_dirfile provides a non-empty property. */
  launcher_copy_properties (entry,
                            &gl_launcher_dirfile[entry->menu_depth].entry);
 }

 /* Fall back to default values. */
 if (entry->label[0] == '\0')
 {
  /* Replace an empty {Name=} property with the folder name. */
  gchar *p = strrchr (dirpath, G_DIR_SEPARATOR);
  strcpy (entry->label, p ? p+1 : dirpath);
 }

 return ROK;
}

/**
map_token_list:
Map a function of #Entry over a list of string tokens.

@func: function taking struct #Entry* and returning enum #Result.
@return_on_first_ok: TRUE to return on the first @func that returns #ROK.
The default value (FALSE) maps @func over all input tokens.
@delim: string containing the list-element delimiters (see strtok_r(3)).
@list: string containing the tokens delimited by a byte in @delim.
Modified per the first argument to strtok_r().
@entry: pointer to the initial #Entry to
return errors. Other struct members will be changed!

Return: the maximum over the #Result value array obtained from mapping
@func until the stop condition defined by @return_on_first_ok occurs.
*/
static enum Result
map_token_list (const fnEntry func,
                const gboolean return_on_first_ok,
                const gchar *delim,
                const gchar *list,
                struct Entry *entry)
{
 gchar *token, *tokens = NULL, *as;
 enum Result max_result = RFAIL, result;
 struct Entry *me;
 /* @entry->_dat returns the value that me->_dat has upon returning
 from the last @func call. This could be the token passed to @func. */

 me = entry_new ();
 if (me == NULL)
 {
  perror ("malloc");
  goto out;
 }
 tokens = strdup (list);
 if (tokens == NULL)
 {
  perror ("strdup");
  goto out;
 }
 /*
 Mapping step plan:
 - Copy *@entry to new Entry me.
 - Extract the next token from @list.
 - Overwrite me->_dat to pass the input/output token to func().
 - Call @func(me); collect the return value and any errors.
 */
 max_result = ROK;
 token = strtok_r (tokens, delim, &as);
 while (token != NULL)
 {
  memcpy (me, entry, sizeof (*me));
  strcpy (me->_dat, token);
  result = func (me);
  entry->error = me->error;
  me->error = NULL; /* disown */
  if (result <= ROK)
  {
   if (return_on_first_ok)
   {
    break;
   }
  }
  if (result > max_result)
  {
   max_result = result;
  }
  token = strtok_r (NULL, delim, &as);
 }
out:
 free (me);
 free (tokens);

 return max_result;
}

/**
*/
enum Result
a_launcher (struct Entry *entry)
{
 const gchar *caller = entry->directive->keyword;
 gchar *_dat = entry->_dat;

 /*
 launcher=file
 launcher=dir
 */
 /* Hidden and symbolic link path restrictions don't
 apply to a scanning directive's argument. */
 if (strchr (_dat, ':') != NULL)
 {
  /* If configure=launcherlistfirst is enabled the first hit suffices. */
  return map_token_list (a_launcher, conf_get_launcherlistfirst (), ":", _dat,
                         entry);
 }
 if (*gl_launcher_dir && strlen (_dat) == 1 && (*_dat == '*' || *_dat == '.'))
 {
  /* Special case for gl_launcher_dir only */
  /* (make_rooted_dirpath already checked buffer size). */
  strcpy (_dat, gl_launcher_dir);
 }
 else
 {
  enum Result result = path_expand (_dat, gl_launcher_dir, caller, entry);
  if (result != ROK)
  {
   return result;
  }
 }
 return launcher_scan (entry, caller, LINE_LAUNCHER);
}

/**
*/
enum Result
a_launchersub (struct Entry *entry)
{
 enum Result result;
 const gchar *caller = entry->directive->keyword;
 gchar *_dat = entry->_dat;

 /*
 launchersub=dir
 */
 if (strchr (_dat, ':') != NULL)
 {
  /* If configure=launcherlistfirst is enabled the first hit suffices. */
  return map_token_list (a_launchersub, conf_get_launcherlistfirst (),
                         ":", _dat, entry);
 }
 result = path_expand (_dat, gl_launcher_dir, caller, entry);
 if (result < RFAIL)
 {
  struct stat sb;

  /* Hidden path and symbolic link path restrictions don't
  apply to a scanning directive's argument. */
  if (stat (_dat, &sb) != 0) /* stat follows symlinks */
  {
   entry_push_error (entry, RFAIL, "`%s=` %s: %s", caller, _dat,
                     strerror (errno));
   return RFAIL;
  }
  if (S_ISDIR (sb.st_mode))
  {
   result = launcher_scan (entry, caller, entry->directive->type);
  }
 }
 return result;
}

/**
launcher_scan_filter:
scandir(3) filter for `launcher` and `launchersub`.

Exclude "." ".." and (if configured) hidden files/directories.
Exclude *".list" *".cache" commonly found in for $XDG_DATA_DIRS/applications.

Return: 1 to include the file, 0 otherwise.
*/
static gint
launcher_scan_filter (const struct dirent *d)
{
 const gchar *p = d->d_name;

 if ((*p == '.' && (!p[1] || (p[1] == '.' && !p[2])))
     || (*p == '.' && conf_get_ignorehidden ())
     || fnmatch ("*.list", d->d_name, 0) == 0
     || fnmatch ("*.cache", d->d_name, 0) == 0)
 {
  return 0; /* exclude */
 }
 return 1;
}

/**
check_launcher_path_ext:
Check if path ends with ".desktop" or ".directory"
*/
static gboolean
check_launcher_path_ext (const gchar *path)
{
 const gchar *p = strchr (path, '\0');
 gsize len = (ptrdiff_t) (p - path);
 return
  (len > 8 && p[-8] == '.' && strcmp (p - 7, "desktop") == 0) ||
  (len > 10 && p[-10] == '.' && strcmp (p - 9, "directory") == 0);
}

/**
build_launcher_path_from_dirent:
Validate and build the full pathname of the launcher file described by dirent.

@e: pointer to struct dirent.
@dirpath: directory location of @e.
@path: pointer to a SIZEOF_COOKED-sized buffer to return the filepath.
@entry: pointer to struct #Entry to return error message.

Return: TRUE and the launcher file path in @path, which can be:
(1) a regular file path that ends with ".desktop" or ".directory".
(2) a symlink whose final target name ends with ".desktop" or ".directory".
Return FALSE and set @path to the empty string otherwise.
*/
static gboolean
build_launcher_path_from_dirent (const struct dirent *e,
                                 const gchar *dirpath,
                                 gchar *path,
                                 struct Entry *entry)
{
 gboolean has_ext = FALSE, can_fit = FALSE, is_dir = FALSE;
 gchar target[PATH_MAX];

 errno = 0;
#ifdef _DIRENT_HAVE_D_TYPE
 is_dir = ISDIR (e);
 if (!is_dir)
 {
  gint r;

  if (ISREG (e))
  {
   has_ext = check_launcher_path_ext (e->d_name);
   if (has_ext)
   {
    r = snprintf (path, SIZEOF_COOKED, "%s%s", dirpath, e->d_name);
    can_fit = r > 0 && (gsize) r < SIZEOF_COOKED;
   }
  }
  else if (ISLNK (e))
  {
   r = snprintf (path, SIZEOF_COOKED, "%s%s", dirpath, e->d_name);
   can_fit = r > 0 && (gsize) r < SIZEOF_COOKED;
   if (can_fit)
   {
    if (realpath (path, target) != NULL)
    {
     errno = 0;
     has_ext = check_launcher_path_ext (target);
    }
   }
  }
 }

#else
 struct stat statbuf;
 can_fit =
 snprintf (path, SIZEOF_COOKED, "%s%s", dirpath, e->d_name) >= SIZEOF_COOKED;
 if (can_fit)
 {
  if (lstat (path, &statbuf) == 0)
  {
   errno = 0;
   is_dir = ISDIR (&statbuf);
   if (!is_dir)
   {
    if (ISREG (&statbuf))
    {
     has_ext = check_launcher_path_ext (e->d_name);
    }
    else if (ISLNK (&statbuf))
    {
     if (realpath (path, target) != NULL)
     {
      errno = 0;
      has_ext = check_launcher_path_ext (target);
     }
    }
   }
  }
 }

#endif
 if (!is_dir)
 {
  if (!errno)
  {
   if (has_ext && can_fit)
   {
    return TRUE;
   }
   if (!has_ext)
   {
#if 0 /* too noisy for regular use */
    if (logmsg_can_emit (RDEBUG))
    {
     fprintf (stderr, "%s%s: %s\n", dirpath, e->d_name,
              "skipped: no \".desktop\" or \".directory\" file extension");
    }
#endif
   }
   else
   {
    errno = ENAMETOOLONG;
   }
  }
  if (errno)
  {
   entry_push_error (entry, RFAIL, "'%s%s': %s", dirpath, e->d_name,
                     strerror (errno));
  }
 }
 path[0] = '\0';
 return FALSE;
}

/**
launcher_label_cmp:
Qsort comparator callback.
*/
static int
launcher_label_cmp (const void *a,
                    const void *b)
{
 const struct la2de *sa = *(struct la2de **) a;
 const struct la2de *sb = *(struct la2de **) b;
 return strcoll (sa->label, sb->label);
}

/**
launcher_cache_namelist_sorted:
Cache the launcher properties of a struct dirent namelist of supposed
launchers files; return the alnum-sorted namelist.

@namelist: array of pointers to struct dirent.
@length: of @namelist.
@dirpath: path prefix leading to the files in @namelist.
@entry: pointer to struct #Entry to return error message.

Return: TRUE and @namelist sorted; FALSE and @namelist unchanged on
error. The output list includes all input files. The list is sorted
with strcoll(3) by the text that will become the menu entry label.
*/
static gboolean
launcher_cache_namelist_sorted (struct dirent **namelist,
                                const gint length,
                                const gchar *dirpath,
                                const enum LineType callerid,
                                struct Entry *entry)
{
 gchar path[SIZEOF_COOKED];
 static struct Entry *e = NULL;
 struct la2de **a = NULL;
 gboolean ret = FALSE;

 *path = '\0';
 if (e == NULL)
 {
  if ((e = entry_new_tracked ()) == NULL)
  {
   goto out;
  }
 }
 if ((a = calloc (sizeof (struct la2de *), length)) == NULL)
 {
  goto out;
 }
 e->error = entry->error;
 for (gint i = 0; i < length; i++)
 {
  if ((a[i] = malloc (sizeof (struct la2de))) == NULL)
  {
   goto out;
  }
  a[i]->de = namelist[i];
  ret = TRUE;
  if (build_launcher_path_from_dirent (namelist[i], dirpath, path, e)
      && (ret = launcher_entry_fill_cached (path, e, namelist[i]->d_name,
                                            TRUE, callerid) < RFAIL))
  {
   strcpy (a[i]->label, e->label);
  }
  else
  {
   /* directory, non-launcher file; invalid launcher file */
   strcpy (a[i]->label, namelist[i]->d_name);
  }
 }
 /* entry owns the error list */
 entry->error = e->error;
 e->error = NULL;

 qsort (a, length, sizeof (struct la2de *), launcher_label_cmp);
 for (gint i = 0; i < length; i++)
 {
  namelist[i] = a[i]->de;
 }

out:
 if (e == NULL || a == NULL)
 {
  perror (__FUNCTION__);
  return ret;
 }
 if (a != NULL)
 {
  for (gint i = 0; i < length && a[i] != NULL; i++)
  {
   free (a[i]);
  }
  free (a);
 }
 return ret;
}

/**
squeeze_slash_dot:
Replace all matches of regex (/\.)+/ with a single '/'.
To replace in-place pass @src == @dst.
@src: source pointer
@dst: destination pointer -- with size of destination buffer > strlen (@src).
*/
inline static void
squeeze_slash_dot (const gchar *src,
                   gchar *dst)
{
 while (*src)
 {
  if (*src == '/')
  {
   const gchar *p = src;

   guint count = 0;
   while (*p == '/' && *(p + 1) == '.')
   {
    p += 2;
    count++;
   }
   if (count > 0)
   {
    if (*p == '/')
    {
     *dst++ = '/';
     src = p + 1;
     continue;
    }
    else if (count > 1) /* can backtrack */
    {
     p -= 2;
     *dst++ = '/';
     src = p + 1;
     continue;
    }
   }
  }
  /* no match at *src */
  *dst++ = *src++;
 }
 *dst = '\0';
}

/**
launcher_find_cb:
*/
static void
launcher_find_cb (const gchar *path,
                  struct lfpod *pod)
{
 gchar *normalized_path = NULL;
 if (strstr (path, "/./") != NULL)
 {
  gchar *dst = normalized_path = malloc (strlen (path) + 1);
  squeeze_slash_dot (path, dst);
  path = normalized_path;
 }
 if (launcher_cache_find_entry (path) == NULL)
 {
  if (launcher_entry_cache_props (path, pod->entry, NULL, TRUE) == ROK)
  {
   ++pod->ncached;
  }
 }
 free (normalized_path);
}

/**
launcher_scan:
Scan a directory for launchers. Optionally recurse into subdirectories.

@entry: pointer to struct #Entry. We start scanning at entry->_dat.
@caller: label for error messages (conventionally the caller's name).
@callerid: caller's directive. We recurse if entry->callerid==LINE_LAUNCHER_SUB.

Returns: enum #Result.
*/
static enum Result
launcher_scan (struct Entry *entry,
               const gchar *caller,
               const enum LineType callerid)
{
 struct stat statbuf;
 gchar *_dat = entry->_dat;

 assert (callerid & (LINE_LAUNCHER | LINE_LAUNCHER_SUB));

 /*
 launcher=file
 launcher=dir
 launchersub=file (UNDOCUMENTED)
 launchersub=dir
 */
 /* Hidden and symbolic link path restrictions (ignorehidden
 and followsymlink) don't apply to a argument of a
 scanning directive. They only apply within the scan. */
 if (stat (_dat, &statbuf) == -1) /* stat follows symlinks */
 {
  enum Result result = errno == ENOENT ? RWARN : RFAIL;
  entry_push_error (entry, result, "'%s': %s", _dat, strerror (errno));
  return result;
 }
 if (S_ISREG (statbuf.st_mode))
 {
  return launcher_app (_dat, entry, callerid);
 }
 /* Enter directory unless max menu depth exceeded. */
 if (gl_menu_depth > MAX_MENU_DEPTH)
 {
  return RWARN; /* silently */
 }

 /*
 Scan directory -- recursive when callerid is LINE_LAUNCHER_SUB.
 */
 guint len0;
 guint cache_signature;
 gint n;
 struct dirent **namelist;

 len0 = strlen (_dat);
 if (len0 >= SIZEOF_COOKED - 2) /* 2 for G_DIR_SEPARATOR and at least 1 char */
 {
  entry_push_error (entry, RFAIL, "%s", "line too long to append '/'");
  return RFAIL;
 }
 if (_dat[len0 - 1] != G_DIR_SEPARATOR)
 {
  _dat[len0] = G_DIR_SEPARATOR;
  _dat[++len0] = '\0';
 }

 n = -1;
 cache_signature = (conf_get_followsymlink () << 1) | conf_get_ignorehidden();
 struct Scandir *cached = launcher_cache_find_scandir (_dat, cache_signature);
 if (cached != NULL)
 {
  n = cached->n; /* >= 0 */
  namelist = cached->namelist;
 }
 else
 {
  if (n < 0)
  {
   n = scandir (_dat, &namelist, launcher_scan_filter, NULL);
  }
  if (n < 0)
  {
   entry_push_error (entry, RFAIL, "`%s`=: %s: '%s'", caller,
                     strerror (errno), _dat);
   return RFAIL;
  }
  else
  {
   (void) launcher_cache_namelist_sorted (namelist, n, _dat, callerid, entry);
  }
  if (cached == NULL)
  {
   struct Scandir sd;
   sd.namelist = namelist; /* steal namelist for the cache */
   sd.n = n;
   launcher_cache_add_scandir (_dat, &sd, cache_signature);
  }
 }

 /* Case {N1}: we can arrive here from (A) a top-level `launcher`/`launchersub`
 or (B) a `launcher`/`launchersub` that is wrapped in a `submenu` or
 launchersubmenu`. In case (B), entry->menu_depth must be made equal to
 the global gl_menu_depth (which was incremented when the submenu started).
 Case (A) is NOP because entry->menu_depth == gl_menu_depth. */
 entry->menu_depth = gl_menu_depth;

 for (gint i = 0; i < n; i++)
 {
  /* Break out the loop with 'goto break_this_loop'. */

  enum Result result;
  gint len1, r;
  gchar curr_path[SIZEOF_COOKED];

  r = snprintf (curr_path, sizeof curr_path, "%s%s", _dat,
                namelist[i]->d_name);
  if (r < 0 || (gsize) r >= sizeof curr_path)
  {
   /* Give up on this launcher. */
   entry_push_error (entry, RWARN, "path truncated: %s\n", curr_path);
   continue;
  }
#ifdef DEBUG_PRINT_INPUT
  if (logmsg_can_emit (RDEBUG))
  {
   fprintf (stderr, "    > %s\n", curr_path);
  }
#endif
  len1 = len0 + strlen (namelist[i]->d_name);

  gboolean enter_dir = FALSE;
  if (callerid == LINE_LAUNCHER_SUB)
  {
   enter_dir = is_path_to_dir (curr_path, namelist[i], NULL, TRUE);
  }
  if (enter_dir)
  {
   /* Recursive: tree walk dirpath or symlink-to-directory path. */

   /***********************
   *  LINE_LAUNCHER_SUB  *
   ***********************/

   gchar saved_path[SIZEOF_COOKED];
   strcpy (saved_path, entry->_dat);

   /* -----
   Count and cache the valid .desktop files in or under directory curr_path.
   ----- */

   struct lfpod pod = {0};
   pod.entry = entry;
   gint maxdepth = MAX_MENU_DEPTH - gl_menu_depth - 1;
   if (maxdepth > 0)
   {
    gchar *dpath = g_strconcat (curr_path, strchr (curr_path, '\0')[-1] == '/'
                                ? "." : "/.", NULL);
    ErrorList *stop = pod.entry->error;
    if (launcher_find (dpath, maxdepth, launcher_find_cb, &pod) == 0)
    {
     result = entry_get_max_error_level (pod.entry, ROK, stop);
#if 0
     g_printerr ("\033[34;7m%s: pre-scan %s: maxdepth=%u visited=%'u found=%'u"
                 " cached=%'u peek_level=%u errors=%u\033[0m\n",
                 __FUNCTION__, curr_path,
                 maxdepth, pod.nvisited, pod.nfound,
                 pod.ncached, pod.peek_level,
                 g_slist_length (pod.entry->error));
#endif
    }
    g_free (dpath);
   }
   if (pod.nfound == 0)
   {
    /* next LINE_LAUNCHER_SUB item */
    continue;
   }

   /* -----
   Found some .desktop files in or under directory curr_path.
   ----- */

   /* KLUDGE { */
   /* Emulate the input loop reading `submenu=`basename(launchepath). */

   strcpy (entry->_dat, curr_path + len0);

   /* If enter_submenu_internal succeeds, it posts a
   leave_submenu callback, and increments gl_menu_depth. */
   result = enter_submenu_internal (entry, 0);
   if (result > ROK)
   {
    entry_prefix_error (entry, "at '%s': ", curr_path);
    strcpy (entry->_dat, saved_path);
    /* Go on to the next sibling processing files but not directories. */
    continue;
   }

   result = launcher_submenu_fill_entry (curr_path, entry);
   if (result > ROK)
   {
    /* Can carry on because *entry is preset with fallback values.
    Accumulated errors are preserved and will bubble up. */
   }

   /* Commit the emulated submenu just like input_fetch_next_directive
   automatically commits an actual `submenu=`. */
   assert (entry->leave == leave_submenu);
   result = leave_submenu (entry);
   if (result < RFAIL)
   {
    entry->menu_depth++;

    /*
    Emulate a recursive `launchersub=`.
    */
    enum Result scan_result;
    gboolean saved_endsubmenu;

    strcpy (entry->_dat, curr_path);
    /* Errors will accumulate and bubble up. */
    scan_result = launcher_scan (entry, caller, callerid);

    /* Temporarily ensure `configure= endsubmenu` is enabled to avoid
    spurious errors that would break the logic of the current loop.  */
    saved_endsubmenu = conf_get_endsubmenu ();
    conf_endsubmenu (NULL, TRUE);

    /* Successful a_endsubmenu sets entry->menu_depth = --gl_menu_depth. */
    result = a_endsubmenu (entry);
    if G_LIKELY (result < RFAIL)
    {
     result = scan_result != ROK ? scan_result : result;
     conf_endsubmenu (NULL, saved_endsubmenu);
    }
    else
    {
     strcpy (_dat, saved_path);
     entry_prefix_error (entry, "'%s': ", curr_path);
    }
   }
   /* } KLUDGE */

   if (result >= RFAIL)
   {
    goto break_this_loop;
   }
   strcpy (entry->_dat, saved_path);
   {
    /* next LINE_LAUNCHER_SUB item */
    continue;
   }
  }
  else
  {
   /* Non-directory path. */

   /****************************************
   *  LINE_LAUNCHER or LINE_LAUNCHER_SUB  *
   ****************************************/
   gboolean ends_with_desktop;

   if (strcmp (curr_path + len1 - 8, ".desktop") != 0)
   {
    /*
    Skip file names not ending with ".desktop" unless the file
    is a symlink targeting a file name that ends with ".desktop".
    */
#ifdef _DIRENT_HAVE_D_TYPE
    if (ISLNK (namelist[i])
        && (conf_get_followsymlink () & CONF_FOLLOWSYMLINK_FILE))
#else
    if (ISLNK (&statbuf)
        && (conf_get_followsymlink () & CONF_FOLLOWSYMLINK_FILE))
#endif
    {
     gchar *link_target = realpath (curr_path, NULL);
     if (link_target == NULL)
     {
      entry_push_error (entry, RWARN, "'%s': %s", curr_path, strerror (errno));
      continue;
     }
     ends_with_desktop =
     strcmp (link_target + strlen (link_target) - 8, ".desktop") == 0;
     free (link_target);
     if (!ends_with_desktop)
     {
      continue;
     }
    }
    else
    {
     /* name does not end with ".desktop" */
     continue;
    }
   }
   else
   {
#ifdef _DIRENT_HAVE_D_TYPE
    if (ISLNK (namelist[i]))
#else
    if (ISLNK (&statbuf))
#endif
    {
     /* Symlink. */

     gchar *buf = realpath (curr_path, NULL);
     if (buf != NULL)
     {
      free (buf);
     }
     else
     {
      entry_push_error (entry, RWARN, "'%s': %s", curr_path, strerror (errno));
      continue;                            /* skip invalid symlinks */
     }
    }
   }
  }

  /* -----
  There is a regular file or symlink that can be processed as a launcher file.
  ----- */

  ErrorList *head_error = entry->error;
  result = launcher_app (curr_path, entry, callerid);
  if (head_error != entry->error)
  {
   entry_prefix_error (entry, "'%s': ", curr_path);
  }
  /* FALLTHROUGH */

break_this_loop:
  if (result >= RFAIL)
  {
   /*
   Here there could be code to break out and return result. However,
   I think it is more useful to carry on, even on severe errors. Errors
   attached to this entry will bubble up and be reported in the main loop.
   */
   continue;
  }
 }
 if (namelist != NULL)
 {
  /* Since namelist and *namelist[i] are cached they cannot be freed. */
  /*free (namelist);*/
 }

 /***********************
 *  LINE_LAUNCHER_SUB  *
 ***********************/

 if (callerid == LINE_LAUNCHER_SUB)
 {
  /* Recursion tail. */

  /* Reset the pending leave_submenu set by enter_submenu above.
  This re-initialization preserves the collected error messages. */
  entry_init (entry, NULL, LINE_LAUNCHER_SUB, ENTRY_FLAG_ALLOW_ICOTIP,
              gl_menu_depth);
 }

 /* Downgrade the importance of the collected errors. */
 return entry->error ? RWARN : ROK;
}

/**
*/
enum Result
a_launcherargs (struct Entry *entry)
{
 strcpy (gl_launcher_args, entry->_dat);
 return ROK;
}

/**
a_launcherdirfile:
Set the launcher_dirfile global variables that apply to either a specific
menu depth or any depth.

This directive accepts array syntax: `launcherdirfile[i]=launcher_dirfile`.
The index, if specified, is clipped in the range [0,MAX_MENU_DEPTH).
If option -v is on, an out-of-range warning is emitted. Omitting the
index is equivalent to repeating the directive for the entire range.
*/
enum Result
a_launcherdirfile (struct Entry *entry)
{
 gchar *_dat = entry->_dat;
 gint a = 0, z = MAX_MENU_DEPTH - 1, depth = -1;
 enum Result result = ROK;

 if (entry->flags & ENTRY_FLAG_ARRAY)
 {
  depth = entry->index;
  if (depth < 0 || depth >= MAX_MENU_DEPTH)
  {
   entry_push_error (entry, RWARN, "index %1$d clipped to [0,%2$d]",
                    entry->index, MAX_MENU_DEPTH - 1);
   depth = MIN (MAX (depth, 0), MAX_MENU_DEPTH - 1);
  }
  a = z = depth;
 }

 if (G_UNLIKELY (_dat[0] == '\0'))     /* Reset indexed/all depths. */
 {
  for (gint i = a; i <= z; i++)
  {
   *gl_launcher_dirfile[i].entry.categories =
   *gl_launcher_dirfile[i].path = '\0';
  }
 }
 else                                  /* Set indexed/all depths */
 {
  result = path_expand (_dat, gl_script_dir, "launcherdirfile", entry);
  if (result == ROK)
  {
    for (gint i = a; i <= z; i++)
    {
     result = launcher_entry_fill_cached (_dat, &gl_launcher_dirfile[i].entry,
                                          NULL, FALSE, LINE_LAUNCHER_DIRFILE);
     if (result == ROK)
     {
      strcpy (gl_launcher_dirfile[i].path, _dat);
      /* From now on, path will only be used to compose error messages. */
      logmsg_str_shorten (gl_launcher_dirfile[i].path);
     }
     else
     {
      break;
     }
    }
  }
 }
 return result;
}

/**
enter_launchersubmenu:
*/
enum Result
enter_launchersubmenu (struct Entry *entry)
{
 struct stat sb;
 struct Entry *me;
 enum Result result = RFAIL;
 gchar *_dat = entry->_dat;

 if (gl_menu_depth >= MAX_MENU_DEPTH - 1)
 {
  entry_push_error (entry, result, "%s", "maximum submenu depth exceeded");
  goto out;
 }
 if (_dat[0] == '\0')
 {
  entry_push_error (entry, result, "`%s=` requires file",
                    entry->directive->keyword);
  goto out;
 }
 if ((path_expand (_dat, gl_script_dir, entry->directive->keyword, entry)) !=
     ROK)
 {
  goto out;
 }
 if (stat (_dat, &sb) == -1 || !(S_ISREG (sb.st_mode) || S_ISLNK (sb.st_mode)))
 {
  entry_push_error (entry, result, "`%s=`'%s': %s", entry->directive->keyword,
                    _dat, strerror (ENODATA));
  goto out;
 }
 if ((me = entry_new_tracked ()) == NULL)
 {
  entry_push_error (entry, result, "`%s=`'%s': %s", entry->directive->keyword,
                    _dat, strerror (errno));
  goto out;
 }

 entry_init (entry, NULL, LINE_LAUNCHER_SUBMENU, ENTRY_FLAG_ALLOW_ICOTIP |
             ENTRY_FLAG_RESET_ERROR, gl_menu_depth);
 result =
 launcher_entry_fill_cached (_dat, entry, NULL, FALSE, LINE_LAUNCHER_SUBMENU);
 if (result < RFAIL)
 {
  entry->leave = leave_launchersubmenu;
  memcpy (me, entry, sizeof (*me));
  me->error = NULL; /* disown */
  g_ptr_array_add (gl_launchersubmenu_stack, me);
  gl_menu_depth++;
 }
out:
 return result;
}

/**
leave_launchersubmenu:
Commit a new submenu node configured by its launcher_dirfile argument.

Return: #Result.
*/
enum Result
leave_launchersubmenu (struct Entry *entry)
{
 struct Entry *e;
 enum Result result;
 guint len = gl_launchersubmenu_stack->len;

 if (len == 0)
 {
  return RFAIL;
 }
 e = (struct Entry *) g_ptr_array_index (gl_launchersubmenu_stack, len - 1);
 e->menu_depth = entry->menu_depth;
 result = leave_submenu (e);
 return result;
}

/**
intersectQ:
Callback for are_categories_intersecting().
*/
static gboolean
intersectQ (gchar *a,
            gchar *b)
{
 gchar *at, *as, *bt, *bs, *bk;

 if (!(bk = strdup (b)))
 {
  perror ("strdup");
  return FALSE;
 }

 at = strtok_r (a, ";", &as);
 while (at)
 {
  strcpy (b, bk);
  bt = strtok_r (b, ";", &bs);
  while (bt)
  {
   if (0 == strcmp (at, bt))
   {
    free (bk);
    return TRUE;
   }
   bt = strtok_r (NULL, ";", &bs);
  }
  at = strtok_r (NULL, ";", &as);
 }
 free (bk);
 return FALSE;
}

/**
are_categories_intersecting:
Calculate the intersection between two category lists.

@a, @b: strings, {Categories=} lists to intersect; a from a launcher .desktop
@file; @b from a `dirfile` equivalent.

Refer to § "Launcher exclusion cases" in docs/scripting_guide.md
to understand what this function computes.

Returns: TRUE (intersection or error), FALSE.
*/
static gboolean
are_categories_intersecting (const gchar *a,
                             const gchar *b)
{
 gboolean ret = TRUE;

 if (!b || !*b)
 {
  goto out;
 }
 if ((!a || !*a) && *b && !conf_get_launchernullcategory ())
 {
  guint len = strlen (b);
  gchar *b2 = calloc (1, len + 3);

  if (b2 == NULL)
  {
   perror ("malloc");
  }
  else
  {
   b2[0] = ';';
   strcpy (b2 + 1, b);
   b2[len + 2] = ';';
   strcpy (b2 + len, ";");
   ret = strstr (b2, ";NULL;") != NULL;
   free (b2);
  }
 }
 else if (a && *a)
 {
  void *p = NULL, *q = NULL;

  if ((p = strdup (a)) != NULL && (q = strdup (b)) != NULL)
  {
   ret = intersectQ (p, q);
  }
  else
  {
   perror ("strdup");
  }
  free (p);
  free (q);
 }

out:
 return ret;
}

/**
launcher_app:
Like leave_item but for a menu entry described by a .desktop file.

@launcherpath:
@entry: pointer to @launcherpath's dirpath struct #Entry.
@callerid:
*/
static enum Result
launcher_app (gchar *launcherpath,
              struct Entry *entry,
              const enum LineType callerid)
{
 const gchar *categories = NULL;
 gchar *cmd;
 GtkWidget *widget;
 enum Result result =
 launcher_entry_fill_cached (launcherpath, entry, NULL, TRUE, callerid);
 if (result >= RFAIL)
 {
  return result;
 }
 else if (entry->launcher_type != LAUNCHER_TYPE_APPLICATION)
 {
  return ROK;
 }
 entry->menu_depth = gl_menu_depth; /* emulate enter_item */
 if (entry->no_display && conf_get_launchernodisplay ())
 {
  return RINFO;
 }

 /*
 Apply {Category=} filters, which could exclude the launcher.
 The launcher's {Category=} property ranks higher that the launcher_dirfile's.
 */
 if (categories == NULL || categories[0] == '\0')
 {
  categories = gl_launcher_dirfile[entry->menu_depth].entry.categories;
 }

 if (!are_categories_intersecting (entry->categories, categories))
 {
  entry_push_error (entry, RINFO, "'%s' excluded by '%s'", launcherpath,
                     gl_launcher_dirfile[entry->menu_depth].path);

  return RINFO;     /* honor exclusion rule */
 }

 cmd = entry->cmd;
 if (cmd[0])
 {
  launcher_exec_data_whiteout_code (cmd);
  if (gl_launcher_args[0])
  {
   gchar buf[member_size (struct Entry, cmd)];
   gint r = snprintf (buf, sizeof buf, "%s %s", cmd, gl_launcher_args);

   if (r < 0 || (gsize) r >= sizeof buf)
   {
    entry_push_error (entry, RFAIL, "`cmd=`+`launcherargs=` too long (%s)",
                      buf);
    return RFAIL;
   }
   cmd = buf;
  }
 }
 struct Entry *tracked = NULL;
 (void) entry_append_leaf_node (entry, NULL, cmd, &widget, &tracked);
 if (widget == NULL)
 {
  result = RFAIL;
 }
 else if (result < RWARN && entry_icon_is_to_render (entry))
 {
  result = entry_add_icon (entry, widget);
 }
 (void) node_attach_tracked_entry (widget, &tracked, entry);
 launcher_prop_free_values ();
 if (result < RFAIL)
 {
  ++gl_launcher_ctr;
 }

 return result;
}

/**
*/
enum Result
a_launcherdir (struct Entry *entry)
{
 enum Result result = make_rooted_dirpath (entry);
 if (result < RFAIL)
 {
  strcpy (gl_launcher_dir, entry->_dat);
 }
 return result;
}

/**
*/
const gchar *
launcher_get_launcherdirectory ()
{
 return gl_launcher_dir;
}

/**
*/
void
launcher_set_launcherdirectory (const gchar *value)
{
 *gl_launcher_dir = '\0';
 strncat (gl_launcher_dir, value, sizeof gl_launcher_dir - 1);
}

/**
launcher_module_init:
Initialize this module.

@cflags: additional cflags passed to regcomp(3).

Returns: 0(OK) -1(Error).
*/
int
launcher_module_init (const int cflags)
{
 if (compile_regex (&gl_rex_launcher_exec_data, gl_pat_launcher_exec_data,
                    REG_EXTENDED | cflags) != 0)
 {
  return -1;
 }
 gl_launcher_ctr = 0;
 *gl_launcher_dir = *gl_launcher_args = '\0';
 gl_launchersubmenu_stack = g_ptr_array_new ();
 for (gsize i = 0; i < G_N_ELEMENTS (gl_launcher_dirfile); i++)
 {
  gl_launcher_dirfile[i].path[0] = '\0';
  entry_init (&gl_launcher_dirfile[i].entry, NULL, LINE_LAUNCHER_DIRFILE,
              ENTRY_FLAG_ALLOW_ICOTIP | ENTRY_FLAG_RESET_ERROR, 0);
 }
 return 0;
}

/**
launcher_module_finalize:
Clean up this module. Call before exit(2).
*/
void
launcher_module_finalize (void)
{
 regfree (&gl_rex_launcher_exec_data);
 g_ptr_array_free (gl_launchersubmenu_stack, TRUE);
 gl_launchersubmenu_stack = NULL;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
