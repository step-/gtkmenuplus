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

#ifndef _COMMON_H
#define _COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include <assert.h>
#include <regex.h>
#include "autoconfig.h"

#if !GTK_CHECK_VERSION (2, 18, 0)
  #define gtk_widget_get_visible(w) GTK_WIDGET_VISIBLE (conf_get_iconsize_width ())
#endif

#define member_size(type, member) (sizeof(((type *) NULL)->member))

/* MAXPATHLEN in <sys/param.h><linux/limits.h>4096; 1024 in gwin32.h */
#define MAX_PATH_LEN 1023
#define MAX_LINE_LENGTH 1022
#if MAX_LINE_LENGTH >= MAX_PATH_LEN
#error "MAX_LINE_LENGTH >= MAX_PATH_LEN in common.h"
#endif
#define MAX_MENU_DEPTH 5

enum Result
{
 /* This section must be sorted by increasing severity. */
 ROK = 0,
 RINFO,
 RWARN,
 RFAIL,
 RFATAL,

 /* Functions shall not return values in this section. */
 RDEBUG,
};

enum LineType
{
 LINE_UNDEFINED             = 0,

 LINE_ITEM                  = 1 << 0,
 LINE_CMD                   = 1 << 1,
 LINE_ICON                  = 1 << 2,
 LINE_SUBMENU               = 1 << 3,

 LINE_SEPARATOR             = 1 << 4,
 LINE_ICON_SIZE             = 1 << 5,
 LINE_POSITION              = 1 << 6,
 LINE_ICON_DIRECTORY        = 1 << 7,
 LINE_ACTIVATION_LOGFILE    = 1 << 8,
 LINE_ACTIVATION_LOGEXCLUDE = 1 << 9,
 LINE_INCLUDE               = 1 << 10,
 LINE_ENDSUBMENU            = 1 << 11,
 LINE_CONFIGURE             = 1 << 12,
 LINE_ONEXIT                = 1 << 13,
 LINE_EOF                   = 1 << 14,
 LINE_ERROR                 = 1 << 15, /* `error=` directive  */
 LINE_ABSOLUTE_PATH         = 1 << 16,

 LINE_FORMAT                = 1 << 17,
 LINE_TOOLTIP               = 1 << 18,
 LINE_TOOLTIP_FORMAT        = 1 << 19,

#define LINE_GROUP_IF \
 (LINE_IF | LINE_ELSE | LINE_ELSEIF | LINE_ENDIF)
 LINE_IF                    = 1 << 20,
 LINE_ELSE                  = 1 << 21,
 LINE_ELSEIF                = 1 << 22,
 LINE_ENDIF                 = 1 << 23,

#define LINE_GROUP_LAUNCHERS \
  (LINE_LAUNCHER | LINE_LAUNCHER_SUB | LINE_LAUNCHER_SUBMENU)
 LINE_LAUNCHER              = 1 << 24,
 LINE_LAUNCHER_SUB          = 1 << 25,
 LINE_LAUNCHER_SUBMENU      = 1 << 26,
 LINE_LAUNCHER_ARGS         = 1 << 27,
 LINE_LAUNCHER_DIRFILE      = 1 << 28,
 LINE_LAUNCHER_DIR          = 1 << 29,

 LINE_SET_VARNAME           = 1 << 30, /* `variable=`         */

/* Extend the enumeration through bit masks. */

#define linetype_mask(x)      ((1 << 31) | (x))
#define is_linetype_mask(x)   (((x) & LINE_MASK) == LINE_MASK)
 LINE_MASK                  = linetype_mask (0),

#define linetype_bad(n)       (LINE_BAD | 1 << (n))
#define is_linetype_bad(x)    (((x) & LINE_BAD) == LINE_BAD)
 LINE_BAD                   = linetype_mask (1 << 30),
 LINE_BAD_LEN               = linetype_bad (0),
 LINE_BAD_NO_EQ             = linetype_bad (1),

#define linetype_extd(n)      (LINE_EXTD | 1 << (n))
#define is_linetype_extd(x)   (((x) & LINE_EXTD) == LINE_EXTD)
 LINE_EXTD                  = linetype_mask (1 << 29),
 /* Placeholder for the next new directive. */
 LINE_LAUNCHER_PLACEHOLDER  = linetype_extd (0),
};

enum LauncherType
{
 LAUNCHER_TYPE_UNKNOWN,
 LAUNCHER_TYPE_APPLICATION,
 LAUNCHER_TYPE_LINK,
 LAUNCHER_TYPE_DIRECTORY,
};

#define NESTED_INCLUDE_LIMIT         10
#define NESTED_IF_LIMIT               5

void
die (const gchar *message);

int
compile_regex (regex_t *r,
               const char *pattern,
               int cflags);

#if defined(FEATURE_CONDITIONAL) || defined(FEATURE_VARIABLE)
enum Result
popen_read (const gchar *label,
            const gchar *expr,
            gchar *value,
            const guint max_size,
            gchar **err_msg);
#endif

/* C99 ignore GTK3 deprecation warnings for 'function' and 'var = function'  */
#define UNDEPR(function, ...)                                                 \
 {                                                                            \
   G_GNUC_BEGIN_IGNORE_DEPRECATIONS;                                          \
   function(__VA_ARGS__);                                                     \
   G_GNUC_END_IGNORE_DEPRECATIONS;                                            \
 }

#define SETUNDEPR(var, function, ...)                                         \
 {                                                                            \
   G_GNUC_BEGIN_IGNORE_DEPRECATIONS;                                          \
   var = function(__VA_ARGS__);                                               \
   G_GNUC_END_IGNORE_DEPRECATIONS;                                            \
 }

#ifdef _DIRENT_HAVE_D_TYPE
#define ISDIR(direntptr) (DT_DIR == (direntptr)->d_type)
#define ISREG(direntptr) (DT_REG == (direntptr)->d_type)
#define ISLNK(direntptr) (DT_LNK == (direntptr)->d_type)
#else
#define ISDIR(statptr) (S_ISDIR((statptr)->st_mode))
#define ISREG(statptr) (S_ISREG((statptr)->st_mode))
#define ISLNK(statptr) (S_ISLNK((statptr)->st_mode))
#endif

gboolean
is_path_to_dir (const gchar *path,
                const struct dirent *de,
                gchar **rpath,
                gboolean quiet);

gboolean
is_path_to_reg (const gchar *path,
                const struct dirent *de,
                gchar **rpath,
                gboolean quiet);

#ifdef __cplusplus
}
#endif
#endif /* _COMMON_H */

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
