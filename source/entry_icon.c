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

#include <sys/stat.h>
#include <gtk/gtk.h>
#include "autoconfig.h"
#include "conf.h"
#include "entry.h"
#include "entry_cmd.h"
#include "entry_icon.h"
#include "input.h"
#include "path.h"

gchar gl_icon_dir[MAX_PATH_LEN + 1];

static regex_t gl_rgx_icon_ext;

static GtkWidget *entry_icon_image_new_from_stock_or_name (struct Entry *);
static GtkWidget *entry_icon_image_new_from_cmd (struct Entry *);
static GtkWidget *entry_icon_image_new_from_file (struct Entry *);

/**
entry_icon_is_to_render:
Test if documented rules imply that an entry icon is to be rendered.

@entry:
*/
gboolean
entry_icon_is_to_render (const struct Entry *entry)
{
 gboolean ret = FALSE;
#ifdef FEATURE_LAUNCHER
 if (entry->directive->type & LINE_GROUP_LAUNCHERS)
 {
  ret = conf_get_launchericons () && *entry->icon;
 }
 else
#endif
 if (entry->directive->type == LINE_ABSOLUTE_PATH)
 {
  ret = conf_get_icons () && *entry->cmd;
 }
 else if (!conf_get_icons ())
 {
  ret = *entry->icon;
 }
 else if (entry->flags & ENTRY_FLAG_INCLUDE_DIR)
 {
  /* `include=`DIR + `icon=`NULL hides the icons of the
  scanner files, per the scripting guide. By contrast,
  `include=`DIR + `icon=`"" does not hide the icons. */

  /* Covers both dir `include=` and dir `include=`+`icon=`. */
  ret = *entry->icon != ENTRY_NULL_ICON;
 }
 else
 {
  ret = *entry->icon != ENTRY_DISALLOW_DIRECTIVE;
 }
 return ret;
}

/**
entry_icon_image_new:
Resolve @entry->icon to a #GtkImage.

Return: #GtkWidget pointer to resolved image;
NULL, pushing an error message, otherwise.

Call #entry_icon_is_to_render before calling this function.
*/
GtkWidget *
entry_icon_image_new (struct Entry *entry)
{
 GtkWidget *img = NULL;

 if (!entry->icon[0] && !entry->cmd[0])
 {
  return img;
 }
 else if (!entry->icon[0])
 {
  /* Try MIME-type from `cmd=datafile` */
  if (strchr (entry->cmd, G_DIR_SEPARATOR) != NULL)
  {
   struct stat    stbuf;
   gchar          *cmdx;
   guint          error = 0;
   static struct Entry *e = NULL;

   if (e == NULL)
   {
    if ((e = entry_new_tracked ()) == NULL)
    {
     perror (__FUNCTION__);
     return NULL;
    }
   }
   strcpy (e->cmd, entry->cmd);
   cmdx = path_expand_full (e->cmd, gl_script_dir, &error);
   if (!error)
   {
    if (cmdx != NULL)
    {
     strcpy (e->cmd, cmdx);
     free (cmdx);
    }
    errno = 0;
    if (strstr (e->cmd, "file:///") == NULL && stat (e->cmd, &stbuf) != 0)
    {
      /* unsupported URI scheme && invalid path */
      error = errno;
    }
    /*
    else if (!S_ISREG (stbuf.st_mode))
    {
      error = EINVAL;
    }
    else if (access (e->cmd, X_OK) != 0)
    {
     error = EACCES;
    }
    */
    else
    {
     e->error = entry->error;
     /* icon from datafile path */
     img = entry_icon_image_new_from_cmd (e);
     /* entry owns the error list */
     entry->error = e->error;
     e->error = NULL;
     if (img == NULL)
     {
      if (e->error)
      {
       entry_prefix_error (entry, "MIME-type icon not found for '%s': ",
                           e->cmd);
       entry_set_error_level (entry, RINFO);
      }
     }
    }
    if (error)
    {
     entry_push_error (entry, RINFO, "MIME-type icon not found for '%s': %s",
                       e->cmd, strerror (error));
    }
   }
  }
 }
 else
 {
  gchar *ext = entry->icon[0] ? strrchr (entry->icon, '.') : NULL;

  if (ext != NULL
      && (strchr (ext, G_DIR_SEPARATOR) != NULL
          || regexec (&gl_rgx_icon_ext, ext, 0, NULL, 0) != 0))
  {
   ext = NULL;
  }
  if (ext != NULL || strchr (entry->icon, G_DIR_SEPARATOR) != NULL)
  {
   img = entry_icon_image_new_from_file (entry); /* push error */
   if (img == NULL)
   {
     /* Fall back from iconname.EXT to iconname and try again. */
    if (ext && ext[0] && strchr (entry->icon, G_DIR_SEPARATOR) == NULL)
    {
     ErrorList *head = entry_pop_error (entry);
     error_list_free (&head);
     ext[0] = '\0';
     img = entry_icon_image_new_from_stock_or_name (entry); /* no push error */
     ext[0] = '.';
     if (img == NULL)
     {
      entry_push_error (entry, RWARN, "named icon '%s' not found", entry->icon);
     }
    }
   }
  }
  else
  {
   img = entry_icon_image_new_from_stock_or_name (entry); /* no push error */
   if (img == NULL)
   {
    entry_push_error (entry, RWARN, "icon '%s' not found", entry->icon);
   }
  }
 }
 return img;
}

/**
entry_icon_image_new_from_file:

Return: #GtkWidget pointer to resolved image;
NULL, pushing an error message, otherwise.
*/
static GtkWidget *
entry_icon_image_new_from_file (struct Entry *entry)
{
 GtkWidget *img = NULL;
 gchar path[SIZEOF_COOKED];

 strcpy (path, entry->icon);
 if (path_expand (path, gl_icon_dir, "icon", entry) < RFAIL)
 {
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  pixbuf = gdk_pixbuf_new_from_file_at_size (path, conf_get_iconsize_width (),
                                             conf_get_iconsize_height (),
                                             &error);
  if (error != NULL)
  {
   entry_push_error (entry, RWARN, "icon file error: %s", error->message);
   g_error_free (error);
  }
  if (pixbuf != NULL)
  {
   img = gtk_image_new_from_pixbuf (pixbuf);
  }
 }
 return img;
}

struct IconSize
{
 gint px_size;
 gint ico_size;
};

/**
compare_icon_size:
*/
inline static int compare_icon_size (gconstpointer a, gconstpointer b)
{
 const struct IconSize *pa = a;
 const struct IconSize *pb = b;
 return pa->px_size - pb->px_size;
}

/**
entry_icon_image_new_from_stock_or_name:

Return: #GtkWidget pointer to resolved image; NULL otherwise.
Function shall not push an error message.
*/
static GtkWidget *
entry_icon_image_new_from_stock_or_name (struct Entry *entry)
{
 GtkWidget* img;
 GtkStockItem it;
 gboolean b;
 static GList *registry = NULL;
 GList *found;
 struct IconSize siit, *psiit;
 gint ico_size;
 gint gw = conf_get_iconsize_width ();
 gint gh = conf_get_iconsize_height ();

 /* Do we have a stock icon or a named icon ? */
 SETUNDEPR (b, gtk_stock_lookup, entry->icon, &it);
 if (b) /* a stock icon */
 {
  /* Scale input icon to the desired size gl_iW by registering a custom size. */
  siit.px_size = gw;
  found = g_list_find_custom (registry, &siit, compare_icon_size);
  if (found == NULL)
  {
   /* register the new custom size */
   gchar size_name[16];

   g_snprintf (size_name, 16, "%d", gw);
   psiit = g_malloc0 (sizeof siit);    /* not freed */
   psiit->px_size = siit.px_size;
   SETUNDEPR (psiit->ico_size, gtk_icon_size_register, size_name, gw, gh);
   registry = g_list_append (registry, psiit);

   /* Can use ico_size for stock (GTK2/3) and named (GTK2 only) icons. */
   ico_size = psiit->ico_size;
  }
  else
  {
   /* Reuse a previously-registered custom icon size. */
   ico_size = ((struct IconSize *)found->data)->ico_size;
  }

  SETUNDEPR (img, gtk_image_new_from_stock, it.stock_id, ico_size);
 }
 else /* a named icon */
 {
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
  /*
  Important difference between GTK3 and GTK2:
  (for each path in $XDG_DATA_DIRS)
  GTK3 will only look under /usr/share/icons, while
  GTK2 will also look under /usr/share/pixmaps
  Therefore, GTK2 may return a valid match where GTK3 doesn't.
  */
  GdkPixbuf *pixbuf =
  gtk_icon_theme_load_icon (icon_theme, entry->icon, gw, 0, NULL);
  if (pixbuf == NULL)
  {
   /* The theme is missing this named icon altogether. */
   /* Function shall not push an error message. */
   return NULL;
  }
  /*
   If the theme is missing an exact-size px image, GTK2 gtk_icon_theme_load_icon
   resizes to the given px dimension while GTK3 doesn't. So resize explicitly.
  */
#if GTK_CHECK_VERSION (3,0,0)
  {
   gint iw = gdk_pixbuf_get_width (pixbuf);
   gint ih = gdk_pixbuf_get_height (pixbuf);
   if (gw != iw || gh != ih)
   {
    /* force scaling image to specific size */
    GdkPixbuf *spb;
    spb = gdk_pixbuf_scale_simple (pixbuf, gw, gh, GDK_INTERP_BILINEAR);
    g_object_unref (pixbuf);
    pixbuf = spb;
   }
  }
#endif
  img = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
 }
 return img;
}

/**
entry_icon_image_new_from_cmd:
Infer entry's icon from its command.

Return: #GtkWidget pointer to resolved image;
NULL, pushing an error message, otherwise.
*/
static GtkWidget *
entry_icon_image_new_from_cmd (struct Entry *entry)
{
 GtkWidget      *img = NULL;
 gint gw = conf_get_iconsize_width ();
 gint gh = conf_get_iconsize_height ();

 assert (strchr (entry->cmd, G_DIR_SEPARATOR) != NULL);
 {
  GAppInfo *app_info = NULL;

  if (!entry_cmd_info_new_for_type (entry, entry->cmd, &app_info, NULL))
  {
   /* entry_cmd_info_new_for_type raised an error */
  }
  else if (app_info != NULL)
  {
   GIcon *gicon = g_app_info_get_icon (app_info);
   if (gicon != NULL)
   {
    GdkPixbuf *pixbuf;
    gchar *sicon = g_icon_to_string (gicon);

    if (sicon != NULL) /* filepath or icon name */
    {
     struct stat stbuf;

     if (stat (sicon, &stbuf) == 0 && S_ISREG (stbuf.st_mode))
     {
      GError *error = NULL;

      pixbuf =
      gdk_pixbuf_new_from_file_at_size (sicon, gw, gh, &error);
      if (error != NULL)
      {
       entry_push_error (entry, RWARN, "%s", error->message);
       g_error_free (error);
      }
      if (pixbuf != NULL)
      {
       img = gtk_image_new_from_pixbuf (pixbuf);
      }
     }
     else
     {
      img = gtk_image_new_from_icon_name (sicon, gw);
     }
    }
    g_free (sicon);
   }
   g_object_unref (app_info);
  }
 }
 if (img != NULL
     && gtk_image_get_pixel_size ((GtkImage *) img) != gw)
 {
  gtk_image_set_pixel_size ((GtkImage *) img, gw);
 }
 return img;
}

/**
entry_icon_module_init:
Initialize this module.

Returns: 0(OK) -1(Error).
*/
int
entry_icon_module_init (void)
{
 return compile_regex (&gl_rgx_icon_ext, "\\.(png|svg|gif|xpm|jpg|DirIcon)$",
                       REG_EXTENDED | REG_ICASE);
}

/**
entry_icon_module_finalize:
Clean up this module. Call before exit(2).
*/
void
entry_icon_module_finalize (void)
{
regfree (&gl_rgx_icon_ext);
}

/* vim:set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
