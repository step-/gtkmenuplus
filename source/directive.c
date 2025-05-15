/*
Copyright (C) 2016-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2

Forked from version 1.00, 2013-04-24, by Alan Campbell
Partially based on myGtkMenu, Copyright (C) 2004-2011 John Vorthman
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

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "autoconfig.h"
#include "directive.h"
#include "entry.h"
#ifdef FEATURE_FORMATTING
#include "fmtg.h"
#endif
#include "input.h"
#ifdef FEATURE_LAUNCHER
#include "launcher.h"
#endif

/*********************************************************************
*                             DIRECTIVE                              *
**********************************************************************/


#define T TRUE
#define F FALSE
#define SL(str)     (str),sizeof((str))-1
static const struct Directive gl_directives[] =
{
 /* SL(keyword,length),enable_indent_rule,niladic,type,block_check,analyze */
 {SL("item"),                 T, F, LINE_ITEM, T, enter_item},
 {SL("cmd"),                  T, F, LINE_CMD, F, a_cmd},
 {SL("icon"),                 T, F, LINE_ICON, F, a_icon},
 {SL("tooltip"),              T, F, LINE_TOOLTIP, F, a_tooltip},
 {SL("submenu"),              T, F, LINE_SUBMENU, T, enter_submenu},
 {SL("endsubmenu"),           F, T, LINE_ENDSUBMENU, T, a_endsubmenu},
 {SL("separator"),            T, T, LINE_SEPARATOR, T, a_separator},
#ifdef FEATURE_CONDITIONAL
 {SL("if"),                   F, F, LINE_IF, F, a_if},
 {SL("elseif"),               F, F, LINE_ELSEIF, F, a_elseif},
 {SL("elif"),                 F, F, LINE_ELSEIF, F, a_elseif},
 {SL("fi"),                   F, T, LINE_ENDIF, F, a_endif},
 {SL("else"),                 F, T, LINE_ELSE, F, a_else},
 {SL("endif"),                F, T, LINE_ENDIF, F, a_endif},
#endif
 {SL("configure"),            F, F, LINE_CONFIGURE, T, a_configure},
 {SL("iconsize"),             F, F, LINE_ICON_SIZE, T, a_iconsize},
 {SL("icondirectory"),        F, F, LINE_ICON_DIRECTORY, T, a_icondir},
 {SL("icondir"),              F, F, LINE_ICON_DIRECTORY, T, a_icondir},
#ifdef FEATURE_FORMATTING
 {SL("format"),               F, F, LINE_FORMAT, T, a_format},
#ifdef FEATURE_TOOLTIP
 {SL("tooltipformat"),        F, F, LINE_TOOLTIP_FORMAT, T, a_tooltipformat},
#endif
#endif
 {SL("error"),                T, F, LINE_ERROR, T, a_error},
#ifdef FEATURE_LAUNCHER
 {SL("launcher"),             T, F, LINE_LAUNCHER, T, a_launcher},
 {SL("launchersub"),          T, F, LINE_LAUNCHER_SUB, T, a_launchersub},
 {SL("launcherargs"),         F, F, LINE_LAUNCHER_ARGS, T, a_launcherargs},
 {SL("launcherdirfile"),      F, F, LINE_LAUNCHER_DIRFILE, T, a_launcherdirfile},
 {SL("launchersubmenu"),      F, F, LINE_LAUNCHER_SUBMENU, T, enter_launchersubmenu},
 {SL("launcherdirectory"),    F, F, LINE_LAUNCHER_DIR, T, a_launcherdir},
 {SL("launcherdir"),          F, F, LINE_LAUNCHER_DIR, T, a_launcherdir},
#else
 /* gracefully degrade */
 {SL("launchersubmenu"),      F, F, LINE_LAUNCHER_SUBMENU, T, enter_submenu},
#endif
#ifdef FEATURE_ACTIVATION_LOG
 {SL("activationlogfile"),    F, F, LINE_ACTIVATION_LOGFILE, T, a_activationlogfile},
 {SL("activationlogexclude"), F, F, LINE_ACTIVATION_LOGEXCLUDE, T, a_activationlogexclude},
#endif
 {SL("include"),              T, F, LINE_INCLUDE, T, enter_include},
 {SL("menupos"),              F, F, LINE_POSITION, T, a_menuposition},
 {SL("menuposition"),         F, F, LINE_POSITION, T, a_menuposition},
 {SL("onexit"),               F, F, LINE_ONEXIT, T, a_onexit},

 {SL("[end of input]"),       F, T, LINE_EOF, T, a_eof},
#ifdef FEATURE_VARIABLE
 {SL("varname ="),            F, T, LINE_SET_VARNAME, T, a_variable},
#endif
 {SL("[.~]/path"),            F, T, LINE_ABSOLUTE_PATH, T, a_absolutepath},

       /* error message sentinel LINE_BAD
       column 1 is used for error messages */
 {SL("line error"),           F, T, LINE_BAD, T, NULL},      /* generic error */
 {SL("line too long"),        F, T, LINE_BAD_LEN, T, NULL},
 {SL("expected `=`"),         F, T, LINE_BAD_NO_EQ, T, NULL},

 {SL("undefined"),            F, T, LINE_UNDEFINED, T, NULL},
 {0,0,0,0,0,0, NULL},
};
#undef SL
#undef T
#undef F

/**
directive_get_table:
*/
const struct Directive *
directive_get_table ()
{
 return gl_directives;
}

/* vim:set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
