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

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <time.h>
#include "autoconfig.h"
#ifdef FEATURE_ACTIVATION_LOG
#include "activ_log.h"
#endif
#include "conf.h"
#include "core.h"
#include "comment.h"
#include "common.h"
#ifdef FEATURE_FORMATTING
#include "fmtg.h"
#endif
#include "entry.h"
#include "entry_cmd.h"
#include "entry_icon.h"
#ifdef FEATURE_VARIABLE
#include "entry_var.h"
#endif
#ifdef FEATURE_SERIALIZATION
#include "json.h"
#endif
#ifdef FEATURE_CONDITIONAL
#include "if.h"
#endif
#include "input.h"
#ifdef FEATURE_LAUNCHER
#include "launcher.h"
#include "launcher_cache.h"
#endif
#include "logmsg.h"
#include "param.h"
#include "path.h"
#include "main.decl.h"

/* All lowercase! */
#define PROGNAME "gtkmenuplus"

#define VERSION_TEXT VERSION ""

#define LOCK_NAME "gtkmenuplus_lockfile"

/* Parameter or Variable Reference Symbol */
#define PV_REF_SYM        "$"
#define PV_REF_SYM_LEN    (sizeof (PV_REF_SYM) - 1)

gchar gl_on_exit[MAX_LINE_LENGTH + 1] = "";
gboolean gl_opt_json_serialize __attribute__((unused)) = FALSE;
const gchar *gl_progname = PROGNAME;
gint  gl_argc;
gchar **gl_argv;

static gboolean gl_opt_force = FALSE;
static gboolean gl_gtk_is_initialized = FALSE;
static gboolean gl_display_the_menu = TRUE;

int
main (int argc, gchar *argv[])
{
 gl_argv = argv;
 gl_argc = argc;
 if (argc > 1 && strcmp (argv[1], "--version") == 0)
 {
  opt_version ();
  /* NOT REACHED */
 }

 gl_gtk_is_initialized = gtk_init_check (&argc, &argv);
 if (!gl_gtk_is_initialized)
 {
  fprintf (stderr, "%s: cannot initialize GTK.\n", gl_progname);
  exit (EXIT_FAILURE);
 }

 /* Function main_options could exit the program. */
 gint last_optindex = main_options (argc, argv) - 1;

 /* Need exactly 1 argument. */
 if (argc == last_optindex + 1)
 {
  opt_help ();
  exit (EXIT_FAILURE);
 }
 /* Now argv[1] is the input source and menu parameters start at 2. */

 /* Find PROGNAME.desktop and look up icon in .../icons/hicolor/... */
 g_set_prgname (PROGNAME);
 gtk_window_set_default_icon_name (PROGNAME);

 struct module_call
 {
  gint (*init)(void);
  void (*finalize)(void);
  const gchar *name;
 };
 const struct module_call require_modules[] = {
  {core_module_init,           NULL,                       "core"},
  {entry_module_init,          entry_module_finalize,      "entry"},
  {entry_cmd_module_init,      entry_cmd_module_finalize,  "entry_cmd"},
  {entry_icon_module_init,     entry_icon_module_finalize, "entry_icon"},
#ifdef FEATURE_VARIABLE
  {entry_var_module_init,      entry_var_module_finalize,  "entry_var"},
#endif
  {logmsg_module_init,         NULL,                       "logmsg"},
  {path_module_init,           path_module_finalize,       "path"},
#ifdef FEATURE_FORMATTING
  {fmtg_module_init,           fmtg_module_finalize,       "fmtg"},
#endif
#ifdef FEATURE_LAUNCHER
  {NULL,                       launcher_module_finalize,   "launcher"},
#endif
  {NULL,                       comment_module_finalize,    "comment"},
#ifdef FEATURE_ACTIVATION_LOG
  {activation_log_module_init, NULL,                       "activation_log"},
#endif
 };

 for (gint i = 0; i < (gint) G_N_ELEMENTS (require_modules); i++)
 {
  const struct module_call *p = require_modules + i;
  if (p->init && p->init ())
  {
   gchar buf[128];
   snprintf (buf, sizeof buf, "cannot initialize the \"%s\" module.", p->name);
   die (buf);
  };
 }
#ifdef FEATURE_LAUNCHER
 if (launcher_module_init (0))
 {
  die ("cannot initialize the \"launcher\" module");
 }
#endif
 if (comment_module_init (0))
 {
  die ("cannot initialize the \"comment\" module");
 }
 gl_menu_counter = gl_menu_depth = 0;
 *gl_script_dir =  *gl_icon_dir;

 struct InputSource isource = {0};
 /* pathname, stdin "-" or directive (invalid paths follow directive)*/
 isource.fname = argv[last_optindex + 1];

 struct ErrorContext errctx = {0};
 struct Entry *entry = entry_new_tracked ();
 if G_UNLIKELY (entry == NULL)
 {
  perror (__FUNCTION__);
  exit (EXIT_FAILURE);
 }
 entry->errctx = &errctx;
 entry->isrc = &isource;
 gboolean cont = input_source_open (&isource, entry);
 if (cont)
 {
  cont =
  path_get_base_dir (gl_icon_dir, isource.fname, isource.fp == NULL, entry);
  if (ENTRY_ERROR (entry->error)->level >= RFAIL)
  {
   entry_set_error_level (entry, RFATAL);
  }
 }
 entry_emit_error (entry);
 entry_free_error (entry);
 if (!cont)
 {
  exit (EXIT_FAILURE);
 }

 strcpy (gl_script_dir, gl_icon_dir);
#ifdef FEATURE_LAUNCHER
 launcher_set_launcherdirectory (gl_script_dir);
#endif

 /* Initialize icon size from GTK. */
 {
  gint            w, h;
  gchar           buf[16];
  gsize           sz;
  gboolean        success;

  success = gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &w, &h)
   && (sz = snprintf (buf, 16, "%u", w)) < 16
   && conf_iconsize (buf, 1) == 1;
  if (!success)
  {
   conf_iconsize ("32", 1);
  }
 }

 enum Result result =
 main_loop (&isource, argc - last_optindex - 1,
            (const gchar **) argv + last_optindex + 1, 0, NULL);

#ifdef FEATURE_ACTIVATION_LOG
 (void) activation_log_module_finalize ();
#endif
 for (gint i = G_N_ELEMENTS (require_modules) - 1; i >= 0; i--)
 {
  const struct module_call *p = require_modules + i;
  if (p->finalize)
  {
   p->finalize ();
  }
 }
 return result == ROK ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
disable_popup_menu:
This is a final menu state, there is no enabling back the menu.

After entering this state the program keeps parsing lines and reporting
errors but it will not display the menu. It is a subjective design choice
for a specific error to call this function. The aim is not to display an
excessively crippled menu.
*/
static void
disable_popup_menu ()
{
 /**
 The program keeps going towards main_finale(), which may itself
 inhibit displaying the menu. By contrast, a function returning
 RFATAL to main_loop() will make the program bail out immediately.
 */
 gl_display_the_menu = FALSE;
 if (logmsg_can_emit (RINFO))
 {
  logmsg_emit (RINFO, "disabling popup menu due to breaking error", NULL);
 }
}

/**
main_loop:
Parse menu description file or included files (`include=`) and render the
resulting GTK menu.

@isrc: pointer to struct #InputSource holding open handle on file/stdin or argv.
@argc: @argv index limit.
@argv: command-line argv or `include=` parameter vector.
@menu_depth_base: current menu depth on entering this function.
@overrides: pointer to struct #Entry holding possible overrides. Nullable.

This function is indirectly recursive.
Typically @overrides is NULL on the first call (@isrc) and can be non-NULL for
subsequent calls, which are initiated by `include` directives.

Returns: #Result.
*/
enum Result
main_loop (struct InputSource *isrc,
           const gint argc __attribute__((unused)),
           const gchar *argv[] __attribute__((unused)),
           const guint menu_depth_base,
           const struct Entry *overrides)
{
 enum Result result;
 struct ErrorContext errctx = {1, NULL, TRUE};
 struct Entry *entry = entry_new ();
 entry_init (entry, NULL, LINE_UNDEFINED, ENTRY_FLAG_NONE, 0);
 entry->isrc = isrc;
 entry->errctx = &errctx;

 if (++gl_include_depth > NESTED_INCLUDE_LIMIT)
 {
  entry_push_error (entry, RFAIL /*sic*/,
                    "`include=%s` recursive include limit (%d) reached",
                    isrc->fname, NESTED_INCLUDE_LIMIT);
  entry_emit_error (entry);
  entry_free_error (entry);
  disable_popup_menu ();
  return RWARN /*sic*/;
 }
#ifdef FEATURE_CONDITIONAL
 else
 {
  /* Fails only if gl_include_depth overran or g_ptr_array_new failed. */
  gboolean if_context_pushed __attribute__((unused)) = if_context_push_new ();
  assert (if_context_pushed);
 }
#endif

#ifdef FEATURE_PARAMETER
 struct Params params;

 {
  const gchar *param0 = isrc->fp != NULL ? isrc->fname : "$0";

  if (!param_array_init (&params, argc - 1, argv + 1, param0))
  {
   return RFATAL;
  }
 }
#endif

 gboolean is_override_directive_allowed = TRUE;
 if (overrides != NULL)
 {
  if (overrides->cmd[0])
  {
   is_override_directive_allowed = FALSE;
  }
#ifdef FEATURE_TOOLTIP
  if (overrides->tooltip[0])
  {
   is_override_directive_allowed = FALSE;
  }
#endif
 }
 if (!is_override_directive_allowed)
 {
#ifdef FEATURE_TOOLTIP
  entry_push_error (entry, RWARN, "`include=`'%s': `cmd=` or `tooltip=`"
                    " is only allowed after `include=`<directory>",
                    isrc->fname);
#else
  entry_push_error (entry, RWARN, "`include=`'%s': `cmd=`"
                    " is only allowed after `include=`<directory>",
                    isrc->fname);
#endif
  entry_emit_error (entry);
  entry_free_error (entry);
 }

#ifdef FEATURE_CONDITIONAL
 gboolean skip_if_body = FALSE;
#else
 gboolean skip_if_body = TRUE;
#endif

 /***************
 *  MAIN LOOP  *
 ***************/
 while (TRUE)
 {
  /* Read next line and get linetype. */

  const struct Directive *directive;
  guint indent_level = 0;

#ifdef FEATURE_CONDITIONAL
  skip_if_body = !(if_get_is_active () ? if_get_evaluates_body () : TRUE);
#endif
  entry->flags = ENTRY_FLAG_NONE;
  entry->index = 0;
  result = ROK;
  directive = input_fetch_next_directive (isrc, entry,
                                          &indent_level, skip_if_body);
#ifdef DEBUG_PRINT_INPUT
  if (logmsg_can_emit (RDEBUG))
  {
   gint off = (gl_menu_depth + gl_include_depth) * 2;
   gchar *p = isrc->raw;
   while (*p)
   {
    switch (*p)
    {
     case ' ':  off +=1; break;
     case '\t': off +=2; break;
     default:   goto out;
    }
    p++;
   }
out:
   fprintf (stderr, "% 4d:%*s%s\n", isrc->lineno, off, "", p);
   if (!directive->niladic)
   {
    fprintf (stderr, "    :%*s%s\n", off, "",
             isrc->cooked + strspn (isrc->cooked, " \t"));
   }
  }
#endif
  if (directive == NULL)
  {
   /*
   In release builds, a NULL directive can only happen when FEATURE_VARIABLE
   is not defined. In development builds, NULL can also indicate that a new
   directive cannot be associated with its a_* parsing function. The assertion
   in input_fetch_directive_from_linetype guards against such case.
   */
   /* Emit error immediately because of the continue statement below. */
   entry_push_error (entry, RFAIL, "%s", "NULL directive");
   entry_emit_error (entry);
   entry_free_error (entry);
   continue;
  }
  if (is_linetype_bad (directive->type))
  {
   result = RFAIL;
   /* Emit error immediately because of the continue statement below. */
   entry_push_error (entry, result, "%s", directive->keyword);
   entry_emit_error (entry);
   entry_free_error (entry);

   disable_popup_menu ();
   continue;                    /* keep error checking the rest of the script */
  }
  entry->directive = directive; /* valid or LINE_UNDEFINED */
  entry->_dat = (gchar *) entry->isrc->cooked;

  indent_level += menu_depth_base;

#ifdef FEATURE_CONDITIONAL
  if (if_get_is_active () && !if_get_evaluates_body () &&
      !(directive->type & (LINE_GROUP_IF | LINE_EOF)))
  {
   continue;
  }
#endif

  if (gl_display_the_menu)
  {
   /* Process errors about pending block directives. */
   /* Only block directives define an entry->leave function. */
   if (directive->block_check)
   {
    result = entry_leave (entry, overrides);
    if (result >= RFAIL)
    {
     if (entry->directive->type == LINE_INCLUDE
         && !(entry->flags & ENTRY_FLAG_INCLUDE_DIR))
     {
      disable_popup_menu ();
     }
    }
    if (entry->error)
    {
     entry_emit_error (entry);
     entry_free_error (entry);
    }
   }
  }
  if (!conf_get_endsubmenu () && directive->enable_indent_rule)
  {
   if (indent_level > gl_menu_depth)
   {
    if (gl_display_the_menu)
    {
     result = RFAIL;
     /* Emit error immediately because of the continue statement below. */
     entry_push_error (entry, result, "`%s=`: wrong indentation",
                       entry->directive->keyword);
     entry_emit_error (entry);
     entry_free_error (entry);

     disable_popup_menu ();
     continue;                  /* keep error checking the rest of the script */
    }
   }
   else if (indent_level < gl_menu_depth)
   {
    while (indent_level < gl_menu_depth)
    {
#ifdef FEATURE_FORMATTING
     if (gl_fmtg_item[gl_menu_depth].fmt_sep == '\0')
     {
      if (fmtg_init_formatting (&gl_fmtg_item[gl_menu_depth], "", 0) != 0
          && result < RFAIL)
      {
       result = RFAIL;
       disable_popup_menu ();
      }
     }
#endif
     --gl_menu_depth;                     /* close submenu */
    }
   }
  }

  /* Process errors about invalid directives. */
  if (entry->error != NULL)
  {
   entry_emit_error (entry);
   entry_free_error (entry);
  }

  if (gl_display_the_menu)
  {
#if defined(FEATURE_PARAMETER) || defined(FEATURE_VARIABLE)
   /*
   Generally, directives expand variables and parameters. However,
   the `cmd=` and `variable==` directives expand variables but not
   parameters to avoid interfering with shell positional parameters.
   */
   gboolean no_expand_param_refs = (directive->type == LINE_CMD);
   if (directive->type == LINE_SET_VARNAME)
   {
    gchar *p = strchr (entry->_dat, '=');
    no_expand_param_refs = p && p[1] == '='; /* `varname==` */
   }

   if (directive->type != LINE_EOF
       && strchr (entry->_dat, PV_REF_SYM[0]) != NULL)
   {
#ifdef FEATURE_PARAMETER
    const struct Params *p = &params;
#else
    const struct Params *p = NULL;
#endif

#ifdef FEATURE_CONDITIONAL
    entry->is_isolated_reference = FALSE;
#endif
    result = entry_expand_params_vars (entry, p, no_expand_param_refs);
    if (result >= RWARN)
    {
     /* entry->_dat remains unexpanded; carry on but report the error. */

     if (entry->error)
     {
      entry_emit_error (entry);
      entry_free_error (entry);
     }
    }
   }
#endif

   if (directive->type == LINE_EOF)
   {
    if (entry->error)
    {
     entry_emit_error (entry);
     entry_free_error (entry);
    }
    if (isrc->fp != NULL)
    {
     fclose (isrc->fp);
    }
    free (isrc->_buf);
   }
  }

  if (gl_display_the_menu && directive->analyze != NULL)
  {
   result = directive->analyze (entry);
   /* enter_submenu and enter_launchersubmenu do gl_menu_depth++ */
  }

  if (entry->error)
  {
   entry_emit_error (entry);
   entry_free_error (entry);
  }

  if (result >= RFATAL)
  {
   entry_init (entry, NULL, LINE_UNDEFINED, ENTRY_FLAG_RESET_ERROR, 0);
   break;
  }
  if (directive->type == LINE_EOF)
  {
#ifdef FEATURE_CONDITIONAL
   if (if_get_is_active () && result < RFAIL)
   {
    if (logmsg_can_emit (RWARN))
    {
     logmsg_emit (RWARN, "`endif` expected before [end of input]", entry);
    }
   }
   if_context_pop ();
#endif
   break;
  }

  /* main_finale() will pop up the menu */
 }

 g_clear_pointer (&errctx.block_line, g_free);
#ifdef FEATURE_PARAMETER
 param_array_destroy (&params);
#endif
 gl_include_depth--;
 entry_free (entry);
 return result;
}

/**
menu_position_cb:
call back routine for gtk_menu_popup
*/
void
menu_position_cb (GtkMenu *menu __attribute__((unused)),
                  gint *x,
                  gint *y,
                  gboolean *push_in,
                  gpointer data __attribute__((unused)))
{
 /* "Gtkmenuplus confines the requested position to default monitor borders." */
 gint w, h;
 gint cx, cy;

#if !GTK_CHECK_VERSION(3,0,0)
 gdk_window_get_geometry (gdk_get_default_root_window (), NULL, NULL, &w, &h,
                          NULL);
#else
 gdk_window_get_geometry (gdk_get_default_root_window (), NULL, NULL, &w, &h);
#endif
 cx = conf_get_menuposition_x ();
 cy = conf_get_menuposition_y ();
 *x = cx > w ? w : cx;
 *y = cy > h ? h : cy;
 *push_in = FALSE;
 if (logmsg_can_emit (ROK))
 {
  gchar m[128];
  if (snprintf (m, sizeof m, "menu position = %d, %d", *x, *y) < (int) sizeof m)
  {
   logmsg_emit (ROK, m, NULL);
  }
 }
}

/**
entry_activate:
Run the command of the activated menu entry. Callback.

@entry: pointer to struct #Entry.

Side effects: exits the main loop therefore must emit errors directly.
*/
void
entry_activate (struct Entry *entry)
{
 static struct Entry *e = NULL;
 gchar cmd[member_size (struct Entry, cmd)];
 const gchar *scheme;
 struct stat st;
 GAppInfo *app_info = NULL;
 GFile *file = NULL;
 GError *error = NULL;
 /* Note that "launched" !=> "did run" because g_app_info_launch hands off the
 command to dbus, which is unaware whether the command did actually run. */
 gboolean launched = FALSE;

 if (e == NULL)
 {
  if ((e = entry_new_tracked ()) == NULL)
  {
   perror (__FUNCTION__);
   return;
  }
 }
#ifdef FEATURE_ACTIVATION_LOG
 strcpy (e->icon, strcmp (entry->icon, "NULL") == 0 ? "" : entry->icon);
 if (entry_activationlog_write (entry, e) != 0)
 {
  if (entry->error)
  {
   if (logmsg_can_emit (RWARN))
   {
    logmsg_emit (RWARN, ENTRY_ERROR (entry->error)->message, NULL);
   }
   entry_free_error (entry);
  }
  logmsg_emit (RWARN, "`activationlogfile=`: no log", NULL);
 }
#endif
 if (logmsg_can_emit (ROK))
 {
  gchar *m = g_strdup_printf ("\brun command: %s", entry->cmd);
  logmsg_emit (ROK, m, NULL);
  g_free (m);
 }
 {
  guint err;
  gchar *cmdx = path_expand_full (entry->cmd, gl_script_dir, &err);
  if (cmdx != NULL)
  {
   strcpy (e->cmd, cmdx);
   free (cmdx);
  }
  else
  {
   strcpy (e->cmd, entry->cmd);
  }
  if (err)
  {
   if (logmsg_can_emit (RFAIL))
   {
    logmsg_emit (RFAIL, "expanded command too long", NULL);
   }
   goto out;
  }
 }
 unsetenv ("GTK2_RC_FILES");

 input_copy_first_word (cmd, e->cmd);
 scheme = g_uri_peek_scheme (cmd);
 if (scheme && strcmp (scheme, "file") == 0)
 {
  /* Cut the chase! */
  scheme = NULL;
  strcpy (cmd, e->cmd + sizeof "file://" - 1);
  strcpy (e->cmd, cmd);
 }

 if (scheme)
 {
  GList *uris;

  app_info = g_app_info_get_default_for_uri_scheme (scheme);
  if (app_info == NULL)
  {
   entry_push_error (e, RFAIL, "the '%s' URI scheme "
                     "is not associated with any application", scheme);
   entry_emit_error (e);
   entry_free_error (e);
  }
  else
  {
   uris = g_list_append (NULL, cmd);
   launched =
   g_app_info_launch_uris (app_info, uris, NULL, &error) && error == NULL;
   g_list_free (uris);
  }
  goto out;
 }

 /* existing, non-executable path || directory */
 if ((stat (cmd, &st) == 0) && (S_ISDIR (st.st_mode) || access (cmd, X_OK)))
 {
  if (!entry_cmd_info_new_for_type (e, cmd, &app_info, &file))
  {
   goto out;
  }
  else if (app_info != NULL && file != NULL)
  {
   /* Note: using g_spawn_command_line_async() instead of g_app_info_launch()
   because the latter prefixes file's path with "file://" assuming that file's
   MIME-type default application can handle the prefix. For Fatdog64 Linux, at
   least, that assumption is too broad. Many Fatdog64 MIME-type handlers are
   simple scripts activated directly by ROX-Filer. */
   const gchar *run = g_app_info_get_executable (app_info);

   gchar *p = g_strdup_printf ("%s %s", run, e->cmd);
   launched = g_spawn_command_line_async (p, &error);
   g_free (p);
   goto out;
  }
 }

 /* executable path || command in $PATH || fallback for all rubbish */
 launched = g_spawn_command_line_async (e->cmd, &error);

out:
 if (app_info != NULL)
 {
  g_object_unref (app_info);
 }
 if (file != NULL)
 {
  g_object_unref (file);
 }
 if (error != NULL)
 {
  launched = FALSE;
  if (logmsg_can_emit (RFAIL))
  {
   logmsg_emit (RFAIL, error->message, NULL);
  }
  g_error_free (error);
 }
 if (!launched && logmsg_can_emit (RFAIL))
 {
  logmsg_emit (RFAIL, "command launch failed", NULL);
 }
 gtk_main_quit ();
}

/**
menu_deactivate_cb:
*/
static void
menu_deactivate_cb (gpointer data __attribute__((unused)))
{
 if (logmsg_can_emit (ROK))
 {
  logmsg_emit (ROK, "menu was deactivated", NULL);
 }
 gtk_main_quit ();
 (void) do_onexit ();
}

/**
lockfile_test_set:
Non-blocking test for another running instance.

Side effects: create and leave the lock file in $XDG_RUNTIME_DIR.

Returns: 0 = Lock file not found, 1 = Lock file found, 2 = Error.
*/
static guint
lockfile_test_set (void)
{
 struct flock fl;
 gint fd;
 gchar *dir;
 gchar lock_path[MAX_PATH_LEN];

 dir = getenv ("XDG_RUNTIME_DIR");
 if (dir == NULL || dir[0] == '\0')
 {
  if (logmsg_can_emit (RFAIL))
  {
   logmsg_emit (RFAIL, "XDG_RUNTIME_DIR is not set", NULL);
  }
  return 2;
 }
 (void) snprintf (lock_path, sizeof lock_path, "%s/%s", dir, LOCK_NAME);
 fd = open (lock_path, O_RDWR | O_CREAT, 0600);
 if (fd < 0)
 {
  if (logmsg_can_emit (RFAIL))
  {
   logmsg_emit (RFAIL, strerror (errno), NULL);
  }
  return 2;
 }
 fl.l_start = fl.l_len = 0;
 fl.l_type = F_WRLCK;
 fl.l_whence = SEEK_SET;
 if (fcntl (fd, F_SETLK, &fl) < 0)
 {
  if (logmsg_can_emit (RWARN))
  {
   logmsg_emit (RWARN, strerror (errno), NULL);
  }
  return 1;
 }
 return 0;
}

/**
lockfile_unlock:
Unlock the path locked by lockfile_test_set().

Returns: fcntl()'s return value.
*/
static gint
lockfile_unlock (void)
{
 gint ret = 0;
 gchar *dir = getenv ("XDG_RUNTIME_DIR");

 if (dir != NULL && dir[0] != '\0')
 {
  struct flock fl;
  gint fd;
  gchar lock_path[MAX_PATH_LEN];

  (void) snprintf (lock_path, sizeof lock_path, "%s/%s", dir, LOCK_NAME);
  fd = open (lock_path, O_RDWR);
  if (fd >= 0)
  {
   fl.l_start = fl.l_len = 0;
   fl.l_type = F_WRLCK;
   fl.l_whence = SEEK_SET;
   ret = fcntl (fd, F_UNLCK, &fl);
   close (fd);
  }
 }
 return ret;
}

#if defined(FEATURE_PARAMETER) || defined(FEATURE_VARIABLE)
/**
entry_expand_params_vars:
Expand parameter and variable references.

@entry: pointer to struct #Entry providing the input/output string ._dat
@params: pointer to #Params struct holding the existing parameter values.
@no_expand_param_refs: pass TRUE to skip parameter expansion.

Return: #Result, expanded input in entry->_dat, modifies
entry->is_isolated_reference, and can push error an error.
Set entry->is_isolated_reference TRUE if the input string consists only
of a valid parameter or variable reference without any surrounding text
(except white space and a comment) that yields an actual, valid expansion.
*/
static enum Result
entry_expand_params_vars (struct Entry *entry,
                          const struct Params *params,
                          const gboolean no_expand_param_refs)
{
 gchar outbuf[SIZEOF_COOKED] = "";          /* output buffer */
 gchar *out = outbuf;                       /* output cursor */
 guint offset;                              /* offset before advancing cursor */
 const gchar *in;                           /* input cursor at PV_REF_SYM     */
 const guint max_size = sizeof outbuf;
 guint off;
#ifdef FEATURE_CONDITIONAL
 gboolean is_isolated;
#else
 gboolean is_isolated __attribute__((unused));
#endif
 enum Result result;
 gchar *_dat = entry->_dat;

 if ((in = strstr (_dat, PV_REF_SYM)) == NULL)
 {
  is_isolated = FALSE;
  result = ROK;
  goto out;
 }

 offset = (ptrdiff_t) (in - _dat);
 if (offset > 0)
 {
  /* Copy head text */
  out = stpncpy (out, _dat, offset);
  out[0] = '\0';
  is_isolated = FALSE;   /* some text comes before the reference */
 }
 else
 {
  is_isolated = TRUE;    /* presumed */
 }

 result = ROK;

 do
 {
  const gchar *pi = in + PV_REF_SYM_LEN;
  gchar *po = out;
  gboolean was_expanded = FALSE;
  gboolean is_digit = isdigit (pi[0]);

#ifdef FEATURE_PARAMETER
  if (is_digit)
  {
   if (no_expand_param_refs)
   {
    /* Copy unexpanded parameter literally. */
    /* Fall into copying PV_REF_SYM because !was_expanded. */
    ;
   }
   else
   {
    gboolean found = FALSE;

    result = expand_param (&pi, &po, max_size - offset, &found, params, entry);
    if (result >= RFAIL)
    {
     /* Max size enforced; error reported; return with found == FALSE */
     goto out;
    }
    was_expanded = was_expanded || found;
   }
  }
#endif

#ifdef FEATURE_VARIABLE
  if (!is_digit && var_name_test (pi))
  {
   gboolean found = FALSE;

   result = entry_var_expand (entry, &pi, &po, max_size - offset, &found);
   if (result >= RFAIL)
   {
    /* Max size enforced; error reported; return with found == FALSE */
    goto out;
   }
   was_expanded = was_expanded || found;
  }
#endif

  /* expand_{param,var} advanced the input and output cursors (pi, po) */
  in = pi;
  if (was_expanded)
  {
   offset += (ptrdiff_t) (po - out);
   out  = po;
  }
  else
  {
   /* Not a parameter / variable: copy PV_REF_SYM literally. */

   if ((off = PV_REF_SYM_LEN) >= max_size - offset)
   {
    goto size_error;
   }

   out    = stpcpy (out, PV_REF_SYM);
   out[0] = '\0';
   offset   += PV_REF_SYM_LEN;
  }

  if (*in)
  {
   gchar *next_in = strstr (in, PV_REF_SYM);

   is_isolated = FALSE;     /* some text follows the reference */

   if (next_in != NULL)
   {
    off = (ptrdiff_t) (next_in - in);

    if (off > 0)
    {
     /* Copy text segment that separates the next reference. */
     if (off >= max_size - offset)
     {
      goto size_error;
     }
     out = stpncpy (out, in, off);
     out[0] = '\0';
     offset += off;
    }
   }
   else
   {
    /* Copy tail. */
    off = strlen (in);
    if (off >= max_size - offset)
    {
     goto size_error;
    }
    strcpy (out, in);
   }
   in = next_in;
  }
 } while (in != NULL && *in);
 goto out;

size_error:
 result = RFAIL;
 is_isolated = FALSE;     /* by decree */
 entry_push_error (entry, result,
                   "while expanding parameter or variable: line too long");

out:
#ifdef FEATURE_CONDITIONAL
 entry->is_isolated_reference = is_isolated;
#endif
 if (result < RFAIL)
 {
  strcpy (_dat, outbuf);
 }
 return result;
}

#endif

/**
entry_leave:
Call a directive's leave function to create a GTK menu entry widget.

@entry: pointer to struct #Entry holding the data for the widget.
@overrides: pointer to struct #Entry holding possible overrides. Nullable.

Return: #Result. This function sets entry->leave to NULL.
*/
static enum Result
entry_leave (struct Entry *entry,
             const struct Entry *overrides)
{
 enum Result result = ROK;

 if (entry->leave != NULL)
 {
  if (overrides != NULL)
  {
   /* If `icon=` is allowed immediately after some block directives, e.g.,
   `include=dirpath`. See the scripting guide for which are such directives. */
   if (entry->icon[0] == '\0' && overrides->icon[0] != '\0')
   {
    strcpy (entry->icon, overrides->icon);
   }
  }
  result = entry->leave (entry);
  entry->leave = NULL;
 }
 return result;
}

/**
do_onexit:
`onexit=` command runner. Runs on menu deactivation.

Returns: system()'s return value.
*/
static gint
do_onexit ()
{
 (void) lockfile_unlock ();
 if (gl_on_exit[0] == '\0')
 {
  return 0;
 }
 return system (gl_on_exit);
}

/**
prune_empty_branches:
Prune the empty branches of a #GtkMenu.
*/
static void
prune_empty_branches (GtkMenu *menu)
{
 GList *children = gtk_container_get_children (GTK_CONTAINER (menu));

 for (GList *iter = children; iter; iter = iter->next)
 {
  GtkWidget *w = GTK_WIDGET (iter->data);
  GtkWidget *submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (w));
  if (submenu && GTK_IS_MENU (submenu))
  {
   prune_empty_branches (GTK_MENU (submenu));

   GList *submenu_children =
   gtk_container_get_children (GTK_CONTAINER (submenu));
   if (submenu_children == NULL)
   {
    gtk_container_remove (GTK_CONTAINER (menu), w);
   }
   g_list_free (submenu_children);
  }
 }
 g_list_free (children);
}

/**
*/
enum Result
main_finale (const struct Entry *entry)
{
 enum Result result = ROK;

 if (!gl_display_the_menu)
 {
  result = RFAIL;
  goto out;
 }

 GtkMenu *the_menu = core_get_menu ();
 if (gl_menu_counter > 0)
 {
  prune_empty_branches (the_menu);
#ifdef FEATURE_SERIALIZATION
  if (gl_opt_json_serialize)
  {
   dump_menu_as_json (the_menu);
  }
#endif
 }
 GList *l = gtk_container_get_children (GTK_CONTAINER (the_menu));
 gboolean menu_is_empty = l == NULL;
 g_list_free (l);
 clock_t time1 = 0, time2 = 0;
 gchar m[256];
 if (logmsg_can_emit (ROK))
 {
  time1 = (clock () * 1000) / CLOCKS_PER_SEC;

  if (snprintf (m, sizeof m, "%'lu ms to process the script", time1) < (int)
      sizeof m)
  {
   logmsg_emit (ROK, m, NULL);
  }
  if (menu_is_empty)
  {
   logmsg_emit (ROK, "empty menu", NULL);
  }
 }
 if (menu_is_empty)
 {
  goto out;
 }
#ifdef FEATURE_SERIALIZATION
  if (gl_opt_json_serialize)
  {
   goto out;
  }
#endif

 switch (lockfile_test_set ())
 {
  case 1:
   if (!gl_opt_force)
   {
    result = RFAIL;
    if (logmsg_can_emit (RFAIL))
    {
     logmsg_emit (result, "another instance is running", entry);
     goto out;
    }
   }
   break;
  case 2:
   /* lockfile_test_set already reported errors. */
   result = RFAIL;
   goto out;
   break;
 }

 gtk_widget_show_all (GTK_WIDGET (the_menu));
 g_signal_connect_swapped (the_menu, "deactivate",
                           G_CALLBACK (menu_deactivate_cb), NULL);

 while (!gtk_widget_get_visible (GTK_WIDGET (the_menu)))
 {
  usleep (50000);
  UNDEPR (gtk_menu_popup, the_menu, NULL, NULL, conf_has_menuposition ()?
          (GtkMenuPositionFunc) menu_position_cb : NULL, NULL, 0,
          gtk_get_current_event_time ());
  gtk_main_iteration ();
 }
 if (logmsg_can_emit (ROK))
 {
  gdk_display_flush (gdk_display_get_default ());
  time2 = (clock () * 1000) / CLOCKS_PER_SEC - time1;
  gchar *egtc =
  g_format_size (entry_get_tracked_count () * sizeof (struct Entry));
  gchar *lcec =
  g_format_size (launcher_cache_entry_count () * sizeof (struct Entry));
  if (snprintf (m, sizeof m, "%'lu ms to display the menu", time2) < (int)
      sizeof m)
  {
   logmsg_emit (ROK, m, NULL);
  }
  if (snprintf (m, sizeof m,
                "menu: item=%1$u submenu=%2$u;"
                " track: entry=%3$u (%4$s);"
                " cache: entry=%5$u (%6$s) scandir=%7$u;",
                gl_menu_widgets, gl_submenu_widgets,
                entry_get_tracked_count (), egtc,
                launcher_cache_entry_count (), lcec,
                launcher_cache_scandir_count ()) < (int)
      sizeof m)
  {
   logmsg_emit (ROK, m, NULL);
  }
  g_free (egtc);
  g_free (lcec);
 }
 gtk_main ();

out:
 return result;
}

/**********************************************************************
*                         Parse command line                         *
**********************************************************************/

/*
gl_usage_text is used both in the terminal and in a GTK_DIALOG.
The OPTIONS section is pre-formatted for alignment in a GTK_DIALOG.
Specifically, pay attention to the occurrences of \t+ vs a single \t.
Function opt_help changes \t+ to \t to print gl_usage_text with
the correct alignment for terminal output.
*/

/* gl_ruler is used to stretch the GTK_DIALOG wide
enough for the "Usage:" and "or:" lines not to wrap. */
static const gchar *gl_ruler =
"__________________________________________________________________________";

/* gl_usage_text is also used to generate the manual page with
help2man. Pay attention to help2man's format requirements. */
static const gchar *gl_usage_text =
"Usage:\tgtkmenuplus [OPTIONS] FILE [PARAMETER...]"
"\n   or:\tgtkmenuplus [OPTIONS] DIRECTIVE[';'DIRECTIVE...] [PARAMETER...]"
"\n\nGenerate a GTK popup menu from text directives."
"\nHome page: <" PACKAGE_URL ">."
"\nRefer to the gtkmenuplus(5) manual page for menu scripting help."
"\n\nOptions:"
"\nUse \"-\" as the FILE name to read from stdin."
"\n"
"\n  -d, --delimiter C\t\tUse character C as the DIRECTIVE delimiter."
"\n  -f, --force      \t\tShow the menu even if another instance is showing."
"\n  -h, --help       \t\tDisplay this help text and exit."
#ifdef FEATURE_SERIALIZATION
"\n  -j, --json       \t\tDump the menu as a JSON object and exit."
#endif
"\n  -L, --license    \t\tDisplay license information and exit."
"\n  -q, --quiet      \t\tRepeat -q to tweak verbose messages."
"\n  -v, --verbose    \t\tRepeat -v to increase verbosity."
"\n  --version        \t\tDisplay version and exit."
"\n  --               \t\t\tEnd of options marker.";

static const gchar *gl_license_text =
"Copyright (C) 2016-2025 step - this program"
"\nCopyright (C) 2013 Alan Campbell - the fork of MyGtkMenu"
"\nCopyright (C) 2004-2011 John Vorthman - MyGtkMenu"
"\nGtkmenuplus comes with ABSOLUTELY NO WARRANTY."
"\nGNU GPLv2 license applies; see the COPYING file.";

static gint
main_options (int argc,
              char *argv[])
{
 static struct option long_options[] = {
  {"delimiter", required_argument, 0, 'd'},
  {"force",     no_argument,       0, 'f'},
  {"help",      no_argument,       0, 'h'},
  {"info",      no_argument,       0, 'i'},
  {"json",      no_argument,       0, 'j'},
  {"license",   no_argument,       0, 'L'},
  {"quiet",     no_argument,       0, 'q'},
  {"verbose",   no_argument,       0, 'v'},
  {"version",   no_argument,       0, 0},
  {"z--1",      no_argument,       0, 0},
  {0, 0, 0, 0}
 };

 while (TRUE)
 {
  gint option_index = 0;
  gint c = getopt_long (argc, argv, "d:fhijLqv", long_options, &option_index);
  if (c == -1)
  {
   break;
  }
  switch (c)
  {
  case '-':
   break;
  case 'd':
   opt_delimiter (optarg);
   break;
  case 'f':
   opt_force ();
   break;
  case 'h':
   opt_help ();
   break;
  case 'j':
#ifdef FEATURE_SERIALIZATION
   gl_opt_json_serialize = TRUE;
#else
   fprintf (stderr, "%s: warning: option --json is not built-in\n",
            gl_progname);
#endif
   break;
  case 'L':
   opt_license ();
   break;
  case 'q':
   opt_quiet ();
   break;
  case 'i':                            /*UNDOCUMENTED *//*DEPRECATED */
  case 'v':
   opt_verbose ();
   break;
  case 0:
   if (strcmp (long_options[option_index].name, "version") == 0)
   {
    opt_version ();
   }
   else if (strcmp (long_options[option_index].name, "z--1") == 0)
   {
    opt_z__1 ();                       /*UNDOCUMENTED *//*INTERNAL */
   }
   break;
  default:                             /* '?' ':' */
   opt_help ();
   exit (EXIT_FAILURE);
  }
 }
 return optind;
}

/**
opt_delimiter:
Change the delimiter character that separates command-line directives.

@arg: command-line argument that comes after the option.

Returns: TRUE if @arg holds a valid delimiter.
*/
static unsigned
opt_delimiter (const char *arg)
{
 if (arg != NULL)
 {
  input_set_directive_separator (arg[0]);
  return TRUE;
 }
 return FALSE;
}

/**
opt_force:
Do not create or test the lock file.
*/
static void
opt_force (void)
{
 gl_opt_force = TRUE;
}

/**
opt_help:
Show the help dialog, print help text to stdout and exit.
*/
static void
opt_help (void)
{
 if (!isatty (fileno (stdin)) && gl_gtk_is_initialized)
 {
  GtkWidget *widget =
  gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                          "\n%s\n\n%s\n\n%s",
                          gl_usage_text, gl_ruler, gl_license_text);
  if (widget != NULL)
  {
   gtk_window_set_title (GTK_WINDOW (widget), gl_progname);
   gtk_window_set_default_icon_name (gl_progname);
   gtk_dialog_run (GTK_DIALOG (widget));
   gtk_widget_destroy (widget);
  }
 }
 opt_z__1 ();
 /*NOT REACHED*/
}

/**
opt_z__1:
Print help text to stdout and exit.
This entry point is also used to build the manual page with help2man.
*/
static void
opt_z__1 (void)
{
 guchar prev = 0;

 for (const gchar *p = gl_usage_text; *p; p++)
 {
  if (prev != '\t' || *p != '\t')
  {
   putchar (*p);
  }
  prev = *p;
 }
 putchar ('\n');
 exit (EXIT_SUCCESS);
}

/**
opt_license:
Print license text and exit.
*/
static void
opt_license (void)
{
 printf ("%s\n", gl_license_text);
 exit (EXIT_SUCCESS);
}

/**
opt_verbose:
Increase message verbosity.
*/
static void
opt_verbose (void)
{
 logmsg_set_verbosity (logmsg_get_verbosity () + 1);
}

/**
opt_quiet:
Disable (error) message output.

1 : suppress all console messages including non-fatal errors.
2 : message dialogs too.
*/
static void
opt_quiet (void)
{
 const guint level = logmsg_get_quiet () + 1;
 logmsg_set_quiet (level);
 if (level == 2)
 {
  conf_errorconsoleonly ("", TRUE);
 }
}

/**
opt_version:
Print version string and exit.
*/
static void
opt_version (void)
{
 printf ("%s\n", VERSION_TEXT);
 exit (EXIT_SUCCESS);
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
