//2013-01-13
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "common.h"

gchar*  expand_abbrev(IN const gchar *cmd, IN const gchar* sAbbrev, IN const regex_t* prAbbrev, IN guint nCharsOfAbbrevToReplace, IN const gchar *sReplace);

struct RegExExpansionPatterns
{
 gchar*  sPattern;
 gchar*  sName;
 guint   nCharsToOverwrite;
 regex_t reg;
};

struct RegExExpansionPatterns gl_regExExpansionPatterns[] =
{
 { "\\~",      "~",   1 },
 { "\\./",     "./",  1 },
 { "\\.\\./" , "../", 0 },
};

// ---------------------------------------------------------------------- AC
gboolean initPathRegex()
// ----------------------------------------------------------------------
{
 int i = 0;
 for (i = 0; i < sizeof(gl_regExExpansionPatterns)/sizeof(struct RegExExpansionPatterns); i++)
 {
  gchar sPattBuff[80];
  sprintf(sPattBuff, "(^|[ \"'\\t])(%s)", gl_regExExpansionPatterns[i].sPattern);
  if (regcomp(&(gl_regExExpansionPatterns [i].reg), sPattBuff, REG_EXTENDED)) // (^|[^./])
  {
   fprintf(stderr, "failed to compile regular expression \"%s\" for file name\n", gl_regExExpansionPatterns[i].sName);
   return FALSE;
  }
 }
 return TRUE;
}

// ---------------------------------------------------------------------- AC
void clearPathRegex()
// ----------------------------------------------------------------------
{
 int i = 0;
 for (i = 0; i < sizeof(gl_regExExpansionPatterns )/sizeof(struct RegExExpansionPatterns); i++)
 {regfree(&(gl_regExExpansionPatterns[i].reg));}
}
// ---------------------------------------------------------------------- AC
gboolean is_line_file(IN gchar* sPath, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 struct stat statbuf;
 stat(sPath, &statbuf);
 if (!S_ISREG(statbuf.st_mode))
 {
//fprintf(stderr, "Error at line # %d\n", gl_uiLineNum);
  if (sErrMsg) sprintf(sErrMsg, "%s is not a file\n", sPath);
// if (bReportError) fprintf(stderr, "%s is not a file\n", sPath);
  return FALSE;
 }
 else
  return TRUE;
}

// ----------------------------------------------------------------------
GdkPixbuf* fileToPixBuf(gchar* sPathToIcon, IN guint uiIconSize, IN gboolean bSuppressErrors, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 if (!is_line_file(sPathToIcon, sErrMsg))
  return NULL;

 GError *error = NULL;
 GdkPixbuf* pGdkPixbuf = gdk_pixbuf_new_from_file_at_size(sPathToIcon, uiIconSize, uiIconSize, &error);
 if (pGdkPixbuf == NULL)
 {
  if (!bSuppressErrors) sprintf(sErrMsg, "attempt to use icon in menu item failed: %s\n", error->message);
  g_error_free(error);
 }
 return pGdkPixbuf;
}

// Update: {{{ TODO delete this stuff
// make_absolute_path isn't needed after all. The reason is that version
// 1.00 defines all script paths to be relative to the path of the menu
// configuration file.  So, specifying argv[1] as a relative path might
// lead to unexpected results, because different relative path bases
// could be compounded (working dir vs. configuration file path), i.e.:
//   cd test/../source/gtk2
//   gtkmenuplus ../../test/START <- relative to dir gtk2
//   # in START:
//   scriptfile = $0  <- value: ../../START
//   mWORKDIR == echo $scriptfile <- relative to dir test
//   # end result:  $mWORKDIR != working dir!

#if NEVER_DEFINED

// step: Make an absolute path. This is a work around for a problem in
// expand_path, which I don't want to touch at the moment.
// expand_path returns an invalid path when argv[1] and the path
// after a "keyword=" are both relative paths.
// Note: GNU realpath(1) command chosen because, unlike realpath(3),
// realpath(1) -m ignores non-existent path elements.
int make_absolute_path(IN const gchar *sPath, OUT gchar *sAbs) // used by initDirectory
// ----------------------------------------------------------------------
{
 int err = -1;
 FILE *fp;
 guint = n;
 gchar cmd[MAX_PATH_LEN + 1];

 if (sprintf(cmd, "realpath -m '%s'", sPath))
 {
  if ((fp = popen(cmd, "r")))
  {
   if (0 < (n = fgets(sAbs, MAX_PATH_LEN + 1, fp)))
   {
    if (*(sAbs + n + 1) == '\n') *(sAbs + n + 1) = '\0';
   }
   if (err = pclose(fp)) perror("pclose");
  }
  else perror("popen");
 }
 else fprint(stderr, "error line %d\n", __LINE__);
 return err; // 0(Ok) <>0(error)
}

#endif //NEVER_DEFINED }}}

//deals with ./, ~.
gchar * expand_path_tilda_dot(IN const gchar *sPath, IN const gchar* sBasePath)
// ----------------------------------------------------------------------
{
 gchar* sResult = NULL;

  int i = 0;
  for (i = 0; i < sizeof(gl_regExExpansionPatterns )/sizeof(struct RegExExpansionPatterns); i++)
  {
   const gchar* sReplaceWith = (strcmp(gl_regExExpansionPatterns [i].sName, "~") == 0) ? getenv("HOME") : sBasePath;
   gchar* sResultNew  = expand_abbrev(sPath, gl_regExExpansionPatterns [i].sName,
                                           &(gl_regExExpansionPatterns [i].reg),
                                             gl_regExExpansionPatterns [i].nCharsToOverwrite, sReplaceWith);  // TO DO COULD RETURN ERROR?
   if (sResultNew)
   {
    if (sResult) g_free(sResult);
    sResult = sResultNew;
   } // if (sResultNew)
  } // for (i = 0; i < sizeof(gl_regExExpansionPatterns )/sizeof(struct RegExExpansionPatterns); i++)
/*
  if (strstr(sPath, "~")) //  && !strpbrk(sPath, "$`") ) // tilde, no other oddities
  sResult = expand_abbrev(sPath, "~", &gl_rgxMatchTilda, 1, getenv("HOME"));  // TO DO COULD RETURN ERROR?

 if (strstr(sPath, "../"))
 {
  gchar* sResult2 = NULL;
  sResult2 = expand_abbrev(sResult == NULL ? sPath : sResult, "../", &gl_rgxMatchDotDotSlash, 0, sBasePath); // sBuffPtr
  if (sResult2)
  {
   if (sResult) g_free(sResult);
   sResult = sResult2;
  }
 } // if (strstr(sPath, "../"))
 if (strstr(sPath, "./")) //  && !strpbrk(sPath, "$`") ) // tilde, no other oddities
 {
//  gchar sBuff[MAX_PATH_LEN];
//  gchar* sBuffPtr = getcwd(sBuff, MAX_PATH_LEN);
  gchar* sResult2 = NULL;
// if (sBuffPtr)
  sResult2 = expand_abbrev(sResult == NULL ? sPath : sResult, "./", &gl_rgxMatchDotSlash, 1, sBasePath); // sBuffPtr
  if (sResult2)
  {
   if (sResult) g_free(sResult);
   sResult = sResult2;
  }
 } // if (strstr(sPath, "./"))
*/
 return sResult;
}

// ---------------------------------------------------------------------
// from app_run.c -- a plugin for fbpanel.
// fbpanel-plugins-1.0.tar.gz
// https://sites.google.com/site/jvinla/fbpanel-plugins
// Copyright (C) 2011 John Vorthman

//altered by AC to return NULL if no ~/ found

// called by expand_path_tilda_dot
gchar* expand_abbrev(IN const gchar *cmd, IN const gchar* sAbbrev, IN const regex_t* prAbbrev, IN guint nCharsOfAbbrevToReplace, IN const gchar *sReplace)
// ---------------------------------------------------------------------
{
 // Replace '~' with 'home_dir' everywhere in cmd
 // Return pointer to result.
 // Note: you must g_free(result) when done with it.
 // cmd must be a NULL terminated string.

 const gchar *cCmd;
 gchar *ans, *cAns, *ansMax;

 if (!cmd) return(NULL);
 gchar* sPos = strstr(cmd, sAbbrev); /// quick and dirty test before using regex
 if (!sPos) return NULL; // no expansion needed

 cCmd = cmd;
// guint nAbbrev = strlen(sAbbrev);

 regmatch_t pmatch[3];
 gint iRegexResult = regexec(prAbbrev, cCmd, 3, pmatch, 0);
 if (iRegexResult != 0)
   return NULL;

 guint nReplace = strlen(sReplace);
 ans = g_new(gchar, MAX_CMD_LEN + 1);
 ansMax = ans + MAX_CMD_LEN;

 cAns = ans;

 gboolean bOverrun = TRUE;
 const gchar* sCmdSearchFrom = cCmd;
 while (iRegexResult == 0 && (cAns < ansMax)) // (*cCmd != '\0')
 {
  sPos = (gchar*) (sCmdSearchFrom  + pmatch[1].rm_eo); // end of (^|[ \"'\\t])
  bOverrun = TRUE;
  guint nLead = sPos - cCmd;

  if (nLead)
  {
   if (cAns + nLead >= ansMax) break;
   strncpy(cAns, cCmd, nLead);
   cAns += nLead;
   cCmd += nLead;
  } // if (nLead)

  if (cAns + nReplace >= ansMax) break;
  bOverrun = FALSE;
  strcpy(cAns, sReplace);
  cAns += nReplace;
  cCmd += nCharsOfAbbrevToReplace;
  if (*(sReplace + strlen(sReplace) - 1) == '/' && *cCmd == '/') cAns--; // adjust for double /

  sCmdSearchFrom = cCmd + (nCharsOfAbbrevToReplace ? 0 : 1);
  if (*sCmdSearchFrom == '\0') break;
// + pmatch[1].rm_eo
  iRegexResult = regexec(prAbbrev, sCmdSearchFrom, 3, pmatch, 0);
 } // while (iRegexResult = 0 && (cAns < ansMax))

 if (!bOverrun)
  if (cAns + strlen(cCmd) >= ansMax) bOverrun = TRUE;

 if (!bOverrun)
  strcpy(cAns, cCmd);
 else
 {
  g_free(ans);
  ans = NULL;
 }
// *cAns = '\0';
/*
 if (*cCmd != '\0')    // cmd is to long, so abort
 {
  g_free(ans);
  ans = g_strndup(cmd, MAX_CMD_LEN);
 }
*/
 return (ans);

} // expand_abbrev

void errorExit(IN gchar* sMsg)
{
 fprintf(stderr, "%s\n", sMsg);
 gboolean bLaunchedFromCLI = isatty(fileno(stdin)); // http://stackoverflow.com/questions/13204177/how-to-find-out-if-running-from-terminal-or-gui
 if (!bLaunchedFromCLI) // false when run from IDE, true from CLI
 {
  GtkWidget* pGtkMsgDlg = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                 "%s\n", sMsg);
  if (pGtkMsgDlg)
  {
   gtk_dialog_run (GTK_DIALOG (pGtkMsgDlg));
   gtk_widget_destroy(pGtkMsgDlg);
  }
 }
 exit(EXIT_FAILURE);
}

// vim: et ts=1 sts=1 sw=1 tw=0 fdm=syntax
