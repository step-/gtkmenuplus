//gdk_pixbuf_new_from_inline

//g_file_query_info seizes up if http and offline
//getGtkImage: dump icon into iconfolder, if it exists?

/*
 * gtkmenuplus - read a description file and generate a menu.
 * version 1.00, 2013-04-24, by Alan Campbell, 2013
 * Versions after 1.00 by step, 2016-2023, forked from Alan Campbell's 1.00
 *
 * based partially on code in myGtkMenu, copyright (C) 2004-2011 John Vorthman
 * (https://sites.google.com/site/jvinla/home).
 * -------------------------------------------------------------------------
 * This programme is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2, as published
 * by the Free Software Foundation.
 *
 * This programme is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this programme; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * -------------------------------------------------------------------------
 *
 * This programme requires the GTK+ libraries, version 2.4 or later.
 *
 * gtkmenuplus.c, along with auxilary files common.c and launcher.c are used to generate
 * the executable gtkmenuplus
 *
 * see INSTALL file for compilation instructions
 *
 */
//will not be portable to non-GNU systems -- those without gcc and glibc.
#define _GNU_SOURCE

//#define PATH_ELEMENT_SPEC_USES_REGEX 0

#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>
//#include <unistd.h>
#include <stdlib.h>  // exit
#include <sys/stat.h>
#include <ctype.h> // isalpha
#include <time.h>
#include <fcntl.h> // flock
//#include <sys/sysctl.h>  // sysctl
//#include <dirent.h>
#include <errno.h>
#include <assert.h>

#ifndef PATH_ELEMENT_SPEC_USES_REGEX
 #include <fnmatch.h>
#endif


#include "common.h"
#include "menuInput.h"

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
#include "launcher.h"
#include <dirent.h>
#endif
#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_) && !defined(_GTKMENUPLUS_NO_CACHE_)
#include "lru_cache.h"
#endif
#if  !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
#include <libgen.h>
#endif

#define PROGNAME "gtkmenuplus"      // All lowercase!
//#define INCLUDE_RECURSE_FLAG '>'

#define LOCK_NAME "gtkmenuplus_lockfile"

#define UI_RECURSION_DEPTH 10

#define PARAM_REF_TAG '$'

#define VERSION_TEXT VERSION ", WIP, 2023-05-21"

#define DEFAULT_CONFIG_FILE  "test_menu.txt"

#define ABSOLUTE_PATH_ELEMENTS_PARTS 2

#ifdef PATH_ELEMENT_SPEC_USES_REGEX
typedef regex_t GLOB_SPEC_TYPE;
#else
typedef gchar   GLOB_SPEC_TYPE;
#endif

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
extern struct LauncherElement gl_launcherElement[];
extern guint                  gl_nLauncherElements;
gchar gl_sLauncherDB[MAX_PATH_LEN + 1]; // launcher=dir/** list file
gchar gl_sReapedErrMsgs[MAX_LINE_LENGTH + 1]; // launcher{sub}= cumulative errors
int   gl_nLauncherReadLineDepth; // set by main()@readLine when it reads "launcher*="
struct DirFile gl_launcherDirFile; // set by onLauncherDirFile
guint gl_nLauncherCount = 0; // how many .desktop files did effectively display
#endif
guint gl_nHushedUpErrors = 0; // how many error lines readFile did not report

struct Params;

enum LineParseResult readFile(IN struct MenuDescFile* pMenuDescFile, IN int argc, IN gchar *argv[],
                              IN gboolean bReadingIncludedFile, IN gboolean bGatherComments,
                              IN guint uiCurDepthBase, IN struct MenuEntry* pMenuEntryPendingOverride);

static void          RunItem(IN const gchar *sCmd);
static void          QuitMenu(IN gchar *Msg);
static void          menu_position(IN GtkMenu * gl_gtkWmenu, IN gint * x, IN gint * y, OUT gboolean * push_in, IN gpointer sData); // used in onEof as call back routine for gtk_menu_popup
guint                 all_ready_running(void);
enum LineParseResult parseInts(IN gchar *sData, OUT guint* pInt1, OUT guint* pInt2);

extern gboolean          gl_bConfigKeywordUseEndSubMenu, gl_bConfigKeywordIcons;
extern gboolean          gl_bConfigKeywordFormattingLocal, gl_bErrorsInConsoleOnly;
//gchar                   gl_sAbsPathTitleModifier[MAX_PATH_LEN + 1];

guint                     gl_uiMenuX = 0, gl_uiMenuY = 0;
//extern guint            gl_uiMenuX, gl_uiMenuY;
extern gint              gl_iW, gl_iH;                                               // Size of menu icon; used by addIcon, set by onIconSize
guint                     gl_iAbsPathParts =       ABSOLUTE_PATH_ELEMENTS_PARTS;
gboolean                  gl_bSetMenuPos =         FALSE;                        // used by onEof, set by onPosition

guint                     gl_nMenuEntries =      INITIAL_NUMB_MENU_ENTRIES;

gchar                     (*gl_sCmds)[MAX_PATH_LEN + 1]; //dynamically reallocated, varies with gl_nMenuEntries


GtkWidget*                gl_gtkWmenu[MAX_SUBMENU_DEPTH];
GtkWidget*                gl_gtkMenuEntry[MAX_SUBMENU_DEPTH];

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
struct Formatting        gl_FormattingSubMenu[MAX_SUBMENU_DEPTH];
#endif

extern guint             gl_uiCurDepth;                                              // Root menu is depth = 0
extern guint             gl_uiCurItem;                                               // Count number of menu entries
extern guint             gl_uiRecursionDepth;
#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
extern gchar             gl_sActivationLogfile[];
#endif
extern struct IfStatus* gl_pIfStatusCurrent;

gboolean                  gl_bOkToDisplay =             TRUE;
//gboolean                gl_bFatalError =              FALSE;

#if !defined(_GTKMENUPLUS_NO_PARAMS_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)
gboolean                  gl_bOneExpandableOnlyOnLine = FALSE;   // set in expand_params_vars, used by parseIfCondition
#endif

const gchar*              gl_sHelpMsg =
"gtkmenuplus version " VERSION_TEXT
"\n  Display your own popup menu based on some description text."
"\nUsage:"
"\n  gtkmenuplus [options '--'] [file] [parameter ...]"
"\n  gtkmenuplus [options '--'] directive[;directive ...] [parameter ...]"
"\nOptions:"
"\n  -c, --gather-comments"
"\n  -i, --info"
"\n  -h, --help"
"\n  -q, --quiet"
"\n  -v, --version"
"\n  --"
"\nSee the manual pages gtkmenuplus(1) and gtkmenuplus(5) for details."
"\n"
"\nGtkmenuplus comes with ABSOLUTELY NO WARRANTY"
"\nGNU GPLv2 license applies - see license file COPYING."
"\nCopyright (C) 2013 Alan Campbell - forked from myGtkMenu"
"\nCopyright (C) 2016-2023 step"
"\nHome page: " PACKAGE_URL
"\n";
//"\ntest_menu.txt is an example menu_configuration_file.\n"


enum LineParseResult  expand_params_vars(OUT gboolean *pbOneExpandableOnlyOnLine, IN struct Params* pParams, IN gboolean bIsCmdLine, OUT gchar* sErrMsg); // enum TriStateResult, operates on gl_sLinePostEq

enum LineParseResult  expand_param(INOUT gchar** psDataPtr, INOUT gchar** psBuffPtr,
                                  INOUT guint* pnCharsInBuff, OUT gboolean* pbOneExpandableOnlyOnLine, IN struct Params* pParams, OUT gchar* sErrMsg);


gchar*                get_cmdline_menu_desc_file(IN gint argc, IN gchar *argv[]); // gets sFileName   , OUT gboolean* pbIsConfigFileArg

gboolean              initDirectory(OUT gchar* sDirBuff, IN guint nBuffSize, IN gchar* sFileName);                               //called from main()
GtkWidget*            makeItem(IN gchar* sItem, IN gchar* sCmd, IN gchar* sTooltip, IN guint uiDepth);
enum LineParseResult resizeCommandBuffer(OUT gchar* sErrMsg);

enum LineParseResult addIcon(INOUT struct MenuEntry* pMenuEntryPending, INOUT GtkWidget *item);

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
gboolean              addFormatting(INOUT GtkWidget *item, IN gboolean bToSubMenu);
#endif

#if !defined(_GTKMENUPLUS_NO_FORMAT_) || !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

enum FormattingResult { formattingResultNone = 0, formattingResultDoItNotFreeMarkedUpText = 1, formattingResultDoItFreeMarkedUpText = 2};

enum FormattingResult applyFormatting(IN gchar* sText, IN guint uiCurDepth,
                                      INOUT struct Formatting* pFormatting, OUT gchar** psMarkedUpText);

#endif


#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
void                  addTooltip(IN gchar* sTooltip, IN gboolean bToSubMenu, INOUT GtkWidget *item);
#endif

enum LineParseResult  set_base_dir(OUT gchar* sDirBuff, IN const gchar* sLabelForErr, OUT gchar* sErrMsg);  // called by onIconDir()

void                  reportLineError(IN enum LineType linetype, IN enum LineType linetypePrev, OUT gchar* sErrMsg);                        // called once by readFile
void                  msgToUser(IN enum LineParseResult lineParseResult, IN gchar* sErrMsg, IN gchar* fname, IN guint errorStartLineNum, IN guint uiLineNum, IN gchar* sLineAsRead);     // called by readFile, launcherList

enum LineParseResult  includeDirectory(IN gint argc, IN  gchar** argvp, INOUT struct MenuEntry* pMenuEntryPending);
enum LineParseResult  includeFile(IN gint argc, IN  gchar** argvp, INOUT struct MenuEntry* pMenuEntryPending);

enum LineParseResult  processIncludedDirectory(IN gchar* sDirectory, IN gchar* sSubDirectory,
                                              IN GLOB_SPEC_TYPE* sFileSpec, IN GLOB_SPEC_TYPE* sSubDirectorySpec, INOUT struct MenuEntry* pMenuEntryPending);

gboolean              checkSubDirectoryForMatchingFiles(IN gchar* sDirectory, IN gchar* sSubDirectory,
                                                       IN GLOB_SPEC_TYPE* sFileSpec, IN GLOB_SPEC_TYPE* sSubDirectorySpec);

gboolean              bPathElementInluded(IN GLOB_SPEC_TYPE* sSpec, IN gchar* sPathElement);

enum LineParseResult makeSubDirectoryInIncludedDirectory(IN gchar* sSubDirectory, INOUT struct MenuEntry* pMenuEntryPending);

gchar*                getContainingFolderNames(IN gchar* sPath, IN guint nLevel);

#if !defined(_GTKMENUPLUS_NO_IF_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)
enum LineParseResult evaluateExpression(IN const gchar* sLabel, IN gchar* sToEval, OUT gchar* sExprResult, IN guint nExprResultLen, OUT gchar* sErrMsg);
#endif

#if !defined(_GTKMENUPLUS_NO_PARAMS_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)
guint                 intertag_buffer_update(INOUT gchar** psBuffPtr, IN gchar* sBuffIn, IN guint nChars);
#endif



#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
enum LineParseResult  processLauncher(IN gchar* sLauncherPath, IN gboolean stateIfNotDesktopFile, INOUT struct MenuEntry *pme, guint iCaller);
#endif

typedef void (*funcOption)(void);
gboolean              checkOptions(IN gchar* sOption, OUT funcOption* pfOptionAction);


#if !defined(_GTKMENUPLUS_NO_VARIABLES_)

struct Variable*      variableFind(IN gchar* sName);
enum LineParseResult  variableEvaluate(INOUT struct Variable* pVariable, OUT gchar* sErrMsg);

void                  commitIncludeTerminate(INOUT gchar ** argvp, gchar* sLinePostEq);

enum LineParseResult  expand_var(INOUT gchar** psDataPtr, INOUT gchar** psBuffPtr,
                                INOUT guint* pnCharsInBuff, OUT gboolean* pbIsVar, OUT gboolean *pbOneExpandableOnlyOnLine);

#endif  // #if !defined(_GTKMENUPLUS_NO_VARIABLES_)

static gint  doOnExit();
static gchar gl_sOnExit[MAX_LINE_LENGTH + 1] = "";

void        onDashDash();
gboolean    gl_bOptGatherComments = FALSE;
void        onGatherComments();
void        onHelp();
guint       gl_nOptInfo = 0;
void        onInfo();
gboolean    gl_nOptQuiet = 0;
void        onQuiet();
void        onVersion();

//vars, structs, const arrays


// ----------------- structs and data structures ------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

#if !defined(_GTKMENUPLUS_NO_VARIABLES_)

extern struct Variable* gl_pVariableHead;

#endif // #if !defined(_GTKMENUPLUS_NO_VARIABLES_)


//text/plain  txt
//text/html  html://
//text/htmls  html://

#if !defined(_GTKMENUPLUS_NO_IF_)

// used to declare gl_keywordLogic
struct KeywordLogic
{
 gchar*   m_sKeyword;
 gboolean m_bMeaning;
};

// used in parseIfCondition
struct KeywordLogic gl_keywordLogic[] =
{
 {"yes"   , TRUE},
 {"true"  , TRUE},
 {"no"    , FALSE},
 {"false" , FALSE}
};

enum LineParseResult parseIfCondition(IN const gchar* sLabel, OUT gchar* sErrMsg);                            // called from onElseif, onIf

#endif // #if !defined(_GTKMENUPLUS_NO_IF_)

//#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
//#include "launcher.data"
//#endif // #if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)

gchar* gl_sLineBadMsgs[] =
 {
 // "unknown keyword", // LINE_KEYWORD_NOT_KNOWN // will become variable def, not error
  "too long",        // LINE_BAD_LEN
  "no = sign"        // LINE_BAD_NO_EQ
 };

//typedef gboolean (*funcOnNoMenuEntry)(void);

//alternative: http://developer.gnome.org/glib/2.33/glib-Commandline-option-parser.html
struct CommandLineOption
{
 const gchar*  m_sOpt;
 gchar         m_cOpt;
 funcOption    m_pActionFunc;
};

struct CommandLineOption gl_commandLineOption[] =
{
 {"",        '-', onDashDash},
 {"gather-comments", 'c', onGatherComments},
 {"help",    'h', onHelp},
 {"info",    'i', onInfo},
 {"quiet",   'q', onQuiet},
 {"version", 'v', onVersion}
};

guint gl_nLastOption = 0; // command-line menu arguments start at this index +1

extern const gchar*  gl_sIconRegexPat;
extern const gchar*  gl_sUriSchema;
extern const  char*  gl_sSharpIsntComment;
extern int compile_regex (regex_t *, const char *, int);

//TO DO not same in launchers_to_menu

const gchar*    gl_sLauncherExecArg = "[[:space:]]+%[fFuUdDnNickvm][[:space:]]*";
//regex_t       gl_rgxLauncherExecArg in launcher.h
#if !defined(_GTKMENUPLUS_NO_FORMAT_)
const gchar*    gl_sMarkupMnemonic = "[[:space:]]+(mnemonic=\"([^\"]*)\")[^>]*>([^<]*)</span";
regex_t gl_rgxMarkupMnemonic;
#endif

//enum LineParseResult { lineParseOk = 0, lineParseWarn = 1, lineParseNoDisplay = 2, lineParseFail = 3, lineParseFailFatal = 4};
const gchar*    gl_sLineParseLabel[] = {"programming error", "warning",
 "programming error 2", "error", "fatal error"} ;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
int main (int argc, gchar *argv[])
// ----------------------------------------------------------------------
{
 // Allow version check even before testing for another running instance.
 funcOption fOptionAction = NULL;
 if (argc > 1 && checkOptions(argv[1], &fOptionAction) && fOptionAction == onVersion)
  onVersion(); // exits

/*
Some hot keys (keybindings) get carried away and start
many instances. Will try to use a lock file so that
only one instance of this programme runs at a given time.
If this check causes you problems, take it out.
*/

 gint i_all_ready_running = all_ready_running();
 if (i_all_ready_running == 1)
 {
  fprintf(stderr, "Another instance is already running.\n");
  exit(EXIT_FAILURE);
 }
 else if (i_all_ready_running == 2)
 {
  fprintf(stderr, "%s: Error in routine all_ready_running(), "
                     "will quit.\n", argv[0]);
  exit(EXIT_FAILURE);
 }

 if (!gtk_init_check(&argc, &argv))
 {
  fprintf(stderr, "Error, can't initialize gtk.\n");
  exit(EXIT_FAILURE);
 }

//http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/ApplicationProperties
 g_set_prgname(PROGNAME);                    // Used to find PROGNAME.desktop file
 gtk_window_set_default_icon_name(PROGNAME); // Look in ../icons/hicolor/..

 guint i;
 for(i = 1; i < argc; i++)
 {
  if (checkOptions(argv[i], NULL))
   gl_nLastOption = i; // option handler may exit()
 }
 argc -= gl_nLastOption; argv += gl_nLastOption; // shift out own options

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
 formattingInit(&(gl_FormattingSubMenu[0]), "\0", 0);
 if (compile_regex(&gl_rgxMarkupMnemonic, gl_sMarkupMnemonic, REG_EXTENDED))
  errorExit("failed to compile 'mnemonic' regex");
#endif

#if  !defined(_GTKMENUPLUS_NO_FORMAT_) && !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 formattingInit(&gl_FormattingTooltip, "\0", 0);
#endif

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
 *gl_sLauncherDirectory = '\0';
 *gl_sLauncherArguments = '\0';

// late, so no need to do  regfree(&gl_rgxLauncherExecArg) except at end
 if (compile_regex(&gl_rgxLauncherExecArg, gl_sLauncherExecArg, REG_EXTENDED))
  errorExit("failed to compile 'launcher' regex");

#endif // #if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)

#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
 *gl_sActivationLogfile = '\0';
#endif

 if (compile_regex(&gl_rgxIconExt, gl_sIconRegexPat, REG_EXTENDED|REG_ICASE))
  errorExit("failed to compile 'icon' regex");

 if (compile_regex(&gl_rgxUriSchema, gl_sUriSchema, REG_EXTENDED|REG_ICASE))
  errorExit("failed to compile 'URI' regex");

 if (compile_regex(&gl_rgxSharpIsntComment, gl_sSharpIsntComment, REG_EXTENDED|REG_ICASE))
  errorExit("failed to compile 'comment' regex");

 if(!initPathRegex())
  exit(EXIT_FAILURE);

 gl_uiCurItem = gl_uiCurDepth = 0;
 *gl_sScriptDirectory =  *gl_sIconDirectory = *gl_sCmdLineConfig = '\0';

//gboolean bIsConfigFileArg = FALSE; //TO DO may be redundant
 gchar* sFileName = get_cmdline_menu_desc_file(argc, argv); // , &bIsConfigFileArg
 struct MenuDescFile menuDescFile;
 menuDescFile.sName = sFileName;

 FILE* pFile = open_menu_desc_file(&menuDescFile); //, &bIsConfigFileArg

 if (pFile == NULL && *gl_sCmdLineConfig == '\0')
  exit(EXIT_FAILURE);
//TO DO check for command-line config

 if (pFile)
 {
  if (!initDirectory(gl_sIconDirectory, MAX_PATH_LEN, sFileName))
   exit(EXIT_FAILURE);

  strcpy(gl_sScriptDirectory, gl_sIconDirectory);
#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
  strcpy(gl_sLauncherDirectory, gl_sScriptDirectory);
#endif
 }

 gl_gtkWmenu[0] = gtk_menu_new();
 if (!gtk_icon_size_lookup(GTK_ICON_SIZE_BUTTON, &gl_iW, &gl_iH))    // Default
  gl_iW = gl_iH = 30;

 gl_sCmds = malloc(gl_nMenuEntries * (MAX_PATH_LEN + 1) * sizeof(gchar));
 if (!gl_sCmds)
  errorExit("fatal error: can't allocate memory for commands.");

 memset(gl_FormattingSubMenu, 0, sizeof(struct Formatting) * MAX_SUBMENU_DEPTH);

//end  void menuPropertiesInit(OUT MenuProperties* pMenuProperties, IN gchar* sFileName);

 if (!argv[1] || argv[1][0] == '-')
 {
  argc = 0;
//  bIsConfigFileArg = FALSE;
 }
 readFile(&menuDescFile, argc, argv, FALSE, gl_bOptGatherComments, 0, NULL); // bReadingIncludedFile, bGatherComments, uiCurDepthBase, pMenuEntryPending

#if !defined(_GTKMENUPLUS_NO_VARIABLES_)
 variablesClear();
#endif // #if !defined(_GTKMENUPLUS_NO_VARIABLES_)

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
 regfree(&gl_rgxLauncherExecArg);
#endif
 regfree(&gl_rgxIconExt);
 regfree(&gl_rgxUriSchema);
 regfree(&gl_rgxSharpIsntComment);
#if !defined(_GTKMENUPLUS_NO_FORMAT_)
 regfree(&gl_rgxMarkupMnemonic);
#endif


 clearPathRegex();
 g_free(gl_sCmds);

 if (gl_nHushedUpErrors)
  fprintf(stderr,
   "Error messages for %d lines were suppressed. Rerun with option -i to view.\n",
   gl_nHushedUpErrors);
 if (gl_nOptInfo == 1)
  fprintf(stderr,
   "Adding multiple -i options may give you more information.\n");


//onEof();                                   // now called from pLinetypeAction->m_pActionFunc in readFile
 return 0;
}  // int main



// ----------------------------------------------------------------------AC
enum LineParseResult readFile(IN struct MenuDescFile* pMenuDescFile, IN int argc, IN gchar *argv[],
                              IN gboolean bReadingIncludedFile, IN gboolean bGatherComments,
                              IN guint uiCurDepthBase, IN struct MenuEntry* pMenuEntryPendingOverride)
// ----------------------------------------------------------------------
{
 if (++gl_uiRecursionDepth > UI_RECURSION_DEPTH)
 {
  fprintf(stderr, "\"include=\" recursion depth exceeds %d.\n", UI_RECURSION_DEPTH);
  gl_bOkToDisplay = FALSE;
  return lineParseOk;
 }
 FILE *pFile = pMenuDescFile->fp;

 enum LineParseResult lineParseResult = lineParseOk;                 // What kind of input are we looking for?
// enum LineType linetypePrev = LINE_UNDEFINED;                      // used in reportLineError

#if !defined(_GTKMENUPLUS_NO_PARAMS_)
 struct Params params;
 if (!paramsInit(&params, argc, argv, !bReadingIncludedFile)) // bIncludesProgName
  return lineParseFail; // fatal malloc failure
#endif

#if !defined(_GTKMENUPLUS_NO_IF_)
 struct IfStatus ifStatuses[IF_STATUS_SET_COUNT];
 ifStatusInit(ifStatuses);
 struct IfStatus* pIfStatusOld = gl_pIfStatusCurrent;
 gl_pIfStatusCurrent =  &(ifStatuses[0]);
#endif

 struct MenuEntry menuEntryPending;
 menuEntrySet(&menuEntryPending, NULL, LINE_UNDEFINED, "", FALSE, FALSE, 0); // bCmdOk, bIconTooltipOk

 if (pMenuEntryPendingOverride && (*(pMenuEntryPendingOverride->m_sCmd) != '\0' || *(pMenuEntryPendingOverride->m_sTooltip) != '\0'))
  fprintf(stderr, "%s\n", "warning: include=menu-configuration_file followed by tooltip=, and/or cmd=.");

 gboolean bIfAccepting = TRUE;

//set by readLine
 guint uiLineNum = 0; // used by main, addIcon on errors
 gchar sLineAsRead[MAX_LINE_LENGTH + 1];

// MAIN LOOP, read file
 while (TRUE)  // Read next line and get 'linetype'
 {
  enum LineType linetype = LINE_UNDEFINED; // Type of input actually read
  gboolean bIndentMatters = TRUE;
  guint uiDepth = 0;
  gchar* sCommentPre = NULL;
  gchar sCommentInline[MAX_LINE_LENGTH + 1];;
#if !defined(_GTKMENUPLUS_NO_IF_)
  bIfAccepting = !gl_pIfStatusCurrent->m_bInUse || (gl_pIfStatusCurrent->m_bInUse  && gl_pIfStatusCurrent->m_bCurrentlyAccepting);
#endif
  guint errorStartLineNum = uiLineNum; // where an error could start
  linetype = readLine(pFile, &bIndentMatters, &uiDepth, &uiLineNum, sLineAsRead,  MAX_LINE_LENGTH, bIfAccepting,
                      bGatherComments ? &sCommentPre  : NULL,
                      bGatherComments ? sCommentInline : NULL); // != LINE_EOF

  if (sCommentPre) // TO DO temporary fix
  {
   g_free(sCommentPre);
   sCommentPre = NULL;
  }

  uiDepth += uiCurDepthBase;
  gboolean bLineBad = (linetype >= LINE_BAD_LIMIT_LOW && linetype <= LINE_BAD_LIMIT_HI);

  if (bLineBad)
  {
   lineParseResult = lineParseFail;
   snprintf(menuEntryPending.m_sErrMsg, MAX_LINE_LENGTH, "%s\n", gl_sLineBadMsgs[linetype - LINE_BAD_LIMIT_LOW]);
   if (pFile == NULL && uiLineNum < 3)
    strncat(menuEntryPending.m_sErrMsg, "command line possibly misinterpreted\n", MAX_LINE_LENGTH);

   gl_bOkToDisplay = FALSE;
  } // if (linetype >= LINE_BAD_LIMIT_LOW)
#if !defined(_GTKMENUPLUS_NO_IF_)
  else if (gl_pIfStatusCurrent->m_bInUse &&
           !gl_pIfStatusCurrent->m_bCurrentlyAccepting &&
           // NOTE:
           // On !bCurrentlyAccepting readLine() doesn't update gl_sLinePostEq.
           linetype != LINE_IF &&
           linetype != LINE_ELSE &&
           linetype != LINE_ELSEIF &&
           linetype != LINE_ENDIF &&
           linetype != LINE_EOF)
   continue;

#endif // #if !defined(_GTKMENUPLUS_NO_IF_)
  struct LinetypeAction* pLinetypeAction = NULL;

  if (gl_bOkToDisplay)
  {
   pLinetypeAction = getLinetypeAction(linetype);

   if (pLinetypeAction)
   {
    lineParseResult = tryCommit(&menuEntryPending, pLinetypeAction, pMenuEntryPendingOverride);
//  menuEntryPending.m_fnCommit = NULL;

    if (lineParseResult != lineParseOk && lineParseResult != lineParseWarn) gl_bOkToDisplay = FALSE;
   }
   else
   {
    snprintf(menuEntryPending.m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "programming error: getLinetypeAction returns LINE_UNDEFINED"); // both MAX_LINE_LENGTH
    lineParseResult = lineParseFail;
    gl_bOkToDisplay = FALSE;
   } // // if (pLinetypeAction)
  } // if (gl_bOkToDisplay)

//TO DO
// if (*(menuEntryPending.m_sErrMsg)) reportLineError(linetype, linetypePrev, menuEntryPending.m_sErrMsg);

  if (!gl_bConfigKeywordUseEndSubMenu && bIndentMatters && uiDepth > gl_uiCurDepth)
  {
   if (gl_bOkToDisplay)
   {
    snprintf(menuEntryPending.m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "Keyword found at incorrect indentation");
    lineParseResult = lineParseFail;
    gl_bOkToDisplay = FALSE;
   } // if (gl_bOkToDisplay)
  } // if (!gl_bConfigKeywordUseEndSubMenu && bIndentMatters && uiDepth > gl_uiCurDepth)
  else if (!gl_bConfigKeywordUseEndSubMenu && bIndentMatters && uiDepth < gl_uiCurDepth)
  {
   while (uiDepth < gl_uiCurDepth)
   {
#if !defined(_GTKMENUPLUS_NO_FORMAT_)
    if (!gl_FormattingSubMenu[gl_uiCurDepth].m_cFormatDivider) // m_cFormatDivider zero if no compound format
     formattingInit(&(gl_FormattingSubMenu[gl_uiCurDepth]), "\0", 0);
#endif
    gl_uiCurDepth--; // Close submenu
   } // while (uiDepth < gl_uiCurDepth)
  } // else if (!gl_bConfigKeywordUseEndSubMenu && bIndentMatters && uiDepth < gl_uiCurDepth)

  if (gl_bOkToDisplay)
  {
#if !defined(_GTKMENUPLUS_NO_PARAMS_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)
   gl_bOneExpandableOnlyOnLine = FALSE;

   lineParseResult =
#if !defined(_GTKMENUPLUS_NO_PARAMS_)
    expand_params_vars(&gl_bOneExpandableOnlyOnLine, &params, linetype == LINE_CMD, menuEntryPending.m_sErrMsg);                        // gl_bOneExpandableOnlyOnLine used in parseIfCondition; operates on gl_sLinePostEq
#else
    expand_params_vars(&gl_bOneExpandableOnlyOnLine, NULL, linetype == LINE_CMD, menuEntryPending.m_sErrMsg);
#endif
   if(lineParseResult >= lineParseFail)
   {
    gl_bOkToDisplay = FALSE;
   }
#endif // #if !defined(_GTKMENUPLUS_NO_PARAMS_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)

   if (linetype == LINE_EOF && pFile) fclose(pFile); // pFile may be NULL if command line is lne string
  } // if (gl_bOkToDisplay)

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
  gl_nLauncherReadLineDepth = gl_uiCurDepth;
#endif
  if (gl_bOkToDisplay && pLinetypeAction)
   lineParseResult = pLinetypeAction->m_pActionFunc(&menuEntryPending);  // onSubMenu will bump gl_uiCurDepth

  if (*(menuEntryPending.m_sErrMsg))
  {
   // Report error if it matters. It doesn't if it's level lineParseOk and cmdline -i wasn't given.
   if (lineParseResult != lineParseOk)
   {
    msgToUser(lineParseResult, menuEntryPending.m_sErrMsg, pMenuDescFile->sName, errorStartLineNum, uiLineNum, sLineAsRead);
    if (gl_nOptInfo == 0)
     gl_nHushedUpErrors++;
   }
// if (linetype != LINE_IF && linetype != LINE_ELSEIF) # if condition warning shouldn;t
   if (lineParseResult == lineParseFailFatal)
    menuEntrySet(&menuEntryPending, NULL, LINE_UNDEFINED, "", FALSE, FALSE, 0); // bCmdOk, bIconTooltipOk
  }

  if (linetype == LINE_EOF) break;
  if (lineParseResult == lineParseFailFatal) break; // fatal error in onItem
 } // while:  END MAIN LOOP

//see if main loop exited requiring error messages

#if !defined(_GTKMENUPLUS_NO_PARAMS_)
 paramsFinish(&params);
#endif

#if !defined(_GTKMENUPLUS_NO_IF_)

 if (gl_pIfStatusCurrent->m_bInUse && (lineParseResult == lineParseOk || lineParseResult == lineParseWarn))
  fprintf(stderr, "if apparently not closed by endif.\n");

 ifStatusFree(ifStatuses); // &ifStatuses[IF_STATUS_SET_COUNT - 1]
 gl_pIfStatusCurrent = pIfStatusOld;

#endif // #if !defined(_GTKMENUPLUS_NO_IF_)

 gl_uiRecursionDepth--;
 return lineParseResult;
}

// ----------------------------------------------------------------------
gchar* shorten(IN gchar *in, OUT gchar *out) //called by msgToUser, reapErrMsg
// ----------------------------------------------------------------------
{ // Shorten string by erasing obvious paths.
 gchar *p; gchar *q; gchar *r;
 gchar *path; guint npath;
 path = gl_sScriptDirectory; // obvious path
 if (!*path)
 {
  if(in != out)
   strcpy(out, in);
  return out;
 }
 npath = strlen(path);
 char *result = strdup(in);
 if (!result)
  perror("strdup");
 else
 {
  p = in;
  q = in;
  r = result;
  while((q = strstr(q, path)))
  {
   while (p != q)
    *r++ = *p++;
   *r = '\0';
   p += npath;
   q += npath;
  }
  while (*p)
   *r++ = *p++;
  *r = '\0';
  strcpy(out, result);
 }
 if (result) free(result);
 return out;
}

// ----------------------------------------------------------------------
//called by readFile, launcherList
void  msgToUser(IN enum LineParseResult lineParseResult,
                IN gchar* sErrMsg,
                IN gchar* fname,
                IN guint errorStartLineNum,
                IN guint uiLineNum,
                IN gchar* sLineAsRead)
// ----------------------------------------------------------------------
{
 gchar ctx[MAX_LINE_LENGTH] = "";
 gchar start[MAX_LINE_LENGTH] = "";
 guint i = 0;

 if (errorStartLineNum > 0 && fname) {
  FILE* fp;
  if ((fp = fopen(fname, "r"))) {
   while (i < errorStartLineNum && fgets(start, MAX_LINE_LENGTH, fp))
    i++; // seek to error start line
   fclose(fp);
  }
 }
 gboolean bRealFile = fname && strcmp(fname, sLineAsRead); // not command-line expression
 snprintf(ctx, MAX_LINE_LENGTH, "* in line range *\n% 4d: %s% 4d: %s%s%s\n",
  errorStartLineNum, i == errorStartLineNum && *start ? start : "-",
  uiLineNum, sLineAsRead,
  bRealFile ? "file: " : "", bRealFile ? fname : "");

 if (lineParseResult == lineParseOk) //shouldn't happen
  fprintf(stderr, "%s: in msgToUser at line %d\n", gl_sLineParseLabel[lineParseResult], uiLineNum);
 else if (gl_nOptInfo > 0 || lineParseResult != lineParseWarn)
 {
  shorten(sErrMsg, sErrMsg);
  fprintf(stderr, "%s: %s%s\n", gl_sLineParseLabel[lineParseResult], sErrMsg, ctx);
 }

 // http://stackoverflow.com/questions/13204177/how-to-find-out-if-running-from-terminal-or-gui
 gboolean bLaunchedFromCLI = isatty(fileno(stdin));

 if (lineParseResult >= lineParseFail && !gl_bErrorsInConsoleOnly && !bLaunchedFromCLI)
 {
  GtkWidget* pGtkMsgDlg = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                  "%s: %s%s\n", gl_sLineParseLabel[lineParseResult], sErrMsg, ctx);
  if (pGtkMsgDlg)
  {
   gtk_dialog_run(GTK_DIALOG(pGtkMsgDlg));
   gtk_widget_destroy(pGtkMsgDlg);
  }
 }

 *sErrMsg = '\0';
}

/*
// ----------------------------------------------------------------------
//called from main loop
//could be implemented as switch(linetype) within switch(stateNow)
*/

// ----------------------------------------------------------------------
/*
//called once by readFile
void reportLineError(enum LineType linetype, enum LineType linetypePrev, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 gchar sMsgBuff[MAX_LINE_LENGTH + 1];
 strcpy(sMsgBuff, "Misplaced line with ");

 guint nLen = strlen(sMsgBuff);
 gchar* sMsgBuffp = sMsgBuff + nLen;

 const gchar* sKeyword = getLineTypeName(linetype);

 if (sKeyword)
 {
  snprintf(sMsgBuffp, MAX_LINE_LENGTH - nLen, "keyword \"%s\"", sKeyword);
 }
 else
  strcat(sMsgBuff, "unknown keyword");

 if (linetypePrev != LINE_UNDEFINED)
 {
  sKeyword = getLineTypeName(linetypePrev);
  if (sKeyword)
  {
   nLen = strlen(sMsgBuff);
   sMsgBuffp = sMsgBuff + nLen;
   snprintf(sMsgBuffp, MAX_LINE_LENGTH - nLen, ".  preceding line had keyword \"%s\"\n", sKeyword);
  } // if (sKeyword)
 } // if (linetypePrev != LINE_UNDEFINED)
 snprintf(sErrMsg, MAX_LINE_LENGTH, "%s", sMsgBuff); // both MAX_LINE_LENGTH
}
*/

// ----------------------------------------------------------------------
//called from main(), onLauncherDir
gboolean initDirectory(OUT gchar* sDirBuff, IN guint nBuffSize, IN gchar* sFileName)
// ----------------------------------------------------------------------
{
 if (strcmp(sFileName, "-") == 0) // piped in
 {
  strcpy(sDirBuff, getenv("HOME"));
  if(0 == gl_nOptQuiet)
   g_print("getting input from stdin, using %s as working directory\n", sDirBuff);
  strcat(sDirBuff, "/");

//fprintf(stderr, "%s\n", "can't get current working directory when getting input from stdin.");
  return TRUE;
 }

 gchar sAbsPath[MAX_PATH_LEN + 1];
 strcpy(sAbsPath, sFileName);
#ifdef NEVER_DEFINED
 if (*sAbsPath != '/') // work-around for a problem in expand_path
  make_absolute_path(sAbsPath, sAbsPath); // ignore error, if any
 // If an error occurred here, there are still chances that it won't
 // matter.  If it will matter then other errors will be caught later.
#endif //NEVER_DEFINED

 gchar* pLastSep = strrchr(sAbsPath, '/');
 gboolean bNoPath = FALSE;
 if (pLastSep)
 {
  guint nLen = pLastSep - sAbsPath + 1;
  if (nLen < nBuffSize - 1)
  {
   strncpy(sDirBuff, sAbsPath, nLen);
   sDirBuff[nLen] = '\0';
  }
  else
  {
   fprintf(stderr, "path too long: %s.\n", sFileName);
   return lineParseFail;
  } // if (nLen < nBuffSize - 1)
 } // if (pLastSep)
 else
  bNoPath = TRUE;

 if (bNoPath || strlen(sDirBuff) == 2)
 {
  if (!getcwd(sDirBuff, nBuffSize))
  {
   fprintf(stderr, "can't get current working directory: %s.\n", sFileName);
   return lineParseFail;
  }
  guint nBuffLen = strlen(sDirBuff);
  if (nBuffLen < nBuffSize - 1 && *(sDirBuff + nBuffLen - 1) != '/')
    strcat(sDirBuff, "/");
 } // if (bNoPath || strlen(sDirBuff) == 2)
 return TRUE;
}

// ----------------------------------------------------------------------
void menu_position (IN GtkMenu * gl_gtkWmenu, IN gint * x, IN gint * y, OUT gboolean * push_in, IN gpointer sData) // used in onEof as call back routine for gtk_menu_popup
// ----------------------------------------------------------------------
{
 *x = gl_uiMenuX;
 *y = gl_uiMenuY;
 *push_in = TRUE;
}    // void menu_position


// ----------------------------------------------------------------------
static void RunItem(IN const gchar *sCmd)
// ----------------------------------------------------------------------
{
 if (!sCmd) return;

#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
 guint writeLogItem(IN const gchar *);
 (void) writeLogItem(sCmd);
#endif // _GTKMENUPLUS_NO_ACTIVATION_LOG_

 GError *error = NULL;
 gchar *sCmdExpanded = NULL;
 gchar *sCmdExpandedWithPAL = NULL;

 if(0 == gl_nOptQuiet)
  g_print("Run: %s\n", sCmd);

 sCmdExpanded = expand_path_tilda_dot(sCmd, gl_sScriptDirectory);

 if (sCmdExpanded) sCmd = sCmdExpanded;

 gchar gcCmd1 = *sCmd;
 if (gcCmd1 == '\'' || gcCmd1 == '"' )
  gcCmd1 = *(sCmd + 1);

 gchar sCmdBuff[MAX_PATH_LEN + 1];
 get_first_arg(sCmd, sCmdBuff);
 if (!is_executable(sCmdBuff))
 {
// guint nChars = strlen(gl_sPreferredApplicationLauncher) +  strlen(sCmd) + 1;
  GAppInfo* pGAppInfo = getAppInfoFromFile(sCmdBuff); // TO DO also called in getGtkImageFromCmd
  if (pGAppInfo)
  {
   const gchar* sExe = g_app_info_get_executable(pGAppInfo);
   guint nChars = strlen(sExe) +  strlen(sCmd) + 2;
   gchar* sTemp = gl_sLinePostEq;
   if (nChars > MAX_LINE_LENGTH)
    sTemp = sCmdExpandedWithPAL = (gchar *) g_malloc((nChars) * sizeof(gchar));
   snprintf(sTemp, nChars * sizeof(gchar), "%s %s", sExe, sCmd); // gl_sPreferredApplicationLauncher
   sCmd = sTemp;
  }
 } // if (!is_executable(sCmd))

 // Don't export menu's custom theme, if any.
 unsetenv("GTK2_RC_FILES");

 if (!g_spawn_command_line_async(sCmd, &error))
 {
  fprintf(stderr, "Error running command.\n");
  fprintf(stderr, "%s\n", error->message);
  g_error_free(error);
 }

 if (sCmdExpanded) g_free(sCmdExpanded);
 if (sCmdExpandedWithPAL) g_free(sCmdExpandedWithPAL);

 gtk_main_quit();

}    // static void RunItem

// ----------------------------------------------------------------------
gchar* get_cmdline_menu_desc_file(IN gint argc, IN gchar *argv[]) // , OUT gboolean* pbIsConfigFileArg
// ----------------------------------------------------------------------
{
 gchar* sFileName = NULL;

 if (argc < 2)
 {
//  *pbIsConfigFileArg = FALSE;
  fprintf(stderr, gl_sHelpMsg);
  fprintf(stderr, "%s","Missing the menu-description filename.\n");
  if(0 == gl_nOptQuiet)
   g_print("%s","Will try to open the default file.\n");
  memset(gl_sLinePostEq, 0, sizeof(gl_sLinePostEq));
  strncpy(gl_sLinePostEq, argv[0], sizeof(gl_sLinePostEq) - 1);    // Get gtkmenuplus path
  guint i = strlen (gl_sLinePostEq);

  while ((i > 0) && (gl_sLinePostEq[i] != '/'))
    gl_sLinePostEq[i--] = '\0';    // Remove filename
  if ((i > 0) && (i < sizeof(gl_sLinePostEq) - strlen(DEFAULT_CONFIG_FILE + 1)))
  {
   strcat(gl_sLinePostEq, DEFAULT_CONFIG_FILE);
   sFileName = gl_sLinePostEq;
  }
  else
   sFileName = DEFAULT_CONFIG_FILE;
 } // if (argc < 2)
 else
 {
//  *pbIsConfigFileArg = TRUE;
  sFileName = (gchar *) argv[1];
 }

 return sFileName;
}

// ----------------------------------------------------------------------
static void QuitMenu(IN gchar *Msg) {
// ----------------------------------------------------------------------
 if(0 == gl_nOptQuiet)
  g_print("Menu was deactivated.\n");
 gtk_main_quit();
 doOnExit();
}    // static void QuitMenu

// ---------------------------------------------------------------------
guint all_ready_running (void)
// ---------------------------------------------------------------------
{
 struct flock fl;
 guint fd;
 gchar *home;
 guint n;
 guint ret = 0;
 gchar *Lock_path;

// Use lock file 'LOCK_NAME' to see if programme is already running.
// 0 = No, 1 = Yes, 2 = Error

// Place lock in the home directory of user
// If user messes with "HOME", will probably have problems

 home = (gchar *) getenv("HOME");

 Lock_path = malloc(MAX_PATH_LEN + 1);
 n = snprintf(Lock_path, MAX_PATH_LEN, "%s/.%s", home, LOCK_NAME);

 if (n > MAX_PATH_LEN || Lock_path == NULL)
 {
  fprintf(stderr, "Error, path name too long: %s.\n", Lock_path);
  ret = 2;
 }

 if (!ret)
 {
  fd = open(Lock_path, O_RDWR | O_CREAT, 0600);
  if (fd < 0)
  {
   fprintf(stderr, "Error opening %s.\n", Lock_path);
   ret = 2;
  }
 } // if (!ret)

 if (!ret)
 {
  fl.l_start = fl.l_len = 0;
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;

  ret = fcntl(fd, F_SETLK, &fl) < 0 ? 1 : 0;
 } // if (!ret)

 free(Lock_path);

 return ret;
} // all_ready_running


#if !defined(_GTKMENUPLUS_NO_FORMAT_)

void eraseMnemonicAttribute(INOUT gchar *s)
{
 // Erase the first occurrence of: mnemonic="value".
 gchar *p, *q;
 if ((p = strstr(s, "mnemonic=")))
 {
  q = index(p + 10, '"'); // closing "
  while(p <= q)
   *(p++) = ' ';
 }
}

void addMnemonic(INOUT gchar *sMarkup, IN struct Formatting *pFormatting, IN guint uiCurDepth)
{
 gchar *p;
 guint len;
 regmatch_t pmatch[4];
 // [[:space:]]+(mnemonic=\"([^\"]*)\")[^>]*>([^<]*)</span>";
 if (regexec(&gl_rgxMarkupMnemonic, sMarkup, 4, pmatch, 0) == 0)
 {
  //      <span mnemonic="value" something else   >this item label</span>
  // p:        0                                                        0
  // p:         1              1
  // p:                   2   2
  // p:                                            3             3
  // Examine "value".
  if (0 == strncmp("1", sMarkup + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so))
  {
   //   case: value="1" and label doesn't already include mnemonic
   //   action: prepend '_' to label
   if (! index(sMarkup + pmatch[3].rm_so, '_'))
   {
    // start with sLabel:
    //     <span      mnemonic="value" something else   >this item label</span>
    // rewrite:
    //     <span      something else   >mething else   >this item label</span>
    strncpy(p = sMarkup + pmatch[1].rm_so, sMarkup + pmatch[1].rm_eo,
            len = pmatch[3].rm_so - pmatch[1].rm_eo);
    //     <span      something else   >_ething else   >this item label</span>
    *(p += len) = '_';
    //     <span      something else   >_this item label</span>em_label</span>
    strncpy(++p, sMarkup + pmatch[3].rm_so,
            len = pmatch[0].rm_eo - pmatch[3].rm_so + 1);
    //     <span      something else   >_this item label</span>
    *(p += len) = '\0';
   } else { // "value" already includes '_'
    eraseMnemonicAttribute(sMarkup);
   }

  } else if(pmatch[2].rm_eo > pmatch[2].rm_so) // non-null "value"
  {
   //   case: value="ANY" (may override existing mnemonic)
   //   action: prepend "_A ", "_N ", "_Y ", "_A ", ... to labels (by menu level)

   // start with sLabel:
   //     <span      mnemonic="ANY" something else   >this item label</span>
   // rewrite:
   //     <span      something else   >thing else   >this item label</span>
   strncpy(p = sMarkup + pmatch[1].rm_so -1, sMarkup + pmatch[1].rm_eo,
           len = pmatch[3].rm_so - pmatch[1].rm_eo);
   //     <span      something else   >_A ng else   >this item label</span>
   *(p += len) = '_';
   // Round-the-clock take the next character of "value" for this menu level.
   *(++p) = *(pFormatting->m_sMnemonicSet +
    (pFormatting->m_uiMnemonicIndex[uiCurDepth]++ % pFormatting->m_uiMnemonicSetLength));
   *(++p) = ' ';
   //     <span      something else   >_A this item label</span>abel</span>
   strncpy(++p, sMarkup + pmatch[3].rm_so,
           len = pmatch[0].rm_eo - pmatch[3].rm_so + 1);
   //     <span      something else   >_this item label</span>
   *(p += len) = '\0';

  } else { // "value" is ""
   eraseMnemonicAttribute(sMarkup);
  }
 }
}

// ---------------------------------------------------------------------- AC
gboolean addFormatting(INOUT GtkWidget *item, IN gboolean bToSubMenu) // TO DO ERRORS NOT POSSIBLE??
// ----------------------------------------------------------------------
{
 GtkWidget * gtkLabel = gtk_bin_get_child((GtkBin*) item);
 const gchar* sItemText = gtk_label_get_label((GtkLabel*) gtkLabel);

 gchar* sMarkedUpText = NULL;
 gint uiCurDepth = MAX(0, gl_uiCurDepth - (bToSubMenu ? 1 : 0));
 enum FormattingResult formattingResult =
  applyFormatting((gchar*) sItemText, uiCurDepth,
    &(gl_FormattingSubMenu[uiCurDepth]), &sMarkedUpText);
 if (formattingResult != formattingResultNone)
  gtk_label_set_markup_with_mnemonic((GtkLabel*) gtkLabel, sMarkedUpText); // ret void

 if (formattingResult == formattingResultDoItFreeMarkedUpText && sMarkedUpText) // && !bIsFormattingLocal
   g_free(sMarkedUpText);

 return TRUE;
}

#endif // #if !defined(_GTKMENUPLUS_NO_FORMAT_)


#if !defined(_GTKMENUPLUS_NO_FORMAT_) || !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

// ---------------------------------------------------------------------- AC
//enum FormattingResult applyFormatting(IN gchar* sText, IN gchar* sFormat, OUT gchar** psMarkedUpText)
enum FormattingResult applyFormatting(IN gchar* sText, IN guint uiCurDepth,
                                      INOUT struct Formatting* pFormatting, OUT gchar** psMarkedUpText)
// ----------------------------------------------------------------------
{
 *psMarkedUpText = NULL;

 // Ignore mnemonic="value" in local formatting.
 eraseMnemonicAttribute(sText); // if any

 gboolean bIsFormattingLocal  = strstr(sText, "<span") != NULL;
//m_cFormatDivider 0 if formatting not compound
 gboolean bIsFormattingGlobal = *(pFormatting->m_sFormat) &&
  (!(pFormatting->m_cFormatDivider) || (uiCurDepth == pFormatting->m_uiMenuLevel));

 if (bIsFormattingLocal)
 {
  if (bIsFormattingGlobal)
  {
   // Nest local <span> (sText) within global <span> (m_sFormatSection).
   gchar *p;

   if ((p = strstr(pFormatting->m_sFormatSection, "mnemonic=")))
   {
    gchar *q = index(p + 10, '"'); // closing "
    if (q) *q = '\0'; // else it's malformed markup; let GTK complain for us
    // inject inner "mnemonic="..."
    gchar *s = g_strdup_printf("<span %s\"%s", p, sText + 5);
    if (q) *q = '"';
    addMnemonic(s, pFormatting, uiCurDepth);
    *psMarkedUpText = g_strdup_printf(pFormatting->m_sFormatSection, s);
    g_free(s);
    // Erase outer "mnemonic="..."; the inner one that we just added remains.
    eraseMnemonicAttribute(*psMarkedUpText);
   }
   else
   {
    *psMarkedUpText = g_strdup_printf(pFormatting->m_sFormatSection, sText);
   }
   formattingNext(pFormatting);
   return formattingResultDoItFreeMarkedUpText;
  }
  else
  {
   *psMarkedUpText  = (gchar*) sText; // cast off const
   return formattingResultDoItNotFreeMarkedUpText;
  }

 }
 else if (bIsFormattingGlobal) // (*sFormat)
 {
//*psMarkedUpText = g_markup_printf_escaped(sFormat, sText); // gl_sItemText always returns not NULL??
  *psMarkedUpText = g_markup_printf_escaped(pFormatting->m_sFormatSection, sText); // gl_sItemText always returns not NULL??
  addMnemonic(*psMarkedUpText, pFormatting, uiCurDepth); // if any
  formattingNext(pFormatting);
  return formattingResultDoItFreeMarkedUpText;
 }
 else
  return formattingResultNone;
}

#endif // #if !defined(_GTKMENUPLUS_NO_FORMAT_)

#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

// called by commitSubMenu, makeItem
// ---------------------------------------------------------------------- AC
void  addTooltip(IN gchar* sTooltip, IN gboolean bToSubMenu, INOUT GtkWidget *item)
// ----------------------------------------------------------------------
{
 if (!sTooltip || !*sTooltip)
  return;

 gchar* sMarkedUpText = NULL;
//enum FormattingResult formattingResult = applyFormatting((gchar*) sTooltip, gl_sTooltipFormat, &sMarkedUpText);
 enum FormattingResult formattingResult = applyFormatting((gchar*) sTooltip, bToSubMenu, &gl_FormattingTooltip, &sMarkedUpText);
 if (formattingResult != formattingResultNone)
  gtk_widget_set_tooltip_markup(item, sMarkedUpText);
 else
  gtk_widget_set_tooltip_text(item, sTooltip);

 if (formattingResult == formattingResultDoItFreeMarkedUpText && sMarkedUpText) // && !bIsFormattingLocal
   g_free(sMarkedUpText);

// *sTooltip = '\0'; // need to keep, will be reused in include

 return;
}

#endif // #if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

// ---------------------------------------------------------------------- AC
enum LineParseResult addIcon(INOUT struct MenuEntry* pMenuEntryPending, INOUT GtkWidget *item)
// ----------------------------------------------------------------------
{
 GtkWidget* pGtkImage = NULL;
 enum LineParseResult lineParseResult = getGtkImage(pMenuEntryPending, &pGtkImage);
 if (lineParseResult != lineParseOk)
   return lineParseResult;

 if (pGtkImage)
 {
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM (item), pGtkImage);
#if !defined(_GTKMENUPLUS_NO_ALWAYS_SHOW_ICONS_)
  gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM (item), TRUE);
#endif
 }

 return lineParseOk;
}

#if !defined(_GTKMENUPLUS_NO_PARAMS_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)
// ---------------------------------------------------------------------- AC
enum LineParseResult expand_params_vars(OUT gboolean *pbOneExpandableOnlyOnLine, IN struct Params* pParams, IN gboolean bIsCmdLine, OUT gchar* sErrMsg) // enum TriStateResult // operates on gl_sLinePostEq
// ----------------------------------------------------------------------
{
 gchar sBuff[MAX_LINE_LENGTH + 1];
 memset(sBuff, 0, (MAX_LINE_LENGTH  + 1) * sizeof(gchar));

 guint nLenParamTag = 1; // strlen(PARAM_REF_TAG);

 guint nCharsInBuff = 0;
 *pbOneExpandableOnlyOnLine = FALSE;

 gchar* sBuffPtr = sBuff;

 gchar* sDataPtr = strchr(gl_sLinePostEq, PARAM_REF_TAG);
 if (!sDataPtr)
  return lineParseOk;

 if (sDataPtr == gl_sLinePostEq)
  *pbOneExpandableOnlyOnLine = TRUE;
 else
  nCharsInBuff += intertag_buffer_update(&sBuffPtr, gl_sLinePostEq, sDataPtr - gl_sLinePostEq);

 enum LineParseResult lineParseResult = lineParseOk;

 while (sDataPtr && *sDataPtr)
 {
  gboolean bCharIsDigit = isdigit(*(sDataPtr + nLenParamTag));
#if !defined(_GTKMENUPLUS_NO_PARAMS_)
  if (bCharIsDigit)
  {
   if (!bIsCmdLine)
   {
    lineParseResult = expand_param(&sDataPtr, &sBuffPtr, &nCharsInBuff, pbOneExpandableOnlyOnLine, pParams, sErrMsg);
    if (lineParseResult >= lineParseFail) break;
   } // if (!bIsCmdLine)
   else
   {
    *sBuffPtr = *sDataPtr;
    sBuffPtr++;
    sDataPtr++;
   }
  } // if (bCharIsDigit)
#endif

#if !defined(_GTKMENUPLUS_NO_VARIABLES_)
  if (!bCharIsDigit)
  {
   gboolean bIsVar = FALSE;
   lineParseResult = expand_var(&sDataPtr, &sBuffPtr, &nCharsInBuff, &bIsVar, pbOneExpandableOnlyOnLine);
   if (lineParseResult >= lineParseFail) {
    snprintf(sErrMsg, MAX_LINE_LENGTH, "line too long after variable expansion\n");
    return lineParseFail;
   }

   if (!bIsVar)
   {
    *sBuffPtr = PARAM_REF_TAG;
    *(sBuffPtr + 1) = '\0';
    sBuffPtr += nLenParamTag;
    sDataPtr += nLenParamTag;
   } // if (!bIsVar)
  } // if (!bCharIsDigit)
#endif

  if  (*sDataPtr)
  {
   gchar* sDataPtrNext = strchr(sDataPtr, PARAM_REF_TAG);
   if (sDataPtrNext)
    nCharsInBuff += intertag_buffer_update(&sBuffPtr, sDataPtr, sDataPtrNext - sDataPtr);
   else
    strcpy(sBuffPtr, sDataPtr);

   sDataPtr = sDataPtrNext;
   *pbOneExpandableOnlyOnLine = FALSE;
  } // if  (*sDataPtr)

 } // while (*sDataPtr)

 strcpy(gl_sLinePostEq, sBuff);
 return lineParseResult; // triStateResultResult;
}

// ---------------------------------------------------------------------- AC
guint intertag_buffer_update(INOUT gchar** psBuffPtr, IN gchar* sBuffIn, IN guint nChars)
// ----------------------------------------------------------------------
{
 if (!nChars) return 0;
 strncpy(*psBuffPtr, sBuffIn, nChars);
// nChars--;
 *psBuffPtr += (nChars);
 *(*psBuffPtr + 1) = '\0';
 return nChars;
}

#endif

#if !defined(_GTKMENUPLUS_NO_VARIABLES_)

// ----------------------------------------------------------------------
enum LineParseResult onVariable(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 struct Variable* pVariable = NULL;
 enum LineParseResult lineParseResult = variableAdd(pMenuEntryPending->m_sErrMsg, &pVariable);
 if (lineParseResult != lineParseOk)
   return lineParseResult;
 if (!pVariable) return lineParseOk; // pVariable non-null by variableAdd if needs evaluation
 return  variableEvaluate(pVariable, pMenuEntryPending->m_sErrMsg);
}

// ----------------------------------------------------------------------
enum LineParseResult variableEvaluate(INOUT struct Variable* pVariable, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 if (!pVariable) return lineParseOk;

 gchar sEvalResult[MAX_LINE_LENGTH + 1];
 memset(sEvalResult, 0, (MAX_LINE_LENGTH  + 1) * sizeof(gchar));

 enum LineParseResult lineParseResult = evaluateExpression("variable==", pVariable->m_sValue, sEvalResult, MAX_LINE_LENGTH * sizeof(gchar), sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;
 if (strlen(sEvalResult) + strlen(pVariable->m_sNameValue) >= MAX_LINE_LENGTH)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "value of evaluated variable %s too long\n", pVariable->m_sNameValue);
  return lineParseFail;
 }
 strcpy(pVariable->m_sValue, sEvalResult);
 pVariable->m_nValueLen = strlen(pVariable->m_sValue);
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult expand_var(INOUT gchar** psDataPtr, INOUT gchar** psBuffPtr,
                    INOUT guint* pnCharsInBuff, OUT gboolean* pbIsVar, OUT gboolean *pbOneExpandableOnlyOnLine)
// ----------------------------------------------------------------------
{
 *pbIsVar = FALSE;
 guint nLenParamTag = 1; // strlen(PARAM_REF_TAG);
 enum LineParseResult lineParseResult = lineParseOk;
 gchar* sVarName = *psDataPtr + nLenParamTag;
 gchar* sEnd = sVarName;
 while (isdigit(*sEnd) || isalpha(*sEnd) || *sEnd == '_') sEnd++;
 if (sEnd == *psDataPtr)
  return lineParseOk;
 gchar cEnd  = *sEnd;
 *sEnd = '\0';

 if (!getenv(sVarName))
 {
  struct Variable* pVariable = variableFind(sVarName);
  if (pVariable)
  {
   guint uiLenVar = pVariable->m_nValueLen;
   pVariable->m_bUsed = TRUE;
   if (uiLenVar) // should always be positve?  TO DO?
   {
    if (*pnCharsInBuff + uiLenVar > MAX_LINE_LENGTH)
    {
     return lineParseFail;
    }
    strcpy(*psBuffPtr, pVariable->m_sValue);
    *psBuffPtr += uiLenVar;
    *psDataPtr = sEnd;
    *pnCharsInBuff += uiLenVar;
    *pbIsVar = TRUE;
   } // if (uiLenVar)
  } // if (pVariable)
 } // if (!getenv(sVarName))
 *sEnd = cEnd;
 return lineParseResult;
}

#endif

#if !defined(_GTKMENUPLUS_NO_PARAMS_)

// ---------------------------------------------------------------------- AC
enum LineParseResult expand_param(INOUT gchar** psDataPtr, INOUT gchar** psBuffPtr,
                      INOUT guint* pnCharsInBuff, OUT gboolean *pbOneExpandableOnlyOnLine, IN struct Params* pParams, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 guint nLenParamTag = 1; // strlen(PARAM_REF_TAG);
 gchar* sPtrEndParamRef = NULL;
 guint nArgNo = strtoul(*psDataPtr + nLenParamTag, &sPtrEndParamRef, 10);
 guint nArgVecNdx = nArgNo;
 if (pParams->m_bIncludesProgName) nArgVecNdx++;
 if (nArgNo < 0 || nArgVecNdx >= pParams->m_nCmdLineParams || !(pParams->m_sCmdLineParamVec))
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "invalid parameter reference number %c%d\n", PARAM_REF_TAG, nArgNo);

  if (!(*pnCharsInBuff) && *sPtrEndParamRef == '\0') *pbOneExpandableOnlyOnLine = TRUE; // only one param ref and nothin else
  *psDataPtr = sPtrEndParamRef;  // ignore the reference completely
  // Semantics: a single unassigned parameter evaluates to FALSE
  return lineParseWarn;
 }

 if (!(pParams->m_bIncludesProgName) && nArgNo == 0)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "%s\n", "$0 invalid in included= file");
  return lineParseFail; //  triStateResultFail;
 }

// guint uiLenParam = (nArgNo == 0) ? (strlen(gl_sScriptDirectory) + strlen(DEFAULT_CONFIG_FILE) + 1) : strlen(pParams->m_sCmdLineParamVec[nArgVecNdx]);
 guint uiLenParam = strlen(pParams->m_sCmdLineParamVec[nArgVecNdx]);
//check length
 if (*pnCharsInBuff + uiLenParam >= MAX_LINE_LENGTH - 1)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "line with parameter %d inserted too long\n", nArgNo);
  return lineParseFail; //  triStateResultFail;
 }

//copy arg in
/*
 if (nArgNo == 0) // $0, no args, using default file  && pParams->m_nCmdLineParams == 0
 {
  if (*gl_sScriptDirectory)
   strcpy(*psBuffPtr, gl_sScriptDirectory);
  strcat(*psBuffPtr, DEFAULT_CONFIG_FILE);
 }
 else
 {
*/
  strcpy(*psBuffPtr, pParams->m_sCmdLineParamVec[nArgVecNdx]);
  if (pParams->m_pCmdLineParamInUse) pParams->m_pCmdLineParamInUse[nArgVecNdx] = TRUE; // pParams->m_pCmdLineParamInUse may be null if no params, or only $0
// } // if (nArgNo == 0)

 if (!(*pnCharsInBuff) && *sPtrEndParamRef == '\0') *pbOneExpandableOnlyOnLine = TRUE; // only one param ref and nothin else
 *psBuffPtr += uiLenParam;

 *psDataPtr = sPtrEndParamRef;
 *pnCharsInBuff += uiLenParam;

 if (*(*psBuffPtr - 1) == '\n')
 {
  (*psBuffPtr)--;
  (*pnCharsInBuff)--;
 }


 if (*pnCharsInBuff >= MAX_LINE_LENGTH - 1)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "%s\n", "line with interpolated variables too long");
  return lineParseFail;
 } // if (nCharsInBuff >= MAX_LINE_LENGTH)
//  if (strncasecmp(sDataPtr, PARAM_REF_TAG, nLenParamTag)  == 0)
 return lineParseOk;
}


#endif // #if !defined(_GTKMENUPLUS_NO_PARAMS_)


#if !defined(_GTKMENUPLUS_NO_FORMAT_)
// ---------------------------------------------------------------------- AC
enum LineParseResult onFormat(INOUT struct MenuEntry* pMenuEntryPending)  // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
//*gl_sItemFormat = '\0';
 formattingInit(&(gl_FormattingSubMenu[gl_uiCurDepth]), "\0", 0);

 if (!*gl_sLinePostEq)  return lineParseOk;

//legnth ok, gl_sLinePostEq is MAX_LINE_LENGTH, gl_sItemFormat MAX_PATH_LEN]
 if (strlen(gl_sLinePostEq) + 15 > MAX_PATH_LEN)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "format= line too long");
  return lineParseFail;
 } // if (nCharsInBuff >= MAX_LINE_LENGTH)

 formattingInit(&(gl_FormattingSubMenu[gl_uiCurDepth]), gl_sLinePostEq, gl_uiCurDepth); //  pMenuEntryPending->m_uiDepth

 return lineParseOk;
}
#endif // #if !defined(_GTKMENUPLUS_NO_FORMAT_)

#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)

// ----------------------------------------------------------------------
enum LineParseResult onActivationLogfile(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------

{
 if (!(*gl_sLinePostEq))
 {
  *gl_sActivationLogfile = '\0';
  return lineParseOk;
 }
 enum LineParseResult lineParseResult = expand_path(gl_sLinePostEq, gl_sScriptDirectory,
     "activationlogfile", pMenuEntryPending->m_sErrMsg); // can rewrite gl_sLinePostEq
 if (lineParseResult != lineParseOk)
  return lineParseResult;
 strcpy(gl_sActivationLogfile, gl_sLinePostEq);

 // Touch (empty) file so the calling application can include it.
 int fd = open(gl_sActivationLogfile, O_CREAT|O_WRONLY);
 if (-1 == fd)
 {
  perror("open");
  return lineParseFail;
 }
 close(fd);
 return lineParseOk;
}

// for makeLogItem, commitItem, processLauncher
struct LogItem {
 guint uiSize;
 gchar *sItem;
};

// ----------------------------------------------------------------------
//called by commitItem, processLauncher
struct LogItem* makeLogItem(IN gchar* sItem, IN gchar* sCmd, IN gchar* sTooltip, IN gchar* sIcon)
// ----------------------------------------------------------------------
{
 if (! *gl_sActivationLogfile)
  return NULL;

 // encode args as concatenated_strings
 guint size = 4*sizeof('\0') + strlen(sItem) + strlen(sCmd) +
  (sTooltip ? strlen(sTooltip) : 0) + strlen(sIcon);
 struct LogItem *log;
 gchar *enc = NULL;
 if(  !(enc = malloc(sizeof('\0')*size))
   || !(log = malloc(sizeof(struct LogItem))))
 {
  perror("malloc");
  if(enc) free(enc);
  return NULL;
 }
 sprintf(enc, "%s%c%s%c%s%c%s%c",
   // sCmd goes first, sTooltip last
   sCmd, '\0', sItem, '\0', sIcon, '\0', //Exec, Name, Icon,
   sTooltip ? sTooltip : "", '\0'); //Comment
 log->uiSize = size;
 log->sItem = enc;
 return log;
}

// ----------------------------------------------------------------------
//called by RunItem
guint writeLogItem(IN const gchar* sItem)
// ----------------------------------------------------------------------
{
 if (! *gl_sActivationLogfile)
  return 0;

 /*
  * Add to activation log file, which is a menu configuration file.
 */
 // Get addresses of adjacent strings Exec, Name, Icon, Comment.
 gchar *exec, *name, *icon, *comment, *p;
 exec = (gchar *) sItem; // assert(0 != strcmp(exec, ""))
 name = exec + strlen(exec) +1;
 icon = name + strlen(name) +1;
 comment = icon + strlen(icon) +1;

 // Deal with special cases in reverse address order.
 if (0 == strcmp("NULL", icon)) // special case "NULL" icon
  *icon = '\0';
 if (!*name) // special case {N2}
  name = exec;
 // special case right-trim name because so does readLine
 for(p = exec + strlen(exec) - 1; p != exec && (*p == ' ' || *p == '\t');)
 {
   *p-- = '\0';
 }

 gchar tmpf[MAX_PATH_LEN]; int fd;
 strcpy(tmpf, gl_sActivationLogfile);
 strcpy(tmpf, dirname(tmpf));
 strncat(tmpf, "/.gtkmenuplus-XXXXXX", MAX_PATH_LEN -1);
 if (-1 == (fd = mkstemp(tmpf)))
 {
  perror("mkstemp");
  return -1;
 }
 FILE *copy = fdopen(fd, "w+");
 FILE *heystack = fopen(gl_sActivationLogfile, "r+");
 if (!copy || !heystack)
 {
  perror("fopen");
  if (copy)
   fclose(copy);
  unlink(tmpf);
  return -1;
 }

 // Is this log item (needle) already in the file (heystack)?
 gchar needle[MAX_PATH_LEN + 2 + sizeof("Item=Cmd=Icon=Tooltip")];
 sprintf(needle, "Item=%s\nCmd=%s\nIcon=%s%c%s%s\n", name, exec, icon,
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
   '\n', "Tooltip=", comment
#else
   "", ""
#endif
 ); // needle formatted for heystack

 // Scan heystack looking for needle.
 gboolean found = FALSE;
 gchar meta[MAX_PATH_LEN + 1];
 gchar data[MAX_PATH_LEN + 2];
 guint activation_count;
 gchar c;
 time_t creation_time, activation_time;
 // Use fgets so as to allow for user data between '}' and '\n'.
 while (fgets(meta, MAX_PATH_LEN, heystack))
 {
  // After needle is found we enter blind copy mode.
  if (found)
  {
   fputs(meta, copy);
   continue;
  }

#define META_FMT "#{%d:%d:%d:%d}"
#define META_DIM 4
#define DATA_END "#{}\n"
  uint n, i;
  if (META_DIM != sscanf(meta, META_FMT, &n, &activation_count,
     (int *)&creation_time, (int *)&activation_time))
  {
   fprintf(stderr, "sscanf %s\n", strerror(ENODATA));
   break;
  }
  // Read n-line data.
  for(
    i = 0, *(p = data) = '\0';
    i < n && fgets(p, MAX_PATH_LEN - (p - data), heystack);
    i++, p += strlen(p))
   ;
  // Chomp DATA_END so as to allow for user data after '}'.
  while (1)
  {
   c = fgetc(heystack);
   if (c == EOF || c == '\n')
    break;
  }

  if ((found = 0 == strcmp(data, needle)))
  {
    // Update meta.
    ++activation_count;
    activation_time = time(NULL);
    gchar *user_data = index(meta, '}');
    fprintf(copy, META_FMT,
      n, activation_count, (int)creation_time, (int)activation_time);
    if (user_data)
     fputs(user_data + 1, copy);
  }
  else
  {
   fputs(meta, copy);
  }
  fputs(data, copy);
  fputs(DATA_END, copy);
 }

 if (!found)
 {
  creation_time = activation_time = time(NULL);
  // Write needle as a menu item, optionally including the tooltip.
  fprintf(copy, META_FMT "\n%s" DATA_END,
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
    4, // how many lines after meta, excluding DATA_END
#else
    3,
#endif
    1, // activation count
    (int)creation_time, (int)activation_time, needle);
 }

 // Re-write logfile (heystack) file instead of renaming tmpf to logfile
 // to follow symlinked logfile on aufs filesystem layer.
 rewind(heystack);
 rewind(copy);
 while((c = fgetc(copy)) != EOF)
  fputc(c, heystack);

 int ret = fclose(heystack);
 if (0 != ret) {
  perror("fclose logfile");
 }
 (void) fclose(copy);
 unlink(tmpf);
 return ret;
}

#endif // _GTKMENUPLUS_NO_ACTIVATION_LOG_

// ---------------------------------------------------------------------- AC
enum LineParseResult commitItem(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 GtkWidget *pGtkWdgtCurrent = makeItem(pMenuEntryPending->m_sTitle, gl_sCmds[gl_uiCurItem],
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
                                       pMenuEntryPending->m_sTooltip,
#else
                                       NULL,
#endif
                                       pMenuEntryPending->m_uiDepth); // counts up gl_uiCurItem

 if (!pGtkWdgtCurrent)
  return lineParseFail;
 enum LineParseResult lineParseResult =
  addIcon(pMenuEntryPending, pGtkWdgtCurrent); // add icon to item; ALWAYS RETURN TRUE, SOFT ERROR IF ICON MISSING

#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
 struct LogItem *logItem = makeLogItem(
   pMenuEntryPending->m_sTitle,
   gl_sCmds[gl_uiCurItem - 1],
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
   pMenuEntryPending->m_sTooltip,
#else
   NULL,
#endif
   pMenuEntryPending->m_sIcon);

 if (logItem)
 {
  // {N2} Theoretically, logItem->sItem may not fit as a whole, but in
  // practice it's very unlikely. Even so, buffer overrun is protected, and
  // no-fit degrades to a .desktop entry with a command but no tooltip,
  // then no icon, then no name in the worst case.
  gchar *p;
  memcpy(p = gl_sCmds[gl_uiCurItem - 1], logItem->sItem,
    MIN(logItem->uiSize, MAX_PATH_LEN));
  p[MAX_PATH_LEN] = '\0';
  free(logItem->sItem);
  free(logItem);
 }
#endif // _GTKMENUPLUS_NO_ACTIVATION_LOG_

 return lineParseResult;
}

// ---------------------------------------------------------------------- AC
//called by commitItem, onIconForLauncher
GtkWidget* makeItem(IN gchar* sItem, IN gchar* sCmd, IN gchar* sTooltip, IN guint uiDepth) // counts up gl_uiCurItem
// ----------------------------------------------------------------------
{
 //insert item into menu
 if (!sItem)
  return NULL;

 GtkWidget *pGtkWdgtCurrent =
   gtk_image_menu_item_new_with_mnemonic(sItem);

 gtk_menu_shell_append(GTK_MENU_SHELL (gl_gtkWmenu[uiDepth]), pGtkWdgtCurrent);
 g_signal_connect_swapped(pGtkWdgtCurrent, "activate",  G_CALLBACK (RunItem), sCmd);

 if (sCmd && !*sCmd)
 {
  gtk_widget_set_sensitive(pGtkWdgtCurrent, FALSE);
//TO DO maybe new property
 } // if (!*gl_sCmds[gl_uiCurItem])

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
 gboolean bResult = addFormatting(pGtkWdgtCurrent, FALSE);
 if (!bResult)
  return NULL;
#endif

#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 addTooltip(sTooltip, FALSE, pGtkWdgtCurrent); // bToSubMenu
#endif

 gl_uiCurItem++;
 return pGtkWdgtCurrent;
}

//TO DO KILL
/*
// ---------------------------------------------------------------------- AC
enum LineParseResult onIconForSubMenu(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult  = menuEntryCheckFieldValidity(pMenuEntryPending->m_sIcon, "icon",
                                                                     pMenuEntryPending->sMenuEntryType,
                                                                     pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 strcpy(pMenuEntryPending->m_sIcon, gl_sLinePostEq); // pMenuEntryPending->m_sIcon, MAX_PATH_LEN, gl_sLinePostEq MAX_LINE_LENGTH
 return lineParseOk; // commitSubMenu(pMenuEntryPending);
}
*/
// ---------------------------------------------------------------------- AC
enum LineParseResult onCmd(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult = onCmdCommon(pMenuEntryPending);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 strcpy(gl_sCmds[gl_uiCurItem], gl_sLinePostEq);
 return lineParseOk; // STATE_CMD_FOUND;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onItem(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult = resizeCommandBuffer(pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 return onItemCommon(pMenuEntryPending); // accesses gl_sLinePostEq
}

// ---------------------------------------------------------------------- AC
enum LineParseResult resizeCommandBuffer(OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 if (gl_uiCurItem >= gl_nMenuEntries)
 {
  gl_nMenuEntries *= 2;
  gl_sCmds = realloc(gl_sCmds, gl_nMenuEntries * (MAX_PATH_LEN + 1) * sizeof(gchar));

  if (!gl_sCmds)
  {
// gl_bFatalError = TRUE;
   snprintf(sErrMsg, MAX_LINE_LENGTH, "can't allocate memory for %d commands\n", gl_nMenuEntries);
   return lineParseFailFatal;
  }
 } // if (gl_uiCurItem >= gl_nMenuEntries)
 return lineParseOk;
}

#if !defined(_GTKMENUPLUS_NO_FORMAT_) && !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
// ---------------------------------------------------------------------- AC
enum LineParseResult onTooltipFormat(INOUT struct MenuEntry* pMenuEntryPending) // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
// *gl_sTooltipFormat = '\0';
 formattingInit(&gl_FormattingTooltip, "\0", 0);
 if (!*gl_sLinePostEq)  return lineParseOk;

 if (strlen(gl_sLinePostEq) + 15 > MAX_PATH_LEN)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "tooltipformat= line too long");
  return lineParseFail;
 }

// snprintf(gl_sTooltipFormat, MAX_PATH_LEN, "<span %s>%%s</span>", gl_sLinePostEq);
 formattingInit(&gl_FormattingTooltip, gl_sLinePostEq, gl_uiCurDepth);
 return lineParseOk;
}

#endif // #if  !defined(_GTKMENUPLUS_NO_FORMAT_) && !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

// ---------------------------------------------------------------------- AC
enum LineParseResult onSubMenuEnd(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 if (!gl_bConfigKeywordUseEndSubMenu)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "\"endsubmenu\" encountered, no previous \"fileformat=endsubmenu\"");
  return lineParseFail;
 }

 if (gl_uiCurDepth <= 0)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "\"endsubmenu\", encountered, no matching \"submenu\" or \"launchersubmenu\"");
  return lineParseFail;
 }
#if !defined(_GTKMENUPLUS_NO_FORMAT_)
 if (!gl_FormattingSubMenu[gl_uiCurDepth].m_cFormatDivider) // m_cFormatDivider zero if no compound format
  formattingInit(&(gl_FormattingSubMenu[gl_uiCurDepth]), "\0", 0);
#endif
 pMenuEntryPending->m_uiDepth = --gl_uiCurDepth;
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult commitSubMenu(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 GtkWidget* pGtkWdgtCurrentSubMenu = gtk_image_menu_item_new_with_mnemonic(pMenuEntryPending->m_sTitle);
 // Pass the sub-menu entry widget up to the caller, should it need to destroy the widget.
 gl_gtkMenuEntry[pMenuEntryPending->m_uiDepth] = pGtkWdgtCurrentSubMenu;
 gtk_menu_shell_append(GTK_MENU_SHELL (gl_gtkWmenu[pMenuEntryPending->m_uiDepth]), pGtkWdgtCurrentSubMenu);
 gl_gtkWmenu[pMenuEntryPending->m_uiDepth + 1] = gtk_menu_new();
 gtk_menu_item_set_submenu(GTK_MENU_ITEM (pGtkWdgtCurrentSubMenu), gl_gtkWmenu[pMenuEntryPending->m_uiDepth + 1]);

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
 if (!addFormatting(pGtkWdgtCurrentSubMenu, TRUE)) // bToSubMenu
  return lineParseFail;
#endif

#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
 addTooltip(pMenuEntryPending->m_sTooltip, TRUE, pGtkWdgtCurrentSubMenu); // bToSubMenu
#endif

 enum LineParseResult lineParseResult = addIcon(pMenuEntryPending, pGtkWdgtCurrentSubMenu); // add icon to item; ALWAYS RETURN TRUE, SOFT ERROR IF ICON MISSING

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
 // Note: Since onSubMenu has already incremented gl_uiCurDepth, here
 // gl_FormattingSubMenu[gl_uiCurDepth - 1] is the formatting record of
 // the sub-menu that contains the sub-menu we are committing.
 if (lineParseResult != lineParseFail)
 {
  // MAX(0,...) guards against segfaults due to malformed submenus.
  guint uiCurDepth = MAX(0, (gint)gl_uiCurDepth - 1);
  if(!gl_FormattingSubMenu[uiCurDepth].m_cFormatDivider
   && gl_uiCurDepth < MAX_SUBMENU_DEPTH) //m_cFormatDivider 0 if formatting not compound
  memcpy(&(gl_FormattingSubMenu[gl_uiCurDepth]), &(gl_FormattingSubMenu[uiCurDepth]), sizeof(struct Formatting));
 }
#endif

 return lineParseResult;
//return STATE_SUBMENU_FOUND;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onSeparator(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 GtkWidget * pGtkWdgtCurrent = gtk_separator_menu_item_new();
 gtk_menu_shell_append(GTK_MENU_SHELL (gl_gtkWmenu[gl_uiCurDepth]), pGtkWdgtCurrent);
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onIconSize(INOUT struct MenuEntry* pMenuEntryPending)  // accesses gl_sLinePostEq
// ----------------------------------------------------------------------
{
 return checkIconSize(atoi(gl_sLinePostEq), pMenuEntryPending->m_sErrMsg);
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onPosition(INOUT struct MenuEntry* pMenuEntryPending)
// ---------------------------------------------------------------------- AC
{
 if (gl_bSetMenuPos)
 {
  fprintf(stderr, "multiple menu position lines.\n");
  return lineParseFail;
 }
 if (parseInts(gl_sLinePostEq, &gl_uiMenuX, &gl_uiMenuY) == lineParseOk)
 {
  gl_bSetMenuPos = TRUE;
  if(0 == gl_nOptQuiet)
   g_print("Menu position = %d, %d.\n", gl_uiMenuX, gl_uiMenuY);
  return lineParseOk;
 }
 else
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "menu position invalid");
  return lineParseWarn;
 }
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onIconDir(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 return  set_base_dir(gl_sIconDirectory, "iconDirectory", pMenuEntryPending->m_sErrMsg);
}

// ---------------------------------------------------------------------- AC
// called by onIconDir
enum LineParseResult set_base_dir(OUT gchar* sDirBuff, IN const gchar* sLabelForErr, OUT gchar* sErrMsg) // sDirBuff always MAX_PATH_LEN, gl_sLinePostEq MAX_LINE_LENGTH
// ----------------------------------------------------------------------
{
 if (!*gl_sLinePostEq)
 {
  strcpy(sDirBuff, gl_sScriptDirectory);
  return lineParseOk;
 }
 enum LineParseResult lineParseResult = expand_path(gl_sLinePostEq, gl_sScriptDirectory, sLabelForErr, sErrMsg); // can rewrite gl_sLinePostEq
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 struct stat statbuf;
 if (stat(gl_sLinePostEq, &statbuf) == -1 || !S_ISDIR(statbuf.st_mode))
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "'%s' value not a directory\n", sLabelForErr);
  return lineParseFail;
 }

 strcpy(sDirBuff, gl_sLinePostEq);
 guint nBuffLen = strlen(sDirBuff);

 if (*(sDirBuff + nBuffLen - 1) != '/')
   strcat(sDirBuff, "/");

 return lineParseOk;
}

#if !defined(_GTKMENUPLUS_NO_IF_)

// ---------------------------------------------------------------------- AC
enum LineParseResult onIf(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult =  onIfCommon(pMenuEntryPending);
 if (lineParseResult != lineParseOk)
  return lineParseResult;
 if (gl_pIfStatusCurrent->m_bCurrentlyAccepting)
  lineParseResult = parseIfCondition("if", pMenuEntryPending->m_sErrMsg);
#if !defined(_GTKMENUPLUS_NO_DEBUG_IF_)
 printIfStatus("onIf", gl_pIfStatusCurrent);
#endif
 return lineParseResult;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onElseif(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult =  onElseIfCommon(pMenuEntryPending);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 return parseIfCondition("elseif", pMenuEntryPending->m_sErrMsg);
}


// ---------------------------------------------------------------------- AC
// called from onElseif, onIf
enum LineParseResult parseIfCondition(IN const gchar* sLabel, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 gchar sIfTest[MAX_LINE_LENGTH + 1];
 memset(sIfTest, 0, (MAX_LINE_LENGTH  + 1) * sizeof(gchar));

#if !defined(_GTKMENUPLUS_NO_PARAMS_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)
 if (*gl_sLinePostEq == '\0' && !gl_bOneExpandableOnlyOnLine)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "Null %s condition not allowed.\n", sLabel);
  return lineParseWarn;
 }

 if (gl_bOneExpandableOnlyOnLine)
 {
  if (*gl_sLinePostEq)
   strcpy(sIfTest, gl_sLinePostEq);
  else
  {
   gl_pIfStatusCurrent->m_bCurrentlyAccepting = TRUE; // = gl_pIfStatusCurrent->m_bTestMode
   return lineParseOk;
  }

 }
 else // if (gl_bOneExpandableOnlyOnLine)
#endif // #if !defined(_GTKMENUPLUS_NO_PARAMS_)
 {
  enum LineParseResult lineParseResult = evaluateExpression(sLabel, gl_sLinePostEq, sIfTest, MAX_LINE_LENGTH * sizeof(gchar), sErrMsg);
  if (lineParseResult != lineParseOk)
   return lineParseResult;
 } // if (gl_bOneExpandableOnlyOnLine)

 gchar* sIfTestEnd = NULL;
 gboolean bFoundTestExpr = FALSE;

 glong  lIfTest = strtol(sIfTest, &sIfTestEnd, 10);
 if (sIfTest < sIfTestEnd)
 {
  gl_pIfStatusCurrent->m_bCurrentlyAccepting = lIfTest != 0;
  bFoundTestExpr = TRUE;
 }
 else // if (sIfTest < sIfTestEnd)
 {
  guint i = 0;
  for (i = 0; i < sizeof(gl_keywordLogic)/sizeof(struct KeywordLogic); i++)
  {
   if (strcasecmp(sIfTest, gl_keywordLogic[i].m_sKeyword) == 0)
   {
    gl_pIfStatusCurrent->m_bCurrentlyAccepting = gl_keywordLogic[i].m_bMeaning;
    bFoundTestExpr = TRUE;
    break;
   } // if ((strcasecmp(sIfTest, gl_keywordLogic[i].m_sKeyword) == 0)
  } // for (i = 0; i < sizeof(gl_keywordLogic)/sizeof(KeywordLogic); i++)
 } // if (sIfTest < sIfTestEnd)

 if (bFoundTestExpr)
 {
  if (gl_pIfStatusCurrent->m_bCurrentlyAccepting) gl_pIfStatusCurrent->m_bTrueConditionFound = TRUE;
 }
 else
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "%s\n", "if test doesn't return anything comprehensible");
  return lineParseFail;
 } // if (sIfTest < sIfTestEnd)

 return lineParseOk;
}


#endif // #if !defined(_GTKMENUPLUS_NO_IF_)

// ---------------------------------------------------------------------- AC
enum LineParseResult onError(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", gl_sLinePostEq);
 return lineParseFail;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onConfigure(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 return checkConfigKeywords(gl_sLinePostEq, pMenuEntryPending->m_sErrMsg);
}

// ----------------------------------------------------------------------
enum LineParseResult onOnExit(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 strcpy(gl_sOnExit, gl_sLinePostEq);
 return lineParseOk;
}

// ----------------------------------------------------------------------
gint doOnExit()
// ----------------------------------------------------------------------
{
 if (!*gl_sOnExit)
  return 0;
 return system(gl_sOnExit);
}

// ----------------------------------------------------------------------AC
enum LineParseResult getAbsPathParts(IN gchar* sBuff, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 return parseInts(sBuff, &gl_iAbsPathParts, NULL);
}
/*

// ----------------------------------------------------------------------AC
enum LineParseResult getAbsPathTitle(IN gchar* sBuff, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 guint i = 0;
 *gl_sAbsPathTitleModifier = '\0';
 while (sBuff[i] != ' ' && sBuff[i] != '\0')
 {
  gl_sAbsPathTitleModifier[i] =  sBuff[i];
  sBuff[i] = ' ';
  i++;
 }
 gl_sAbsPathTitleModifier[i] = '\0';
//MAX_PATH_LEN
 return lineParseOk;
}

*/
// ----------------------------------------------------------------------AC
enum LineParseResult getMenuPosArg(IN gchar* sBuff, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 if (gl_bSetMenuPos)
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH,  "on \"configure=\" line, menu position set twice\n");
  return lineParseFail;
 }

 gl_bSetMenuPos = TRUE;
 enum LineParseResult lineParseResult = parseInts(sBuff, &gl_uiMenuX, &gl_uiMenuY);

 if (lineParseResult == lineParseOk) {
  if(0 == gl_nOptQuiet)
   g_print("Menu position = %d, %d.\n", gl_uiMenuX, gl_uiMenuY);
 }
 else
  snprintf(sErrMsg, MAX_LINE_LENGTH,  "on \"configure=\" line, illegal menu position arguments\n");
 return lineParseOk;
}

// ----------------------------------------------------------------------AC
enum LineParseResult getIconSizeArg(IN gchar* sBuff, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
  guint nSize;
  enum LineParseResult lineParseResult = parseInts(sBuff, &nSize, NULL);
  if (lineParseResult != lineParseOk)
   return lineParseResult;
  return checkIconSize(nSize, sErrMsg);
}


// ---------------------------------------------------------------------- AC
enum LineParseResult onAbsolutePath(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult = resizeCommandBuffer(pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
 gchar* sExt = strrchr(gl_sLinePostEq, '.');
 if (sExt && strcasecmp(sExt, ".desktop") == 0)
  return onLauncher(pMenuEntryPending);
#endif
 strcpy(pMenuEntryPending->m_sTooltip, gl_sLinePostEq);
 gchar* sTitle = getContainingFolderNames(gl_sLinePostEq, gl_iAbsPathParts);
 gchar* sCmdToUse = gl_sLinePostEq;
 gchar *sCmdExpanded = expand_path_tilda_dot(gl_sLinePostEq, gl_sScriptDirectory);
 if (sCmdExpanded) sCmdToUse  = sCmdExpanded;

 strcpy(gl_sCmds[gl_uiCurItem], sCmdToUse);

 GtkWidget *pGtkWdgtCurrent = makeItem(sTitle, gl_sCmds[gl_uiCurItem],
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
                                       pMenuEntryPending->m_sTooltip,
#else
                                       NULL,           // tooltip
#endif
                                       gl_uiCurDepth); // counts up gl_uiCurItem
 if (pGtkWdgtCurrent)
 {
  menuEntrySet(pMenuEntryPending, NULL, LINE_ABSOLUTE_PATH, "absolute path", TRUE, FALSE, gl_uiCurDepth); // bCmdOk, bIconTooltipOk
  strcpy(pMenuEntryPending->m_sTitle, sTitle);
  strcpy(pMenuEntryPending->m_sCmd, gl_sLinePostEq);
  *(pMenuEntryPending->m_sIcon) = '\0';
  lineParseResult = addIcon(pMenuEntryPending, pGtkWdgtCurrent); // add icon to item; ALWAYS RETURN TRUE, SOFT ERROR IF ICON MISSING
  menuEntrySet(pMenuEntryPending, NULL, LINE_UNDEFINED, "", FALSE, FALSE, 0); // bCmdOk, bIconTooltipOk
 } // if (pGtkWdgtCurrent)
 else
  lineParseResult = lineParseFail;

 if (sCmdExpanded) g_free(sCmdExpanded);

//TO DO; commit, clear pMenuEntryPending
 return lineParseResult;
}

// ---------------------------------------------------------------------- AC
gchar* getContainingFolderNames(IN gchar* sPath, IN guint nLevel)
// ----------------------------------------------------------------------
{
 if (nLevel == 0) return sPath;
 gchar* sPos = sPath;
 gchar* sPosNow = sPath + strlen(sPath) - 1;
 {
  for (;sPosNow >= sPath ; sPosNow--)
  {
   if (*sPosNow == '/')
   {
    nLevel--;
    sPos = sPosNow + 1;
    if (nLevel == 0) return sPos;
   }
  }
 }
 return sPath;
}

#if !defined(_GTKMENUPLUS_NO_IF_) || !defined(_GTKMENUPLUS_NO_VARIABLES_)

// ---------------------------------------------------------------------- AC
enum LineParseResult evaluateExpression(IN const gchar* sLabel, IN gchar* sToEval, OUT gchar* sExprResult, IN guint nExprResultLen, OUT gchar* sErrMsg)
// ----------------------------------------------------------------------
{
 gchar* sToEvalExpanded = expand_path_tilda_dot(sToEval, gl_sScriptDirectory);

 if (sToEvalExpanded) sToEval = sToEvalExpanded;

 FILE* fp = popen(sToEval, "r");
 if (fp == NULL )
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH, "can't spawn process for %s test\n", sLabel);
  return lineParseFail;
 }

 if (!fread(sExprResult, 1, nExprResultLen, fp))
 {
  snprintf(sErrMsg, MAX_LINE_LENGTH,
   "No output in '%s' test: %s\nvvv(input)vvv   ^^^(shell)^^^",  sLabel, sToEval);
  return lineParseFail;
 }

 pclose(fp);

 gchar* pEnd = sExprResult + strlen(sExprResult) - 1;

 if (*pEnd  == '\n')  *pEnd= '\0';
 if (sToEvalExpanded) g_free(sToEvalExpanded);
 return lineParseOk;
}

#endif


#if !defined(_GTKMENUPLUS_NO_VARIABLES_)
// ---------------------------------------------------------------------- AC
struct Variable* variableFind(IN gchar* sName)
// ----------------------------------------------------------------------
{
 struct Variable* pVariable = gl_pVariableHead;
 while (pVariable)
 {
  if (strcmp(pVariable->m_sNameValue, sName) == 0)
  {
   return pVariable;
  }
  pVariable = pVariable->m_pVariableNext;
 }

 return NULL;
}


#endif  // #if !defined(_GTKMENUPLUS_NO_VARIABLES_)


#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
// ----------------------------------------------------------------------
off_t createLauncherDB(IN const gchar *rpath, OUT gchar *outf, OUT gchar* sErrMsg) // used by onLauncher
// ----------------------------------------------------------------------
/*
 * *outf - return launcher data base file path (in /tmp).
 * return:
 *   >  0 : size of *outf file - caller must unlink *outf
 *   == 0 : empty *outf - no need to unlink
 *   <  0 : errors - no need to unlink
 */
{
 int fd;
 if (0 >= snprintf(outf, MAX_PATH_LEN, "%s/.gtkmenuplus-XXXXXX", P_tmpdir)
  || -1 == (fd = mkstemp(outf)))
 {
   snprintf(sErrMsg, MAX_LINE_LENGTH, "mkstemp %s", strerror(errno));
   *outf = '\0';
   return -1;
 }
 close(fd);
 gchar cmd[MAX_PATH_LEN + 1];
 int maxdepth = MAX_SUBMENU_DEPTH - gl_nLauncherReadLineDepth - 1;
 maxdepth = maxdepth > 0 ? maxdepth : 1;
 if (snprintf(cmd, MAX_PATH_LEN,
       "\"${GTKMENUPLUS_FIND:-find}\" -L '%s' -maxdepth ${GTKMENUPLUS_SCAN_DEPTH:-%d} '(' -type f -o -type l ')' -name '*.desktop' > '%s' 2>&1",
         rpath, maxdepth, outf))
 {
  system(cmd);
  struct stat sb;
  if (stat(outf, &sb) == -1)
  {
    unlink(outf);
    snprintf(sErrMsg, MAX_LINE_LENGTH, "stat '%s' %s", outf, strerror(errno));
    *outf = '\0';
    return -1;
  }
  if(0 == sb.st_size) unlink(outf);
  return sb.st_size;
 }
 return -1;
}

// ----------------------------------------------------------------------
int lookupLauncherDB(IN const gchar *needle, IN const gchar *dbf) // used by onLauncher
// ----------------------------------------------------------------------
{
 /* gchar cmd[MAX_PATH_LEN + 1]; */
 /* return 0 < sprintf(cmd, "grep -q '^%s' '%s'", needle, dbf) */
 /*   ? system(cmd) : -1; */

 gchar s[MAX_LINE_LENGTH + 1];
 FILE *fp;

 if (NULL == (fp = fopen(dbf, "r")))
 {
  perror("in fopen");
  return -1;
 }
 int result = 1; // not-found(1), found(0)
 while (fgets(s, MAX_LINE_LENGTH + 1, fp)) // ends with \n\0
 {
  if (0 == strstr(s, needle) - s)
  {
   result = 0; // matched needle at ^
   break;
  }
 }
 fclose(fp);
 return result;
}

// ----------------------------------------------------------------------
void reapErrMsg (INOUT struct MenuEntry* pMenuEntryPending, enum LineParseResult lineParseResult, IN gchar* sLocation) // used by onLauncherCommon
// ----------------------------------------------------------------------
{
 // Rationale: we reap error messages rather than just printing them
 // to stderr because on exiting the program displays all accumulated
 // messages in a GUI box, and it also prints them to stderr.

 if (strlen(gl_sReapedErrMsgs) >= MAX_LINE_LENGTH - 1)
  return; // No more room left to store messages.
 // TODO rewrite to hold a dynamic list of strings, which msgToUser will print on exit.

 // *** NON-RE-ENTRANT ***
 static gchar sPrevious[MAX_LINE_LENGTH];

 gchar *sErrMsg = pMenuEntryPending->m_sErrMsg;
 if(sErrMsg && *sErrMsg)
 {
  if (lineParseResult < lineParseFail && gl_nOptInfo == 0)
   return; // Hush up!

  gchar *mp = malloc(MAX_LINE_LENGTH + 1);
  if (mp)
  {
   // While reaping ErrMsg, check if it starts with its Location. If
   // it doesn't then prepend Location+": " to ErrMsg.
   gboolean bPrepended = sLocation
     && sErrMsg == strstr(sErrMsg, sLocation)
     && ':' == *(sErrMsg + strlen(sLocation));
   snprintf(mp, MAX_LINE_LENGTH, "%s%s%s%s",
       gl_sReapedErrMsgs,
       bPrepended ? "" : sLocation,
       bPrepended ? "" : ": ",
       sErrMsg);

   shorten(mp, mp);

   // NON-RE-ENTRANT
   // See if we can further trim any leading prefix in common between
   // current and previous messages. Note that the current message (r)
   // is the last line of a string of lines (mp), which is initialized
   // to "\n" (gl_sReapedErrMsgs) and ends with '\n'.
   gchar *p , *q, *r;
   q = r = mp + strlen(mp) - 1; // normally mp ends with '\n'
   *q = '\0'; // temporarily overwrite last '\n'
   r = rindex(mp, '\n'); // find last line
   r = r ? r+1 : mp; // r is the current message
   *q = '\n'; // restore/posit last '\n'

   for(p = sPrevious, q = r; *p && *q && *p == *q; p++, q++)
    ;
   strcpy(sPrevious, r); // save for next call
   guint n = p - sPrevious;
   if (n > 2) // trim leading prefix
   {
    // add short indication that message is trimmed
    snprintf(q -= 3, 3, "%2d", n % 99);
    *(q + 2) = '`';
    // mp from r+1 to q-1 is to be discarded
    *r = '\0'; // end of messages before the current one
   }
   n = snprintf(gl_sReapedErrMsgs, MAX_LINE_LENGTH, "%s%s", mp, *r ? "" : q);
   free(mp);
   *sErrMsg = '\0';
  }
  else
  {
    perror("malloc");
    fprintf(stderr, "%s: %s\n", sLocation, sErrMsg);
  }
 }
}

// ----------------------------------------------------------------------
enum LineParseResult fillMenuEntry(IN const gchar* sFilePath, INOUT struct MenuEntry* pme, gboolean bRequired, guint iCaller) // used by fillSubMenuEntry, onLauncherDirFile, onLauncherSubMenu, processLauncher
// ----------------------------------------------------------------------
{
#if !defined(_GTKMENUPLUS_NO_CACHE_)
 struct MenuEntry *cached = find_in_cache(sFilePath);
 if (cached)
 {
  memcpy(pme, cached, sizeof(struct MenuEntry));
  //{N1} Reset cached members that don't tie to a .desktop "Entry=" field.
  pme->m_fnCommit = NULL; //{N1}
  pme->m_uiDepth = gl_uiCurDepth; //{N1}
  return lineParseOk;
 }
#endif

  // Parse .desktop file sFilePath.
  // Code taken from processLauncher. See comments there.
  clearLauncherElements();
  //http://developer.gnome.org/glib/2.28/glib-Key-value-file-parser.html
  GKeyFile* pGKeyFile = g_key_file_new();
  GError* gerror = NULL;
  if (!g_key_file_load_from_file(pGKeyFile, sFilePath, 0, &gerror))
  {
   snprintf(pme->m_sErrMsg, MAX_LINE_LENGTH, "'%s' %s\n",
     sFilePath, gerror->message);
   g_error_free(gerror);
   return lineParseFail;
  }

  int i = 0;
  gboolean bOk = TRUE;
  gchar * sValue = NULL;
  for (i=0; i < gl_nLauncherElements; i++)
  {
   // If key can't be found then NULL is returned and gerror is set to
   // G_KEY_FILE_ERROR_KEY_NOT_FOUND.  If the value associated with key
   // can't be interpreted or no suitable translation can be found then
   // the untranslated value is returned.
   sValue = NULL;
   gerror = NULL;
   if (gl_launcherElement[i].m_bTryLocalised)
    sValue = g_key_file_get_locale_string(pGKeyFile, "Desktop Entry",
        gl_launcherElement[i].m_sKeyword, NULL, NULL);
   if (!sValue)
   {
    gerror = NULL;
    sValue = g_key_file_get_string(pGKeyFile, "Desktop Entry",
        gl_launcherElement[i].m_sKeyword, NULL);
   }
   gl_launcherElement[i].sValue = sValue;

   if (bRequired)
   {
    if (!sValue && gl_launcherElement[i].bRequired)
    {
     snprintf(pme->m_sErrMsg, MAX_LINE_LENGTH, "Can't find %s= entry in '%s'\n",
         gl_launcherElement[i].m_sKeyword, sFilePath);
     bOk = FALSE;
     break;
    }
   }
  }
  g_key_file_free(pGKeyFile);
  if (!bOk) return lineParseFail;

  STRCPY_IF(pme->m_sTitle, gl_launcherElement[LAUNCHER_ELEMENT_NAME].sValue);
  STRCPY_IF(pme->m_sCmd, gl_launcherElement[LAUNCHER_ELEMENT_EXEC].sValue);
  STRCPY_IF(pme->m_sIcon,  gl_launcherElement[LAUNCHER_ELEMENT_ICON].sValue);
#if  !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
  STRCPY_IF(pme->m_sTooltip, gl_launcherElement[LAUNCHER_ELEMENT_COMMENT].sValue);
#endif
  STRCPY_IF(pme->m_sCategory, gl_launcherElement[LAUNCHER_ELEMENT_CATEGORY].sValue);
  sValue = gl_launcherElement[LAUNCHER_ELEMENT_NODISPLAY].sValue;
  pme->m_bNoDisplay = sValue ? 0 == strcmp("true", sValue) : FALSE;
#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
  if (iCaller == LINE_LAUNCHER_DIRFILE)
  {
   STRCPY_IF(gl_launcherDirFile.m_sFormatEq,
     gl_launcherElement[LAUNCHER_ELEMENT_FORMAT].sValue);
  }
#endif

#if !defined(_GTKMENUPLUS_NO_CACHE_)
  add_to_cache(sFilePath, pme);
#endif
  return lineParseOk;
}

// ----------------------------------------------------------------------
enum LineParseResult fillSubMenuEntry(IN const gchar* sLauncherPath, INOUT struct MenuEntry* pme, guint iCaller) // used by onLauncherCommon
// ----------------------------------------------------------------------
{
 // Is there a local dirfile?
 int fd = -1;
 gboolean bLocalDirFile = FALSE;
 void *dirfile = malloc(MAX_PATH_LEN + 1);
 snprintf(dirfile, MAX_PATH_LEN, "%s/.desktop.directory", sLauncherPath);
 bLocalDirFile = -1 != (fd = open(dirfile, O_RDONLY | O_NOATIME)); // #define _GNU_SOURCE_

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
 gl_launcherElement[LAUNCHER_ELEMENT_FORMAT].sValue = NULL; //init cascade
#endif

 if (bLocalDirFile)
 { // Fill pme from local dirfile ".desktop.directory".

  close(fd);
  // Don't overwrite m_uiDepth otherwise the sub-menu will nest at a higher level.
  guint sav = pme->m_uiDepth;
  enum LineParseResult lineParseResult =
   fillMenuEntry(dirfile, pme, FALSE, iCaller); //sets gl_LauncherElement[]
  pme->m_uiDepth = sav;
  free(dirfile);
  if (lineParseResult != lineParseOk)
   return lineParseResult;
 }
 else if (*gl_launcherDirFile.m_sPath)
 { // Fill pme from launcherdirfile=dirfile cached values.

  // Don't overwrite m_uiDepth otherwise the sub-menu will nest at a higher level.
  guint sav = pme->m_uiDepth;
  memcpy(pme, &gl_launcherDirFile.m_menuEntry, sizeof(struct MenuEntry));
  pme->m_uiDepth = sav;

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
  gl_launcherElement[LAUNCHER_ELEMENT_FORMAT].sValue = //set cascade
    strdup(gl_launcherDirFile.m_sFormatEq);
#endif
 }
 // If none of the above cases, fall back to the sufficient values that onSubMenu set for pme.

 // Catchall for dirfiles that set just the Icon= value. Fill in folder name for Name=.
 if (!*pme->m_sTitle)
 {
  gchar *p = rindex(sLauncherPath, '/');
  strcpy(pme->m_sTitle, p ? p+1 : sLauncherPath);
 }

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
 // Cascade formatting.
 gchar *fmt;
 if((fmt = gl_launcherElement[LAUNCHER_ELEMENT_FORMAT].sValue) && *fmt)
 { // pretend "format=" - code extracted from onFormat, cf. for comments
  if (strlen(fmt) + 15 > MAX_PATH_LEN)
  {
   snprintf(pme->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", ".desktop Format= line too long");
   return lineParseFail;
  }
  // Note: this pretend "format=" for "launcher=dir" operates on
  // level gl_uiCurDepth -1 because the simulated input sequence that
  // leads us here is ~submenu=~ followed by ~format=~. Contrast that
  // with the real formatting for "submenu=", which operates on level
  // gl_uiCurDepth, because the actual input sequence is "format="
  // followed by "submenu=".
  formattingInit(&(gl_FormattingSubMenu[gl_uiCurDepth - 1]), fmt, gl_uiCurDepth - 1);
 }
#endif
 return lineParseOk;
}

// ----------------------------------------------------------------------
enum LineParseResult launcherList(const IN funcOnMenuEntry func, INOUT struct MenuEntry* pme)
// ----------------------------------------------------------------------
// used by onLauncher, onLauncherSub to loop over gl_sLinePostEq ':' elements
{
 gchar *a, *ak, *at, *as;
 extern gboolean gl_bConfigKeywordLauncherListFirst;

 if (!(a = strdup(gl_sLinePostEq)) || !(ak = strdup(gl_sLinePostEq)))
 {
  perror("strdup");
  if (a)
   free(a);
  return lineParseFailFatal;
 }

 enum LineParseResult listResult = lineParseOk;

 struct MenuEntry *pme1;
 if (!(pme1 = malloc(sizeof(struct MenuEntry))))
 {
  perror("malloc");
  return lineParseFailFatal;
 }

 at = strtok_r(a, ":", &as);
 while (at)
 {
  strcpy(gl_sLinePostEq, at); // onLauncher{Sub} global input arg
  memcpy(pme1, pme, sizeof(struct MenuEntry)); // onLauncher{Sub} input arg
  enum LineParseResult lineParseResult = func(pme1);

  if (lineParseResult == lineParseOk)
  {
   if (gl_bConfigKeywordLauncherListFirst)
    break; // first found is enough if configure=launcherlistfirst
  }
  else if (*pme1->m_sErrMsg)
   msgToUser(lineParseResult, pme1->m_sErrMsg, NULL, 0, 0, at);

  if (lineParseResult > listResult)
   listResult = lineParseResult; // remember highest error and keep going

  at = strtok_r(NULL, ":", &as);
 }
 free(pme1);
 free(a);
 strcpy(gl_sLinePostEq, ak);
 free(ak);
 if (listResult != lineParseOk)
  strcpy(pme->m_sErrMsg, "^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^\n");
 return listResult;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onLauncher(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 gchar sCaller[] = "launcher";
 gchar msg[] = "%s= requires file or directory\n";

 if (!(*gl_sLinePostEq))
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, msg, sCaller);
  return lineParseFail;
 }

 if (index(gl_sLinePostEq, ':'))
  return launcherList(onLauncher, pMenuEntryPending);

 if (*gl_sLauncherDirectory && strlen(gl_sLinePostEq) == 1
   && (*gl_sLinePostEq == '*' || *gl_sLinePostEq == '.'))
 {
  // special case, only gl_sLauncherDirectory; set_base_dir already
  // checked len gl_sLauncherDirectory < len gl_sLinePostEq
  strcpy(gl_sLinePostEq, gl_sLauncherDirectory);
 }
 else
 {
  enum LineParseResult lineParseResult =
   expand_path( // can rewrite gl_sLinePostEq
     gl_sLinePostEq, gl_sLauncherDirectory, sCaller,
     pMenuEntryPending->m_sErrMsg);
  if (lineParseResult != lineParseOk)
   return lineParseResult;
 }

 return onLauncherCommon(pMenuEntryPending, sCaller, LINE_LAUNCHER);
}

// ----------------------------------------------------------------------
enum LineParseResult onLauncherSub(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 gchar sCaller[] = "launchersub";
 gchar msg[] = "%s= requires directory\n";

 if (!(*gl_sLinePostEq))
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, msg, sCaller);
  return lineParseFail;
 }

 if (index(gl_sLinePostEq, ':'))
  return launcherList(onLauncherSub, pMenuEntryPending);

 enum LineParseResult lineParseResult =
  expand_path( // can rewrite gl_sLinePostEq
    gl_sLinePostEq, gl_sLauncherDirectory, sCaller,
    pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 return onLauncherCommon(pMenuEntryPending, sCaller, LINE_LAUNCHER_SUB);
}

// ----------------------------------------------------------------------
char* getComparName(const struct dirent **a)
// used by launcherCompar
// ----------------------------------------------------------------------
{
 // TODO return m_sTitle also for directories tied to a dirfile= or .desktop.directory file.
 // TODO test case
 char *as = (char*)(*a)->d_name, *ak = NULL;
 if (
#ifdef _DIRENT_HAVE_D_TYPE
     DT_DIR != (*a)->d_type &&
#endif
     strcmp(as + strlen(as) - 8, ".desktop") == 0 &&
     (ak = malloc(sizeof(char) * MAX_LINE_LENGTH + 1)))
 {
  struct MenuEntry me; memset(&me, 0, sizeof(struct MenuEntry));
  strcpy(ak, gl_sLinePostEq);
  strncat(ak, as, MAX_LINE_LENGTH);
  if (lineParseFail > fillMenuEntry(ak, &me, FALSE, -1)) // iCaller -1 no harm
   strcpy(ak, me.m_sTitle);
  else
   strcpy(ak, as);
  return ak;
 }
 return NULL;
}

// ----------------------------------------------------------------------
int launcherFilter(const struct dirent *d)
// used by onLauncherCommon
// ----------------------------------------------------------------------
{
#define KEEP 1
#define SKIP 0
 char *p = (char*)(d->d_name + strlen(d->d_name) - 1);
 // skip paths .../. and .../..
 if (*p != '.')
  return KEEP;
 if (p == d->d_name)
  return SKIP;
 if (*(p-1) != '.')
  return KEEP;
 if (p-1 == d->d_name || *(p-2) == '/')
  return SKIP;
 return KEEP;
}

// ----------------------------------------------------------------------
int launcherCompar(const struct dirent **a, const struct dirent **b)
// used by onLauncherCommon - fetch a's & b's filenames from the cache and compare them
// ----------------------------------------------------------------------
{
 char *as, *bs, *am, *bm;
 if (!(as = am = getComparName(a)))
  as = (char*)(*a)->d_name;
 if (!(bs = bm = getComparName(b)))
  bs = (char*)(*b)->d_name;

 int ret = strcasecmp(as, bs);
 if (am) free(am);
 if (bm) free(bm);
 return ret;
}

// ----------------------------------------------------------------------
enum LineParseResult onLauncherCommon(INOUT struct MenuEntry* pMenuEntryPending, gchar *sCaller, guint iCaller) // used by onLauncher, onLauncherSub
// ----------------------------------------------------------------------
{
 //fprintf(stderr, "%s\n", gl_sLinePostEq); //DEBUG
 struct stat statbuf;
 if (stat(gl_sLinePostEq, &statbuf) == -1)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH,
   "%s=: %s: '%s'\n", sCaller, strerror(errno), gl_sLinePostEq);
  return lineParseFail;
 }

 // Permit launcher{sub}=file or symlink-to-file -- stat(2) follows links.
 // Note: launchersub='file' is undocumented. Only 'dir' is documented.
 if (S_ISREG(statbuf.st_mode))
 {
  return processLauncher(gl_sLinePostEq, lineParseFail,
    pMenuEntryPending, iCaller);
 }

 // Enter (linked-to) directory unless max menu depth exceeded.
 if (gl_uiCurDepth >= MAX_SUBMENU_DEPTH)
  return lineParseWarn;

 // Scan directory -- will recurse when iCaller is LINE_LAUNCHER_SUB.
 int len0 = strlen(gl_sLinePostEq);
 if (*(gl_sLinePostEq + len0 - 1) != '/')
 {
    gl_sLinePostEq[len0] = '/';
    gl_sLinePostEq[++len0] = '\0';
 }

 struct dirent **namelist;
 int n = -1;
#if !defined(_GTKMENUPLUS_NO_CACHE_)
 struct ScanDir *cached = find_in_scandir_cache(gl_sLinePostEq);
 if (cached)
 {
  n = cached->n; // >= 0
  namelist = cached->namelist;
 }
#endif
 if (n < 0)
  n = scandir(gl_sLinePostEq, &namelist, launcherFilter, launcherCompar);
 if (n < 0)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH,
    "%s=: %s: '%s'\n", sCaller, strerror(errno), gl_sLinePostEq);
  return lineParseFail;
 }
#if !defined(_GTKMENUPLUS_NO_CACHE_)
 if (!cached)
 {
  struct ScanDir sd;
  sd.namelist = namelist;
  sd.n = n;
  add_to_scandir_cache(gl_sLinePostEq, &sd);
 }
#endif
 // {N1} Correction for case 'readLine found "launcher{sub}=" nested in "submenu="'.
 pMenuEntryPending->m_uiDepth = gl_uiCurDepth;

 int i;
 for (i = 0; i < n; i++)
 {
  // Note: the clean way to break out of this loop is 'goto break_this_loop'
  enum LineParseResult lineParseResult;
  gchar sLauncherPath1[MAX_PATH_LEN + 1];
  snprintf(sLauncherPath1, MAX_PATH_LEN, "%s%s", gl_sLinePostEq, namelist[i]->d_name);
  //fprintf(stderr, "%s\n", sLauncherPath1); //DEBUG
  int len1 = len0 + _D_EXACT_NAMLEN(namelist[i]);
#ifdef _DIRENT_HAVE_D_TYPE
  int d_type = namelist[i]->d_type;
#endif
#if defined(_GTKMENUPLUS_NO_CACHE_) // *defined*
  free(namelist[i]);
#endif

  // -----------------
  // LINE_LAUNCHER_SUB
  // -----------------
  if (
   // ----------------------------
   LINE_LAUNCHER_SUB == iCaller // recursive
   // ----------------------------
   && (
#ifdef _DIRENT_HAVE_D_TYPE
   DT_DIR == d_type || // speed up common case "directory"
#endif
   (stat(sLauncherPath1, &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
   ))
  // Path of directory or of symlink-to-directory : walk the tree.
  {

   if('.' == sLauncherPath1[len1 -1]) continue; // skip . and ..

   gchar gl_sLinePostEq1[MAX_LINE_LENGTH];
   strcpy(gl_sLinePostEq1, gl_sLinePostEq);

   if (0 == gl_uiCurDepth - gl_nLauncherReadLineDepth) // Depth-1 directory path
   {
    // Create data base of .desktop files in and under depth-1 directory path.
    *gl_sLauncherDB = '\0';
    off_t size = createLauncherDB(sLauncherPath1, gl_sLauncherDB, pMenuEntryPending->m_sErrMsg);
    if (0 > size)
    {
     lineParseResult = lineParseFail;
     goto break_this_loop;
    }
    else if (0 == size) continue; // prune empty depth-1 directory
   }
   else // Depth > 1: are there any .desktop files at or below this path?
   {
    if (0 != lookupLauncherDB(sLauncherPath1, gl_sLauncherDB))
     continue; // prune "empty" sub-directory tree
   }
   // Getting here means that there are some .desktop files in or under
   // directory path sLauncherPath1.

   // Pretend readLine read "submenu=".
   strcpy(gl_sLinePostEq, sLauncherPath1 + len0);
   lineParseResult = onSubMenu(pMenuEntryPending); // if Ok: sets pending commitSubmenu and gl_uiCurDepth++
   if (lineParseResult != lineParseOk)
   {
    reapErrMsg(pMenuEntryPending, lineParseResult, sLauncherPath1);
    strcpy(gl_sLinePostEq, gl_sLinePostEq1);
    // On to the next sibling: a file will succeed, another directory will fail.
    continue;
   }

   // Fill sub-menu elements from in-node configuration files, if any.
   lineParseResult = fillSubMenuEntry(sLauncherPath1, pMenuEntryPending, iCaller);
   if (lineParseResult != lineParseOk)
   {
    reapErrMsg(pMenuEntryPending, lineParseResult, sLauncherPath1);// if any
    // And carry on: *pMenuEntryPending is filled with fallback values anyway.
   }

   // Manually commit the sub-menu, which normally readLine commits automatically.
   lineParseResult = commitSubMenu(pMenuEntryPending);
   if (lineParseResult == lineParseOk)
   {
    pMenuEntryPending->m_uiDepth++; // since commitSubMenu Ok: m_uiDepth <- gl_uiCurDepth

    // Recursive pretend "launchersub="
    //   After onLauncherSub thou shall call onSubMenuEnd and their results be paired
    strcpy(gl_sLinePostEq, sLauncherPath1);
    uint nCountBefore = gl_nLauncherCount;
    enum LineParseResult pairedResult =
     onLauncherCommon(pMenuEntryPending, sCaller, iCaller);
    reapErrMsg(pMenuEntryPending, pairedResult, sLauncherPath1); // if any
    //
    // Pretend "submenuend"
    gboolean sav = gl_bConfigKeywordUseEndSubMenu;
    gl_bConfigKeywordUseEndSubMenu = TRUE; // pretend "configure=submenuend"
    lineParseResult = onSubMenuEnd(pMenuEntryPending); // if Ok: .m_uiDepth = --gl_uiCurDepth
    assert(lineParseResult == lineParseOk);
    gl_bConfigKeywordUseEndSubMenu = sav;
    //reapErrMsg(pMenuEntryPending, lineParseResult, NULL); // if any
    lineParseResult = pairedResult != lineParseOk ? pairedResult : lineParseResult;

    // Did we actually add any entries to the committed sub-menu?
    if (gl_nLauncherCount == nCountBefore)
    { //No
     if (gl_nOptInfo > 0) // TODO place message in the standard sErrMsg queue
       fprintf(stderr, "No launchers displayed for %s\n", sLauncherPath1);
     // Uncommit the sub-menu entry.
     gtk_widget_destroy(gl_gtkMenuEntry[pMenuEntryPending->m_uiDepth]);
     gl_gtkMenuEntry[pMenuEntryPending->m_uiDepth] = NULL;
    }
    //fprintf(stderr, "%d launchers added\n", gl_nLauncherCount - nCountBefore); //DEBUG
   }
   if (lineParseResult >= lineParseFail)
    goto break_this_loop;

   strcpy(gl_sLinePostEq, gl_sLinePostEq1);
   continue; // on to the next item
  }
  else // Non-directory path.
  // ----------------------------------
  // LINE_LAUNCHER or LINE_LAUNCHER_SUB
  // ----------------------------------
  {
   if (
    // skip filename !~ .desktop$...
    strcmp(sLauncherPath1 + len1 - 8, ".desktop") != 0
   )
   {
    // ...unless it's a link target name ultimately ~ .desktop$
#ifdef _DIRENT_HAVE_D_TYPE
    if (DT_LNK == d_type)
#else
    if(S_ISLNK(statbuf.st_mode))
#endif
    { // Symlink.
     char *link_target = realpath(sLauncherPath1, NULL);
     if(link_target == NULL)
     {
      perror(sLauncherPath1);
      continue; // skip bad link
     }
     int not_ends_with_desktop =
      strcmp(link_target + strlen(link_target) - 8, ".desktop") != 0;
     free(link_target);
     if(not_ends_with_desktop)
      continue; // skip link target name !~ .desktop$
    }
    else
     continue; // skip non-link name !~ *.desktop$
   }
   else
   {
#ifdef _DIRENT_HAVE_D_TYPE
    if (DT_LNK == d_type)
#else
    if(S_ISLNK(statbuf.st_mode))
#endif
    { // Symlink.
     gchar buf[MAX_PATH_LEN + 1];
     if(NULL == realpath(sLauncherPath1, buf))
     {
      snprintf(pMenuEntryPending->m_sErrMsg, MAX_PATH_LEN,
        "%s\n", strerror(errno));
      reapErrMsg(pMenuEntryPending, lineParseWarn, sLauncherPath1);
      continue; // skip invalid symlinks
     }
    }
   }
  }
  // Getting here means that there is a non-directory path (file,
  // symlink) that can be processed as a launcher file.

  lineParseResult = processLauncher(sLauncherPath1,
      lineParseOk, pMenuEntryPending, iCaller);
  if (lineParseResult != lineParseOk)
    reapErrMsg(pMenuEntryPending, lineParseResult, sLauncherPath1);
  if (lineParseResult >= lineParseFail)
  {
break_this_loop: // IN lineParseResult

   // But even if severe errors occurred we don't want to break this loop.
   continue;

   // If instead you wanted to bail out on the first severe error:
   /* if (0 == gl_uiCurDepth - gl_LauncherReadLineDepth && *gl_sLauncherDB != '\0') unlink(gl_sLauncherDB); */
   /* for (i = i + 1; i < n; i++) free(namelist[i]); */
   /*#if defined(_GTKMENUPLUS_NO_CACHE_) // *defined* */
   /* free(namelist); */
   /*#endif */
   /* return lineParseResult; */

  }
 }

#if defined(_GTKMENUPLUS_NO_CACHE_) // *defined*
 if (namelist) free(namelist);
#endif

 // -----------------
 // LINE_LAUNCHER_SUB
 // -----------------
 if (LINE_LAUNCHER_SUB == iCaller)
 // Recursion tail.
 // Reset pending commitSubmenu, which onSubMenu set. Resets pMenuEntryPending->m_sErrorMsg.
 {
  gchar sav[MAX_LINE_LENGTH + 1];
  strncpy(sav, pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH);
  menuEntrySet(pMenuEntryPending, NULL, LINE_LAUNCHER_SUB, sCaller, FALSE, TRUE, gl_uiCurDepth); // bCmdOk, bIconTooltipOk
  // Saw error message.
  strcpy(pMenuEntryPending->m_sErrMsg, sav);
 }

 switch (gl_uiCurDepth - gl_nLauncherReadLineDepth)
 {
  case 1: // On exiting a level-1 tree walk
   if (*gl_sLauncherDB != '\0') unlink(gl_sLauncherDB);
  break;
  case 0: // On exiting the whole tree walk
   if (*gl_sLauncherDB != '\0') unlink(gl_sLauncherDB);
   if (*gl_sReapedErrMsgs != '\0')
   {
    strncpy(pMenuEntryPending->m_sErrMsg, gl_sReapedErrMsgs, MAX_LINE_LENGTH);
    return lineParseWarn;
    // In this case warning vs. error is a matter of personal preference.
   }
  break;
 }
 return lineParseOk;
}

// ----------------------------------------------------------------------
enum LineParseResult onLauncherArgs(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 strcpy(gl_sLauncherArguments, gl_sLinePostEq);
 return lineParseOk;
}

// ----------------------------------------------------------------------
enum LineParseResult onLauncherDirFile(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 if (!(*gl_sLinePostEq))
 {
  *gl_FormattingSubMenu[pMenuEntryPending->m_uiDepth].m_sFormat =
  gl_FormattingSubMenu[pMenuEntryPending->m_uiDepth].m_cFormatDivider =
  *gl_launcherDirFile.m_menuEntry.m_sCategory =
  *gl_launcherDirFile.m_sPath = '\0';
  return lineParseOk;
 }
 enum LineParseResult lineParseResult = expand_path(gl_sLinePostEq, gl_sScriptDirectory,
     "launcherdirfile", pMenuEntryPending->m_sErrMsg); // can rewrite gl_sLinePostEq
 if (lineParseResult != lineParseOk)
  return lineParseResult;

lineParseResult = fillMenuEntry(gl_sLinePostEq,
  &gl_launcherDirFile.m_menuEntry,
  FALSE, //no check for required
  LINE_LAUNCHER_DIRFILE);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 // after this, dirfile path is only used for error messages, so we shorten it
 strcpy(gl_launcherDirFile.m_sPath, gl_sLinePostEq);
 shorten(gl_launcherDirFile.m_sPath, gl_launcherDirFile.m_sPath);

 return lineParseOk;
}

// ----------------------------------------------------------------------
enum LineParseResult onLauncherSubMenu(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 gchar msg[] = "launchersubmenu= requires file\n";
 if (!(*gl_sLinePostEq))
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, msg);
  return lineParseFail;
 }

 enum LineParseResult lineParseResult = expand_path(gl_sLinePostEq, gl_sScriptDirectory,
     "launchersubmenu", pMenuEntryPending->m_sErrMsg); // can rewrite gl_sLinePostEq
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 struct stat sb;
 if (stat(gl_sLinePostEq, &sb) == -1 || !(S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode)))
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH,
      "launchersubmenu='%s': %s\n", gl_sLinePostEq, strerror(ENODATA));
  return lineParseFail;
 }

 lineParseResult = onSubMenu(pMenuEntryPending); // if OK ++gl_uiCurDepth
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 strcpy(pMenuEntryPending->m_sMenuEntryType, "launchersubmenu=");
 lineParseResult = fillMenuEntry(gl_sLinePostEq,
   pMenuEntryPending,
   FALSE, //no check for required
   LINE_LAUNCHER_SUBMENU);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
 // TODO not sure what to do about "Format=" which DirFile may carry in
 // gl_launcherElement[LAUNCHER_ELEMENT_FORMAT].sValue); Ultimately,
 // commitSubMenu >> addFormatting to handle formatting stored in
 // gl_FormattingSubMenu[uiCurDepth]
#endif

 return lineParseOk;
}

// ----------------------------------------------------------------------
gboolean intersectQ(INOUT gchar *a, INOUT gchar *b)
// ----------------------------------------------------------------------
{
 gchar *at, *as, *bt, *bs, *bk;

 if (! (bk = strdup(b)))
 {
  perror("strdup");
  return FALSE; //error
 }

 at = strtok_r(a, ";", &as);
 while (at)
 {
  strcpy(b, bk);
  bt = strtok_r(b, ";", &bs);
  while (bt)
  {
   if (0 == strcmp(at, bt))
   {
    free(bk);
    return TRUE;
   }
   bt = strtok_r(NULL, ";", &bs);
  }
  at = strtok_r(NULL, ";", &as);
 }
 free(bk);
 return FALSE;
}

// ----------------------------------------------------------------------
gboolean intersectingCategoriesQ(IN const gchar *a, IN const gchar *b) //used by processLauncher
// *a from the .desktop file; *b from the .desktop.directory dirfile
// ----------------------------------------------------------------------
{
 extern gboolean gl_bConfigKeywordLauncherNullCategory;
 if (!(a && *a) && b && !gl_bConfigKeywordLauncherNullCategory)
 {
  guint l1 = strlen(b) + 1;
  gchar *b2 = malloc(l1 + 2);
  if (!b2) {
   perror("malloc");
   return FALSE;
  }
  *b2 = ';';
  strcpy(b2 + 1, b);
  strcpy(b2 + l1, ";");
  gboolean match = NULL != strstr(b2, ";NULL;");
  free(b2);
  return match;
 }

 if (!(a && *a && b && *b))
  return TRUE;
 void *p = strdup(a);
 void *q = strdup(b);
 if (!p || !q)
 {
  perror("strdup");
  return TRUE;
 }
 gboolean bIntersecting = intersectQ(p, q);
 free(p);
 free(q);
 return bIntersecting;
}

// ----------------------------------------------------------------------
enum LineParseResult processLauncher(IN gchar* sLauncherPath, IN gboolean stateIfNotDesktopFile, INOUT struct MenuEntry *pme, guint iCaller)
// ----------------------------------------------------------------------
{
 enum LineParseResult lineParseResult =
  fillMenuEntry(sLauncherPath, pme, TRUE, iCaller);
 if(lineParseResult >= lineParseFail)
  return lineParseResult;

 pme->m_uiDepth = gl_uiCurDepth; // like onItem

 extern gboolean gl_bConfigKeywordLauncherNoDisplay;
 if (gl_bConfigKeywordLauncherNoDisplay && pme->m_bNoDisplay)
  return lineParseNoDisplay; // honor NoDisplay="true"

 // Apply Category=filter_list, if any.
 //TODO global *gl_sLauncherDirFile' categories vs. local .desktop.directory categories
 if (! intersectingCategoriesQ(
    pme->m_sCategory,
    gl_launcherDirFile.m_menuEntry.m_sCategory))
 {
  if (gl_nOptInfo > 1)
   snprintf(pme->m_sErrMsg, MAX_LINE_LENGTH, "<- excluded by '%s'\n",
      gl_launcherDirFile.m_sPath); // sLauncherPath
  return lineParseWarn;
 }

 //http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
 gchar *sValue = pme->m_sCmd;
 if (sValue)
 {
  regmatch_t pmatch[2];
  if (regexec(&gl_rgxLauncherExecArg, sValue, 1, pmatch, 0) == 0)
  {
   // Erase the first %f token of entry Exec= (%F, %u, etc.).
   gchar *p;
   for(p = sValue + pmatch[0].rm_so; p < sValue + pmatch[0].rm_eo; p++)
     *p = ' ';
  }
  if(*gl_sLauncherArguments)
  {
   // Append "launcherargs=" arguments, if any.
   gchar buf[MAX_PATH_LEN + 1];
   if (sizeof buf <= snprintf(buf, sizeof buf, "%s %s", sValue, gl_sLauncherArguments))
   {
    snprintf(pme->m_sErrMsg, MAX_LINE_LENGTH, "cmd + launcherargs too long (%s)\n", buf);
    return lineParseFail;
   }
   sValue = buf;
  }
  strcpy(gl_sCmds[gl_uiCurItem], sValue);
 } // if (sValue)
 lineParseResult = resizeCommandBuffer(pme->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;
 lineParseResult = onIconForLauncher(sLauncherPath, pme);
 if (lineParseResult < lineParseFail)
  ++gl_nLauncherCount;

#if !defined(_GTKMENUPLUS_NO_ACTIVATION_LOG_)
 struct LogItem *logItem = makeLogItem(
  pme->m_sTitle,
  gl_sCmds[gl_uiCurItem - 1],
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
  pme->m_sTooltip,
#else
  NULL,
#endif
  pme->m_sIcon);

 if (logItem)
 {
  // See {N2} elsewhere.
  gchar *p;
  memcpy(p = gl_sCmds[gl_uiCurItem - 1], logItem->sItem,
    MIN(logItem->uiSize, MAX_PATH_LEN));
  p[MAX_PATH_LEN] = '\0';
  free(logItem->sItem);
  free(logItem);
 }
#endif // _GTKMENUPLUS_NO_ACTIVATION_LOG_

 return lineParseResult;
}

// ----------------------------------------------------------------------
enum LineParseResult onIconForLauncher(IN gchar* sLauncherPath, INOUT struct MenuEntry* pme)
// ----------------------------------------------------------------------
{
 OUT gchar* sErrMsg = pme->m_sErrMsg;

 GtkWidget *pGtkWdgtCurrent = makeItem( // counts up gl_uiCurItem
   pme->m_sTitle,
   gl_sCmds[gl_uiCurItem], // need to be available at runItem time
   pme->m_sTooltip,
   pme->m_uiDepth);
 if (!pGtkWdgtCurrent)
  return lineParseFail;

 gchar* sIconPath = pme->m_sIcon;

retry:
 if (sIconPath)
 {
  gchar* sIconExt = strrchr(sIconPath, '.');
  if (sIconExt && regexec(&gl_rgxIconExt, sIconExt, 0, NULL, 0) != 0) sIconExt = NULL;

  GdkPixbuf* pGdkPixbuf = NULL;
  if (strchr(sIconPath, '/') == NULL && sIconExt == NULL)
  {
   // sIconPath isn't a full path and doesn't match an icon file name extension
   gchar* sIconPathNew = getIconPath(sIconPath, gl_iW, FALSE);
   if (sIconPathNew)
   {
    //FIXME ??leak?? g_free(gl_nLauncherElements[...ICON].sValue);
    strcpy(pme->m_sIcon, sIconPathNew);
    free(sIconPathNew);
   }
  } // if (strchr(sIconPath, '/') == NULL && sIconExt == NULL)

  sIconPath = pme->m_sIcon;
  sIconExt = strrchr(sIconPath, '.');
  if (sIconExt && regexec(&gl_rgxIconExt, sIconExt, 0, NULL, 0) != 0) sIconExt = NULL;
   // if sIconPath isn't a full path and doesn't match an icon file name extension
  if (strchr(sIconPath, '/') == NULL && sIconExt == NULL)
   pGdkPixbuf = getIconBuiltInPixBuf(sIconPath, gl_iW, FALSE);
  else
   pGdkPixbuf = fileToPixBuf(sIconPath, gl_iW, FALSE, sErrMsg); // resizes

  if (!pGdkPixbuf)
  {
    if (sIconExt)
    {
      *sIconExt = *sErrMsg = '\0';
      goto retry;
    }
   snprintf(sErrMsg, MAX_LINE_LENGTH, "Can't get 'Icon=%s'\n", sIconPath);
   //Don't destroy items w/o icon: gtk_widget_destroy(pGtkWdgtCurrent);
   return lineParseWarn;
  }

  GtkWidget* image = gtk_image_new_from_pixbuf(pGdkPixbuf);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM (pGtkWdgtCurrent), image);
#if !defined(_GTKMENUPLUS_NO_ALWAYS_SHOW_ICONS_)
  gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM (pGtkWdgtCurrent), TRUE);
#endif

 } // if (*sIconPath)

 // TODO revisit
 freeLauncherElementsMem(); //(gl_launcherElement, sizeof(gl_launcherElement)/sizeof(struct LauncherElement));

 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult onLauncherDir(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 return set_base_dir(gl_sLauncherDirectory, "launcherDirectory", pMenuEntryPending->m_sErrMsg);
}

#endif // #if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)

// ---------------------------------------------------------------------- AC
enum LineParseResult commitInclude(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 gchar sLinePostEq[MAX_LINE_LENGTH + 2];
 strcpy(sLinePostEq, gl_sLinePostEq);    // save read-in line; will be overwritten by readLine < readFile < includeFile

 enum LineParseResult lineParseResult = expand_path(pMenuEntryPending->m_sTitle, gl_sScriptDirectory, "include", pMenuEntryPending->m_sErrMsg);
 if (lineParseResult != lineParseOk)
  return lineParseResult;

 gint argc = 0;
 gchar ** argvp = NULL;
 GError* pGError = NULL;
 gboolean bOk = g_shell_parse_argv(pMenuEntryPending->m_sTitle,  &argc, &argvp, &pGError);
 if (!bOk || argc == 0)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s.\n", "\"include=\" line without file or directory name");
  commitIncludeTerminate(argvp, sLinePostEq);
  return lineParseFail;
 }

 if (*(pMenuEntryPending->m_sCmd))
  strcat(pMenuEntryPending->m_sCmd, " ");

 gboolean bIsWildcard  = strpbrk(argvp[0], "*?[") != NULL;

 struct stat statbuf;

 if (!bIsWildcard && stat(argvp[0], &statbuf) == 0 && S_ISREG(statbuf.st_mode))
  lineParseResult = includeFile(argc, argvp, pMenuEntryPending); // &MenuEntryPendingLocal
 else // if (!bIsWildcard && stat(argvp[0], &statbuf) == 0 && S_ISREG(statbuf.st_mode))
  lineParseResult = includeDirectory(argc, argvp, pMenuEntryPending); // &MenuEntryPendingLocal

 commitIncludeTerminate(argvp, sLinePostEq);
 return lineParseResult;
}


// ---------------------------------------------------------------------- AC
void commitIncludeTerminate(INOUT gchar ** argvp, gchar* sLinePostEq)
// ----------------------------------------------------------------------
{
 if (argvp) g_strfreev(argvp);
 strcpy(gl_sLinePostEq, sLinePostEq); // recover read-in line
}

// ---------------------------------------------------------------------- AC
enum LineParseResult includeFile(IN gint argc, IN  gchar** argvp, INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 struct MenuDescFile menuDescFile;
 menuDescFile.sName = argvp[0];
 FILE* pFile = open_menu_desc_file(&menuDescFile);
 if (!pFile)
  return lineParseFail;

//save context
#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
// gchar  sItemFormat[MAX_PATH_LEN + 1];
 struct Formatting formattingSubMenu[MAX_SUBMENU_DEPTH];
 memcpy(&formattingSubMenu, &gl_FormattingSubMenu, sizeof(struct Formatting) * MAX_SUBMENU_DEPTH);
//strcpy(sItemFormat, gl_sItemFormat);

#if  !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
// gchar  sTooltipFormat[MAX_PATH_LEN + 1];
 struct  Formatting formattingTooltip;
 memcpy(&formattingTooltip, &gl_FormattingTooltip, sizeof(struct Formatting));
// strcpy(sTooltipFormat, gl_sTooltipFormat);
#endif

#endif // #if  !defined(_GTKMENUPLUS_NO_FORMAT_)

 gchar sIconDirectory[MAX_PATH_LEN + 1];
 strcpy(sIconDirectory, gl_sIconDirectory);

 gchar sScriptDirectory[MAX_PATH_LEN + 1];
 strcpy(sScriptDirectory, gl_sScriptDirectory);

 if (!initDirectory(gl_sScriptDirectory, MAX_PATH_LEN, argvp[0]))
  return lineParseFail;

 if (strcmp(sScriptDirectory, gl_sIconDirectory) == 0)
  strcpy(gl_sIconDirectory, gl_sScriptDirectory);

#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
 gchar sLauncherDirectory[MAX_PATH_LEN + 1]; // used by onLauncher(), set by onLauncherDir()
 strcpy(sLauncherDirectory, gl_sLauncherDirectory);

 if (strcmp(sScriptDirectory, gl_sLauncherDirectory) == 0)
   strcpy(gl_sLauncherDirectory, gl_sScriptDirectory);
#endif

 enum LineParseResult lineParseResult =
   readFile(&menuDescFile, argc, argvp, TRUE, FALSE, pMenuEntryPending->m_uiDepth, pMenuEntryPending); // bReadingIncludedFile,,bGatherComments

//restore context
 strcpy(gl_sIconDirectory, sIconDirectory);
 strcpy(gl_sScriptDirectory, sScriptDirectory);

#if  !defined(_GTKMENUPLUS_NO_FORMAT_)
 if (gl_bConfigKeywordFormattingLocal)
  memcpy(&gl_FormattingSubMenu, &formattingSubMenu, sizeof(struct Formatting) * MAX_SUBMENU_DEPTH);

#if  !defined(_GTKMENUPLUS_NO_TOOLTIPS_)

 if (gl_bConfigKeywordFormattingLocal)
  memcpy(&gl_FormattingTooltip, &formattingTooltip, sizeof(struct Formatting));

#endif

#endif // #if  !defined(_GTKMENUPLUS_NO_FORMAT_)



#if  !defined(_GTKMENUPLUS_NO_LAUNCHERS_)
 strcpy(gl_sLauncherDirectory, sLauncherDirectory);
#endif
 return lineParseResult;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult includeDirectory(IN gint argc, IN  gchar** argvp, INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 gchar* sDirectorySpec = argvp[0];
 gchar* sDirectory = sDirectorySpec;
 gchar* sSubDirectorySpec = "";
/*
 gchar* sDirectorySpecEnd = sDirectorySpec + strlen(sDirectorySpec) - 1;
 if (*sDirectorySpecEnd == INCLUDE_RECURSE_FLAG)
 {
  bRecurse = TRUE;
  *sDirectorySpecEnd = '\0';
  trim_trailing(sDirectorySpec, sDirectorySpecEnd - 1);
 }
*/
 gchar* sFileSpec = "*";

 guint nSubDirectorySpecArg = 1;
 if (argc > 2)
 {
  nSubDirectorySpecArg = 2;
 }
 else // !argc > 2; filespec part of first arg
 {
  /* TODO review */
  /* simplistic first-level "tokenizer" (caveat: no backslash escapes) */
  /* - POSIX special "opening" characters *?[+ why ! */
  /* - GNU fnmatch(3) extended globbing characters @ why not | and () */
  gchar* sSpecPattern = strpbrk(sDirectorySpec, "*?[@+!");

  if (sSpecPattern)
  {
   gchar cSpecPattern = *sSpecPattern;
   *sSpecPattern = '\0';
   gchar* sSpecBefore = strrchr(sDirectorySpec, '/');
   if (sSpecBefore)
   {
    sFileSpec = sSpecBefore + 1;  // TO DO else??
    *sSpecBefore = '\0';
   }
   else
    sFileSpec = sSpecPattern;

   *sSpecPattern = cSpecPattern;
  } // if (sSpecPattern)

 } // if (argc > 2)

 if (argc > nSubDirectorySpecArg)
 {
  sSubDirectorySpec = argvp[nSubDirectorySpecArg];
 }

#ifdef PATH_ELEMENT_SPEC_USES_REGEX
 regex_t rgxFileSpec;
 regex_t rgxSubDirectorySpec;

 if (regcomp(&rgxFileSpec, sFileSpec, REG_EXTENDED | REG_ICASE))
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "\"include=\" file_glob regex doesn't compile");
  return lineParseFail;
 }

 if (*sSubDirectorySpec && regcomp(&rgxSubDirectorySpec, sSubDirectorySpec, REG_EXTENDED | REG_ICASE))
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "\"include=\" subdirectory _glob regex doesn't compile");
  return lineParseFail;
 }
#endif

 struct stat statbuf;
 if (stat(sDirectory, &statbuf) == -1 || !S_ISDIR(statbuf.st_mode))
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "%s\n", "invalid file or directory or wildcard in \"include=\" expression");
  return lineParseFail;
 }

// return processIncludedDirectory(sDirectory, "", &rgxFileSpec, *sSubDirectorySpec ? &rgxSubDirectorySpec : NULL, pMenuEntryPending);
 enum LineParseResult lineParseResult = processIncludedDirectory(sDirectory, "", sFileSpec, sSubDirectorySpec, pMenuEntryPending);

#ifdef PATH_ELEMENT_SPEC_USES_REGEX
 regfree(&rgxFileSpec);
 regfree(&rgxSubDirectorySpec);
#endif
 return lineParseResult;

}

// ---------------------------------------------------------------------- AC
enum LineParseResult processIncludedDirectory(IN gchar* sDirectory, IN gchar* sSubDirectory,
                                              IN GLOB_SPEC_TYPE* sFileSpec, IN GLOB_SPEC_TYPE* sSubDirectorySpec, INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
 struct stat statbuf;

 enum LineParseResult lineParseResult = lineParseOk;
 struct dirent **namelist;
 gchar sDirectoryFull[MAX_PATH_LEN + 1];
 strcpy(sDirectoryFull, sDirectory);
 if (*sSubDirectory)
 {
  strcat(sDirectoryFull, "/");
  strcat(sDirectoryFull, sSubDirectory);
 }

 int n = scandir(sDirectoryFull, &namelist, 0, alphasort);
 int i = 0;

 if (*sSubDirectorySpec)
 {
  for (i = 0; i < n; i++)
  {
   gchar* sName = namelist[i]->d_name;
   gchar sPathFull[MAX_PATH_LEN + 1];
   snprintf(sPathFull, MAX_PATH_LEN, "%s/%s", sDirectoryFull, sName);
   if (stat(sPathFull, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) && access(sPathFull, R_OK) == 0 &&
        strcmp(sName, "..") != 0 && strcmp(sName, ".") != 0 )
   {
    if (bPathElementInluded(sSubDirectorySpec, sName)) // matched pattern  FNM_CASEFOLD FNM_NOESCAPE   | FNM_PERIOD |
    {
     if (checkSubDirectoryForMatchingFiles(sDirectoryFull, sName, sFileSpec, sSubDirectorySpec))
     {
      lineParseResult = makeSubDirectoryInIncludedDirectory(sName, pMenuEntryPending); // bumps up gl_uiCurDepth whether failed or not
      pMenuEntryPending->m_uiDepth++;
      if (lineParseResult == lineParseOk || lineParseResult == lineParseWarn)
       lineParseResult = processIncludedDirectory(sDirectoryFull, sName, sFileSpec, "*", pMenuEntryPending); // sSubDirectorySpec

#if !defined(_GTKMENUPLUS_NO_FORMAT_)
      if (!gl_FormattingSubMenu[gl_uiCurDepth].m_cFormatDivider) // m_cFormatDivider zero if no compound format
        formattingInit(&(gl_FormattingSubMenu[gl_uiCurDepth]), "\0", 0);
#endif
      gl_uiCurDepth--;
      pMenuEntryPending->m_uiDepth--;
      if (lineParseResult != lineParseOk && lineParseResult != lineParseWarn)
        break;
     } // if (checkSubDirectoryForMatchingFiles(sDirectoryFull, sName, sFileSpec, sSubDirectorySpec))
    } // if (!bPathElementInluded(sSubDirectorySpec, sName))
   } // if (S_ISDIR(statbuf.st_mode) && strcmp(sName, "..") != 0 && strcmp(sName, ".") != 0 )
  } // for (i = 0; i < n; i++)
 } //  if (*sSubDirectorySpec)

 for (i = 0; i < n; i++)
 {
  gchar* sName = namelist[i]->d_name;
  gchar sPathFull[MAX_PATH_LEN + 1];
  snprintf(sPathFull, MAX_PATH_LEN, "%s/%s", sDirectoryFull, sName);

  if (stat(sPathFull, &statbuf) == 0 && S_ISREG(statbuf.st_mode) && access(sPathFull, R_OK) == 0)
  {
   if (bPathElementInluded(sFileSpec, sName)) // matched pattern  FNM_CASEFOLD FNM_NOESCAPE FNM_PERIOD
   {
    *(gl_sCmds[gl_uiCurItem]) = '\0';
    if (*(pMenuEntryPending->m_sCmd))
     strcpy(gl_sCmds[gl_uiCurItem], pMenuEntryPending->m_sCmd);
    strncat(gl_sCmds[gl_uiCurItem], sPathFull, MAX_PATH_LEN);
//  strcpy(pMenuEntryPending->m_sCmd, sPathFull);
    pMenuEntryPending->m_uiDepth = gl_uiCurDepth;

    GtkWidget *pGtkWdgtCurrent = makeItem(sName, gl_sCmds[gl_uiCurItem],
#if !defined(_GTKMENUPLUS_NO_TOOLTIPS_)
              pMenuEntryPending->m_sTooltip,
#else
              NULL,
#endif
              pMenuEntryPending->m_uiDepth); // counts up gl_uiCurItem

//   add icon to item; ALWAYS RETURN TRUE, SOFT ERROR IF ICON MISSING
    lineParseResult = (pGtkWdgtCurrent != NULL) ? addIcon(pMenuEntryPending, pGtkWdgtCurrent) : lineParseFail; //aborts all furth item creation
   } // if (bPathElementInluded(sFileSpec, sName))
  } // if (S_ISREG(statbuf.st_mode))
 } // for (i = 0; i < n; i++)

 for (i = 0; i < n; i++)
 {
  free(namelist[i]);//
 } // for (i = 0; i < n; i++)

 if (namelist) free(namelist);
 return lineParseResult;
}

// ---------------------------------------------------------------------- AC
gboolean bPathElementInluded(IN GLOB_SPEC_TYPE* sSpec, IN gchar* sPathElement)
// ----------------------------------------------------------------------
{

#ifdef PATH_ELEMENT_SPEC_USES_REGEX
 return (regexec(&sSpec, sPathElement, 0, NULL, 0) == 0);
#else
 return (!fnmatch(sSpec, sPathElement, FNM_PATHNAME | FNM_EXTMATCH));// matched pattern  FNM_CASEFOLD FNM_NOESCAPE   | FNM_PERIOD |
#endif
}

// ---------------------------------------------------------------------- AC
gboolean checkSubDirectoryForMatchingFiles(IN gchar* sDirectory, IN gchar* sSubDirectory,
                                           IN GLOB_SPEC_TYPE* sFileSpec, IN GLOB_SPEC_TYPE* sSubDirectorySpec)
// ----------------------------------------------------------------------
{
 struct stat statbuf;
 gboolean bFound = FALSE;
 struct dirent **namelist;
 gchar sDirectoryFull[MAX_PATH_LEN + 1];
 strcpy(sDirectoryFull, sDirectory);
 if (*sSubDirectory)
 {
  strcat(sDirectoryFull, "/");
  strcat(sDirectoryFull, sSubDirectory);
 }

 int n = scandir(sDirectoryFull, &namelist, 0, alphasort);
 int i = 0;

 for (i = 0; i < n; i++)
 {
  gchar* sName = namelist[i]->d_name;
  gchar sPathFull[MAX_PATH_LEN + 1];
  snprintf(sPathFull, MAX_PATH_LEN, "%s/%s", sDirectoryFull, sName);

  if (stat(sPathFull, &statbuf) == -1) // failed
   continue;

  if (S_ISREG(statbuf.st_mode) && access(sPathFull, R_OK) == 0)
  {
   if (bPathElementInluded(sFileSpec, sName)) // matched pattern  FNM_CASEFOLD FNM_NOESCAPE FNM_PERIOD
   {
    bFound = TRUE;
    break;
   } // if (bPathElementInluded(sFileSpec, sName))
  } // if (S_ISREG(statbuf.st_mode))


//TO DO
  if (!bFound && *sSubDirectorySpec && S_ISDIR(statbuf.st_mode) && access(sPathFull, R_OK) == 0 &&
       strcmp(sName, "..") != 0 && strcmp(sName, ".") != 0 )
  {
   if (bPathElementInluded(sSubDirectorySpec, sName)) // matched pattern  FNM_CASEFOLD FNM_NOESCAPE FNM_PERIOD  | FNM_PERIOD |
   {
    bFound = checkSubDirectoryForMatchingFiles(sDirectoryFull, sName, sFileSpec, sSubDirectorySpec);
    if (bFound) break;
   } // if (bPathElementInluded(sSubDirectorySpec, sName))
  } // if (*sSubDirectorySpec && S_ISDIR(statbuf.st_mode))


 } // for (i = 0; i < n; i++)

 for (i = 0; i < n; i++)
 {
  free(namelist[i]);//
 } // for (i = 0; i < n; i++)

 if (namelist) free(namelist);
 return bFound;
}

// ---------------------------------------------------------------------- AC
enum LineParseResult makeSubDirectoryInIncludedDirectory(IN gchar* sSubDirectory, INOUT struct MenuEntry* pMenuEntryPending)
{
 strcpy(pMenuEntryPending->m_sTitle, sSubDirectory);  // pMenuEntryPending->m_sTitle, MAX_LINE_LENGTH, gl_sLinePostEq MAX_LINE_LENGTH
 enum LineParseResult lineParseResult = commitSubMenu(pMenuEntryPending);
 if (lineParseResult != lineParseFail) gl_uiCurDepth++;
// return commitSubMenu(pMenuEntryPending); // &menuEntry
 return lineParseResult;
}


// ---------------------------------------------------------------------- AC
enum LineParseResult onEof(INOUT struct MenuEntry* pMenuEntryPending)
// ----------------------------------------------------------------------
{
//gtk_menu_set_tearoff_state(GTK_MENU(gl_gtkWmenu[0]), TRUE);
//gtk_menu_set_title(GTK_MENU(gl_gtkWmenu[0]), "yes it is"); // makes titlebar but nothing esle shows, lots of GTK errors
//IA__gtk_window_get_height: assertion IS_WINDOW failed
//IA__gtk_window_get_width: assertion IS_WINDOW failed
//window minimal, can drag iopen,
//IA__gtk_window_get_position: assertion IS_WINDOW failed
//but menu items don;t respond to click

 if (gl_uiRecursionDepth > 1) return lineParseOk;
 if (!gl_bOkToDisplay) return lineParseOk;

 if (gl_uiCurItem <= 0)
 {
  snprintf(pMenuEntryPending->m_sErrMsg, MAX_LINE_LENGTH, "\n%s\n", "Empty menu.\nYou have no choices.\nVery zen.");
  return lineParseFail;
 }


 gtk_widget_show_all(gl_gtkWmenu[0]);

 g_signal_connect_swapped(gl_gtkWmenu[0], "deactivate", G_CALLBACK (QuitMenu), NULL);

 glong uliTime = (clock() * 1000) / CLOCKS_PER_SEC;

 if(0 == gl_nOptQuiet)
  g_print("%lu msec since programme start.\n", uliTime );
// gtk_menu_set_title(GTK_MENU(gl_gtkWmenu[0]), "yes it is"); DOES NOTHING
 while(!gtk_widget_get_visible(gl_gtkWmenu[0]))  // Keep trying until startup
 {
  usleep(50000);                           // button (or key) is released

  gtk_menu_popup(GTK_MENU (gl_gtkWmenu[0]), NULL,           // parent_menu_shell
                           NULL,                            // parent_menu_item,
                           gl_bSetMenuPos ? (GtkMenuPositionFunc) menu_position : NULL,
                           NULL,                            // user supplied data to be passed to func
                           0,                               // button
                           gtk_get_current_event_time ());  // activate_time

  gtk_main_iteration();
 }

//gtk_menu_set_tearoff_state(GTK_MENU(gl_gtkWmenu[0]), TRUE);
//gtk_menu_set_title(GTK_MENU(gl_gtkWmenu[0]), "yes it is"); // makes titlebar but nothing esle shows, lots of GTK errors
//comes up as minimal window, can;t expand, same error messages

 gtk_main();
 return lineParseOk;
}

// ---------------------------------------------------------------------- AC
//called by main
gboolean checkOptions(IN gchar* sOption, OUT funcOption* pfOptionAction)
// ----------------------------------------------------------------------
{
 guint i;
 gboolean bShort = TRUE;
 gchar* sOpt = sOption + 2;
 gchar  cOpt = tolower(*(sOption + 1));
 guint nLen = strlen(sOption);

 if (nLen == 2)
  bShort = TRUE;
 else if (nLen > 4 && strncmp(sOption, "--", 2) == 0)
  bShort = FALSE;
 else
  return FALSE;

 for (i = 0; i < sizeof(gl_commandLineOption)/sizeof(struct CommandLineOption); i++)
 {
  if ((bShort && gl_commandLineOption[i].m_cOpt ==  cOpt) ||  (!bShort && strcasecmp(gl_commandLineOption[i].m_sOpt, sOpt) ==  0))
  {
   if(pfOptionAction) // return the action function
    *pfOptionAction = gl_commandLineOption[i].m_pActionFunc;
   else // run the action function
    gl_commandLineOption[i].m_pActionFunc();
   return TRUE;
  } // for (i = 0; i < sizeof(gl_keyword)/sizeof(struct Keyword); i++)
 }
 return FALSE;
}

// ---------------------------------------------------------------------- AC
void onDashDash()
// ----------------------------------------------------------------------
{
 ; // reached end of options marker "--"
}

// ----------------------------------------------------------------------
void onGatherComments()
// ----------------------------------------------------------------------
{
 // enable comment-gathering for backward-compatibility with old scripts
 gl_bOptGatherComments = TRUE;
}

// ---------------------------------------------------------------------- AC
void onHelp()
// ----------------------------------------------------------------------
{
 g_print(gl_sHelpMsg);

 if (!isatty(fileno(stdin)))
 {
  GtkWidget* pGtkMsgDlg = gtk_message_dialog_new(
    NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
    gl_sHelpMsg);
  if (pGtkMsgDlg)
  {
   gtk_dialog_run(GTK_DIALOG(pGtkMsgDlg));
   gtk_widget_destroy(pGtkMsgDlg);
  }
 }
 exit(EXIT_SUCCESS);
}

// ----------------------------------------------------------------------
void onInfo()
// ----------------------------------------------------------------------
{
 gl_nOptInfo++; // increase message verbosity
}

// ----------------------------------------------------------------------
void onQuiet()
// ----------------------------------------------------------------------
{
 gl_nOptQuiet++;
 // 1(suppress stats)
 if (2 == gl_nOptQuiet)
  gl_bErrorsInConsoleOnly = TRUE;
}

// ---------------------------------------------------------------------- AC
void onVersion()
// ----------------------------------------------------------------------
{
 g_print("%s\n", VERSION_TEXT);
 exit(EXIT_SUCCESS);
}

// ----------------------------------------------------------------------
enum LineParseResult parseInts(IN gchar *sData, OUT guint* pInt1, OUT guint* pInt2)
// ----------------------------------------------------------------------
{
 gint n, i;

 n = strlen(sData);
 if ((n == 0) | !isdigit(sData[0]))
  return lineParseFail;

 *pInt1 = atoi(sData);
 i = 0;

 while (isdigit(sData[i])) {sData[i] = ' '; i++;}

 if (!pInt2) return lineParseOk;

  if (i == n)
   return lineParseFail;

 // Skip over first number
//Find start of the next number
 while (!isdigit(sData[i]))
 {
  i++;
  if (i == n) return lineParseFail;
 }

 *pInt2 = atoi(&sData[i]);
  while (isdigit(sData[i])) {sData[i] = ' '; i++;}

 return lineParseOk;
}    // parseInts

// vim: et ts=1 sts=1 sw=1 tw=0 fdm=syntax
