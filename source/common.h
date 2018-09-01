#ifndef _COMMON_H
#define _COMMON_H

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include <regex.h>
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if !GTK_CHECK_VERSION (2, 18, 0)
  #define gtk_widget_get_visible(gl_iW) GTK_WIDGET_VISIBLE (gl_iW)
#endif

//conveniences, for some added clarity in code:
#define IN
#define OUT
#define INOUT
#define STRCPY_IF(A, B) {if ((B)) strcpy((A), (B)); else *(A) = '\0';}

//MAXPATHLEN in <sys/param.h> but broken; in gwin32.h it's 1024
#define MAX_PATH_LEN 1024
#define MAX_CMD_LEN  1024 // used in expand_abbrev

//regex_t gl_rgxMatchTilda;
//regex_t gl_rgxMatchDotSlash;
//regex_t gl_rgxMatchDotDotSlash;

gboolean      initPathRegex();
void          clearPathRegex();

void          errorExit(IN gchar* sMsg);

int           make_absolute_path(IN const gchar *sPath, OUT gchar *sAbs);
gchar*        expand_path_tilda_dot(IN const gchar *sCmd, IN const gchar* sBasePath);
GdkPixbuf*    fileToPixBuf(gchar* sPathToIcon, IN guint uiIconSize, IN gboolean bSuppressErrors, OUT gchar* sErrMsg);
gboolean      is_line_file(IN gchar* sPath, OUT gchar* sErrMsg);

#ifdef __cplusplus
}
#endif	/* C++ */

#endif /* _COMMON_H */
