/*
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

#include "autoconfig.h"
#include "conf.h"
#include "core.h"
#include "entry.h"
#include "fmtg.h"

#define FMTG_SEPARATORS ";,"
#define FMTG_FMT_INIT_SZ member_size(struct Formatting, fmt_init)
#define FMTG_FMT_NEXT_SZ member_size(struct Formatting, fmt_next)

struct Formatting gl_fmtg_item[MAX_MENU_DEPTH] = {0};
struct Formatting gl_fmtg_tooltip = {0};

static regex_t gl_rgx_markup_nemo;

/**
a_format:
Set the global variables that apply formatting from the current menu depth down.
*/
enum Result
a_format (struct Entry *entry)
{
 const guint depth = gl_menu_depth;
 enum Result result = ROK;

 if (fmtg_init_formatting (&gl_fmtg_item[depth], entry->_dat, depth) != 0)
 {
  result = RFAIL;
  entry_push_error (entry, result, "%s",
                    "expanded format= too long or invalid");
 }
 return result;
}

#ifdef FEATURE_TOOLTIP
/**
*/
enum Result
a_tooltipformat (struct Entry *entry)
{
 struct Formatting *fmtg = &gl_fmtg_tooltip;
 enum Result result = ROK;

 if (fmtg_init_formatting (fmtg, "", 0) != 0)
 {
  result = RFAIL;
 }
 else if (entry->_dat[0] != '\0')
 {
  if (fmtg_init_formatting (fmtg, entry->_dat, gl_menu_depth) != 0)
  {
   result = RFAIL;
  }
 }
 if (result >= RFAIL)
 {
  entry_push_error (entry, result, "%s",
                    "expanded tooltip format too long or invalid");
 }
 return result;
}

#endif
/**
fmtg_init_formatting:
Initialize #Formatting struct from a `format=` directive.

@fmtg: pointer to the #Formatting struct to be initialized.
@format: string argument of a `format=` directive.
@menu_level: formatting scope.

Returns: 0 (OK), -1 (error).
*/
gint
fmtg_init_formatting (struct Formatting *fmtg,
                      const gchar *format,
                      const guint menu_level)
{
 gint r;

 /* fmt_init: initial printf format for `format=` directive */
 if (format[0] == '\0')
 {
  fmtg->fmt_init[0] = '\0';
 }
 else
 {
  r = snprintf (fmtg->fmt_init, FMTG_FMT_INIT_SZ, "%s", format);
  if ( r < 0 || (gsize) r >= FMTG_FMT_INIT_SZ)
  {
   return -1;
  }

  /* nemo_chars, nemo_byte_len: custom `format=mnemonic="chars"` */
  /* nemo_chars_idx: rotating pick index in nemo_chars */
  fmtg->nemo_byte_len = fmtg->nemo_chars_idx[menu_level] = 0;
  fmtg->nemo_chars = strstr (fmtg->fmt_init, "mnemonic=\"");
  if (fmtg->nemo_chars != NULL)
  {
   fmtg->nemo_chars += sizeof "mnemonic=\"" - 1;
   gchar *p = strchr (fmtg->nemo_chars, '"');
   if (p != NULL)
   {
    fmtg->nemo_byte_len = p - fmtg->nemo_chars;
   }
   if (p == NULL || fmtg->nemo_byte_len == 0)
   {
    /* reject unterminated/empty \"chars string */
    fmtg->nemo_byte_len = fmtg->nemo_chars[0] = '\0';
   }
  }

  /* Extract the first formatting value, and initialize a printf template. */
  r = snprintf (fmtg->fmt_next, FMTG_FMT_NEXT_SZ, "<span %s", fmtg->fmt_init);
  if (r > 0 && (gsize) r < FMTG_FMT_NEXT_SZ)
  {
   gchar *template = strpbrk(fmtg->fmt_next, FMTG_SEPARATORS);
   if (template == NULL)
   {
    template = strchr (fmtg->fmt_next, '\0');
   }
   gsize len = template - fmtg->fmt_next + 1;
   len = FMTG_FMT_NEXT_SZ - len;
   r = snprintf (template, len, ">%%s</span>");
   if (r < 0 || (gsize) r >= len)
   {
    return -1;
   }
  }
  else
  {
   return -1;
  }
 }

 /* Initialize a pointer to the `format=` list's next segment.
 It will be used when calling fmtg_next_formatting () to extract
 the next segment, and make a new printf template with it. */
 fmtg->fmt_next_end = strpbrk(fmtg->fmt_init, FMTG_SEPARATORS);
 if (fmtg->fmt_next_end != NULL)
 {
  /* found multiple `section`s => fmt_next will cycle */
  fmtg->fmt_sep = fmtg->fmt_next_end[0];
  fmtg->fmt_next_end[0] = '\0';
 }
 else
 {
  /* Found `format=single_format` => fmt_next will be constant. */
  fmtg->fmt_sep = '\0';
 }

 /* Set the menu scope. */
 fmtg->menu_level = menu_level;

 return 0;
}

/**
fmtg_next_formatting:
Update #Formatting struct and notably its fmt_next printf format.

@fmtg: pointer to the #Formatting to be updated.

Returns: 0(OK), -1(error).
*/
static gint
fmtg_next_formatting (struct Formatting *fmtg)
{
 gint r;

 if (fmtg->fmt_sep != '\0')
 {
  /* fmtg_chars consists of multiple `section`s. */
  gchar *next;

   /* for the committing `item`, `submenu`, `launcher` or `launchersub` */
  if (fmtg->fmt_next_end == NULL)
  {
   /* first time through */
   next = fmtg->fmt_init;
  }
  else
  {
   fmtg->fmt_next_end[0] = fmtg->fmt_sep;
   next = fmtg->fmt_next_end + 1;
  }
  fmtg->fmt_next_end = strchr(next, fmtg->fmt_sep);
  if (fmtg->fmt_next_end != NULL)
  {
   fmtg->fmt_next_end[0] = '\0';
  }
  /* next printf format to be used by fmtg_apply_formatting () */
  r = snprintf (fmtg->fmt_next, FMTG_FMT_NEXT_SZ, "<span %s>%%s</span>", next);
  if (r < 0 || (gsize) r >= FMTG_FMT_NEXT_SZ)
  {
   return -1;
  }
 }
 return 0;
}

/**
markup_whiteout_nemo:
Whiteout in place the first occurrence of "mnemonic="value", if any.

@s: target string.

Return: TRUE if the string was changed.
*/
static gboolean
markup_whiteout_nemo (gchar *s)
{
 gchar *p, *end;
 gint ret = s != NULL && (p = strstr (s, "mnemonic=")) != NULL
  && (end = strchr (p + sizeof ("mnemonic="), '"')) != NULL;

 if (ret)
 {
  while(p <= end)
   *(p++) = ' ';
 }
 return ret;
}

/**
markup_replace_nemo:
Replace, e.g., "<span mnemonic="H">Home</span>" with "<span>_Home</span>".
But more complex because `mnemonic="value"` follows several rules (man page).

@markup: string to be in-place modified.
@fmtg: pointer to #Formatting.
@depth: (sub)menu depth.

Returns: TRUE if mnemonic was replaced otherwise FALSE.
*/
static gboolean
markup_replace_nemo (gchar *markup,
                     struct Formatting *fmtg,
                     const guint depth)
{
 gchar *p;
 guint len;
 regmatch_t m[4];
 gboolean ret = regexec (&gl_rgx_markup_nemo, markup, 4, m, 0) == 0;

 if (ret)
 {
  /*
  Examine "value".
  */
  if (strncmp ("1", markup + m[2].rm_so, m[2].rm_eo - m[2].rm_so) == 0)
  {
   if (strchr (markup + m[3].rm_so, '_') == NULL)
   {
    /*
    Case: value="1" and markup doesn't include '_'.
    Method: write over 'mnemonic="value"' to insert '_' before the first
    non-white space character of the label m[3].

    Initial sample markup (with match group start/end):
       0s         1s             1e                                         0e
       <span      mnemonic="value" keep this part   >    an item label</span>
                            2s  2e                       3s          3e
    Overwrite m[1] with what follows up to m[3] excluded:
       <span       keep this part   >    his part   >    an item label</span>
    */
    p = markup + m[1].rm_so;
    len = m[3].rm_so - m[1].rm_eo;
    memmove (p, markup + m[1].rm_eo, len);
    /*
    Overwrite next character with '_':
       <span       keep this part   >    _is part   >    an item label</span>
    */
    *(p += len) = '_';
    /*
    Pull left the remainder of the line:
       <span       keep this part   >    _an item label</span>em label</span>
    and chop tail garbage:
     <span       keep this part   >    _an item label</span>
    */
    len = m[0].rm_eo - m[3].rm_so;
    memmove (++p, markup + m[3].rm_so, len);
    p[len] = '\0';
   }
   else
   {
    /*
    Case: m[3] already includes '_'.
    Method: whiteout (redundant) m[1].
    */
    for (gint i = m[1].rm_so; i < m[1].rm_eo; i++)
    {
     markup[i] = ' ';
    }
   }
  }
  else if (m[2].rm_eo > m[2].rm_so) /* non-empty "value" */
  {
   /*
   Case: value="ANY" (may override existing mnemonic).
   Method: prepend "_A ", "_N ", "_Y ", "_A ", ... to m[3] by menu level.

   Initial sample markup:
     <span      mnemonic="ANY"   keep this part   >    an item label</span>
   Overwrite m[1]:
     <span         keep this part   >    art   >  an item label</span>
   */
   p = markup + m[1].rm_so;
   len = m[3].rm_so - m[1].rm_eo;
   memmove (p, markup + m[1].rm_eo, len);
   /*
   Overwrite next character with '_':
     <span         keep this part   >    _rt   >  an item label</span>
   */
   *(p += len) = '_';
   /*
   Cycle through characters of "value" for the current menu level.
   Overwrite next character with the current "value" character plus ' ':
     <span         keep this part   >    _A    >  an item label</span>
   */
   /* First character (UTF-8 can be up to 4 bytes + null terminator). */
   gchar *utf8 = fmtg->nemo_chars + fmtg->nemo_chars_idx[depth];
   gint i = 0;
   do
   {
    *(++p) = utf8[i++];
   }
   while ((utf8[i] & 0xC0) == 0x80 && i < 4);
   *(++p) = ' ';
   fmtg->nemo_chars_idx[depth] =
    (fmtg->nemo_chars_idx[depth] + i) % fmtg->nemo_byte_len;
   /*
   Pull left the remainder of the line:
     <span         keep this part   >    _A an item label</span>/span>
   and chop tail garbage:
     <span         keep this part   >    _A an item label</span>
   */
   len = m[0].rm_eo - m[3].rm_so + 1;
   memmove (++p, markup + m[3].rm_so, len);
   p[len] = '\0';

  }
  else /* empty "value" */
  {
    /*
    Method: whiteout (useless) m[1].
    */
    for (gint i = m[1].rm_so; i < m[1].rm_eo; i++)
    {
     markup[i] = ' ';
    }
  }
 }
 return ret;
}

/**
fmtg_apply_formatting:
Apply formatting.

@fmtg: pointer to #Formatting.
@text: the text to be formatted.
@depth: @text's depth in the menu.
@marked: pointer to enum #fmtg_markup_type. Nullable.

Return: NULL on errors; a newly allocated string otherwise. The caller
owns the memory (g_free). The value assigned to *@marked reflects
whether the new string includes Pango Markup and the kind of Markup.
*/
static gchar *
fmtg_apply_formatting (struct Formatting *fmtg,
                       const gchar *text,
                       const guint depth,
                       enum fmtg_markup_type *marked)
{
 gchar *s, *markup = NULL; /* we return markup, and *marked */
 gboolean has_inline_fmtg = strstr (text, "<span") != NULL;
 gboolean has_outer_fmtg  = fmtg->fmt_init[0] &&
  (fmtg->fmt_sep == '\0' || depth == fmtg->menu_level);

 if (marked != NULL)
 {
  *marked = (has_inline_fmtg || has_outer_fmtg) ?
   FMTG_MARKUP_NORMAL : FMTG_MARKUP_NONE;
 }
 if (has_inline_fmtg)
 {
  markup = g_strdup (text);
  if (markup == NULL)
  {
   return NULL;
  }
  /* `<span mnemonic="value"` is invalid pango markup:    */
  /* whiteout nemo, if any, keeping the rest of the span. */
  (void) markup_whiteout_nemo (markup);

  if (has_outer_fmtg)
  {
   gchar *nemo = strstr (fmtg->fmt_next, "mnemonic=\"");
   if (nemo != NULL)
   {
    /*
    Replace nemo coming from `format_section` with '_' in result markup.
    */
    gchar *end = strchr (nemo + sizeof "mnemonic=\"", '"');
    if (end != NULL)
    {
     end[0] = '\0';
     /* Else markup is malformed and GTK will report it. */
    }
    s = g_strdup_printf ("<span %s\" %s", nemo, markup + sizeof "<span");
    g_free (markup);
    if (end != NULL)
    {
     end[0] = '"';
    }
    if (s == NULL)
    {
     return NULL;
    }
    if (markup_replace_nemo (s, fmtg, depth) && marked != NULL)
    {
     *marked = FMTG_MARKUP_MNEMONIC;
    }

    /* Apply formatting. */
    markup = g_strdup_printf (fmtg->fmt_next, s);
    g_free(s);
    if (markup != NULL)
    {
     /* erase duplicates (rare) */
     (void) markup_whiteout_nemo (markup);
    }
   }
   else
   {
    /* Apply formatting. */
    markup = g_strdup_printf (fmtg->fmt_next, s = markup);
    g_free (s);
   }
  }
 }
 else if (has_outer_fmtg)
 {
  /* Apply formatting. */
  markup = g_markup_printf_escaped (fmtg->fmt_next, text);
  if (markup != NULL && fmtg->nemo_chars != NULL)
  {
   if (markup_replace_nemo (markup, fmtg, depth) && marked != NULL)
   {
    *marked = FMTG_MARKUP_MNEMONIC;
   }
  }
 }
 else
 {
  markup = g_strdup (text);
  if (markup == NULL)
  {
   return NULL;
  }
 }

 if (has_outer_fmtg)
 {
  if (fmtg_next_formatting (fmtg) != 0)
  {
   g_free (markup);
   markup = NULL;
  }
 }

 return markup;
}

/**
fmtg_format_widget_label:

@widget: pointer to #GtkWidget menu widget holding the label child.

Returns: 0(OK) -1(error)
*/
gint
fmtg_format_widget_label (GtkWidget *widget)
{
 gint ret = 0;
 GtkWidget *child = gtk_bin_get_child ((GtkBin *) widget);
 const gchar *label = gtk_label_get_label ((GtkLabel *) child);
 const gboolean is_submenu =
 gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget)) != NULL;
 const gint depth  = MAX (0, (gint) gl_menu_depth - (is_submenu ? 1 : 0));
 enum fmtg_markup_type marked = FMTG_MARKUP_NONE;
 gchar *markup =
 fmtg_apply_formatting (&gl_fmtg_item[depth], label, depth, &marked);

 if (markup != NULL)
 {
  if (marked == FMTG_MARKUP_NONE || !conf_get_markup ())
  {
   if (conf_get_mnemonic ())
   {
    gtk_label_set_text_with_mnemonic ((GtkLabel*) child, markup);
   }
   else
   {
    gtk_label_set_text ((GtkLabel *) child, markup);
   }
  }
  else
  {
   if (conf_get_mnemonic ())
   {
    gtk_label_set_markup_with_mnemonic ((GtkLabel *) child, markup);
   }
   else
   {
    gtk_label_set_markup ((GtkLabel *) child, markup);
   }
  }
  g_free (markup);
 }
 else
 {
  ret = -1;
  /* with initial label still set */
 }
 return ret;
}

#ifdef FEATURE_TOOLTIP
/**
fmtg_format_widget_tooltip:

@widget: pointer to #GtkWidget menu widget holding a tooltip child.

Returns: 0(OK) -1(error)
*/
gint
fmtg_format_widget_tooltip (GtkWidget *widget)
{
 gint ret = 0;
 gchar *tooltip = gtk_widget_get_tooltip_text (widget);
 if (tooltip != NULL)
 {
  const gboolean is_submenu =
  gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget)) != NULL;
  enum fmtg_markup_type marked = FMTG_MARKUP_NONE;
  gchar *markup = fmtg_apply_formatting (&gl_fmtg_tooltip, tooltip,
                                         is_submenu ? 1 : 0, &marked);
  if (markup != NULL)
  {
   if (marked != FMTG_MARKUP_NONE && conf_get_markup ())
   {
    gtk_widget_set_tooltip_markup (widget, markup);
   }
   g_free (markup);
  }
  g_free (tooltip);
 }
 return ret;
}

#endif
/**
fmtg_module_init:
*/
gint
fmtg_module_init (void)
{
 return (compile_regex
         (&gl_rgx_markup_nemo,
          "[[:space:]]+(mnemonic=\"([^\"]*)\")[^>]*>[ \t]*([^<]*)</span>",
          REG_EXTENDED)) ? -1 : 0;
}

/**
fmtg_module_finalize:
*/
void
fmtg_module_finalize (void)
{
 regfree (&gl_rgx_markup_nemo);
}

/* vim:set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
