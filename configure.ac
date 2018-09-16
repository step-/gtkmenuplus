# Project name, version number, bug report address, tar name, url
AC_INIT([gtkmenuplus], [1.1.8], [https://www.github.com/step-/gtkmenuplus/issues/], [gtkmenuplus], [https://www.github.com/step-/gtkmenuplus/])

# Write -D defines to a header file
AC_CONFIG_HEADERS([config.h])

# Unique file identifying we're in the right directory - safety check in case
# user overwrote --srcdir
AC_CONFIG_SRCDIR([gtkmenuplus.c])

# Store the auxiliary build tools (e.g., install-sh, config.sub, config.guess)
# in ./build-aux
AC_CONFIG_AUX_DIR([build-aux])

# Init automake, and specify this project doesn't follow GNU coding standards
# hence no ChangeLog, COPYING, AUTHORS, INSTALL, README etc. files.
AM_INIT_AUTOMAKE([1.11 foreign -Wall -Werror])

# Check for C compiler
AC_PROG_CC

AC_ARG_WITH([gtk],
	[AS_HELP_STRING([--with-gtk=gtk2|gtk3],[set the GTK+ version to use (default - gtk2)])],
	[case "$with_gtk" in
        gtk2|gtk3) ;;
        *) AC_MSG_ERROR([invalid gtk version specified]) ;;
     esac],
    [with_gtk=gtk2])

case "$with_gtk" in
    gtk3) 
        GTK_MODULE="gtk+-3.0"
        GTK_REQUIRED="3.0.0"
	DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED -DGSEAL_ENABLE"
	AC_MSG_NOTICE([Deprecated functions will be only partially disabled because gtkmenuplus])
	AC_MSG_NOTICE([relies on a menu widget deprecated since GTK3 version 3.10.0, see])
	AC_MSG_NOTICE([https://developer.gnome.org/gtk3/stable/GtkImageMenuItem.html])
        ;;
    gtk2)
        GTK_MODULE="gtk+-2.0"
        GTK_REQUIRED="2.24.0"
	DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE"
        ;;
esac
PKG_CHECK_MODULES([GTK], [$GTK_MODULE >= $GTK_REQUIRED])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

AC_ARG_ENABLE([debug-if],
	[AS_HELP_STRING([--enable-debug-if],
		[Enable debugging output for gtkmenuplus IF command])],
	[enable_debug_if=$enableval], [enable_debug_if=no])
if test x$enable_debug_if = xno; then
    AC_DEFINE([_GTKMENUPLUS_NO_DEBUG_IF_], [1], [Define this to disable IF command debugging info])
fi

AC_ARG_ENABLE([deprecated],
	[AC_HELP_STRING([--enable-deprecated],
		[Enable deprecated GTK functions (for GTK3 deprecated functions are always enabled)])], 
	, [enable_deprecated=no])
if test x$enable_deprecated = xyes; then
  DISABLE_DEPRECATED=
else
  : DISABLE_DEPRECATED is set by --with-gtk above
fi
AC_SUBST(DISABLE_DEPRECATED)

AC_ARG_ENABLE([activation-log],
	[AS_HELP_STRING([--disable-activation-log],
			[Disable gtkmenuplus activation log feature])],
	[enable_activation_log=$enableval], [enable_activation_log=yes])
if test x$enable_activation_log = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_ACTIVATION_LOG_], [1], [Define this to disable the activation log feature])
fi

AC_ARG_ENABLE([always-show-icons],
	[AS_HELP_STRING([--disable-always-show-icons],
			[Disable gtkmenuplus always showing menu icons regardless of user's configuration])],
	[enable_always_show_icons=$enableval], [enable_always_show_icons=yes])
if test x$enable_always_show_icons = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_ALWAYS_SHOW_ICONS_], [1], [Define this to keep gtkmenuplus from always showing menu icons regardless of user's configuration])
fi

AC_ARG_ENABLE([cache],
	[AS_HELP_STRING([--disable-cache],
			[Disable gtkmenuplus cache feature])],
	[enable_cache=$enableval], [enable_cache=yes])
if test x$enable_cache = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_CACHE_], [1], [Define this to disable the cache feature and get a smaller but slower binary])
fi
AM_CONDITIONAL([FEATURE_CACHE], [test x$enable_cache = xyes])

AC_ARG_ENABLE([launchers],
	[AS_HELP_STRING([--disable-launchers],
			[Disable gtkmenuplus LAUNCHERS command])],
	[enable_launchers=$enableval], [enable_launchers=yes])
if test x$enable_launchers = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_LAUNCHERS_], [1], [Define this to disable the LAUNCHERS command and related features])
fi
AM_CONDITIONAL([FEATURE_LAUNCHERS], [test x$enable_launchers = xyes])

AC_ARG_ENABLE([format],
	[AS_HELP_STRING([--disable-format],
			[Disable gtkmenuplus FORMAT command])],
	[enable_format=$enableval], [enable_format=yes])
if test x$enable_format = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_FORMAT_], [1], [Define this to disable menu formatting commands])
fi

AC_ARG_ENABLE([tooltips],
	[AS_HELP_STRING([--disable-tooltips],
			[Disable gtkmenuplus TOOLTIP command])],
	[enable_tooltips=$enableval], [enable_tooltips=yes])
if test x$enable_tooltips = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_TOOLTIPS_], [1], [Define this to disable the TOOLTIP command])
fi

AC_ARG_ENABLE([if],
	[AS_HELP_STRING([--disable-if],
			[Disable gtkmenuplus IF command])],
	[enable_if=$enableval], [enable_if=yes])
if test x$enable_if = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_IF_], [1], [Define this to disable the IF command])
fi

AC_ARG_ENABLE([params],
	[AS_HELP_STRING([--disable-params],
			[Disable gtkmenuplus menu parameters])],
	[enable_params=$enableval], [enable_params=yes])
if test x$enable_params = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_PARAMS_], [1], [Define this to disable the menu parameters feature])
fi

AC_ARG_ENABLE([variables],
	[AS_HELP_STRING([--disable-variables],
			[Disable gtkmenuplus menu variables])],
	[enable_variables=$enableval], [enable_variables=yes])
if test x$enable_variables = xno; then
  AC_DEFINE([_GTKMENUPLUS_NO_VARIABLES_], [1], [Define this to disable variables in menu definitions])
fi

# Tell automake to create a Makefile
AC_CONFIG_FILES([Makefile])

# Generate files/dirs
AC_OUTPUT
