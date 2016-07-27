//2013-01-13

#if  !defined(_EM_NO_LAUNCHERS_)

#include <stdlib.h>
#include <string.h>

#include "launcher.h"
//#define _SVID_SOURCE
//#include <dirent.h>



//could be local to processLauncher
struct LauncherElement gl_launcherElement[] =
{
//sKeyword       sLineKeyword   bTryLocalised bRequired bUseForGtkMenu sFilter pRegexFilter sValue
 {"Categories",  "#categories", FALSE,        FALSE,    TRUE,          NULL,   NULL,        NULL },
 {"Name",        "item",        TRUE,         TRUE,     TRUE,          NULL,   NULL,        NULL },
 {"Exec",        "cmd",         FALSE,        TRUE,     TRUE,          NULL,   NULL,        NULL },
 {"Comment",     "tooltip",     TRUE,         FALSE,    FALSE,         NULL,   NULL,        NULL },
 {"Icon",        "icon",        FALSE,        FALSE,    TRUE,          NULL,   NULL,        NULL },
 {"Format",      "icon",        FALSE,        FALSE,    FALSE,         NULL,   NULL,        NULL },
 //Format= not in .desktop spec, it's for .directory.desktop only
};

guint gl_nLauncherElements = sizeof(gl_launcherElement)/sizeof(struct LauncherElement);

// ----------------------------------------------------------------------
void clearLauncherElements() // (IN struct LauncherElement launcherElement[], IN guint nLauncherElements)
// ----------------------------------------------------------------------
{
 guint nLauncherElements = sizeof(gl_launcherElement)/sizeof(struct LauncherElement);

 int i = 0;
 for (i = 0; i < nLauncherElements; i++)
 {
  gl_launcherElement[i].sValue = NULL;
 }
}

// ----------------------------------------------------------------------
void freeLauncherElementsMem() // (IN struct LauncherElement launcherElement[], IN guint nLauncherElements)
// ----------------------------------------------------------------------
{
 guint nLauncherElements = sizeof(gl_launcherElement)/sizeof(struct LauncherElement);
 int i = 0;
 for (i = 0; i < nLauncherElements; i++)
 {
  gchar* sValue = gl_launcherElement[i].sValue;
  if (sValue) g_free(sValue);
  gl_launcherElement[i].sValue = NULL;
 }
}

// ----------------------------------------------------------------------
gboolean checkFieldRegex(IN struct LauncherElement launcherElement[], IN guint nLauncherElements)
// ----------------------------------------------------------------------
{
 int i = 0;

 for (i = 0; i < nLauncherElements; i++)
 {
  if (!(launcherElement[i].sFilter) || launcherElement[i].pRegexFilter)
   continue;
  launcherElement[i].pRegexFilter = (regex_t*) malloc(sizeof(regex_t));
  if (!launcherElement[i].pRegexFilter)
  {
   fprintf(stderr, "failed to get memory for regular expression for %s \n", launcherElement[i].m_sKeyword);
   exit(EXIT_FAILURE);
  }
  if (regcomp(launcherElement[i].pRegexFilter, launcherElement[i].sFilter, REG_ICASE | REG_NOSUB | REG_EXTENDED))
  {
   fprintf(stderr, "failed to compile regular expression for %s \n", launcherElement[i].m_sKeyword);
   exit(EXIT_FAILURE);
  }
 } // for (i=0; i < nLauncherElements; i++)
 return TRUE;
}

// ----------------------------------------------------------------------
gboolean applyFieldRegex(IN struct LauncherElement launcherElement[], IN guint nLauncherElements, IN gboolean bOrFilters)
// ----------------------------------------------------------------------
{
 gboolean bResult = TRUE;

 if (!(launcherElement[LAUNCHER_ELEMENT_NAME].sValue))
  return TRUE;

 int i = 0;

 for (i = 0; i < nLauncherElements; i++)
 {
  if (!(launcherElement[i].pRegexFilter))
   continue;

  bResult = bOrFilters ? FALSE : TRUE; // at least oine filter exists
  if (regexec(launcherElement[i].pRegexFilter, launcherElement[i].sValue, 0, NULL, 0) == 0) // successfu match
  {
   if (bOrFilters)
    return TRUE;
  }
  else
  {
   if (!bOrFilters)
     return FALSE; // regex failed, doing and
  }
 } // for (i=0; i < nLauncherElements; i++)
 return bResult;
}

// ----------------------------------------------------------------------
void clearFieldRegex(IN struct LauncherElement launcherElement[], IN guint nLauncherElements)
// ----------------------------------------------------------------------
{
 int i = 0;
 for (i = 0; i < nLauncherElements; i++)
 {
  if (!launcherElement[i].pRegexFilter)
   continue;
  regfree(launcherElement[i].pRegexFilter);
  g_free(launcherElement[i].pRegexFilter);
 } // for (i=0; i < nLauncherElements; i++)
}


// ----------------------------------------------------------------------
gchar* getIconPath(IN gchar* sIconPath, IN guint uiIconSize, IN gboolean bSuppressErrors)
// ----------------------------------------------------------------------
{
 if (!sIconPath)
  return NULL;

 GtkIconTheme* pGtkIconTheme = gtk_icon_theme_get_default();
 if (!pGtkIconTheme)
 {
  if (!bSuppressErrors) fprintf(stderr, "Can't open gtk_icon_theme\n");
  return NULL;
 }

 GtkIconInfo* pGtkIconInfo = gtk_icon_theme_lookup_icon(pGtkIconTheme, sIconPath, uiIconSize, 0); //GtkIconLookupFlags flags);  24

 if (pGtkIconInfo)
 {
  gchar* sIconPathOut = (gchar*) gtk_icon_info_get_filename(pGtkIconInfo);
  if (sIconPathOut)
   return strdup(sIconPathOut);
 }
 return NULL;
}

// ----------------------------------------------------------------------
GdkPixbuf* getIconBuiltInPixBuf(IN gchar* sIconPath, IN guint uiIconSize, IN gboolean bSuppressErrors)
// ----------------------------------------------------------------------
{
 GtkIconTheme* pGtkIconTheme = gtk_icon_theme_get_default();
 if (!pGtkIconTheme)
 {
  if (!bSuppressErrors) fprintf(stderr, "Can't open gtk_icon_theme\n");
  return NULL;
 }
  GtkIconInfo* pGtkIconInfo = gtk_icon_theme_lookup_icon(pGtkIconTheme, sIconPath, uiIconSize, GTK_ICON_LOOKUP_USE_BUILTIN); //GtkIconLookupFlags flags);

 if (!pGtkIconInfo)
 {
  if (!bSuppressErrors) fprintf(stderr, "Can't open gtk_icon_theme_lookup_icon\n");
  return NULL;
 }

 return gtk_icon_info_get_builtin_pixbuf(pGtkIconInfo);
}

#endif // #if  !defined(_EM_NO_LAUNCHERS_)
