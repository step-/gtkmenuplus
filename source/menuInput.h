#ifndef _MENUINPUT_H
#define _MENUINPUT_H 1

// version 1.10, 2016-07-25

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include "common.h"

#define MAX_LINE_LENGTH 512     // MUST BE < MAX_PATH_LEN  in common.h
#define MENU_ENTRY_FIELD_DISALLOWED_CHAR '\xFF'
#define INITIAL_NUMB_MENU_ENTRIES 2000
#define MAX_SUBMENU_DEPTH 5
#define MAX_ICON_SIZE 200
#define MIN_ICON_SIZE 10


#define IF_STATUS_SET_COUNT 5

gchar       gl_sIconDirectory[MAX_PATH_LEN + 1];

gchar       gl_sScriptDirectory[MAX_PATH_LEN + 1];

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
gchar       gl_sLauncherDirectory[MAX_PATH_LEN + 1]; // used by onLauncher(), set by onLauncherDir()
gchar       gl_sLauncherArguments[MAX_PATH_LEN + 1]; // used by processLauncher(), set by onLauncherArgs()
#endif

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)

struct Formatting
{
 gchar      m_sFormat[MAX_PATH_LEN + 1];
 gchar      m_sFormatSection[MAX_PATH_LEN + 1];;
 gchar*     m_sFormatSectionEnd;
 gchar      m_cFormatDivider;
 guint      m_uiMenuLevel; // needed for check for submenu lines
};

void   formattingInit(INOUT struct Formatting* pFormatting, IN gchar* sFormat, IN guint nMenuLevel);
void   formattingNext(INOUT struct Formatting* pFormatting);

#endif


#if !defined(_GTKMENUPLUS_NO_FORMAT_) && !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
struct     Formatting gl_FormattingTooltip;
//gchar       gl_sTooltipFormat[MAX_PATH_LEN + 1];

#endif


gchar       gl_sLinePostEq[MAX_LINE_LENGTH + 1];   // used by on...() functions, set by readline
gchar       gl_sCmdLineConfig[MAX_LINE_LENGTH + 1];
//==============================================================================================
//==============================================================================================

FILE*                open_menu_desc_file(IN gchar* sFileName); // sets gl_pFile  , OUT gboolean* pbIsConfigFileArg
gboolean             is_executable(IN gchar* sPath); // called by RunItem
void                 get_first_arg(IN gchar* sPath, OUT gchar* sPathOut); // called by RunItem

void                 trim_trailing(IN gchar* sPosBeg, IN gchar* sPos);

//==============================================================================================
//==============================================================================================

enum LineParseResult { lineParseOk = 0, lineParseWarn = 1, lineParseFail = 3, lineParseFailFatal = 3}; // gl_sLineParseLabel depends on order

enum LineType {                         // returned by readLine
 LINE_UNDEFINED                    = 0, // never returned by readLine


 LINE_INDEXABLE_INTO_TABLE_LOWEST  = 2,
 LINE_ITEM                         = 2, //do not change this value relative to LINE_INDEXABLE_INTO_TABLE_LOWEST
 LINE_CMD                          = 3, //do not change this value relative to LINE_INDEXABLE_INTO_TABLE_LOWEST
 LINE_ICON                         = 4, //do not change this value relative to LINE_INDEXABLE_INTO_TABLE_LOWEST
 LINE_SUBMENU                      = 5, //do not change this value relative to LINE_INDEXABLE_INTO_TABLE_LOWEST
 LINE_INDEXABLE_INTO_TABLE_HIGHEST = 5,
 LINE_TOOLTIP                      = 6,
 LINE_FORMAT                       = 7,
 LINE_TOOLTIP_FORMAT               = 8,
 LINE_SEPARATOR                    = 9,
 LINE_ICON_SIZE                    = 10,
 LINE_POSITION                     = 11,
// LINE_START_PREFERRED_APP_WITH       = 12,
 LINE_ICON_DIRECTORY               = 13,
 LINE_IF                           = 14,
 LINE_ELSE                         = 15,
 LINE_ELSEIF                       = 16,
 LINE_ENDIF                        = 17,
 LINE_LAUNCHER                     = 18,
 LINE_LAUNCHER_ARGS                = 19,
 LINE_LAUNCHER_DIR                 = 20,
 LINE_INCLUDE                      = 21,
 LINE_SUBMENU_END                  = 22,
 LINE_CONFIGURE                    = 23,
 LINE_EOF                          = 24,
 LINE_ERROR                        = 25,  // the error keyword
 LINE_KEYWORD_IS_VARIABLE          = 26,  // will become variable def, not error
 LINE_ABSOLUTE_PATH                = 27,
 LINE_BAD_LIMIT_LOW                = 28,  //move when LINE_KEYWORD_IS_VARIABLE not an error
 LINE_BAD_LEN                      = 28,
 LINE_BAD_NO_EQ                    = 29,
 LINE_BAD_LIMIT_HI                 = 29
};

struct Keyword
{
 const gchar*   m_sKey;
 const gint     m_nLen;
 const gboolean m_bIndentMatters;
 enum  LineType m_linetype;
};

enum LineType        getLinetype(IN gchar * sBuff, IN struct Keyword * keywords, guint uiKwSize, IN gchar * sCharsTerminating, OUT gboolean* pIndentMatters);
const gchar*         getLineTypeName(IN enum LineType linetype);                                            // called by reportLineError

enum LineType        readLine(IN FILE* pFile, OUT gboolean* pbIndentMatters, OUT guint* piDepth, OUT guint* puiLineNum,
                              OUT gchar* sLineAsRead, IN guint nLineBuffLen, IN gboolean bIfAccepting, OUT gchar** psCommentPre, OUT gchar* sCommentInline);

void                 addCommentPre(OUT gchar** psCommentPre, OUT gchar* sCommentInline, INOUT guint* pnCommentBuffLen);

enum LineParseResult checkIconSize(IN guint nSize, OUT gchar* sErrMsg);

//==============================================================================================
//==============================================================================================

struct MenuEntry;
//prototype for LinetypeStateAction
typedef enum LineParseResult (*funcOnMenuEntry)(struct MenuEntry*);

struct MenuEntry
{
 funcOnMenuEntry m_fnCommit;
 guint           m_uiDepth;
 gchar           m_sTitle[MAX_LINE_LENGTH + 1];
 gchar           m_sCmd[MAX_PATH_LEN + 1];
 gchar           m_sIcon[MAX_PATH_LEN + 1];
 gchar           m_sMenuEntryType[20];
 gchar           m_sErrMsg[MAX_LINE_LENGTH + 1];

#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 gchar           m_sTooltip[MAX_LINE_LENGTH + 1];
#endif

};

void                 menuEntrySet(struct MenuEntry* pmeCurrent, IN funcOnMenuEntry fnCommitIn, IN enum LineType lineTypeNow, IN gchar* sMenuEntryType,
                                  IN gboolean bCmdOk, IN gboolean bIconTooltipOk, IN guint uiCurDepth);

enum LineParseResult menuEntryCheckFieldValidity(INOUT gchar* sField, IN gchar* sFieldName, IN gchar* sMenuEntryType, OUT gchar* sErrMsg);
void                 menuEntryFieldOverride(IN gchar* sTarget, IN gchar* sSource);

//==============================================================================================
//==============================================================================================

gboolean      gl_bConfigKeywordUseEndSubMenu;   // set by onConfigure, used by onSubMenuEnd, readFile
gboolean      gl_bConfigKeywordNoIcons;   // set by onConfigure, used by addIcon


// reffed in struct KeywordConfigure gl_keywordConfigure []
enum LineParseResult getAbsPathParts(IN gchar* sBuff, OUT gchar* sErrMsg);
enum LineParseResult getMenuPosArg(IN gchar* sBuff, OUT gchar* sErrMsg);
enum LineParseResult getIconSizeArg(IN gchar* sBuff, OUT gchar* sErrMsg);
//enum LineParseResult getAbsPathTitle(IN gchar* sBuff, OUT gchar* sErrMsg);

enum LineParseResult checkConfigKeywords(IN gchar* sLinePostEq, OUT gchar* sErrMsg);

#if !defined(_GTKMENUPLUS_NO_IF_)

struct IfStatus
{
 gboolean m_bInUse;
 gboolean m_bElseFound;
 gboolean m_bOnHeap;
 gboolean m_bCurrentlyAccepting;
 gboolean m_bTrueConditionFound;
//gboolean bTestMode;
 struct IfStatus* m_pIfStatusBack;
 struct IfStatus* m_pIfStatusFwd;
};

void                 ifStatusInit(OUT struct IfStatus* pIfStatus);
void                 ifStatusFree(OUT struct IfStatus* pIfStatus);

#endif // #if !defined(_GTKMENUPLUS_NO_IF_)

//==============================================================================================
//==============================================================================================

#if !defined(_GTKMENUPLUS_NO_PARAMS_)

struct Params
{
 guint      m_nCmdLineParams;        // may be used by expand_params_vars
 gboolean   m_bIncludesProgName;
 gboolean*  m_pCmdLineParamInUse;
 gchar**    m_sCmdLineParamVec;   // may be used by expand_params_vars
};

gboolean paramsInit(OUT struct Params* pParams, IN int argc, IN gchar *argv[], IN gboolean bIncludesProgName);
void     paramsFinish(OUT struct Params* pParams);

#endif

//==============================================================================================
//==============================================================================================

struct LinetypeAction
{
 enum LineType  m_linetype;
 gboolean        m_bMenuEntryCommit;
 funcOnMenuEntry m_pActionFunc;
};

struct LinetypeAction* getLinetypeAction(IN enum LineType linetype);
enum LineParseResult tryCommit(IN struct MenuEntry* pMenuEntryPending, struct LinetypeAction* pLinetypeAction, IN struct MenuEntry* pMenuEntryPendingOverride);

//==============================================================================================
//==============================================================================================

enum LineParseResult onItem(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onCmd(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onIcon(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onSubMenu(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onIconForSubMenu(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onSeparator(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onIconSize(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onPosition(INOUT struct MenuEntry* pMenuEntryPending);
//enum LineParseResult onPreferredApplicationLauncher(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onIconDir(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onError(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onSubMenuEnd(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onConfigure(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onAbsolutePath(INOUT struct MenuEntry* pMenuEntryPending);

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
enum LineParseResult onLauncher(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onLauncherArgs(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onLauncherDir(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onIconForLauncher(IN gchar* sLauncherPath, IN guint uiDepth, OUT gchar* sErrMsg);
#endif

enum LineParseResult onInclude(INOUT struct MenuEntry* pMenuEntryPending);

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
enum LineParseResult onFormat(INOUT struct MenuEntry* pMenuEntryPending);
#endif

#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
enum LineParseResult onTooltip(INOUT struct MenuEntry* pMenuEntryPending);
#endif

#if  !defined(_GTKMENUPLUS_NO_FORMAT_) && !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
enum LineParseResult onTooltipFormat(INOUT struct MenuEntry* pMenuEntryPending);
#endif

#if !defined(_GTKMENUPLUS_NO_IF_)
enum LineParseResult onIf(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onElse(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onElseif(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onEndif(INOUT struct MenuEntry* pMenuEntryPending);
#endif  // #if !defined(_GTKMENUPLUS_NO_IF_)

enum LineParseResult onEof(INOUT struct MenuEntry* pMenuEntryPending);

#if !defined(_GTKMENUPLUS_NO_VARIABLES_)
enum LineParseResult onVariable(INOUT struct MenuEntry* pMenuEntryPending);
#endif


enum LineParseResult onCmdCommon(INOUT struct MenuEntry* pMenuEntryPending); // accesses gl_sLinePostEq
enum LineParseResult onItemCommon(INOUT struct MenuEntry* pMenuEntryPending); // accesses gl_sLinePostEq

#if !defined(_GTKMENUPLUS_NO_IF_)
enum LineParseResult onIfCommon(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult onElseIfCommon(INOUT struct MenuEntry* pMenuEntryPending);
#endif

//==============================================================================================
//==============================================================================================

#if !defined(_GTKMENUPLUS_NO_VARIABLES_)

struct Variable
{
 gboolean          m_bUsed;
 gboolean          m_bEvaluate;
 guint             m_nValueLen;

//gboolean bTestMode;
 gchar             m_sNameValue[MAX_LINE_LENGTH + 1];
 gchar*            m_sValue;
 struct Variable* m_pVariableNext;
// struct Variable* pVariableBack;
};

void                 variablesClear();
enum LineParseResult variableAdd(OUT gchar* sErrMsg, OUT struct Variable** ppVariableToEval);

#endif  // #if !defined(_GTKMENUPLUS_NO_IF_)

//==============================================================================================
//==============================================================================================

enum LineParseResult commitItem(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult commitSubMenu(INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult commitInclude(INOUT struct MenuEntry* pMenuEntryPending);

//==============================================================================================
//==============================================================================================


enum LineParseResult expand_path(INOUT gchar *sIconPath, IN gchar *sBasePath, IN const gchar* sLabelForErr, OUT gchar* sErrMsg);

//const gchar*  gl_sIconRegexPat = "\\.(png|svg|xpm|gif|jpg|jpeg)$";
//const gchar*  gl_sIconRegexPat = "\\.[:alpha:]{3,4}$";
regex_t       gl_rgxIconExt;
regex_t       gl_rgxUriSchema;

enum LineParseResult getGtkImage(INOUT struct MenuEntry* pMenuEntryPending, OUT GtkWidget** ppGtkImage);
GAppInfo*            getAppInfoFromFile(IN gchar* sCmd);
enum LineParseResult getGtkImageFromFile(IN const gchar* sFileName, OUT gchar* sErrMsg, OUT GtkWidget** ppGtkImage);
GtkWidget*           getGtkImageFromName(IN const gchar* sIcon);
GtkWidget*           getGtkImageFromCmd(IN gchar* sCmd);


#ifdef __cplusplus
}
#endif	/* C++ */

#endif /* ifndef _MENUINPUT_H */

