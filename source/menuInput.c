#include <string.h>
#include <stdlib.h> // malloc
#include <ctype.h>  // isspace
#include <sys/stat.h> //stat
#include <gtk/gtk.h>

#include "menuInput.h"

// version 1.1.7, 2017-11-26

//required because __USE_GNU
char * strcasestr (const char *haystack, const char *needle);

gboolean             checkConfigKeyword(IN gchar* sSwitch, IN gboolean bCheckNeg, OUT gboolean* pbResult, OUT gchar** psArgs);

const gchar*  gl_sIconRegexPat = "\\.[a-z]{3,4}$";
const gchar*  gl_sUriSchema = "^[a-z]+://";
const gchar*  gl_sSharpIsntComment =
  "^\\s*cmd\\s*" // cmd=
#if !defined(_GTKMENUPLUS_NO_IF_)
 "|^\\s*(else)?if\\s*=" // if= | elseif=
#endif
#if !defined(_GTKMENUPLUS_NO_VARIABLES_)
 "|^\\s*\\w+\\s*==" // variable_name==
 "|^\\s*\\w+\\s*=\\s*\"[^\"]*#[^\"]*\"" // variable_name="...#..."
 "|^\\s*\\w+\\s*=\\s*'[^']*#[^']*'" // variable_name='...#...'
#endif
 "|\"#[0-9A-Fa-f]{6}\"|'#[0-9A-Fa-f]{6}'" // quoted HTML color
 "|\"#[0-9A-Fa-f]{3}\"|'#[0-9A-Fa-f]{3}'" // quoted HTML color (short form)
 ;

guint         gl_uiCurDepth =              0;        // Root menu is depth = 0
guint         gl_uiCurItem =               0;        // Count number of menu entries
guint         gl_uiRecursionDepth =        0;

gchar*        gl_sCmdLineConfigNext =      NULL;
gboolean      gl_bConfigKeywordIcons = TRUE,  gl_bConfigKeywordUseEndSubMenu = FALSE, gl_bConfigKeywordFormattingLocal = FALSE,
	      gl_bConfigKeywordLauncherNoDisplay = TRUE,
	      gl_bConfigKeywordLauncherNullCategory = TRUE,
	      gl_bConfigKeywordLauncherListFirst = FALSE,
              gl_bErrorsInConsoleOnly = FALSE;

/*
gchar*        gl_sPrefixDirBuff;
gchar*        gl_sMenuPosBuff;
gchar*        gl_sIconSizeBuff;
*/

gint          gl_iW = 0, gl_iH = 0;           // Size of menu icon; used by addIcon, set by onIconSize

//==============================================================================================
//==============================================================================================

#if !defined(_GTKMENUPLUS_NO_IF_)

//struct IfStatus  gl_ifStatuses[IF_STATUS_SET_COUNT]; //TO DO kill
struct IfStatus* gl_pIfStatusCurrent = NULL;

// ----------------------------------------------------------------------AC
void  ifStatusFree(OUT struct IfStatus* pIfStatus)
// ----------------------------------------------------------------------
{
 struct IfStatus* pIfStatusCurrent = pIfStatus;

 while (pIfStatusCurrent->m_pIfStatusFwd)
 {
  struct IfStatus* pIfStatusNext =  pIfStatusCurrent->m_pIfStatusFwd;
  if (pIfStatusCurrent->m_bOnHeap) g_free(pIfStatusCurrent);
  pIfStatusCurrent = pIfStatusNext;
 }
}

// ----------------------------------------------------------------------AC
void  ifStatusInit(OUT struct IfStatus* pIfStatus)
// ----------------------------------------------------------------------
{
 memset(pIfStatus, 0, IF_STATUS_SET_COUNT * sizeof(struct IfStatus));

 gint i = 0;
 for (i = 0; i < IF_STATUS_SET_COUNT; i++)
 {
  if (i > 0)  pIfStatus[i].m_pIfStatusBack = &(pIfStatus[i - 1]);
  if (i < IF_STATUS_SET_COUNT - 1)  pIfStatus[i].m_pIfStatusFwd = &(pIfStatus[i + 1]);
 }
}

#if !defined(_GTKMENUPLUS_NO_DEBUG_IF_)
// ----------------------------------------------------------------------
void printIfStatus(IN gchar* msg, IN struct IfStatus* p)
// ----------------------------------------------------------------------
{
 gchar s[] = "T";
 fprintf(stderr, "{{{{ %s\n", msg);
 do {
  fprintf(stderr,
   "%s InUse(%d) CurrentlyAccepting(%d) TrueConditionFound(%d) ElseFound(%d)"
   " OnHeap(%d)\n", s,
   p->m_bInUse,
   p->m_bCurrentlyAccepting,
   p->m_bTrueConditionFound,
   p->m_bElseFound,
   p->m_bOnHeap);
  strcpy(s, p->m_pIfStatusBack ? "\u2191" : "\u21A5");
  p = p->m_pIfStatusBack;
 } while(p);
 fprintf(stderr, "}}}}\n");
}
#endif  // #if !defined(_GTKMENUPLUS_DEBUG_IF_)
#endif  // #if !defined(_GTKMENUPLUS_NO_IF_)

//==============================================================================================
//==============================================================================================

#if !defined(_GTKMENUPLUS_NO_PARAMS_)

// ----------------------------------------------------------------------AC
gboolean paramsInit(OUT struct Params* pParams, IN int argc, IN gchar *argv[], IN gboolean bIncludesProgName)
// ----------------------------------------------------------------------
{
 pParams->m_bIncludesProgName = bIncludesProgName;
//argc =- (bIncludesProgName ? 2 : 1);  // may be used by expand_params_vars arc now number of tests
 gint nCmdLineParams = argc;
// if  (bIncludesProgName) nCmdLineParams--;

// if (nCmdLineParams > 0)
 pParams->m_nCmdLineParams = nCmdLineParams;
//else
//  argc = pParams->m_nCmdLineParams = 0;

 pParams->m_sCmdLineParamVec = argv;  // may be used by expand_params_vars; tests now from argv[1] onward
 pParams->m_pCmdLineParamInUse = NULL;
 if (nCmdLineParams)
 {
  uint nMem = nCmdLineParams * sizeof(gboolean);
  pParams->m_pCmdLineParamInUse = (gboolean*) malloc(nMem);
  if (!pParams->m_pCmdLineParamInUse)
  {
   fprintf(stderr, "fatal error: can't allocate memory for commands.\n");
   return FALSE;
  } // if (!pParams->m_pCmdLineParamInUse)
  memset(pParams->m_pCmdLineParamInUse, 0, nMem);
 } // if (argc)
 return TRUE;
}

// ----------------------------------------------------------------------AC
void paramsFinish(OUT struct Params* pParams)
// ----------------------------------------------------------------------
{
 if (pParams->m_pCmdLineParamInUse) // // pParams->m_pCmdLineParamInUse may be null if no params, or only $0
 {
  int i = pParams->m_bIncludesProgName ? 2 : 1;
  for (; i < pParams->m_nCmdLineParams; i++)
  {
  if (!pParams->m_pCmdLineParamInUse[i])
   fprintf(stderr, "warning: parameter %d provided but never referenced.\n", i - (pParams->m_bIncludesProgName ? 1 : 0));
  }
  g_free(pParams->m_pCmdLineParamInUse);
 } // if (pParams->m_pCmdLineParamInUse)
 pParams->m_nCmdLineParams = 0;
 pParams->m_sCmdLineParamVec = NULL;
 pParams->m_pCmdLineParamInUse = NULL;
}

#endif

//==============================================================================================
//==============================================================================================


typedef enum LineParseResult (*funcGetKeyWordArgs)(gchar*, gchar*);

struct KeywordConfigure
{
 gchar*             sKeyword;
 gboolean*          pbResult;
 funcGetKeyWordArgs funcForKWargs;
};

struct KeywordConfigure gl_keywordConfigure [] =
{
 {"abspathparts",     NULL,                                getAbsPathParts},
 {"menuposition",     NULL,                                getMenuPosArg},
 {"iconsize",         NULL,                                getIconSizeArg},
 {"endsubmenu",       &gl_bConfigKeywordUseEndSubMenu ,    NULL},
 {"icons",            &gl_bConfigKeywordIcons,             NULL},  // must follow "iconsize" or collision
 {"formattinglocal",  &gl_bConfigKeywordFormattingLocal,   NULL},
 {"launchernodisplay",    &gl_bConfigKeywordLauncherNoDisplay,    NULL},
 {"launchernullcategory", &gl_bConfigKeywordLauncherNullCategory, NULL},
 {"launcherlistfirst",    &gl_bConfigKeywordLauncherListFirst, NULL},
 {"errorconsoleonly", &gl_bErrorsInConsoleOnly,            NULL},
// {"abspathtitle",    NULL,                                getAbsPathTitle} // must be at aend
// {"findabsenticons", &gl_bFindAbsentIcons,  1}
};

// ---------------------------------------------------------------------
enum LineParseResult checkConfigKeywords(IN gchar* sLinePostEq, OUT gchar* sErrMsg)
// ---------------------------------------------------------------------
{
 guint i = 0;
 for (i = 0; i < sizeof(gl_keywordConfigure)/sizeof(struct KeywordConfigure); i++)
 {
  gchar* sBuff = NULL;
  gboolean bResult = FALSE;

  if (checkConfigKeyword(gl_keywordConfigure[i].sKeyword, gl_keywordConfigure[i].funcForKWargs == NULL, &bResult, &sBuff)) // found
  {
   if (gl_keywordConfigure[i].pbResult) *(gl_keywordConfigure[i].pbResult) = bResult;
   if (bResult && sBuff && gl_keywordConfigure[i].funcForKWargs) gl_keywordConfigure[i].funcForKWargs(sBuff, sErrMsg);
  } // if (checkConfigKeyword(gl_keywordConfigure[i].sKeyword, gl_keywordConfigure[i].funcForKWargs == NULL, &bResult, &sBuff)) // found
 } // for (i = 0; i < sizeof(gl_keywordConfigure)/sizeof(struct KeywordConfigure); i++)

 if (strspn(gl_sLinePostEq, " \t") != strlen(gl_sLinePostEq))
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "%s\n", "\"configure\", contains unknown keyword or text");
  return lineParseFail;
 }
 else
  return lineParseOk;
}

// ---------------------------------------------------------------------- AC
gboolean checkConfigKeyword(IN gchar* sSwitch, IN gboolean bCheckNeg, OUT gboolean* pbResult, OUT gchar** psArgs)
// ----------------------------------------------------------------------
{
 gchar* sPos =  strcasestr(gl_sLinePostEq, sSwitch);
 if (!sPos)
  return FALSE;
 *pbResult = TRUE;
 guint nLen = strlen(sSwitch);
 if (bCheckNeg)
 {
  if (strncasecmp(sPos - 2, "no", 2) == 0)
  {
   sPos -= 2;
   nLen += 2;
   *pbResult = FALSE;
  }
 }
 int i = 0;
 for (i = 0; i < nLen; i++) {*(sPos + i) = ' ';}
 if (psArgs)
 {
  sPos += i;
   while (*sPos == ' ' || *sPos == '\t' ) sPos++;
  *psArgs = sPos;
 }
 return TRUE;
}

//==============================================================================================
//==============================================================================================

struct Keyword gl_keyword[] = // used in getLineTypeName, readLine
{
//m_sKey                 m_nLen  m_bIndentMatters m_linetype;

 {"item",                     4, TRUE,   LINE_ITEM},
 {"cmd",                      3, TRUE,   LINE_CMD},
 {"icon",                     4, TRUE,   LINE_ICON},

#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 {"tooltip",                  7, TRUE,   LINE_TOOLTIP},
#endif

 {"submenu",                  7, TRUE,   LINE_SUBMENU},
 {"iconsize",                 8, FALSE,  LINE_ICON_SIZE},
 {"icondirectory",           13, FALSE,  LINE_ICON_DIRECTORY},
 {"icondir",                  7, FALSE,  LINE_ICON_DIRECTORY},

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
 {"format",                   6, FALSE,  LINE_FORMAT},
#endif

#if  !defined(_GTKMENUPLUS_NO_FORMAT_) && !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 {"tooltipformat",           13, FALSE,  LINE_TOOLTIP_FORMAT},
#endif

// {"startpreferredappwith",   21, FALSE,  LINE_START_PREFERRED_APP_WITH},
 {"error",                    5, TRUE,   LINE_ERROR},

#if !defined(_GTKMENUPLUS_NO_IF_)
 {"if",                       2, FALSE,  LINE_IF},
 {"elseif",                   6, FALSE,  LINE_ELSEIF},
 {"elif",                     4, FALSE,  LINE_ELSEIF},
#endif

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
 {"launcher",                 8, TRUE,   LINE_LAUNCHER},
 {"launchersub",             11, TRUE,   LINE_LAUNCHER_SUB},
 {"launcherargs",            12, FALSE,  LINE_LAUNCHER_ARGS},
 {"launcherdirfile",         15, FALSE,  LINE_LAUNCHER_DIRFILE},
 {"launchersubmenu",         15, FALSE,  LINE_LAUNCHER_SUBMENU},
 {"launcherdirectory",       17, FALSE,  LINE_LAUNCHER_DIR},
 {"launcherdir",             11, FALSE,  LINE_LAUNCHER_DIR},
#endif // #if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)

#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
 {"activationlogfile",       17, FALSE,  LINE_ACTIVATION_LOGFILE},
#endif // #if  !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)

 {"include",                  7, TRUE,   LINE_INCLUDE},
 {"menupos",                  7, FALSE,  LINE_POSITION},
 {"menuposition",            12, FALSE,  LINE_POSITION},
 {"configure",                9, FALSE,  LINE_CONFIGURE},
 {"onexit",                   6, FALSE,  LINE_ONEXIT},
 //only used as text in error messages
 {"end of file",              0, FALSE,  LINE_EOF},
 {"undefined",                0, FALSE,  LINE_UNDEFINED},
 {"line too long",            0, FALSE,  LINE_BAD_LEN},
 {"not followed by = sign",   0, FALSE,  LINE_BAD_NO_EQ},
};

struct Keyword gl_keywordNoVal[] = // used in getLineTypeName, readLine
{
 {"separator",                9, TRUE,   LINE_SEPARATOR},
 {"endsubmenu",              10, FALSE,  LINE_SUBMENU_END},
#if !defined(_GTKMENUPLUS_NO_IF_)
 {"else",                     4, FALSE,  LINE_ELSE},
 {"endif",                    5, FALSE,  LINE_ENDIF},
 {"fi",                       2, FALSE,  LINE_ENDIF}
#endif // #if !defined(_GTKMENUPLUS_NO_IF_)
};

// ---------------------------------------------------------------------
enum LineType getLinetype(IN gchar * sBuff, IN struct Keyword *keywords, guint uiKwSize, IN gchar * sCharsTerminating, OUT gboolean* bpIndentMatters)
// ---------------------------------------------------------------------
{
 *bpIndentMatters = TRUE;
 guint i = 0;
 for (i = 0; i < uiKwSize/sizeof(struct Keyword); i++)
 {
  if (!keywords[i].m_nLen) break; // gl_keyword[i].m_nLen blocks keywords used only for messages
  if (strncasecmp(sBuff, keywords[i].m_sKey, keywords[i].m_nLen) == 0 && strchr(sCharsTerminating, *(sBuff + keywords[i].m_nLen)))
  {
   *bpIndentMatters = keywords[i].m_bIndentMatters;
   return keywords[i].m_linetype;
  }
 } // for (i = 0; i < uiKwSize/sizeof(struct Keyword); i++)
 return LINE_UNDEFINED;
}

// ---------------------------------------------------------------------- AC
//called by twice reportLineError
const gchar * getLineTypeName(enum LineType linetype)
// ----------------------------------------------------------------------
{
 gint i;
 for (i = 0; i < sizeof(gl_keywordNoVal)/sizeof(struct Keyword); i++)
 {
  if (gl_keywordNoVal[i].m_linetype == linetype)  return gl_keywordNoVal[i].m_sKey;
 } // for (i = 0; i < sizeof(gl_keyword)/sizeof(struct Keyword); i++)

 for (i = 0; i < sizeof(gl_keyword)/sizeof(struct Keyword); i++)
 {
  if (gl_keyword[i].m_linetype == linetype)  return gl_keyword[i].m_sKey;
 } // for (i = 0; i < sizeof(gl_keyword)/sizeof(struct Keyword); i++)

 return NULL;
}

//==============================================================================================
//==============================================================================================

struct LinetypeAction gl_linetypeActionMap[] =
{
// linetype             m_bMenuEntryCommit  m_pActionFunc
 { LINE_ITEM,                      TRUE,  onItem},         // do not change relative order of thses lines; see LINE_INDEXABLE_INTO_TABLE_LOWEST
 { LINE_CMD,                       FALSE, onCmd},          // do not change relative order of thses lines; see LINE_INDEXABLE_INTO_TABLE_LOWEST
 { LINE_ICON,                      FALSE, onIcon},         // do not change relative order of thses lines; see LINE_INDEXABLE_INTO_TABLE_LOWEST
 { LINE_SUBMENU,                   TRUE,  onSubMenu},      // do not change relative order of thses lines; see LINE_INDEXABLE_INTO_TABLE_LOWEST
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 { LINE_TOOLTIP,                   FALSE, onTooltip},
#endif // #if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
 { LINE_FORMAT,                    TRUE,  onFormat},
#endif

#if  !defined(_GTKMENUPLUS_NO_FORMAT_) && !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 { LINE_TOOLTIP_FORMAT,            TRUE,  onTooltipFormat},
#endif

 { LINE_SEPARATOR,                 TRUE,  onSeparator},

#if !defined(_GTKMENUPLUS_NO_IF_)
 { LINE_IF,                        FALSE, onIf},  // an if/endif in the middle of a menu entry makes sense
 { LINE_ELSE,                      FALSE, onElse},
 { LINE_ELSEIF,                    FALSE, onElseif},
 { LINE_ENDIF,                     FALSE, onEndif},
#endif // #if !defined(_GTKMENUPLUS_NO_IF_)

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
 { LINE_LAUNCHER,                  TRUE,  onLauncher},
 { LINE_LAUNCHER_SUB,              TRUE,  onLauncherSub},
 { LINE_LAUNCHER_DIR,              TRUE,  onLauncherDir},
 { LINE_LAUNCHER_ARGS,             TRUE,  onLauncherArgs},
 { LINE_LAUNCHER_DIRFILE,          TRUE,  onLauncherDirFile},
 { LINE_LAUNCHER_SUBMENU,          TRUE,  onLauncherSubMenu},
#endif // #if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)

#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
 { LINE_ACTIVATION_LOGFILE,        TRUE,  onActivationLogfile},
#endif // #if  !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)

 { LINE_INCLUDE,                   TRUE,  onInclude},
 { LINE_ICON_SIZE,                 TRUE,  onIconSize},
 { LINE_POSITION,                  TRUE,  onPosition},
// { LINE_START_PREFERRED_APP_WITH,  TRUE,  onPreferredApplicationLauncher},
 { LINE_KEYWORD_IS_VARIABLE,       TRUE,  onVariable},
 { LINE_ICON_DIRECTORY,            TRUE,  onIconDir},
 { LINE_EOF,                       TRUE,  onEof},
 { LINE_ERROR,                     TRUE,  onError},
 { LINE_SUBMENU_END,               TRUE,  onSubMenuEnd},
 { LINE_CONFIGURE,                 TRUE,  onConfigure},
 { LINE_ONEXIT,                    TRUE,  onOnExit},
 { LINE_ABSOLUTE_PATH,             TRUE,  onAbsolutePath}
};

// ----------------------------------------------------------------------
struct LinetypeAction* getLinetypeAction(enum LineType linetype)
// ----------------------------------------------------------------------
{
 gint i;
 struct LinetypeAction* pLinetypeAction = NULL;
//speed up by direct indexing into table
 if (linetype >= LINE_INDEXABLE_INTO_TABLE_LOWEST && linetype <= LINE_INDEXABLE_INTO_TABLE_HIGHEST)
  pLinetypeAction = &gl_linetypeActionMap[(gint) linetype - LINE_INDEXABLE_INTO_TABLE_LOWEST];

 if (!pLinetypeAction)
 {
  for (i = LINE_INDEXABLE_INTO_TABLE_HIGHEST - LINE_INDEXABLE_INTO_TABLE_LOWEST + 1; i < sizeof(gl_linetypeActionMap)/sizeof(struct LinetypeAction); i++)
  {
   if (gl_linetypeActionMap[i].m_linetype == linetype)
   {
    pLinetypeAction = &gl_linetypeActionMap[i];
    break;
   } // if (gl_linetypeActionMap[i].m_linetype == linetype && gl_linetypeActionMap[i].m_bMenuEntryCommit)
  } // for (i = LINE_INDEXABLE_INTO_TABLE_HIGHEST - LINE_INDEXABLE_INTO_TABLE_LOWEST + 1; i < sizeof(gl_linetypeActionMap)/sizeof(struct LinetypeAction); i++)
 } //  if (!pLinetypeAction )
 return pLinetypeAction;
}

// ----------------------------------------------------------------------
//called from readFile
enum LineParseResult tryCommit(IN struct MenuEntry* pMenuEntryPending, struct LinetypeAction* pLinetypeAction, IN struct MenuEntry* pMenuEntryPendingOverride)
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult = lineParseOk;

 if (pLinetypeAction->m_bMenuEntryCommit && pMenuEntryPending->m_fnCommit) // may not be set, no entry in progress
 {
  if (pMenuEntryPendingOverride) // if this coming down after an include, may pass icon down to included file
   menuEntryFieldOverride(pMenuEntryPending->m_sIcon, pMenuEntryPendingOverride->m_sIcon);

  lineParseResult = pMenuEntryPending->m_fnCommit(pMenuEntryPending); // commitItem, commitSubMenu
  pMenuEntryPending->m_fnCommit = NULL;
//  if (lineParseResult == lineParseFail || lineParseResult == lineParseFailFatal)
//   return lineParseResult;
 } // if (pLinetypeAction->m_bMenuEntryCommit && pMenuEntryPending->m_fnCommit)
 return lineParseResult;
}

//==============================================================================================
//==============================================================================================

// ----------------------------------------------------------------------
enum LineParseResult onItemCommon(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 menuEntrySet(pMenuEntryPending, commitItem, LINE_ITEM, "item=", TRUE, TRUE, gl_uiCurDepth); // bCmdOk, bIconTooltipOk
 strcpy(pMenuEntryPending->m_sTitle, gl_sLinePostEq);  // pMenuEntryPending->m_sTitle, MAX_LINE_LENGTH, gl_sLinePostEq MAX_LINE_LENGTH
 return lineParseOk; // STATE_ITEM_FOUND;
}

// ----------------------------------------------------------------------
enum LineParseResult onCmdCommon(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult  = menuEntryCheckFieldValidity(pMenuEntryPending->m_sCmd, "cmd",
                                                                     pMenuEntryPending->m_sMenuEntryType,
                                                                     pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 strcpy(pMenuEntryPending->m_sCmd, gl_sLinePostEq); // pMenuEntryPending->m_sCmd, MAX_PATH_LEN, gl_sLinePostEq MAX_LINE_LENGTH
 return lineParseOk; // STATE_CMD_FOUND;
}

// ----------------------------------------------------------------------
enum LineParseResult onIcon(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult  = menuEntryCheckFieldValidity(pMenuEntryPending->m_sIcon, "icon",
                                                                     pMenuEntryPending->m_sMenuEntryType,
                                                                     pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 if (*gl_sLinePostEq)
  strcpy(pMenuEntryPending->m_sIcon, gl_sLinePostEq); // pMenuEntryPending->m_sIcon, MAX_PATH_LEN, gl_sLinePostEq MAX_LINE_LENGTH
 else
  strcpy(pMenuEntryPending->m_sIcon, "NULL");
 return lineParseOk; // commitItem(pMenuEntryPending);
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onSubMenu(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 if (gl_uiCurDepth >= MAX_SUBMENU_DEPTH - 1)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "Maximum submenu depth exceeded");
  return lineParseFail;
 }

 menuEntrySet(pMenuEntryPending, commitSubMenu, LINE_SUBMENU, "submenu=", FALSE, TRUE, gl_uiCurDepth); // bCmdOk, bIconTooltipOk
 strcpy(pMenuEntryPending->m_sTitle, gl_sLinePostEq);  // pMenuEntryPending->m_sTitle, MAX_LINE_LENGTH, gl_sLinePostEq MAX_LINE_LENGTH
 gl_uiCurDepth++;

 return lineParseOk; // STATE_SUBMENU_FOUND;
// return commitSubMenu(pMenuEntryPending);
}

#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
// ----------------------------------------------------------------------
enum LineParseResult onTooltip(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult  = menuEntryCheckFieldValidity(pMenuEntryPending->m_sTooltip, "tooltip",
                                                                     pMenuEntryPending->m_sMenuEntryType,
                                                                     pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;
 strcpy(pMenuEntryPending->m_sTooltip, gl_sLinePostEq);  // pMenuEntryPending->m_sTooltip, MAX_LINE_LENGTH, gl_sLinePostEq MAX_LINE_LENGTH
 return lineParseOk; // STATE_TOOLTIP_FOUND_POST_CMD;
}

#endif // #if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

#if !defined(_GTKMENUPLUS_NO_VARIABLES_)

struct Variable* gl_pVariableHead = NULL;

// ----------------------------------------------------------------------
enum LineParseResult variableAdd(OUT gchar* sErrMsg, OUT struct Variable** ppVariableToEval)
// ----------------------------------------------------------------------
{
 gchar* sEq = strchr(gl_sLinePostEq, '=');

 if (!sEq)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "%s\n", "missing \"=\" in variable declaration");
  return lineParseFail;
 }

 if (sEq == gl_sLinePostEq)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "%s\n", "empty string  before \"=\"");
  return lineParseFail;
 }

 *sEq = '\0';

 gchar* sName = gl_sLinePostEq;
 while (*sName == ' ' || *sName == '\t' ) sName++;

 gchar* sPos = sEq - 1;

 trim_trailing(sName, sPos);

 gboolean bToEvaluate = FALSE;
 gchar* sValue = sEq + 1;
 if (*sValue == '=' )
 {
  sValue++;
  bToEvaluate = TRUE;
 }

 while (*sValue == ' ' || *sValue == '\t' ) sValue++;

 gchar cValue = *sValue;
 if (cValue == '\'' || cValue == '"' ) // check for enclosing quotes
 {
  sValue++;
  sPos = strrchr(sValue, cValue);
  if (sPos) *sPos = '\0';
 }

 if (!isalpha(*sName) && *sName != '_')
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "variable name %s doesn't start with alpha character or underscore\n", sName);
  return lineParseFail;
 }

 gchar* sChar = sName;
 while (*sChar != '\0')
 {
  if (!isalnum(*sChar) && *sChar != '_')
  {
   snprintf(sErrMsg, MAX_LINE_LENGTH, "variable name %s includes other than alpha, digit or underscore\n", sName);
   return lineParseFail;
  }
  sChar++;
 }

 if (getenv(sName))
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "variable name %s already exists in the environment\n", sName);
  return lineParseFail;
 }

 guint nValueLen = strlen(sValue);

 struct Variable* pVariable = gl_pVariableHead;
// struct Variable* pVariableLast = NULL;

 while (pVariable)
 {
  if (strcmp(pVariable->m_sNameValue, sName) == 0)
  {
   pVariable->m_nValueLen = nValueLen;
   if (pVariable->m_nValueLen  > 0)
    strcpy(pVariable->m_sValue, sValue);
   else
   {
   *(pVariable->m_sValue) = '\0';
   }
   *sEq = '=';
   if (bToEvaluate)
    *ppVariableToEval = pVariable;
   return lineParseOk;
  }
//pVariableLast = pVariable;
  pVariable = pVariable->m_pVariableNext;
 }

//name not found, create new var

 pVariable = (struct Variable*) malloc(sizeof(struct Variable));
 if (!pVariable)
 {
  fprintf(stderr, "fatal error: can't allocate memory for new variable.\n");
  return lineParseFailFatal;
 }

 pVariable->m_bUsed = pVariable->m_bEvaluate = FALSE;
 strcpy(pVariable->m_sNameValue, sName);
 guint nNameLen = strlen(sName) + 1;
 pVariable->m_nValueLen = nValueLen;
 pVariable->m_sValue = pVariable->m_sNameValue + nNameLen;
 if (nValueLen)
  strcpy(pVariable->m_sValue, sValue); // name and value in same buffer
 else
  *(pVariable->m_sValue) = '\0';

 pVariable->m_pVariableNext = NULL;
 pVariable->m_pVariableNext = gl_pVariableHead;
 gl_pVariableHead = pVariable;
 *sEq = '=';

 if (bToEvaluate)
  *ppVariableToEval = pVariable;

 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
void variablesClear()
// ----------------------------------------------------------------------
{
 struct Variable* pVariable = gl_pVariableHead;
 while (pVariable)
 {
  if (!pVariable->m_bUsed)
   fprintf(stderr, "variable \"%s\" defined but never used.\n", pVariable->m_sNameValue);

  struct Variable* pVariableNow = pVariable;
  pVariable = pVariable->m_pVariableNext;
  g_free(pVariableNow);
 }
}

#endif

#if !defined(_GTKMENUPLUS_NO_IF_)

// ----------------------------------------------------------------------
enum LineParseResult onIfCommon(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 if (gl_pIfStatusCurrent->m_bInUse)
 {
  if (gl_pIfStatusCurrent->m_pIfStatusFwd)
   gl_pIfStatusCurrent = gl_pIfStatusCurrent->m_pIfStatusFwd;
  else
  {
// TO DO could allocae block
   struct IfStatus* pIfStatusNew = (struct IfStatus*) malloc(sizeof(struct IfStatus)); // need another IfStatus block
   if (!pIfStatusNew)
   {
    fprintf(stderr, "fatal error: can't allocate memory for new if.\n");
//  gl_bFatalError = TRUE;
    return lineParseFailFatal;
   }
   memset(pIfStatusNew, 0, sizeof(struct IfStatus));
   gl_pIfStatusCurrent->m_pIfStatusFwd = pIfStatusNew;
   pIfStatusNew->m_pIfStatusBack = gl_pIfStatusCurrent;
   pIfStatusNew->m_bOnHeap = TRUE;
  } // if (gl_pIfStatusCurrent->m_pIfStatusFwd)
 } // if (gl_pIfStatusCurrent->m_bInUse)

 gl_pIfStatusCurrent->m_bInUse = TRUE;
 gl_pIfStatusCurrent->m_bElseFound = FALSE;
 // Set the initial bCurrentlyAccepting = the outer if='s, if any.
 gl_pIfStatusCurrent->m_bCurrentlyAccepting =
  gl_pIfStatusCurrent->m_pIfStatusBack
   ? gl_pIfStatusCurrent->m_pIfStatusBack->m_bCurrentlyAccepting
   : TRUE; // outermost if= is always accepting.
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onElse(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 if (!gl_pIfStatusCurrent->m_bInUse) // gl_iIfLevel == NO_IFS ||
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "else without if");
  return lineParseFail;
 }

 if (gl_pIfStatusCurrent->m_bElseFound)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "multiple elses after if");
  return lineParseFail;
 }

// if (!gl_pIfStatusCurrent->m_bTestMode)
// {
//  if (!gl_pIfStatusCurrent->m_bTrueConditionFound)
 // This else block is bCurrentlyAccepting if its if= condition is FALSE.
 // AND the outer if=, if any, is also bCurrentlyAccepting.
 gl_pIfStatusCurrent->m_bCurrentlyAccepting =
  !gl_pIfStatusCurrent->m_bTrueConditionFound;
 if (gl_pIfStatusCurrent->m_pIfStatusBack)
  gl_pIfStatusCurrent->m_bCurrentlyAccepting &=
   gl_pIfStatusCurrent->m_pIfStatusBack->m_bCurrentlyAccepting;
// }
 gl_pIfStatusCurrent->m_bElseFound = TRUE;
#if !defined(_GTKMENUPLUS_NO_DEBUG_IF_)
 printIfStatus("onElse", gl_pIfStatusCurrent);
#endif
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onElseIfCommon(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 if (!gl_pIfStatusCurrent->m_bInUse) // gl_iIfLevel == NO_IFS ||
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "elseif without if");
  return lineParseFail;
 }
 if (gl_pIfStatusCurrent->m_bElseFound)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "elseif after else");
  return lineParseFail;
 }

 if (gl_pIfStatusCurrent->m_bTrueConditionFound) //  && !gl_pIfStatusCurrent->m_bTestMode)
 {
  gl_pIfStatusCurrent->m_bCurrentlyAccepting = FALSE;
  return lineParseOk;
 }
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onEndif(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
#if !defined(_GTKMENUPLUS_NO_DEBUG_IF_)
 printIfStatus("onEndif", gl_pIfStatusCurrent);
#endif
 if (!gl_pIfStatusCurrent->m_bInUse) // gl_iIfLevel <= NO_IFS ||
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "endif without if");
  return lineParseFail;
 }
 gl_pIfStatusCurrent->m_bInUse = gl_pIfStatusCurrent->m_bCurrentlyAccepting = gl_pIfStatusCurrent->m_bElseFound =
                               gl_pIfStatusCurrent->m_bTrueConditionFound =  FALSE; // gl_pIfStatusCurrent->m_bTestMode =
 if (gl_pIfStatusCurrent->m_pIfStatusBack)  gl_pIfStatusCurrent = gl_pIfStatusCurrent->m_pIfStatusBack;
 return lineParseOk;
}

#endif


// ---------------------------------------------------------------------- AC
enum LineParseResult onInclude(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 menuEntrySet(pMenuEntryPending, commitInclude, LINE_INCLUDE, "include=", TRUE, TRUE, gl_uiCurDepth); // bCmdOk, bIconTooltipOk
 strcpy(pMenuEntryPending->m_sTitle, gl_sLinePostEq);  // pMenuEntryPending->m_sTitle, MAX_LINE_LENGTH, gl_sLinePostEq MAX_LINE_LENGTH
 return lineParseOk; // STATE_ITEM_FOUND;
}

//==============================================================================================
//==============================================================================================

// ---------------------------------------------------------------------
void addCommentPre(OUT gchar** psCommentPre, OUT gchar* sCommentInline, INOUT guint* pnCommentBuffLen)
// ---------------------------------------------------------------------
{
 guint nCommentLen  = 0;
 guint nCommentBuffLenMin = 1024;

 guint nCommentLenHere = strlen(sCommentInline) + 1;
 if (!(*psCommentPre))
 {
  *psCommentPre = malloc(nCommentBuffLenMin * sizeof(gchar));
  *pnCommentBuffLen = nCommentBuffLenMin;
  strcpy(*psCommentPre,  sCommentInline);
 }
 else
 {
  nCommentLen  = strlen(*psCommentPre) + 1;
  if (nCommentLenHere + nCommentLen >= *pnCommentBuffLen)
  {
   *pnCommentBuffLen *= 2;
   *psCommentPre = realloc(*psCommentPre, *pnCommentBuffLen * sizeof(gchar));
  }
  if (*sCommentInline != '\n') strcat(*psCommentPre, "\n");
  strcat(*psCommentPre, sCommentInline);
 } // if (!(*psCommentPre))
 *sCommentInline = '\0';
}

// ----------------------------------------------------------------------
// called in readLine
gchar* findComment(IN gchar* buf)
// ----------------------------------------------------------------------
{
 // Purpose: decide if a '#' character in buf is to be interpreted as
 // the start of a comment. If so return its position otherwise NULL.
 // Cases when '#' isn't a comment:
 // - Shell code that follows if=/ifelse= and variable evaluation
 // - Quoted variable assignment
 // - HTML color specification, i.e., "#123ABC"
 // FIXME: Handle multiple #s separately, and catch valid comments at EOL preceded by non-comment #s.

 while (' ' == *buf || '\t' == *buf)
  ++buf;
 if(*buf == '#') return buf;
 gchar *sharp =  strchr(buf, '#');
 if (sharp == NULL) return(NULL);

 if (0 == regexec(&gl_rgxSharpIsntComment, buf, 0, NULL, 0))
  return(NULL);

 return sharp;
}

// ----------------------------------------------------------------------
// drive main loop

enum LineType readLine(IN FILE* pFile, OUT gboolean* pbIndentMatters, OUT guint* piDepth,
                       OUT guint*  puiLineNum,   OUT gchar* sLineAsRead,  IN guint nLineBuffLen, IN gboolean bIfAccepting,
                       OUT gchar** psCommentPre, OUT gchar* sCommentInline) // MAX_LINE_LENGTH == nLineBuffLen

// ----------------------------------------------------------------------
// Return kind of line, set and stripped text (gl_sLinePostEq)
// return(LINE_BAD_<Error>), return(0) = EOF, return(>0) = keyword
// doesn't write error messages, that's decided by mainloop
{
 gchar *chop;
 gint i, len, count;
 enum LineType linetype = LINE_UNDEFINED;

 gchar tmp[MAX_LINE_LENGTH + 1];
 gchar *str1, *str2;
 if (sCommentInline) *sCommentInline = '\0';

 guint nCommentBuffLen = 0; // set by addCommentPre
 len = 0;

 while (len == 0)
 {
  if (psCommentPre && sCommentInline && *sCommentInline)
   addCommentPre(psCommentPre, sCommentInline, &nCommentBuffLen);

// Read one line
// *************
  if (pFile)
  {
   if (fgets(sLineAsRead, nLineBuffLen, pFile) == NULL)
    return (LINE_EOF);
   //fprintf(stderr, "%s", sLineAsRead); //DEBUG

   // If an input line is longer than the buffer size we check if it's
   // a comment line. If so, and we aren't "gathering comments", that
   // is storing them, we can let this oversized comment line be.
   if (!psCommentPre) {
    gchar *p = sLineAsRead;
    while (*p == ' ' || *p == '\t')
     ++p;
    if ('#' == *p && '\n' != p[strlen(p) - 1]) {
     // Yep, it's and oversized comment line. Let's move past it.
     int c;
     while ((c = fgetc(pFile)) != EOF && c != '\n')
      ;
     if(c == EOF)
      return (LINE_EOF);
     // So everyone knows we moved past this line.
     sLineAsRead[strlen(sLineAsRead) - 1] = '\n';
    }
   }
   strcpy(tmp, sLineAsRead);
   (*puiLineNum)++; // DEBUG: here set breakpoints on input line #.
   len = strlen(tmp);
   if (tmp[len-1] == '\n')
    tmp[len-1] = '\0';
   else
    return(LINE_BAD_LEN); // line too long
  }
// Or extract next command-line argument section
// *********************************************
  else
  {
   if (!gl_sCmdLineConfigNext)
    gl_sCmdLineConfigNext = strtok(gl_sCmdLineConfig, ";");
   else
    gl_sCmdLineConfigNext = strtok(NULL, ";");
   if (!gl_sCmdLineConfigNext)
    return (LINE_EOF);
   strcpy(sLineAsRead, gl_sCmdLineConfigNext);
   strcpy(tmp, sLineAsRead);
  } // if (pFile)

// Chop comment
  chop = findComment(tmp);
  if (chop != NULL)
  {
   if (sCommentInline) strcpy(sCommentInline, chop);
   *chop = '\0';
  }

  len = strlen(tmp);

  // Remove trailing spaces & CR
  if (len > 0)
  {
//TO DO trim_trailing sag faults
// trim_trailing(tmp, tmp + len - 1);
   chop = &tmp[len - 1];
   while ((chop >= tmp) && (isspace(*chop) != 0))
   {
    *chop = '\0';
    chop--;
   }

   len = strlen(tmp);
  }

  if (sCommentInline && !(*sCommentInline)) strcpy(sCommentInline, "\n");
 }; // while (len == 0) {

 // Big error?  TO DO
 if (len >= nLineBuffLen)
 {
  strncpy(gl_sLinePostEq, tmp, MAX_LINE_LENGTH - 1); //TO DO: WHY
  *(gl_sLinePostEq + MAX_LINE_LENGTH - 1) = '\0';
  return (LINE_BAD_LEN);
 }

 count = 0;

// Calculate menu depth
// ********************
 for (i = 0; i < len; i++)
 {
  if (tmp[i] == ' ')
   count += 1;
  else if (tmp[i] == '\t')    // Tab character = 4 spaces
   count += 4;
  else
   break;
 }

 *piDepth = count / 4;

// Skip leading white space
// ************************
 if (count > 0)
 {
  str1 = tmp;
  str2 = tmp;
  while ((*str2 == ' ') || (*str2 == '\t'))
  {
   str2++;
   len--;
  }
  for (i = 0; i <= len; i++)
   *str1++ = *str2++;
 }
 *pbIndentMatters = TRUE;

 //fprintf(stderr, "DEBUG >readLine %s\n", tmp);
 if (*tmp == '/' || *tmp == '~')
 {
  strcpy(gl_sLinePostEq, tmp);
  return LINE_ABSOLUTE_PATH; // TO DO does *pbIndentMatters = TRUE;
 }

 else
  linetype = getLinetype(tmp, gl_keywordNoVal, sizeof(gl_keywordNoVal), " \t\0", pbIndentMatters); // "separator". maybe else, endif

 if (linetype == LINE_UNDEFINED)
 {
  if (strchr (tmp, '=') == NULL)                 // Its a bad line
  {
   strcpy(gl_sLinePostEq, tmp);
   return (LINE_BAD_NO_EQ);
  }

  linetype = getLinetype(tmp, gl_keyword, sizeof(gl_keyword), "= \t\0", pbIndentMatters);
 } // if (linetype != LINE_UNDEFINED)

 if (linetype == LINE_UNDEFINED) // must be variable
 {
  strcpy(gl_sLinePostEq, tmp);
  return (LINE_KEYWORD_IS_VARIABLE);
 }

#if !defined(_GTKMENUPLUS_NO_IF_)
//no need to get = content if not bCurrentlyAccepting
 if (!bIfAccepting && linetype != LINE_ELSEIF) // don;t need to clean up for endif, else
  return (linetype);
#endif

 trim_trailing(tmp, tmp + strlen(tmp) - 1); // white space

// str2 = tmp + strlen(tmp) - 1;
// while (*str2 == ' ' || *str2 == '\t') str2--;
// if (*(str2 + 1) == ' ' || *(str2 + 1) == '\t')  *(str2 + 1) = '\0';

//remove keywords and leading white space

 *gl_sLinePostEq = '\0'; // in case no "="

 str1 = strchr (tmp, '=');

 if (!str1)  // no =
  return (linetype);

 str2  = str1 + 1;
 while ((*str2 == ' ') || (*str2 == '\t'))
   str2++;
 strcpy(gl_sLinePostEq, str2);
 return (linetype);
} // enum LineType readLine

//==============================================================================================
//==============================================================================================

// ----------------------------------------------------------------------AC
void menuEntrySet(struct MenuEntry* pmeCurrent, IN funcOnMenuEntry fnCommitIn, IN enum LineType lineTypeNow,
                  IN gchar* sMenuEntryType, IN gboolean bCmdOk, IN gboolean bIconTooltipOk, IN guint uiCurDepth)
// ----------------------------------------------------------------------
{
 pmeCurrent->m_fnCommit = fnCommitIn;
 *(pmeCurrent->m_sTitle) = *(pmeCurrent->m_sCmd + 1) = *(pmeCurrent->m_sIcon + 1) = '\0';
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 *(pmeCurrent->m_sTooltip + 1) =  '\0';
 *(pmeCurrent->m_sTooltip) = bIconTooltipOk ? '\0' : MENU_ENTRY_FIELD_DISALLOWED_CHAR;
#endif
 *(pmeCurrent->m_sIcon) = bIconTooltipOk ? '\0' : MENU_ENTRY_FIELD_DISALLOWED_CHAR;
 *(pmeCurrent->m_sCmd) = bCmdOk ? '\0' : MENU_ENTRY_FIELD_DISALLOWED_CHAR;
 strcpy(pmeCurrent->m_sMenuEntryType, sMenuEntryType);
 pmeCurrent->m_uiDepth = uiCurDepth;
 *(pmeCurrent->m_sErrMsg) = '\0';
 *(pmeCurrent->m_sCategory) = '\0';
}

// ---------------------------------------------------------------------- AC
enum LineParseResult menuEntryCheckFieldValidity(INOUT gchar* sField, IN gchar* sFieldName, IN gchar* sMenuEntryType, OUT gchar* sErrMsg)
{
// ----------------------------------------------------------------------
 if (*sField == MENU_ENTRY_FIELD_DISALLOWED_CHAR)
 {
  gchar sContext[100];
  if (*sMenuEntryType)
   sprintf(sContext, "following %s", sMenuEntryType);
  else
   sprintf(sContext, "%s", " outside scope of \"item=\",  \"submenu=\" or \"include=\" lines");
  snprintf(sErrMsg, MAX_LINE_LENGTH, "\"%s=\" field not allowed %s.\n", sFieldName, sContext);
  return lineParseFail;
 }
 else if (*sField != '\0')
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "unexpected \"%s=\" occurs twice following a %s entry.\n", sFieldName, sMenuEntryType);
  return lineParseFail;
 }
 else
  return lineParseOk;
}

// ---------------------------------------------------------------------- AC
void menuEntryFieldOverride(IN gchar* sTarget, IN gchar* sSource)
{
// ----------------------------------------------------------------------
  if (*sSource != '\0' && *sTarget == '\0') strcpy(sTarget, sSource);
}

//==============================================================================================
//==============================================================================================

// ---------------------------------------------------------------------- AC
enum LineParseResult getGtkImage(INOUT struct MenuEntry* pMenuEntryPending, OUT GtkWidget** ppGtkImage)
// ----------------------------------------------------------------------
{
// uses global gint gl_iW, gint gl_iH, sIcon, gl_sIconDirectory
 IN gchar* sIcon = pMenuEntryPending->m_sIcon;
 if (strncasecmp(sIcon, "NULL", 4) == 0) return lineParseOk;    // No icon

 if (*sIcon == '\0' && (!gl_bConfigKeywordIcons || *(pMenuEntryPending->m_sCmd) == '\0')) return lineParseOk; // icon=, or no icon= line !gl_bFindAbsentIcons ||

retry:
 if (*sIcon)
 {
  gchar* sExt = (*sIcon) ? strrchr(sIcon, '.') : NULL;
  if (sExt && regexec(&gl_rgxIconExt, sExt, 0, NULL, 0) != 0) sExt = NULL;

  gboolean bIsPath = (*sIcon) && strchr(sIcon, '/') != NULL;
  if (sExt || bIsPath)
  {
   enum LineParseResult lineParseResult =
    getGtkImageFromFile(pMenuEntryPending->m_sIcon, pMenuEntryPending->m_sErrMsg, ppGtkImage);
   if(lineParseResult != lineParseOk)
   {
     *pMenuEntryPending->m_sErrMsg = '\0';
     if (sExt)
     {
      *sExt = '\0';
      goto retry;
     }
   }
  }
  else // is sIcon, but no extension, and not a filepath
   *ppGtkImage = getGtkImageFromName(pMenuEntryPending->m_sIcon);
 }
 else //if (!*sIcon): no icon line
  *ppGtkImage = getGtkImageFromCmd(pMenuEntryPending->m_sCmd);

 return lineParseOk;
} // enum LineParseResult addIcon(..

// ---------------------------------------------------------------------- AC
enum LineParseResult getGtkImageFromFile(IN const gchar* sFileName, OUT gchar* sErrMsg, OUT GtkWidget** ppGtkImage)
{
 *ppGtkImage = NULL;
 gchar sIcon[MAX_PATH_LEN + 1];
 strcpy(sIcon, sFileName);
 enum LineParseResult lineParseResult = expand_path(sIcon, gl_sIconDirectory, "icon", sErrMsg);// can rewrite sIcon
 if (lineParseResult != lineParseOk)
  return lineParseResult;
// If sIcon is a dir name, programme will hang.

 if (!is_line_file(sIcon, sErrMsg)) // bReportError
  return lineParseWarn;
 GdkPixbuf* pGdkPixbuf = fileToPixBuf(sIcon, gl_iW, FALSE, sErrMsg); // resizes
 if (!pGdkPixbuf) return lineParseWarn; // soft error?
  *ppGtkImage = gtk_image_new_from_pixbuf(pGdkPixbuf);
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
GtkWidget* getGtkImageFromName(IN const gchar* sIcon)
// ----------------------------------------------------------------------
{
 GtkWidget* pGtkImage = gtk_image_new_from_icon_name(sIcon, gl_iW);
 if (gtk_image_get_pixel_size((GtkImage*) pGtkImage ) != gl_iW)
  gtk_image_set_pixel_size((GtkImage*) pGtkImage , gl_iW);
 return pGtkImage ;
}

// ---------------------------------------------------------------------- AC
GtkWidget* getGtkImageFromCmd(IN gchar* sCmd)
// ----------------------------------------------------------------------
{
 GtkWidget* pGtkImage = NULL;
//GtkWidget* pGtkImageMissing  = gtk_image_new_from_icon_name("xxxxx", gl_iW);

 gchar sCmdBuff[MAX_PATH_LEN + 1];
 gchar* sCmdToUse = sCmdBuff;
 get_first_arg(sCmd, sCmdBuff);
 gchar *sCmdExpanded = expand_path_tilda_dot(sCmdBuff, gl_sScriptDirectory);
 if (sCmdExpanded) sCmdToUse  = sCmdExpanded;

 if (is_executable(sCmdToUse))
  pGtkImage  = gtk_image_new_from_icon_name(sCmdToUse, gl_iW);
 else
 {
//not an executable file
  GAppInfo* pGAppInfo = getAppInfoFromFile(sCmdToUse); // also called in RunItem
  if (pGAppInfo)
  {
//http://developer.gnome.org/gio/2.29/GFileInfo.html#g-file-info-get-icon
   GIcon* pGIcon = g_app_info_get_icon(pGAppInfo);
   if (pGIcon)
   {
    gchar* sIconLocal = g_icon_to_string(pGIcon);
    if (sIconLocal)
     pGtkImage = gtk_image_new_from_icon_name(sIconLocal, gl_iW);
//     printf("%s\n", sIconLocal);
//   else
//     printf("%s\n", "no icon name");
   }
  } // if (pGAppInfo)
 } // if (is_executable(sCmdBuff))

 if (pGtkImage)
 {
  if (gtk_image_get_pixel_size((GtkImage*) pGtkImage) != gl_iW)
    gtk_image_set_pixel_size((GtkImage*) pGtkImage, gl_iW);
 } // if (pGtkImage)
// if (pGAppInfo)  g_free(pGAppInfo);   //sig fault

//  if (pGFile) g_free(pGFile); //sig fault
 if (sCmdExpanded) g_free(sCmdExpanded);
 return pGtkImage;
}

// ---------------------------------------------------------------------- AC
// called in RunItem, getGtkImageFromCmd
GAppInfo* getAppInfoFromFile(IN gchar* sCmd)
// ----------------------------------------------------------------------
{
 GFile* pGFile = NULL;

 regmatch_t pmatch[2];

 if (regexec(&gl_rgxUriSchema, sCmd, 1, pmatch, 0) == 0)
 {
  pGFile = g_file_new_for_uri(sCmd);
 }
 else
  pGFile = g_file_new_for_path(sCmd);

 GError* pGError = NULL;
 const gchar* sMimeType = NULL;
 GFileInfo* pGFileInfo =  NULL;

 pGFileInfo = g_file_query_info(pGFile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL,  // cancellable   "standard::content-type,access::can-execute"
                                 &pGError);         // sig fault if g_free(pGFileInfo);
 if (!pGFileInfo)
   return NULL;
//http://stackoverflow.com/questions/1629172/how-do-you-get-the-icon-mime-type-and-application-associated-with-a-file-in-th
  sMimeType = g_file_info_get_content_type(pGFileInfo);
  if (!sMimeType)
   return NULL;

 return g_app_info_get_default_for_type(sMimeType, FALSE); // must_support_uris
}


// ---------------------------------------------------------------------- AC
//called from set_base_dir, addIcon
enum LineParseResult expand_path(INOUT gchar *sPath, IN gchar *sBasePath, IN const gchar* sLabelForErr, OUT gchar* sErrMsg) // sPath is always gl_sLinePostEq, MAX_LINE_LENGTH
// ----------------------------------------------------------------------
{
 gchar cPath1 = *sPath;
 gchar *sPathExpanded = NULL;
 if (cPath1 == '\'' || cPath1 == '"' ) cPath1 = *(sPath + 1);
 if (cPath1 == '/') return lineParseOk; // absolute path

 if (*sBasePath && !strchr(".~", cPath1)) // first gchar not ~, variable, etc;
 {
  guint nChars = strlen(sBasePath) + strlen(sPath) + 1;
  if (nChars >= MAX_PATH_LEN - 1)
  {
   if (sErrMsg) snprintf(sErrMsg, MAX_LINE_LENGTH, "%s path including base path too long.\n", sLabelForErr);
   return lineParseFail;
  }

  gchar sPathBuff[MAX_PATH_LEN + 1];

  snprintf(sPathBuff, MAX_PATH_LEN, "%s%s", sBasePath, sPath);
  strcpy(sPath, sPathBuff);
  return lineParseOk;
 }
 else
 {
  sPathExpanded = expand_path_tilda_dot(sPath, gl_sScriptDirectory); // , NULL
  enum LineParseResult lineParseResult = lineParseOk;
  if (sPathExpanded)
  {
   if (strlen(sPathExpanded) >= MAX_LINE_LENGTH - 1)
   {
    if (sErrMsg) snprintf(sErrMsg, MAX_LINE_LENGTH, "%s expanded path too long\n", sLabelForErr);
    lineParseResult = lineParseFail;
   }
   else
    strcpy(sPath, sPathExpanded);

   g_free(sPathExpanded);
  } // if (sIconExpanded)
  return lineParseResult;
 } // if (*sBasePath && !strchr("/$~(", cIconPath1)) // first gchar not ~, variable, etc;
}


// ----------------------------------------------------------------------
gboolean is_executable(IN gchar* sPath) // called by RunItem
// ----------------------------------------------------------------------
{
 gboolean bExecutable = FALSE;
 if (!strchr(sPath, '/')) return TRUE; // TO DO assume on path; need to check if so

 struct stat sb;
 if(!stat(sPath, &sb))
  bExecutable  = S_ISREG(sb.st_mode) && sb.st_mode & 0111 && access(sPath, F_OK|X_OK) == 0;

 return bExecutable;
}

// ----------------------------------------------------------------------
void get_first_arg(IN const gchar* sPath, OUT gchar* sPathOut) // called by RunItem
// ----------------------------------------------------------------------
{
 strcpy(sPathOut, sPath);
 gchar cPath1 = *sPathOut;
 if (cPath1 == '\'' || cPath1 == '"' ) // quote
 {
  gchar* sPathEnd = strchr(sPathOut + 1, cPath1);
  if (!sPathEnd) return;
  *(sPathEnd + 1) = '\0';
  return; // *(sPathEnd + 1) == '\0' && *(sPath + 1) == '/';
 }
 else
 {
  gchar* sPosWS = strpbrk(sPathOut, " \t");
  if  (!sPosWS )
   return;
  *sPosWS = '\0';
  return;
 } // if (cPath1 == '\'' || cPath1 == '"' ) // quote
//  return strpbrk(sPath, " \t") == NULL; // && cPath1 == '/'; // no white space, no quotes, begins with /
}

// ----------------------------------------------------------------------AC
FILE* open_menu_desc_file(IN gchar* sFileName) // , OUT gboolean* pbIsConfigFileArg
// ----------------------------------------------------------------------
{
 if (strcmp(sFileName, "-") == 0)
 {
//  *pbIsConfigFileArg = FALSE;
  g_print("%s\n", "Reading stdin");
  return stdin;
 }
 else
 {
//  *pbIsConfigFileArg = TRUE;
  FILE* pFile = fopen(sFileName, "r");
  if (pFile == NULL)
  {
//   if (strcasestr(sFileName, "include") && strchr(sFileName, '='))
   g_print("assuming a command string: %s\n", sFileName);
   strcpy(gl_sCmdLineConfig, sFileName);
//   else
//    fprintf(stderr, "Can't open the file.\n");
//   *pbIsConfigFileArg = FALSE;
  }
  else
   g_print("reading the file: %s\n", sFileName);

  return pFile ;
 }
}

// ----------------------------------------------------------------------AC
void trim_trailing(IN gchar* sPosBeg, IN gchar* sPosEnd)
// ----------------------------------------------------------------------
{
 while ((sPosEnd >= sPosBeg) && (*sPosEnd == ' ' || *sPosEnd == '\t')) // isspace(*sPosEnd)
 {
  *sPosEnd = '\0';
  sPosEnd--;
 }
// while (*str2 == ' ' || *str2 == '\t') str2--;
// if (*(str2 + 1) == ' ' || *(str2 + 1) == '\t')  *(str2 + 1) = '\0';
}

/*
   chop = &tmp[len - 1];
   while ((chop >= tmp) && (isspace(*chop) != 0))
   {
    *chop = '\0';
    chop--;
   }
*/


// ---------------------------------------------------------------------- AC
enum LineParseResult checkIconSize(IN guint nSize, OUT gchar* sErrMsg)  // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 if ((nSize < MIN_ICON_SIZE) || (nSize  > MAX_ICON_SIZE))
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "%s\n", "Illegal size for menu icon");
  return lineParseWarn;
 }
 gl_iH = gl_iW = nSize;
 printf("New icon size = %d.\n", gl_iW);
 return lineParseOk;
}

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)

#define S_FORMAT_DIVIDERS ";,"

// ----------------------------------------------------------------------
void formattingInit(INOUT struct Formatting* pFormatting, IN gchar* sFormat, IN guint nMenuLevel)
// ----------------------------------------------------------------------
{
 if (*sFormat == '\0')
  *(pFormatting->m_sFormat) = '\0';
 else
 {
  strcpy(pFormatting->m_sFormat, sFormat);

  pFormatting->m_sMnemonicSet = NULL;
  pFormatting->m_uiMnemonicSetLength =
   pFormatting->m_uiMnemonicIndex[nMenuLevel] = 0;
  if ((pFormatting->m_sMnemonicSet =
     strstr(pFormatting->m_sFormat, "mnemonic=")))
  {
   pFormatting->m_sMnemonicSet += 10; // start of "value"
   pFormatting->m_uiMnemonicSetLength =
    index(pFormatting->m_sMnemonicSet, '"') - pFormatting->m_sMnemonicSet;
   // DON'T terminate m_sMnemonicSet with '\0'.
  }
 }

 pFormatting->m_sFormatSectionEnd = strpbrk(pFormatting->m_sFormat, S_FORMAT_DIVIDERS);

 if (pFormatting->m_sFormatSectionEnd)
 {
  pFormatting->m_cFormatDivider = *(pFormatting->m_sFormatSectionEnd);
  *(pFormatting->m_sFormatSectionEnd) = '\0';
 }
 else
  pFormatting->m_cFormatDivider = '\0';

 snprintf(pFormatting->m_sFormatSection, MAX_LINE_LENGTH, "<span %s>%%s</span>", pFormatting->m_sFormat);
 pFormatting->m_uiMenuLevel = nMenuLevel;
}

// ----------------------------------------------------------------------
void formattingNext(INOUT struct Formatting* pFormatting)
// ----------------------------------------------------------------------
{
 if (!(pFormatting->m_cFormatDivider))
  return;

 gchar* sFormattingNext = NULL;
 if (pFormatting->m_sFormatSectionEnd)
 {
  *(pFormatting->m_sFormatSectionEnd) = pFormatting->m_cFormatDivider;
  sFormattingNext = pFormatting->m_sFormatSectionEnd + 1;
 }
 else
  sFormattingNext = pFormatting->m_sFormat;

 pFormatting->m_sFormatSectionEnd = strchr(sFormattingNext, pFormatting->m_cFormatDivider);
 if (pFormatting->m_sFormatSectionEnd)
  *(pFormatting->m_sFormatSectionEnd) = '\0';

 snprintf(pFormatting->m_sFormatSection, MAX_LINE_LENGTH, "<span %s>%%s</span>", sFormattingNext);
}

#endif // #if  !defined(_GTKMENUPLUS_NO_FORMAT_)

// vim: et ts=1 sts=1 sw=1 tw=0 fdm=syntax
