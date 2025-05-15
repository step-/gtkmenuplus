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
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "common_gnu.h"
#include "conf.h"
#include "entry.h"
#include "logmsg.h"

/*********************************************************************
*                            `configure=`                            *
**********************************************************************/

#define MAX_ICON_SIZE 256 /* ceiling */
#define MIN_ICON_SIZE 8   /* floor   */

static guint    gl_conf_abspathparts = 2;
static gboolean gl_conf_endsubmenu = FALSE;
static gboolean gl_conf_errorconsoleonly = FALSE;
static gboolean gl_conf_formattinglocal = FALSE;
static gboolean gl_conf_icons = TRUE;
static gint     gl_conf_iconsize_height = 0;
static gint     gl_conf_iconsize_width = 0;
static gboolean gl_conf_launchelistfirst = FALSE;
static gboolean gl_conf_launchericons = TRUE;
static gboolean gl_conf_launchernodisplay = TRUE;
static gboolean gl_conf_launchernullcategory = TRUE;
static gboolean gl_conf_markup = TRUE;
static gboolean gl_conf_menuposition = FALSE;
static guint    gl_conf_menuposition_x = 0;
static guint    gl_conf_menuposition_y = 0;
static gboolean gl_conf_mnemonic = TRUE;
static gboolean gl_conf_ignorehidden = TRUE;
static enum conf_followsymlink_flags
                gl_conf_followsymlink = CONF_FOLLOWSYMLINK_FILE;

struct conf_keyword
{
 const gchar    *name;
 const guint     numarg;                       /* number of arguments */
 const gboolean  is_bool;                      /* boolean keyword */
 guint (*apply)  (const gchar *, const guint); /* keyword handler */
};

/**
conf_abspathparts:
conf_endsubmenu:
conf_errorconsoleonly:
conf_formattinglocal:
conf_icons:
conf_iconsize:
conf_launcherlistfirst:
conf_launchernodisplay:
conf_launchernullcategory:
conf_markup:
conf_menuposition:
conf_mnemonic:
Parse and set global configuration variables for the `configure=` directive.

@buf: input string.
@state, @numarg: integers, depend on the specific function.

Side effects: module variables.

Return: the value of @state or @numarg on success, a different value otherwise.
*/
static guint
conf_abspathparts (const gchar *buf,
                   const guint numarg)
{
 gint n = -1, ret = sscanf (buf, " %d", &n);

 if (n < 0 || ret == EOF || ret != (gint) numarg)
 {
  return 0;
 }
 gl_conf_abspathparts = n;
 return ret;
}

guint
conf_get_abspathparts ()
{
 return gl_conf_abspathparts;
}

/**
*/
guint
conf_menuposition (const gchar *buf,
                   const guint numarg)
{
 gint x = -1, y = -1, ret = sscanf (buf, " %d %d", &x, &y);

 if (x < 0 || y < 0 || ret == EOF || ret != (gint) numarg)
 {
  return 0;
 }
 gl_conf_menuposition_x = x;
 gl_conf_menuposition_y = y;
 gl_conf_menuposition = TRUE;
 return ret;
}

guint
conf_get_menuposition_x ()
{
 return gl_conf_menuposition_x;
}

guint
conf_get_menuposition_y ()
{
 return gl_conf_menuposition_y;
}

gboolean
conf_has_menuposition ()
{
 return gl_conf_menuposition;
}

/**
*/
guint
conf_iconsize (const gchar *buf,
               const guint numarg)
{
 gint sz = -1, ret = sscanf (buf, " %u", &sz);
 if (sz < 0 || ret == EOF || ret != (gint) numarg)
 {
  return 0;
 }
 /* "Gtkmenuplus restricts the requested size to a range of 8 to 256 pixels." */
 if (sz < MIN_ICON_SIZE)
 {
  sz = MIN_ICON_SIZE;
 }
 else if (sz > MAX_ICON_SIZE)
 {
  sz = MAX_ICON_SIZE;
 }
 if (logmsg_can_emit (RINFO))
 {
  gchar m[128];
  if (snprintf (m, sizeof m, "icon size = %d", sz) < (gint) sizeof m)
  {
   logmsg_emit (RINFO, m, NULL);
  }
 }
 gl_conf_iconsize_height = gl_conf_iconsize_width = sz;
 return ret;
}

guint
conf_get_iconsize_width ()
{
 return gl_conf_iconsize_width;
}

guint
conf_get_iconsize_height ()
{
 return gl_conf_iconsize_height;
}

/**
*/
guint
conf_endsubmenu (const gchar *buf __attribute__((unused)),
                 const guint state)
{
 return gl_conf_endsubmenu = state;
}

guint
conf_get_endsubmenu ()
{
 return gl_conf_endsubmenu;
}

/**
*/
static guint
conf_icons (const gchar *buf __attribute__((unused)),
            const guint state)
{
 return gl_conf_icons = state;
}

guint
conf_get_icons ()
{
 return gl_conf_icons;
}

/**
*/
static guint
conf_formattinglocal (const gchar *buf __attribute__((unused)),
                      const guint state)
{
 return gl_conf_formattinglocal = state;
}

guint
conf_get_formattinglocal ()
{
 return gl_conf_formattinglocal;
}

/**
*/
static guint
conf_launchericons (const gchar *buf __attribute__((unused)),
                    const guint state)
{
 return gl_conf_launchericons = state;
}

guint
conf_get_launchericons ()
{
 return gl_conf_launchericons;
}

/**
*/
static guint
conf_launchernodisplay (const gchar *buf __attribute__((unused)),
                        const guint state)
{
 return gl_conf_launchernodisplay = state;
}

guint
conf_get_launchernodisplay ()
{
 return gl_conf_launchernodisplay;
}

/**
*/
static guint
conf_launchernullcategory (const gchar *buf __attribute__((unused)),
                           const guint state)
{
 return gl_conf_launchernullcategory = state;
}

guint
conf_get_launchernullcategory ()
{
 return gl_conf_launchernullcategory;
}

/**
*/
static guint
conf_launcherlistfirst (const gchar *buf __attribute__((unused)),
                        const guint state)
{
 return gl_conf_launchelistfirst = state;
}

guint
conf_get_launcherlistfirst ()
{
 return gl_conf_launchelistfirst;
}

/**
*/
guint
conf_errorconsoleonly (const gchar *buf __attribute__((unused)),
                       const guint state)
{
 return gl_conf_errorconsoleonly = state;
}

guint
conf_get_errorconsoleonly ()
{
 return gl_conf_errorconsoleonly;
}

/**
*/
static guint
conf_markup (const gchar *buf __attribute__((unused)),
                       const guint state)
{
 return gl_conf_markup = state;
}

guint
conf_get_markup ()
{
 return gl_conf_markup;
}

/**
*/
static guint
conf_mnemonic (const gchar *buf __attribute__((unused)),
                       const guint state)
{
 return gl_conf_mnemonic = state;
}

guint
conf_get_mnemonic ()
{
 return gl_conf_mnemonic;
}

/**
*/
static guint
conf_ignorehidden (const gchar *buf __attribute__((unused)),
                   const guint state)
{
 return gl_conf_ignorehidden = state;
}

guint
conf_get_ignorehidden ()
{
 return gl_conf_ignorehidden;
}

/**
*/
static guint
conf_followsymlink (const gchar *buf,
                    const guint numarg)
{
 gint n = -1, ret = sscanf (buf, " %d", &n);

 if (n < 0 || ret == EOF || ret != (gint) numarg)
 {
  return 0;
 }
 gl_conf_followsymlink = n;
 return ret;
}

guint
conf_get_followsymlink ()
{
 return gl_conf_followsymlink;
}

static struct conf_keyword gl_conf_keywords [] =
{
 {"abspathparts",         1, 0, conf_abspathparts},
 {"menuposition",         2, 0, conf_menuposition},
 {"iconsize",             1, 0, conf_iconsize},
 {"endsubmenu",           0, 1, conf_endsubmenu},
 {"icons",                0, 1, conf_icons},
 {"formattinglocal",      0, 1, conf_formattinglocal},
 {"launchericons",        0, 1, conf_launchericons},
 {"launchernodisplay",    0, 1, conf_launchernodisplay},
 {"launchernullcategory", 0, 1, conf_launchernullcategory},
 {"launcherlistfirst",    0, 1, conf_launcherlistfirst},
 {"errorconsoleonly",     0, 1, conf_errorconsoleonly},
 {"markup",               0, 1, conf_markup},
 {"mnemonic",             0, 1, conf_mnemonic},
 {"ignorehidden",         0, 1, conf_ignorehidden},
 {"followsymlink",        1, 1, conf_followsymlink},
 {0,0,0,0},
};

/**
conf_apply_keywords_from_entry:

@entry: pointer to struct #Entry holding _dat, the
option string, and the error message buffer.

Side effects: sets module variables corresponding to the various options.

Returns: #Result.
*/
enum Result
conf_apply_keywords_from_entry (struct Entry *entry)
{
 const gchar    *s, *e, *m, *msg = "";
 const gchar     err_invalid[] = "invalid option";
 const gchar     err_numargs[] = "argument expected";
 const gchar    *cursor = NULL;  /* current keyword or argument */
 guint           arg2, ret;
 struct conf_keyword *k;
 enum Result result = RFAIL;

 for (cursor = s = entry->_dat; *s; )
 {
  /* Get word boundaries. */
  for (; *s && (*s == ' ' || *s == '\t'); s++)
   ;
  cursor = e = s;
  for (; *e && *e != ' ' && *e != '\t'; e++)
   ;

  /* Match keyword option. */
  result = RFAIL;
  for (guint i = 0; (k = &gl_conf_keywords[i])->name != NULL; i++)
  {
   m = STRCASESTR (s, k->name);                /* match ?        */

   if (m != NULL && (ptrdiff_t) (e - m) > 0)   /* matched inside */
   {
    const guint l = (ptrdiff_t) (e - s);       /* match length   */

    arg2 = k->is_bool ? TRUE : k->numarg;

    /* If match starts and ends on a word boundary. */
    if (m == s && (s[l] == '\0' || s[l] == ' ' || s[l] == '\t'))
    {
      result = ROK;
      break;
    }
    else if ( /* option can be negated with "no" */
              k->is_bool
              /* match offset into k->name */
              && (ptrdiff_t) (m - s) == 2
              /* match ends on word boundary       */
              && (m[l - 2] == '\0' || m[l - 2] == ' ' || m[l - 2] == '\t')
              /* match starts with "no"            */
              && (m[-2] == 'n' || m[-1] == 'o' || m[-2] == 'N' || m[-1] == 'O'))
    {
     arg2 = FALSE;
     result = ROK;
     break;
    }
   }
  }

  if (result >= RFAIL)
  {
   msg = err_invalid;
   break;
  }

  ret = (k->apply)(cursor = e + 1, arg2);
  if (ret != arg2)
  {
   result = RFAIL;
   if (k->numarg > 0)
   {
    msg = err_numargs;
   }
   break;
  }

  if (k->numarg > 0)
  {
   /* Advance e to the end of option arguments. */
   for (guint i = 0; i < k->numarg; i++)
   {
    for (; *e && (*e == ' ' || *e == '\t'); e++)
     ;
    for (; *e && *e != ' ' && *e != '\t'; e++)
     ;
   }
  }
  s = e;
  if (*e)
  {
   s++;
  }
 }

 if (result >= RFAIL)
 {
  result = RFAIL;
  entry_push_error (entry, result, "`configure=`: %s ~~> %s", msg, cursor);
 }
 return result;
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
