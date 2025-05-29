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

#include <gtk/gtk.h>
#include "entry.h"
#include "main.h"

#define IW 2 /* indentation multiplier */

/**
append_unicode_escape:
Encode a single Unicode codepoint as \uXXXX (hex).
*/
static void
append_unicode_escape (char *buf,
                       size_t *pos,
                       uint16_t codepoint)
{
 static const char hex[] = "0123456789abcdef";
 buf[(*pos)++] = '\\';
 buf[(*pos)++] = 'u';
 buf[(*pos)++] = hex[(codepoint >> 12) & 0xF];
 buf[(*pos)++] = hex[(codepoint >> 8) & 0xF];
 buf[(*pos)++] = hex[(codepoint >> 4) & 0xF];
 buf[(*pos)++] = hex[codepoint & 0xF];
}

/**
utf8_decode:
Decode one UTF-8 codepoint from string s.

Return: codepoint and advance *@i; -1 on invalid UTF-8.
*/
static int32_t
utf8_decode (const char *s,
             size_t len,
             size_t *i)
{
 unsigned char c = (unsigned char) s[*i];
 if (c < 0x80)
 {
  (*i)++;
  return c;
 }
 else if ((c >> 5) == 0x6 && *i + 1 < len)
 {
  unsigned char c1 = (unsigned char) s[*i + 1];
  if ((c1 & 0xC0) != 0x80)
  {
   return -1;
  }
  int32_t cp = ((c & 0x1F) << 6) | (c1 & 0x3F);
  *i += 2;
  return cp;
 }
 else if ((c >> 4) == 0xE && *i + 2 < len)
 {
  unsigned char c1 = (unsigned char) s[*i + 1];
  unsigned char c2 = (unsigned char) s[*i + 2];
  if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80)
  {
   return -1;
  }
  int32_t cp = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
  *i += 3;
  return cp;
 }
 else if ((c >> 3) == 0x1E && *i + 3 < len)
 {
  unsigned char c1 = (unsigned char) s[*i + 1];
  unsigned char c2 = (unsigned char) s[*i + 2];
  unsigned char c3 = (unsigned char) s[*i + 3];
  if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
  {
   return -1;
  }
  int32_t cp = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) |
   (c3 & 0x3F);
  *i += 4;
  return cp;
 }
 return -1;
}

/**
utf8_to_json:
Convert a UTF-8 string to a JSON string with proper escaping.

Return: a newly allocated string. The caller owns the memory.
*/
static char *
utf8_to_json (const char *input)
{
 size_t out_pos = 0;
 size_t i = 0;
 size_t len = strlen (input);
 /* Allocate output buffer: worst case 6x input size + 3 for quotes and '\0'. */
 const size_t max_out = len * 6 + 3;
 char *out = malloc (max_out);
 if (!out)
 {
  perror (__FUNCTION__);
  return NULL;
 }
 out[out_pos++] = '"';
 while (i < len)
 {
  int32_t cp = utf8_decode (input, len, &i);
  if (cp < 0)
  {
   /* Replace invalid UTF-8 byte with U+FFFD. */
   cp = 0xFFFD;
   i++;
  }
  /* Escape according to JSON spec. */
  if (cp == '\"')
  {
   out[out_pos++] = '\\';
   out[out_pos++] = '"';
  }
  else if (cp == '\\')
  {
   out[out_pos++] = '\\';
   out[out_pos++] = '\\';
  }
  else if (cp == '\b')
  {
   out[out_pos++] = '\\';
   out[out_pos++] = 'b';
  }
  else if (cp == '\f')
  {
   out[out_pos++] = '\\';
   out[out_pos++] = 'f';
  }
  else if (cp == '\n')
  {
   out[out_pos++] = '\\';
   out[out_pos++] = 'n';
  }
  else if (cp == '\r')
  {
   out[out_pos++] = '\\';
   out[out_pos++] = 'r';
  }
  else if (cp == '\t')
  {
   out[out_pos++] = '\\';
   out[out_pos++] = 't';
  }
  else if (cp < 0x20)
  {
   /* Escape control characters as \uXXXX. */
   append_unicode_escape (out, &out_pos, (uint16_t) cp);
  }
  else if (cp <= 0xFFFF)
  {
   if (cp >= 0x80)
   {
    // Escape non-ASCII BMP character as \uXXXX. */
    append_unicode_escape (out, &out_pos, (uint16_t) cp);
   }
   else
   {
    out[out_pos++] = (char) cp; /* printable ASCII */
   }
  }
  else if (cp <= 0x10FFFF)
  {
   /* Encode as UTF-16 surrogate pair \uXXXX\uXXXX. */
   cp -= 0x10000;
   uint16_t high_surrogate = 0xD800 + (cp >> 10);
   uint16_t low_surrogate = 0xDC00 + (cp & 0x3FF);
   append_unicode_escape (out, &out_pos, high_surrogate);
   append_unicode_escape (out, &out_pos, low_surrogate);
  }
  else
  {
   /* Replace invalid codepoint with U+FFFD. */
   append_unicode_escape (out, &out_pos, 0xFFFD);
  }
 }
 out[out_pos++] = '"';
 out[out_pos] = '\0';
 return out;
}

/**
dump_menu_tree:
Print #GtkMenu object as JSON to stdout.

@iw: indentation level.
*/
static void
print_menu_as_json (GtkMenu *menu,
                    const guint iw)
{
 GList *children = gtk_container_get_children (GTK_CONTAINER (menu));
 const guint count = g_list_length (children);
 const guint off = iw * IW;
 gchar *json = NULL;

 printf ("\n%*s\"count\": %d", off , "", count);
 if (count)
 {
  printf (",\n%*s\"children\": [", off , "");
  for (GList *iter = children; iter; iter = iter->next)
  {
   GtkWidget *w = GTK_WIDGET (iter->data);
   GtkWidget *submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (w));
   gboolean is_separator = GTK_IS_SEPARATOR_MENU_ITEM (w);

   const struct Entry *eptr;
   const gchar *label = NULL;
   const gchar *tooltip = NULL;
   gchar *atooltip = NULL;

   eptr = (struct Entry *) g_object_get_data (G_OBJECT (w), "entry");
   if (eptr != NULL)
   {
    label = eptr->label;
#if FEATURE_TOOLTIP
    tooltip = eptr->tooltip;
#endif
   }
   if (label == NULL)
   {
    label = gtk_menu_item_get_label (GTK_MENU_ITEM (w));
   }
   assert (label);
#if FEATURE_TOOLTIP
   if (tooltip == NULL)
   {
    tooltip = atooltip = gtk_widget_get_tooltip_text (w);
   }
#endif

   printf ("\n%*s{", off + IW, "");
   printf ("\n%*s\"label\": %s", off + 2 * IW, "",
           (label[0] ? json = utf8_to_json (label) : "\"\""));
   g_clear_pointer (&json, free);
   if (label[0] == 0)
   {
    if (is_separator)
    {
     printf (",\n%*s\"is_separator\": true", off + 2 * IW, "");
    }
   }
#if FEATURE_TOOLTIP
   if (tooltip && *tooltip && *tooltip != ENTRY_DISALLOW_DIRECTIVE)
   {
    json = utf8_to_json (tooltip);
    printf (",\n%*s\"tooltip\": %s", off + 2 * IW, "", json);
    g_clear_pointer (&json, free);
   }
   g_free (atooltip);
#endif
   if (eptr && eptr->icon[0] && eptr->icon[0] != ENTRY_DISALLOW_DIRECTIVE)
   {
    if (eptr->icon[0] == ENTRY_NULL_ICON)
    {
     printf (",\n%*s\"icon\": null", off + 2 * IW, "");
    }
    else
    {
     json = utf8_to_json (eptr->icon);
     printf (",\n%*s\"icon\": %s", off + 2 * IW, "", json);
     g_clear_pointer (&json, free);
     printf (",\n%*s\"icon_size\": %u", off + 2 * IW, "", eptr->icon_size);
    }
   }
   if (!is_separator)
   {
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    if (gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (w)) != NULL)
    {
     printf (",\n%*s\"icon_rendered\": true", off + 2 * IW, "");
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;
   }
   if (eptr && eptr->cmd[0] && eptr->cmd[0] != ENTRY_DISALLOW_DIRECTIVE)
   {
    json = utf8_to_json (eptr->cmd);
    printf (",\n%*s\"cmd\": %s", off + 2 * IW, "", json);
    g_clear_pointer (&json, free);
   }
   if (submenu && GTK_IS_MENU (submenu))
   {
    putchar (',');
    print_menu_as_json (GTK_MENU (submenu), iw + 2);
   }
   printf ("\n%*s}", off + IW, "");
   if (iter->next)
   {
    putchar (',');
   }
  }
  printf ("\n%*s]", off, "");
 }
 g_list_free (children);
}

/**
print_strv_as_json:
Serialize a string vector as JSON to stdout.

@sv: string vector pointer.
@sc: string vector count.
@iw: indentation level.
*/
static void
print_strv_as_json (const gchar **sv,
                    const gint sc,
                    const guint iw)
{
 const guint off = iw * IW;

 if (sc > 0)
 {
  printf ("[\n");
  for (gint i = 0; i < sc; i++)
  {
   gchar *json = utf8_to_json (sv[i]);
   printf ("%*s%s", off + IW, "", json);
   free (json);
   if (i < sc - 1)
   {
    putchar (',');
   }
   putchar ('\n');
  }
  printf ("%*s]", off, "");
 }
}

/**
dump_menu_as_json:
Dump a #GtkMenu variable to stdout.
*/
void
dump_menu_as_json (GtkMenu *menu)
{
 printf ("{\n%*s\"cmdline\": ", IW, "");
 print_strv_as_json ((const gchar **) gl_argv, gl_argc, 1);
 putchar (',');
 print_menu_as_json (menu, 1);
 printf ("\n}\n");
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
