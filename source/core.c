/*
gtkmenuplus - Generate a GTK popup menu from text directives.

Copyright (C) 2016-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2

Forked from version 1.00, 2013-04-24, by Alan Campbell
Partially based on myGtkMenu, Copyright (C) 2004-2011 John Vorthman
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

#include <glob.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "autoconfig.h"
#ifdef FEATURE_ACTIVATION_LOG
#include "activ_log.h"
#endif
#include "conf.h"
#include "common_gnu.h"
#ifdef FEATURE_FORMATTING
#include "fmtg.h"
#endif
#include "entry.h"
#include "entry_icon.h"
#include "input.h"
#ifdef FEATURE_LAUNCHER
#include "launcher.h"
#endif
#include "main.h"
#include "path.h"
#include "core.decl.h"

guint gl_menu_depth                      = 0; /* 0 => top menu */
guint gl_menu_counter                    = 0; /* Total entries */
guint gl_menu_widgets                    = 0;
guint gl_submenu_widgets                 = 0;
guint gl_include_depth                   = 0; /* Include directive */

static GtkWidget *gl_menu_w[MAX_MENU_DEPTH]     = {0};
static GtkWidget *gl_menu_entry[MAX_MENU_DEPTH] = {0};
/**
entry_add_icon:
*/
enum Result
entry_add_icon (struct Entry *entry,
                GtkWidget *menu_item)
{
 GtkWidget *img;
 enum Result result = ROK;
 ErrorList *head_error = entry->error;

 img = entry_icon_image_new (entry);
 if (img == NULL)
 {
  result = RINFO;
 }
 if (head_error != entry->error)
 {
  /* Downgrade the severity of the error pushed by entry_icon_image_new. */
  entry_set_error_level (entry, result);
 }
 else
 {
  UNDEPR (gtk_image_menu_item_set_image, GTK_IMAGE_MENU_ITEM (menu_item), img);
#ifdef FEATURE_ICONS_ALWAYS_ON
  /*
  Define to ignore the GtkSettings:gtk-menu-images setting, which
  controls whether icons are displayed in menus. Enabling this
  feature forces menus to always display available icons. See
  https://stackoverflow.com/questions/8989833.
  */
  UNDEPR (gtk_image_menu_item_set_always_show_image,
          GTK_IMAGE_MENU_ITEM (menu_item), TRUE);
#endif
 }
 return result;
}

#ifdef FEATURE_ACTIVATION_LOG
/**
*/
enum Result
a_activationlogfile (struct Entry *entry)
{
 gchar *_dat = entry->_dat;
 const activation_log *log = NULL;
 enum Result result;
 int fd;

 if (_dat[0] == '\0')
 {
  return activation_log_module_finalize () ? RFAIL : ROK;
 }

 if ((result = path_expand (_dat, gl_script_dir, "activationlogfile", entry))
     == ROK)
 {
  if ((log = activation_log_fetch (_dat)))
  {
   /* Touch the file so the calling application can include it. */
   if ((fd = open (log->path, O_CREAT | O_WRONLY, 0600)) >= 0)
   {
    close (fd);
   }
   else
   {
    result = RFAIL;
    entry_push_error (entry, result, "`%s=` open: %s",
                      entry->directive->keyword, strerror (errno));
   }
  }
 }
 return result;
}

/**
*/
enum Result
a_activationlogexclude (struct Entry *entry)
{
 gchar *_dat = entry->_dat;

 if (*_dat && activation_log_fetch (NULL) == NULL)
 {
  entry_push_error (entry, RFAIL, "`%s=`: no log is set",
                    entry->directive->keyword);
  return RFAIL;
 }
 if (activation_log_exclude (_dat) != 0)
 {
  entry_push_error (entry, RFAIL, "`%s=` error", entry->directive->keyword);
  return RFAIL;
 }
 return ROK;
}

#endif

/**
*/
enum Result
a_cmd (struct Entry *entry)
{
 enum Result result = entry_is_directive_allowed (entry, entry->cmd);
 if (result < RFAIL)
 {
  strcpy (entry->cmd, entry->_dat);
 }
 return result;
}

/**
*/
enum Result
a_icon (struct Entry *entry)
{
 enum Result result =
 entry_is_directive_allowed (entry, entry->icon);
 if (result < RFAIL)
 {
  strcpy (entry->icon, entry->_dat[0] ? entry->_dat : "NULL");
 }
 return result;
}

/**
*/
enum Result
a_tooltip (struct Entry *entry __attribute__((unused)))
{
#ifdef FEATURE_TOOLTIP
 enum Result result =
 entry_is_directive_allowed (entry, entry->tooltip);
 if (result < RFAIL)
 {
  strcpy (entry->tooltip, entry->_dat);
 }
 return result;
#else
 return ROK;
#endif
}

/**
entry_append_leaf_node :
Append a leaf node to the GTK menu.

@entry: pointer to #Entry - members are not modified, except .error.
@label: alternative menu label overriding @entry->label. Nullable.
@cmd: alternative activation command overriding @entry->cmd. Nullable.
@widgetptr: return location for #GtkWidget pointer.

Return: %Result. Set *@widget pointing to the GTK menu item, or NULL on error.
*/
enum Result
entry_append_leaf_node (struct Entry *entry,
                        const gchar *label,
                        const gchar *cmd,
                        GtkWidget **widgetptr)
{
 GtkWidget *widget = NULL;
 enum Result result = RFAIL;
 const gchar *lbl = label != NULL ? label : entry->label;
 struct Entry *me = NULL;

 if (conf_get_mnemonic ())
 {
  SETUNDEPR (widget, gtk_image_menu_item_new_with_mnemonic, lbl);
 }
 else
 {
  SETUNDEPR (widget, gtk_image_menu_item_new_with_label, lbl);
 }
 if (widget != NULL)
 {
  gl_menu_widgets++;
  result = ROK;
  gtk_menu_shell_append (GTK_MENU_SHELL (gl_menu_w[entry->menu_depth]), widget);
  ++gl_menu_counter;
#ifdef FEATURE_SERIALIZATION
  if (gl_opt_json_serialize)
  {
   if ((me = entry_new_tracked ()) == NULL)
   {
    entry_push_error (entry, RWARN, "entry `%s': cannot serialize", lbl);
    result = RWARN;
   }
   else
   {
    memcpy (me, entry, sizeof (*entry));
    me->error = NULL; /* disown */
   }
  }
#endif
  if (entry->cmd[0] || (cmd != NULL && cmd[0]))
  {
   if (me == NULL)
   {
    if ((me = entry_new_tracked ()) == NULL)
    {
     entry_push_error (entry, RWARN, "entry `%s': cannot activate", lbl);
     result = RWARN;
    }
    else
    {
     memcpy (me, entry, sizeof (*entry));
     me->error = NULL; /* disown */
    }
   }
  }
  if (me != NULL)
  {
   if (cmd && cmd[0])
   {
    strcpy (me->cmd, cmd);
   }
   g_signal_connect_swapped (widget, "activate", G_CALLBACK (entry_activate),
                             me);
   g_object_set_data (G_OBJECT (widget), "entry", me);
  }
  else
  {
   gtk_widget_set_sensitive (widget, FALSE);
  }
#ifdef FEATURE_FORMATTING
  if (fmtg_format_widget_label (widget) != 0)
  {
   result = RWARN;
   entry_push_error (entry, result, "entry `%s': formatting error", lbl);
  }
#endif
#ifdef FEATURE_TOOLTIP
  if (entry->tooltip[0] != '\0')
  {
   gtk_widget_set_tooltip_text (widget, entry->tooltip);
#ifdef FEATURE_FORMATTING
   if (fmtg_format_widget_tooltip (widget) != 0)
   {
    entry_push_error (entry, RWARN, "entry `%s': tooltip formatting error",
                      lbl);
    result = RWARN;
   }
#endif
  }
#endif
 }
 *widgetptr = widget;
 return result;
}

/**
*/
enum Result
enter_item (struct Entry *entry)
{
 entry_init (entry, leave_item, LINE_ITEM, ENTRY_FLAG_ALLOW_CMD |
             ENTRY_FLAG_ALLOW_ICOTIP | ENTRY_FLAG_RESET_ERROR,
             gl_menu_depth);
 strcpy (entry->label, entry->_dat);
 return ROK;
}

/**
leave_item:
Make menu entry from open item block: `item=`, `icon=`, `tooltip=`, `cmd=`.
*/
enum Result
leave_item (struct Entry *entry)
{
 GtkWidget *widget;
 enum Result result = entry_append_leaf_node (entry, NULL, NULL, &widget);
 if (widget != NULL)
 {
  if (entry_icon_is_to_render (entry) && result < RWARN)
  {
   result = entry_add_icon (entry, widget);
  }
 }
 return result;
}

/**
*/
enum Result
a_endsubmenu (struct Entry *entry)
{
 enum Result result;

 if (!conf_get_endsubmenu ())
 {
  entry_push_error (entry, RFAIL, "`%s` without `configure=endsubmenu`",
                    entry->directive->keyword);
  return RFAIL;
 }
 if (gl_menu_depth <= 0)
 {
  entry_push_error (entry, RFAIL, "`%s` without `submenu` or `launchersubmenu`",
                    entry->directive->keyword);
  return RFAIL;
 }

 result = ROK;
#ifdef FEATURE_FORMATTING
 if (gl_fmtg_item[gl_menu_depth].fmt_sep != '\0')
 {
  if (fmtg_init_formatting (&gl_fmtg_item[gl_menu_depth], "", 0) != 0)
  {
   result = RFAIL;
  }
 }
#endif
 entry->menu_depth = --gl_menu_depth;
 return result;
}

/**
enter_submenu_internal:

@entry:
@feat: #EntryFlags feature bit mask currently
passing only ENTRY_FLAG_RESET_ERROR.
*/
enum Result
enter_submenu_internal (struct Entry *entry,
                        const enum EntryFlags feat)
{
 if (gl_menu_depth >= MAX_MENU_DEPTH - 1)
 {
  entry_push_error (entry, RFAIL, "%s", "maximum submenu depth exceeded");
  return RFAIL;
 }
 entry_init (entry, leave_submenu, LINE_SUBMENU, ENTRY_FLAG_ALLOW_ICOTIP |
             feat, gl_menu_depth);
 strcpy (entry->label, entry->_dat);
 gl_menu_depth++;
 return ROK;
}

/**
*/
enum Result
enter_submenu (struct Entry *entry)
{
 return enter_submenu_internal (entry, ENTRY_FLAG_RESET_ERROR);
}

/**
leave_submenu:
Commit a new submenu node.

Return: #Result and save the widgets to gl_menu_entry[] and gl_menu_w[].

All the directory-scanning directives that create submenus call leave_submenu.
*/
enum Result
leave_submenu (struct Entry *entry)
{
 enum Result result = RFAIL;
 GtkWidget *w = NULL, *submenu = NULL;
 const guint depth = entry->menu_depth;

 if (conf_get_mnemonic ())
 {
  SETUNDEPR (submenu, gtk_image_menu_item_new_with_mnemonic, entry->label);
 }
 else
 {
  SETUNDEPR (submenu, gtk_image_menu_item_new_with_label, entry->label);
 }
 if (submenu == NULL)
 {
   goto out;
 }
 gl_submenu_widgets++;
 if ((w = gtk_menu_new ()))
 {
  gl_menu_entry[depth] = submenu;
  gtk_menu_shell_append (GTK_MENU_SHELL (gl_menu_w[depth]), submenu);
  ++gl_menu_counter;
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (submenu), gl_menu_w[depth + 1] = w);
#ifdef FEATURE_SERIALIZATION
  if (gl_opt_json_serialize)
  {
   struct Entry *me = entry_new_tracked ();
   if (me == NULL)
   {
    result = RWARN;
    entry_push_error (entry, result, "entry `%s': cannot serialize",
                      entry->label);
   }
   else
   {
    memcpy (me, entry, sizeof (*entry));
    me->error = NULL; /* disown */
    g_object_set_data (G_OBJECT (submenu), "entry", me);
   }
  }
#endif
 }
 else
 {
   gtk_widget_destroy (submenu);
   goto out;
 }
#ifdef FEATURE_FORMATTING
 if (fmtg_format_widget_label (submenu) != 0)
 {
  entry_push_error (entry, result, "entry `%s': formatting error",
                    entry->label);
  goto out;
 }
#endif
#ifdef FEATURE_TOOLTIP
 if (entry->tooltip[0] != '\0')
 {
  gtk_widget_set_tooltip_text (submenu, entry->tooltip);
#ifdef FEATURE_FORMATTING
  if (fmtg_format_widget_tooltip (submenu) != 0)
  {
   entry_push_error (entry, result, "entry `%s': tooltip formatting error",
                     entry->tooltip);
   goto out;
  }
#endif
 }
#endif
 if (entry_icon_is_to_render (entry))
 {
  result = entry_add_icon (entry, submenu);
 }
 else
 {
  result = ROK;
 }
 if (result >= RFAIL)
 {
  entry_push_error (entry, result, "%s", "icon error");
  goto out;
 }
#ifdef FEATURE_FORMATTING
 /* Formatting cascades to the next menu level if
 there is only one `format_section` (fmt_sep == 0). */
 if (gl_fmtg_item[depth].fmt_sep == '\0' && depth + 1 < MAX_MENU_DEPTH)
 {
  memcpy (&gl_fmtg_item[depth + 1], &gl_fmtg_item[depth], sizeof *gl_fmtg_item);
 }
#endif
 result = ROK;
out:
 return result;
}

/**
*/
enum Result
a_separator (struct Entry *entry __attribute__((unused)))
{
 GtkWidget *widget = gtk_separator_menu_item_new ();
 gtk_menu_shell_append (GTK_MENU_SHELL (gl_menu_w[gl_menu_depth]), widget);
 ++gl_menu_counter;
 return ROK;
}

/**
a_iconsize:
Set menu icon size.

Returns: #Result.
*/
enum Result
a_iconsize (struct Entry *entry)
{
 if (conf_iconsize (entry->_dat, 1) != 1)
 {
  entry_push_error (entry, RWARN, "invalid icon size '%s'", entry->_dat);
  return RWARN;
 }
 return ROK;
}

/**
a_menuposition
Set menu position for directive `menuposition=`.
*/
enum Result
a_menuposition (struct Entry *entry)
{
 if (conf_menuposition (entry->_dat, 2) != 2)
 {
  entry_push_error (entry, RWARN, "%s", "cannot change menu position");
  return RWARN;
 }
 return ROK;
}

/**
*/
enum Result
a_icondir (struct Entry *entry)
{
 enum Result result = make_rooted_dirpath (entry);
 if (result < RFAIL)
 {
  strcpy (gl_icon_dir, entry->_dat);
 }
 return result;
}

/**
make_rooted_dirpath:
Given a (relative) dirpath, root it in BASE DIRECTORY.

@entry: pointer to struct #Entry. @entry->_dat is the input dirpath.

Returns: #LineParseResul. The output dirpath is copied to @entry->_dat.
The output dirpath is path-expanded, error-checked and ends with '/'.
*/
enum Result
make_rooted_dirpath (struct Entry *entry)
{
 enum Result result;
 struct stat statbuf;
 gchar *_dat = entry->_dat;

 if (_dat[0] == '\0')
 {
  strcpy (_dat, gl_script_dir);
  return ROK;
 }
 result = path_expand (_dat, gl_script_dir, entry->directive->keyword, entry);
 if (result < RFAIL)
 {
  if (stat (_dat, &statbuf) == -1 || !S_ISDIR (statbuf.st_mode))
  {
   result = RFAIL;
   entry_push_error (entry, result, "'`%s`: invalid directory",
                     entry->directive->keyword);
  }
  else
  {
   gchar *p = strchr (_dat, '\0');
   if (_dat[-1] != G_DIR_SEPARATOR
       && (ptrdiff_t) (p - _dat) < (glong) SIZEOF_COOKED)
   {
    *p++ = G_DIR_SEPARATOR;
    *p = '\0';
   }
   else
   {
    result = RFAIL;
    entry_push_error (entry, result, "'`%s`: path too long",
                      entry->directive->keyword);
   }
  }
 }
 return result;
}

/**
*/
enum Result
a_error (struct Entry *entry)
{
 entry_push_error (entry, RFAIL, "%s", entry->_dat);
 return RFAIL;
}

enum Result
a_configure (struct Entry *entry)
{
 return conf_apply_keywords_from_entry (entry);
}

/**
*/
enum Result
a_onexit (struct Entry *entry)
{
 strcpy (gl_on_exit, entry->_dat);
 return ROK;
}

/**
*/
enum Result
a_absolutepath (struct Entry *entry)
{
 const gchar *cmd, *label, *scheme;
 gchar *cmdx;
 guint error;
 GtkWidget *widget;
 enum Result result;
 gchar *_dat = entry->_dat;

#ifdef FEATURE_LAUNCHER
 const gchar *ext = strrchr (_dat, '.');
 if (ext != NULL && strcasecmp (ext, ".desktop") == 0)
 {
  return a_launcher (entry);
 }
#endif
 result = ROK;
#ifdef FEATURE_TOOLTIP
 strcpy (entry->tooltip, _dat);
#endif
 scheme = g_uri_peek_scheme (_dat);
 label = scheme ? _dat : path_rcomponents (_dat, conf_get_abspathparts ());
 cmd = _dat;
 cmdx = path_expand_full (_dat, gl_icon_dir, &error);
 if (error)
 {
  result = RFAIL;
  entry_push_error (entry, result, "`%s=`: expanded path too long",
                    entry->directive->keyword);
  goto out;
 }
 if (cmdx != NULL)
 {
  cmd  = cmdx;
 }
 result = entry_append_leaf_node (entry, label, cmd, &widget);
 if (widget != NULL)
 {
  entry_init (entry, NULL, LINE_ABSOLUTE_PATH, ENTRY_FLAG_ALLOW_CMD,
              gl_menu_depth);
  strcpy (entry->label, label);
  strcpy (entry->cmd, _dat);
  entry->icon[0] = '\0';
  if (entry_icon_is_to_render (entry) && result < RWARN)
  {
   result = entry_add_icon (entry, widget);
  }
  entry_init (entry, NULL, LINE_UNDEFINED, 0, 0);
 }
 else
 {
  result = RFAIL;
 }
out:
 free (cmdx);
 return result;
}

/**
path_rcomponents:
Return the sub-path consisting of the last N components of a pathname.
*/
static const gchar *
path_rcomponents (gchar *path,
                  guint n)
{
 if (n == 0)
 {
  return path;
 }
 for (gchar *start = path, *p = strchr (path, '\0') - 1; p >= path; p--)
 {
  if (*p == G_DIR_SEPARATOR)
  {
   n--;
   start = p + 1;
   if (n == 0)
   {
    return start;
   }
  }
 }
 return path;
}

/**
*/
enum Result
enter_include (struct Entry *entry)
{
 entry_init (entry, leave_include, LINE_INCLUDE, ENTRY_FLAG_ALLOW_CMD |
             ENTRY_FLAG_ALLOW_ICOTIP | ENTRY_FLAG_RESET_ERROR,
             gl_menu_depth);
 strcpy (entry->label, entry->_dat);
 return ROK;
}

/**
leave_include:
Finalize the pending `include=` directive distinguishing
between File Include vs Directory Include.

@entry: pointer to struct #Entry.

Returns: #Result.
*/
enum Result
leave_include (struct Entry *entry)
{
 struct stat statbuf;
 enum Result result;
 gint argc     = 0;
 gchar **argv = NULL;
 GError *err   = NULL;

 if (!g_shell_parse_argv (entry->label, &argc, &argv, &err) || argc == 0)
 {
  result = RFAIL;
  entry_push_error (entry, result, "invalid `include=`: %s",
                    err != NULL ? err->message : "missing pathname");
  goto out;
 }
 const gboolean is_compound_path = strchr (argv[0], G_DIR_SEPARATOR) != NULL;
 result = path_expand (argv[0], gl_script_dir, "include", entry);
 if (result >= RFAIL)
 {
  goto out;
 }

 /*
 include = script_file [parameter1 [parameter2 ...]]
 include = path_to_directory[/file_glob] [directory_glob]
 */
 /* Hidden path and symbolic link path restrictions don't
 apply to a scanning directive's argument. */
 guint errnum = 0;
 if (stat (argv[0], &statbuf) == 0) /* stat follows symlinks */
 {
  if (S_ISREG (statbuf.st_mode))
  {
   result = include_file ((const guint) argc, (const gchar **) argv, entry);
  }
  else if (S_ISDIR (statbuf.st_mode))
  {
   result =
   include_directory ((const guint) argc, (const gchar **) argv, entry);
  }
  goto out;
 }
 else
 {
  errnum = errno;

  /* Assume we are dealing with a Directory Include, and validate
  this assumption, where possible, before starting to scan. */
  entry->flags |= ENTRY_FLAG_INCLUDE_DIR;
  gboolean is_dirpath = FALSE;
  glob_t glob_info;
  if (is_compound_path)
  {
   gchar *p = strrchr (argv[0], '/');
   *p = '\0';
   is_dirpath = stat (argv[0], &statbuf) == 0;
   errnum = errno;
   *p = '/';
  }
  else if (glob (argv[0], GLOB_NOSORT, NULL, &glob_info) != GLOB_NOMATCH)
  {
   is_dirpath = glob_info.gl_pathc > 0;
   globfree (&glob_info);
  }

  if (is_dirpath)
  {
   errnum = 0;

   /* Note that dirpath/file_glob is undistinguishable
   from dirpath/non_existent_file, if dirpath exists. We
   break the ambiguity by favoring the former case. */

   /* include = path_to_directory/file_glob [directory_glob] */
   result =
   include_directory ((const guint) argc, (const gchar **) argv, entry);
  }
 }
 if (entry->error)
 {
  result = entry_get_max_error_level (entry, result, NULL);
 }
 if (errnum)
 {
  result = (entry->flags & ENTRY_FLAG_INCLUDE_DIR) ? RFAIL : RFATAL;
  entry_push_error (entry, result, "`include=`%s: %s", argv[0],
                    strerror (errnum));
 }

out:
 if (argv != NULL)
 {
  g_strfreev (argv);
 }

 return result;
}

/**
include_file:
Include file (glob) from `include=` directive.

@argc: length of @argv.
@argv: vector holding the expanded file glob.
@entry: pointer to push errors.

Returns: #Result.
*/
static enum Result
include_file (const gint argc,
              const gchar **argv,
              struct Entry *entry)
{
 struct InputSource isource;
 isource.fname = argv[0];
 if (!input_source_open (&isource, entry))
 {
  return RFAIL;
 }

 /*
 Save the current base path context.
 */
 gchar save_icon_dir[MAX_PATH_LEN + 1];
 gchar save_script_dir[MAX_PATH_LEN + 1];
 strcpy (save_icon_dir, gl_icon_dir);
 strcpy (save_script_dir, gl_script_dir);

 if (!path_get_base_dir (gl_script_dir, argv[0], FALSE, entry))
 {
   return RFAIL;
 }
 if (strcmp (save_script_dir, gl_icon_dir) == 0)
 {
  strcpy (gl_icon_dir, gl_script_dir);
 }
#ifdef FEATURE_LAUNCHER
 gchar save_launcher_dir[MAX_PATH_LEN + 1];

 strcpy (save_launcher_dir, launcher_get_launcherdirectory ());
 if (strcmp (save_script_dir, launcher_get_launcherdirectory ()) == 0)
 {
  launcher_set_launcherdirectory (gl_script_dir);
 }
#endif

 /*
 Save current formatting context.
 */
#ifdef FEATURE_FORMATTING
 struct Formatting fmtg_ctx[MAX_MENU_DEPTH];
 memcpy (&fmtg_ctx, &gl_fmtg_item, sizeof (struct Formatting) * MAX_MENU_DEPTH);
#ifdef FEATURE_TOOLTIP
 struct Formatting fmtg_tooltip;
 memcpy (&fmtg_tooltip, &gl_fmtg_tooltip, sizeof (struct Formatting));
#endif
#endif

 /*
 Recursive
 */
 enum Result result =
 main_loop (&isource, argc, (const gchar **) argv, entry->menu_depth, entry);

 /*
 Restore previous contexts
 */
 strcpy (gl_icon_dir, save_icon_dir);
 strcpy (gl_script_dir, save_script_dir);
#ifdef FEATURE_FORMATTING
 if (conf_get_formattinglocal ())
 {
  memcpy (&gl_fmtg_item, &fmtg_ctx,
          sizeof (struct Formatting) * MAX_MENU_DEPTH);
#ifdef FEATURE_TOOLTIP
  memcpy (&gl_fmtg_tooltip, &fmtg_tooltip, sizeof (struct Formatting));
#endif
 }
#endif
#ifdef FEATURE_LAUNCHER
 launcher_set_launcherdirectory (save_launcher_dir);
#endif

 return result;
}

/**
include_directory:
Include directory (glob) from `include=` directive.

@argc: length of @argv.
@argv: vector holding the expanded file glob.
@entry: pointer to struct #Entry.

Returns: #Result.

*/
static enum Result
include_directory (const gint argc,
                   const gchar **argv,
                   struct Entry *entry)
{
 /* include = directory_path[/file_glob] [depth_1_dir_glob] */
 const gchar *depth_1_dir_glob = argc >= 2 ? argv[1] : "";
 gchar *dirpath, *file_glob, *str;
 enum Result result = RFAIL;

 dirpath = file_glob = str = NULL;
 if (access (argv[0], F_OK) == 0)
 {
  dirpath = (gchar *) argv[0];
  file_glob = (gchar *) "*";
 }
 else
 {
  str = strdup (argv[0]);
  if (str == NULL)
  {
   perror ("strdup");
  }
  else
  {
   dirpath = dirname (str);
   /* '/' is not G_DIR_SEPARATOR but a poorly-chosen token separator.
   Refer to docs/scripting_guide.md § "Include directive errors". */
   file_glob = strrchr (argv[0], '/') + 1;

   entry_push_error (entry, RINFO, "including '%1$s' with file glob '%2$s'",
                     dirpath, file_glob);
  }
 }
 if (dirpath != NULL)
 {
  entry->flags |= ENTRY_FLAG_INCLUDE_DIR;
  result = include_directory_scan ((const gchar *) dirpath, "",
                                   (const gchar *) file_glob, depth_1_dir_glob,
                                   entry);
 }
 free (str);

 return result;
}

/**
include_scan_filter:
scandir(3) filter for the directory `include` directive.

Exclude "." ".." and (if configured) hidden files/directories.

Return: 1 to include the file, 0 otherwise.
*/
static gint
include_scan_filter (const struct dirent *d)
{
 const gchar *p = d->d_name;

 if ((*p == '.' && (!p[1] || (p[1] == '.' && !p[2])))
     || (*p == '.' && conf_get_ignorehidden ()))
 {
  return 0; /* exclude */
 }
 return 1;
}

/**
include_directory_scan:
Recursively process an included directory.

@dir: `include=` directory dirpath.
@subdir: `include=` directory name.
@file_glob: glob string.
@dir_glob: glob string.

Returns: #Result.
*/
static enum Result
include_directory_scan (const gchar *dir,
                        const gchar *subdir,
                        const gchar *file_glob,
                        const gchar *dir_glob,
                        struct Entry *entry)
{
 enum Result result = ROK;
 struct dirent **namelist = NULL;
 gchar fulldirpath[MAX_PATH_LEN + 1];
 gint n;
 const ErrorList *head_error = entry->error;

 if (subdir[0])
 {
  n = snprintf (fulldirpath, sizeof fulldirpath, "%s/%s", dir, subdir);
 }
 else
 {
  n = snprintf (fulldirpath, sizeof fulldirpath, "%s", dir);
 }
 if (n < 0 || (gsize) n >= sizeof fulldirpath)
 {
  entry_push_error (entry, RFAIL, "'%1$s/%2$s': %3$s", dir, subdir,
                    strerror (ENAMETOOLONG));
  return RFAIL;
 }

 n = scandir (fulldirpath, &namelist, include_scan_filter, alphasort);
 if (dir_glob[0])
 {
  for (gint i = 0; i < n; i++) /* this loop */
  {
   gchar *name = namelist[i]->d_name;
   gchar fullpath[MAX_PATH_LEN + 1];
   gint r;

   r = snprintf (fullpath, sizeof fullpath, "%s/%s", fulldirpath, name);
   if (r < 0 || (gsize) r >= sizeof fullpath)
   {
    entry_push_error (entry, RWARN, "'%1$s/%2$s': %3$s", fulldirpath, name,
                      strerror (ENAMETOOLONG));
    continue;
   }
   if (is_path_to_dir (fullpath, namelist[i], NULL, TRUE)
       && access (fullpath, R_OK) == 0)
   {
    if (match_wildcard (dir_glob, name))
    {
     if (include_directory_match_glob
         (fulldirpath, name, file_glob, dir_glob, entry->menu_depth, entry))
     {
      gint saved_depth = entry->menu_depth;

      /* Enter directory unless max menu depth exceeded. */
      if (entry->menu_depth >= MAX_MENU_DEPTH)
      {
       result = RWARN;
      }
      else
      {
       enum Result scan_result = ROK;
       result =
       include_directory_enter_subdir (name, entry); /*++gl_menu_depth*/
       if (result >= RFAIL)
       {
        goto break_this_loop;
       }
       entry->menu_depth++;
       assert (gl_menu_depth == entry->menu_depth);
       if (entry->menu_depth < MAX_MENU_DEPTH)
       {
        scan_result =
        include_directory_scan (fulldirpath, name, file_glob,
                                entry->menu_depth == MAX_MENU_DEPTH - 1 ?
                                "" : "*", entry);
       }
       if (scan_result != ROK)
       {
        result = MAX (scan_result, result);
       }
      }
      assert (gl_menu_depth == entry->menu_depth);
#ifdef FEATURE_FORMATTING
      if (gl_fmtg_item[gl_menu_depth].fmt_sep != 0)
      {
        if (fmtg_init_formatting (&gl_fmtg_item[gl_menu_depth], "", 0) != 0)
        {
         result = RFAIL;
        }
      }
#endif

break_this_loop:
      gl_menu_depth = entry->menu_depth = saved_depth;
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
    }
   }
  } /* this loop */
 }

 for (gint i = 0; i < n; i++)
 {
  gchar *name = namelist[i]->d_name;
  gchar fullpath[MAX_PATH_LEN + 1];

  if (snprintf (fullpath, MAX_PATH_LEN, "%s/%s", fulldirpath,
                name) >= MAX_PATH_LEN)
  {
     entry_push_error (entry, RWARN, "'%1$s/%2$s': %3$s", fulldirpath, name,
                       strerror (ENAMETOOLONG));
     continue;
  }
  else if (is_path_to_reg (fullpath, namelist[i], NULL, FALSE)
           && access (fullpath, R_OK) == 0)
  {
   if (match_wildcard (file_glob, name))
   {
    gchar buf[member_size (struct Entry, cmd)], *cmd;
    gint r;

    r = snprintf (buf, sizeof buf, "%s%s\"%s\"", entry->cmd, entry->cmd[0] ?
                  " " : "", fullpath);
    if (r > 0 && (gsize) r < sizeof buf)
    {
     cmd = buf;
    }
    else
    {
     entry_push_error (entry, RFAIL, "at '%s': not enough room "
                       "to insert command", fullpath);
     cmd = NULL;
    }

    GtkWidget *widget;
    (void) entry_append_leaf_node (entry, name, cmd, &widget);
    if (widget != NULL)
    {
     if (entry_icon_is_to_render (entry))
     {
      if (entry->icon[0])
      {
       /* From `icon=` directive immediately after `include=/dirpath`. */
       result = entry_add_icon (entry, widget);
      }
      else
      {
       /* From scanned file's MIME-type. */
       strcpy (buf, entry->cmd);
       strcpy (entry->cmd, fullpath);
       result = entry_add_icon (entry, widget);
       strcpy (entry->cmd, buf);
      }
     }
     else
     {
      result = ROK;
     }
     if (result > RINFO)
     {
      entry_push_error (entry, result, "%s", fullpath);
     }
    }
    else
    {
     entry_push_error (entry, RFAIL, "%s", fullpath);
    }
   }
  }
 }
 for (gint i = 0; i < n; i++)
 {
  free (namelist[i]);
 }
 if (namelist != NULL)
 {
  free (namelist);
 }
 result = entry_get_max_error_level (entry, ROK, head_error);
 return result;
}

/**
include_directory_match_glob:
Test if glob matches any file under the included directory.

@dir: `include=` directory dirpath.
@subdir: `include=` directory name.
@file_glob: glob string.
@dir_glob: glob string.
@entry: pointer to struct to return errors.

Returns: TRUE if matched.
*/
static gboolean
include_directory_match_glob (const gchar *dir,
                              const gchar *subdir,
                              const gchar *file_glob,
                              const gchar *dir_glob,
                              guint depth,
                              struct Entry *entry)
{
 struct stat statbuf;
 gboolean found = FALSE;
 struct dirent **namelist;
 gchar fulldirpath[MAX_PATH_LEN + 1];
 gint n, r;

 if (subdir[0])
 {
  n = snprintf (fulldirpath, sizeof fulldirpath, "%s/%s", dir, subdir);
 }
 else
 {
  n = snprintf (fulldirpath, sizeof fulldirpath, "%s", dir);
 }
 if (n < 0 || (gsize) n >= sizeof fulldirpath)
 {
  entry_push_error (entry, RWARN, "'%1$s': %2$s", fulldirpath,
                    strerror (errno = ENAMETOOLONG));
  return FALSE;
 }

 n = scandir (fulldirpath, &namelist, include_scan_filter, alphasort);
 for (gint i = 0; i < n; i++)
 {
  const gchar *name = namelist[i]->d_name;
  gchar fullpath[MAX_PATH_LEN + 1];
  gboolean is_err = FALSE;

  r = snprintf (fullpath, sizeof fullpath, "%s/%s", fulldirpath, name);
  if (r < 0 || (gsize) r >= sizeof fullpath)
  {
   is_err = TRUE;
   entry_push_error (entry, RWARN, "'%1$s/%2$s': %3$s", fulldirpath, name,
                     strerror (errno = ENAMETOOLONG));
  }
  else if (stat (fullpath, &statbuf) == -1)
  {
   is_err = TRUE;
   entry_push_error (entry, RWARN, "'%1$s': %2$s", fullpath, strerror (errno));
  }
  if (is_err)
  {
   continue;
  }
  if (S_ISREG (statbuf.st_mode) && access (fullpath, R_OK) == 0)
  {
   if (match_wildcard (file_glob, name))
   {
    found = TRUE;
    break;
   }
  }

  if (!found && dir_glob[0] && S_ISDIR (statbuf.st_mode)
      && access (fullpath, R_OK) == 0)
  {
   if (match_wildcard (dir_glob, name))
   {
    if (depth < MAX_MENU_DEPTH -1)
    {
     found = include_directory_match_glob (fulldirpath, name, file_glob,
                                           dir_glob, ++depth, entry);
    }
    if (found)
    {
     break;
    }
   }
  }
 }
 for (gint i = 0; i < n; i++)
 {
  free (namelist[i]);
 }
 if (namelist) {
  free (namelist);
 }
 return found;
}

/**
include_directory_enter_subdir:
Crete a new submenu node for a subdirectory
entered during a directory `include=`.

@subdir: new submenu node label.
@entry: provides the menu depth and the error message pointer.
*/
static enum Result
include_directory_enter_subdir (const gchar *subdir,
                                struct Entry *entry)
{
#define SZ member_size (struct Entry, label)
 enum Result result = RFAIL;
 gint r = snprintf (entry->label, SZ, "%s", subdir);
 if (r > 0 && (gsize) r < SZ)
 {
  result = leave_submenu (entry);
  if (result < RFAIL)
  {
   gl_menu_depth++;
  }
 }
 return result;
#undef SZ
}

/**
a_eof:
Return from top-level script or from `include=` scripts.
*/
enum Result
a_eof (struct Entry *entry)
{
 enum Result result;
 if (gl_include_depth <= 1)
 {
  result = main_finale (entry);
 }
 else
 {
  result = entry_get_max_error_level (entry, ROK, NULL);
 }
 return result;
}

/**
core_get_menu:

Return: the (GtkMenu *) popup menu root.
*/
GtkMenu *
core_get_menu (void)
{
 return GTK_MENU (gl_menu_w[0]);
}

/**
core_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
core_module_init (void)
{
 gl_menu_w[0] = gtk_menu_new ();
 return gl_menu_w[0] ? 0 : -1;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
