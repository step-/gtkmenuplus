//2013-01-13

#ifndef _LAUNCHER_H
#define _LAUNCHER_H 1

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include <regex.h>
#include "common.h"

regex_t gl_rgxLauncherExecArg; // used to check launcher exec fields for %f etc

struct LauncherElement
{
 gchar*    m_sKeyword;
 gchar*    m_sLineKeyword;
 gboolean  m_bTryLocalised;
 gboolean  bRequired;
 gboolean  bUseForGtkMenu;
 gchar*    sFilter;
 regex_t*  pRegexFilter;
 gchar*    sValue;
};

enum LauncherElementNdx {LAUNCHER_ELEMENT_CATEGORY = 0, LAUNCHER_ELEMENT_NAME = 1, LAUNCHER_ELEMENT_EXEC = 2, LAUNCHER_ELEMENT_COMMENT = 3, LAUNCHER_ELEMENT_ICON = 4};

enum IconFromLauncher { ICON_FROM_LAUNCHER_NONE = 0, ICON_FROM_LAUNCHER_FOUND = 0, ICON_FROM_LAUNCHER_COPIED = 0 };

struct LauncherStats
{
 guint nLaunchersEncountered;
 guint nLaunchersAccepted;
 guint nItems;
 guint nIconsFound;
 guint nIconsSaved;
};

gboolean      checkFieldRegex(IN struct LauncherElement launcherElement[], IN guint nLauncherElements);
void         clearFieldRegex(IN struct LauncherElement launcherElement[], IN guint nLauncherElements);
gboolean      applyFieldRegex(IN struct LauncherElement launcherElement[], IN guint nLauncherElements, IN gboolean bOrFilters);

//GdkPixbuf*    getIconPathForLauncher(INOUT gchar** psIconPath, IN guint uiIconSize, IN gboolean bSuppressErrors);
gchar*        getIconPath(IN gchar* sIconPath, IN guint uiIconSize, IN gboolean bSuppressErrors);
GdkPixbuf*    getIconBuiltInPixBuf(IN gchar* sIconPath, IN guint uiIconSize, IN gboolean bSuppressErrors);

void          freeLauncherElementsMem(); // (IN struct LauncherElement launcherElement[], IN guint nLauncherElements);
void          clearLauncherElements(); //(IN struct LauncherElement launcherElement[], IN guint nLauncherElements);
#ifdef __cplusplus
}
#endif	/* C++ */

#endif /* ifndef _LAUNCHER_H */
