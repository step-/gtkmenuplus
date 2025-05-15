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

#include <gdk/gdk.h>
#include "conf.h"
#include "entry.h"
#include "input.h"
#include "logmsg.h"
#include "main.h"
#include "path.h"

/* labels for enum Result */
static const gchar* gl_verbosity_label[] =
{
 "ok",
 "info",
 "warning",
 "error",
 "fatal error",
 "debug",
};

static guint gl_quiet = 0;
static guint gl_verbosity = 0;

/**
logmsg_get_verbosity:
Get output verbosity level.

Returns: level.
*/
guint
logmsg_get_verbosity ()
{
 return gl_verbosity;
}

/**
logmsg_set_verbosity:
Set output verbosity level.

@verbosity: level, non-negative integer.
*/
void
logmsg_set_verbosity (const guint value)
{
 gl_verbosity = value;
}

/**
logmsg_get_quiet:
*/
guint
logmsg_get_quiet ()
{
 return gl_quiet;
}

/**
logmsg_set_quiet:
*/
void
logmsg_set_quiet (const guint value)
{
 gl_quiet = value;
}

/**
logmsg_str_shorten:
Shorten @str in place by replacing "{}/" for the menu script directory path.

@str: if string starts with '\b' it is shifted by -1 and not replaced.
*/
void
logmsg_str_shorten (gchar *str)
{
 assert (str && *str);
 if (gl_script_dir[0])
 {
  const guint sl = strlen (str);
  const guint dl = strlen (gl_script_dir);
  if G_UNLIKELY (*str == '\b')
  {
   memmove (str, str + 1, sl);
   return;
  }
  if (sl > dl && dl > 3)
  {
   assert (gl_script_dir[dl - 1] == G_DIR_SEPARATOR);
   for (gchar *p = str; (p = strstr (str, gl_script_dir)); )
   {
    guint len = sl - dl - (p - str);
    memcpy (p, "{}/", 3);
    memmove (p + 3, p + dl, len);
    p[3 + len] = '\0';
   }
  }
 }
}

/**
logmsg_can_emit:
Return whether an (error) message can be emitted.

@level: message level. The message can be emitted according to this table:
  ----------------
  -q  -v    @level
  ----------------
   *   0+   RFATAL
   2-  0+   RFAIL
   2-  1+   ROK
   2-  2+   RINFO
   2-  3+   RWARN
   *   4+   RDEBUG
*/
gboolean
logmsg_can_emit (const enum Result level)
{
 const guint v = logmsg_get_verbosity ();
 const guint q = logmsg_get_quiet ();
 const gboolean can_emit =
 level == RFATAL || (q < 3 && (
  level == RFAIL || (v >= 1 && level == ROK)
  || (v >= 2 && level == RINFO) || (v >= 3 && level == RWARN)))
  || (v >= 4 && level == RDEBUG);
 return can_emit;
}

/**
logmsg_emit:
Present a message to stderr and as a dialog.

@result: #Result message level (error severity).
@message: NUL-terminated message string NOT ending with '\n'.
@entry: #Entry struct pointer providing message context. NULLABLE.
*/
void
logmsg_emit (enum Result result,
             const gchar *message,
             const struct Entry *entry)
{
 static gboolean dialog_is_disabled = FALSE;
 gchar ctx[MAX_LINE_LENGTH] = "";
 gsize ctx_len = 0;
 const gboolean show_lines = result > RINFO &&
 logmsg_get_verbosity () && logmsg_get_quiet () < 1;
 /*
 %location: either the script file path or "-" (for stdin) or
  the current token of the command-line list of directives.
 %from: context line range start. See CONTEXT.
 %from_line: input line or token corresponding to %from.
 %to: context line range end. See CONTEXT.
 %to_line: input line or token corresponding to %to.

CONTEXT:

The full output format is:
  '['@result']' <label>': '%message' @'%location': '%from'-'%to
  <line %from>
  <line %to>

The format is cut down automatically, to reduce output noise, by these rules:

1. Print <line %from> and <line %to> only if -v and not -q and @result > RINFO.
   Typically, %from is the line number of the last-seen block directive
   while %to is the current input line number.
2. If %from == 0 && %to == 0 omit '@'%location and the <line *>s.
3. If %to == 0 print %location but omits the <line *>s.
 */
 const gchar *location;
 guint from;
 const gchar *from_line = "";
 guint to;
 const gchar *to_line;

 if (entry)
 {
  location = entry->isrc->fp ? entry->isrc->fname : "";
  if (entry->errctx)
  {
   from = entry->errctx->block_lineno;
   from_line = entry->errctx->block_line;
  }
  else
  {
   from = 0;
  }
  to_line = input_fetch_current_line (entry->isrc, &to);
 }
 else
 {
  location = to_line = "";
  from = to = 0;
 }

 /* Format line range context. */
 {
#define SZ (sizeof ctx - sizeof (FMT) - 1)

  if (from == 0 && to == 0)
  {
   if (to_line && *to_line)
   {
#define FMT "@%s"
    ctx_len = snprintf (ctx, SZ, FMT, to_line);
#undef FMT
   }
   else
   {
    ctx[0] = '\0';
   }
  }
  else if (to == 0)
  {
#define FMT "@%s"
   ctx_len = snprintf (ctx, SZ, FMT, location);
#undef FMT
  }
  else if (to_line != NULL)
  {
   if (from == to)
   {
    if (show_lines)
    {
#define FMT "@%s:%d\n% 4d: %s"
     ctx_len = snprintf (ctx, SZ, FMT, location, to, to, to_line);
#undef FMT
    }
    else
    {
#define FMT "@%s:%d"
     ctx_len = snprintf (ctx, SZ, FMT, location, to);
#undef FMT
    }
   }
   else
   {
    if (from)
    {
     if (show_lines)
     {
#define FMT "@%s:%d-%d\n% 4d: %s\n% 4d: %s"
      ctx_len =
      snprintf (ctx, SZ, FMT, location, from, to, from, from_line, to, to_line);
#undef FMT
     }
     else
     {
#define FMT "@%s:%d-%d"
      ctx_len = snprintf (ctx, SZ, FMT, location, from, to);
#undef FMT
     }
    }
    else
    {
     if (show_lines)
     {
#define FMT "@%s:%d-%d\n% 4d: %s"
      ctx_len = snprintf (ctx, SZ, FMT, location, to, to, to, to_line);
#undef FMT
     }
     else
     {
#define FMT "@%s:%d-%d"
      ctx_len = snprintf (ctx, SZ, FMT, location, to, to);
#undef FMT
     }
    }
   }
  }
  else
  {
   if (show_lines)
   {
#define FMT "@%s:%d-%d\n% 4d: %s"
    ctx_len = snprintf (ctx, SZ, FMT, location, from, to, from, from_line);
#undef FMT
   }
   else
   {
#define FMT "@%s:%d-%d"
    ctx_len = snprintf (ctx, SZ, FMT, location, from, to);
#undef FMT
   }
  }
#undef SZ
 }

 /* Report message. */
 {
  gchar *buf = strdup (message);
  if (buf)
  {
   logmsg_str_shorten (buf);
  }
  /* Always report to stderr. */
  fprintf (stderr, "[%1$d] %2$s: %3$s%4$s%5$s%6$s", result,
           gl_verbosity_label[result], buf ? buf : message, *ctx ? " " : "",
           ctx, !ctx_len || ctx[ctx_len - 1] != '\n' ? "\n" : "");

  /* Also display a message dialog for severe errors. */
  if (result >= RFAIL && !dialog_is_disabled
   /* unless a terminal is attached */
   && !isatty (fileno (stdin))
   /* unless `configure=errorconsoleonly` */
   && !conf_get_errorconsoleonly ())
  {
   static guint counter = 0;
   GtkWidget *dialog =
   gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
                           GTK_BUTTONS_YES_NO, "\n%s: %s %s\n\n(%d) %s",
                           gl_verbosity_label[result],
                           buf != NULL ? buf : message, ctx, ++counter,
                           "Suspend these messages?");
   gtk_window_set_title (GTK_WINDOW (dialog), gl_progname);
   gtk_window_set_default_icon_name (gl_progname);
   if (dialog)
   {
    gint response = gtk_dialog_run (GTK_DIALOG(dialog));
    dialog_is_disabled = response == GTK_RESPONSE_YES;
    gtk_widget_destroy (dialog);
   }
  }
  free (buf);
 }
}

/**
logmsg_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
logmsg_module_init (void)
{
 if (conf_get_errorconsoleonly () && logmsg_can_emit (ROK))
 {
  logmsg_emit (ROK, "message dialog disabled: "
               "messages are sent only to stderr", NULL);
 }
 return 0;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
