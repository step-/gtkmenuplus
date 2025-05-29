
# Project name, version number, bug report address, tar name, url.
AC_INIT([gtkmenuplus], [2.1.0], [https://github.com/step-/gtkmenuplus/issues], [gtkmenuplus], [https://github.com/step-/gtkmenuplus])

AC_CONFIG_HEADERS([autoconfig.h])

# Sanity check.
AC_CONFIG_SRCDIR([source/main.c])

# Keep AC tools in their own directory ./build-aux.
AC_CONFIG_AUX_DIR([build-aux])

# 'foreign': follow minimal GNU coding standards not to
# clobber ChangeLog, COPYING, AUTHORS, INSTALL, README etc.
# 'subdir-objects': keep .o files in their own sub-directory.
# 'no-installman': require "make install-man" to install the manual pages.
# 'silent-rules': run Makefile rules silently.
AM_INIT_AUTOMAKE([1.11 foreign subdir-objects no-installman silent-rules
	-Wall -Werror])

AC_PATH_PROG(HELP2MAN, [help2man], [false])
AC_PATH_PROG(LOWDOWN, [lowdown], [false])
AC_PATH_PROG(PANDOC, [pandoc], [false])
AC_CONFIG_MACRO_DIRS([acinclude])
CHECK_RAGEL([source/if_truth_value.c], [6.0])

# C compiler check.
AC_PROG_CC

AC_ARG_WITH([gtk],
	[AS_HELP_STRING([--with-gtk=gtk2|gtk3],[set the GTK+ version to use (default - gtk3)])],
	[case "$with_gtk" in
        gtk2|gtk3) ;;
        *) AC_MSG_ERROR([invalid gtk version specified]) ;;
     esac],
    [with_gtk=gtk3])

case "$with_gtk" in
    gtk3)
        GTK_MODULE="gtk+-3.0"
        GTK_REQUIRED="3.0.0"
	DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED -DGSEAL_ENABLE"
	AC_MSG_NOTICE([Some deprecated functions will not be disabled because gtkmenuplus])
	AC_MSG_NOTICE([relies on a menu widget deprecated since GTK version 3.10.0, see])
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

AC_ARG_ENABLE([debug-print-input],
	[AS_HELP_STRING([--enable-debug-print-input],
		[Show raw/cooked lines and launcher paths (off)])],
	[enable_debug_print_input=$enableval], [enable_debug_print_input=no])
if test x$enable_debug_print_input = xyes; then
	AC_DEFINE([DEBUG_PRINT_INPUT], [1],
	[Verbose -vvvv to print input lines and launcher paths.])
fi

AC_ARG_ENABLE([debug-show-condition],
	[AS_HELP_STRING([--enable-debug-show-condition],
		[Show internal states of conditional directives (off)])],
	[enable_debug_show_condition=$enableval],
	[enable_debug_show_condition=no])
if test x$enable_debug_show_condition = xyes; then
	AC_DEFINE([DEBUG_SHOW_CONDITION], [1],
	[Verbose -vvvv to print conditional states.])
fi

AC_ARG_ENABLE([deprecated],
	[AS_HELP_STRING([--enable-deprecated],
		[Enable deprecated GTK functions
		(for GTK-3 deprecated functions are always enabled)])],
	, [enable_deprecated=no])
if test x$enable_deprecated = xyes; then
  DISABLE_DEPRECATED=
else
  : DISABLE_DEPRECATED is set by --with-gtk above
fi
AC_SUBST(DISABLE_DEPRECATED)

AC_ARG_ENABLE([activation-log],
	[AS_HELP_STRING([--enable-activation-log],
		[Enable activation log directive group (on)])],
	[enable_activation_log=$enableval], [enable_activation_log=yes])
if test x$enable_activation_log = xyes; then
	AC_DEFINE([FEATURE_ACTIVATION_LOG], [1],
	[Define to enable the activation log directive group.])
fi
AM_CONDITIONAL([FEATURE_ACTIVATION_LOG], [test x$enable_activation_log = xyes])

AC_ARG_ENABLE([conditional],
	[AS_HELP_STRING([--enable-conditional],
		[Enable conditional directive group (on)])],
	[enable_conditional=$enableval], [enable_conditional=yes])
if test x$enable_conditional = xyes; then
	AC_DEFINE([FEATURE_CONDITIONAL], [1],
	[Define to enable the conditional directive group.])
fi
AM_CONDITIONAL([FEATURE_CONDITIONAL], [test x$enable_conditional = xyes])

AC_ARG_ENABLE([formatting],
	[AS_HELP_STRING([--enable-formatting],
		[Enable menu formatting (on)])],
	[enable_formatting=$enableval], [enable_formatting=yes])
if test x$enable_formatting = xyes; then
	AC_DEFINE([FEATURE_FORMATTING], [1],
	[Define to enable menu formatting.])
fi
AM_CONDITIONAL([FEATURE_FORMATTING], [test x$enable_formatting = xyes])

AC_ARG_ENABLE([enable-icons-always-on],
	[AS_HELP_STRING([--enable-icons-always-on],
		[Force the display of icons,
		ignoring GtkSettings:gtk-menu-images setting (off)])],
	[enable_icons_always_on=$enableval], [enable_icons_always_on=no])
if test x$enable_icons_always_on = xyes; then
	AC_DEFINE([FEATURE_ICONS_ALWAYS_ON], [1],
	[Define to ignore the GtkSettings:gtk-menu-images
	setting, which controls whether icons are displayed
	in menus. Enabling this feature calls the deprecated
	gtk_image_menu_item_set_always_show_image function,
	forcing menus to always display available icons. See
	https://stackoverflow.com/questions/8989833.])
fi

AC_ARG_ENABLE([launcher],
	[AS_HELP_STRING([--enable-launcher],
		[Enable launcher directive group (on)])],
	[enable_launcher=$enableval], [enable_launcher=yes])
if test x$enable_launcher = xyes; then
	AC_DEFINE([FEATURE_LAUNCHER], [1],
	[Define to enable the launcher directive group.])
fi
AM_CONDITIONAL([FEATURE_LAUNCHER], [test x$enable_launcher = xyes])

AC_ARG_ENABLE([parameter],
	[AS_HELP_STRING([--enable-parameter],
		[Enable menu parameters (on)])],
	[enable_parameter=$enableval], [enable_parameter=yes])
if test x$enable_parameter = xyes; then
	AC_DEFINE([FEATURE_PARAMETER], [1],
		[Define to enable menu parameters.])
fi
AM_CONDITIONAL([FEATURE_PARAMETER], [test x$enable_parameter = xyes])

AC_ARG_ENABLE([serialization],
	[AS_HELP_STRING([--enable-serialization],
		[Enable json serialization (off)])],
	[enable_serialization=$enableval], [enable_serialization=no])
if test x$enable_serialization = xyes; then
	AC_DEFINE([FEATURE_SERIALIZATION], [1],
		[Define to enable menu serialization to JSON.
		 This feature is required to be able to run
		 the automated test suite.])
fi
AM_CONDITIONAL([FEATURE_SERIALIZATION], [test x$enable_serialization = xyes])

AC_ARG_ENABLE([tooltip],
	[AS_HELP_STRING([--enable-tooltip],
		[Enable menu tooltips (on)])],
	[enable_tooltip=$enableval], [enable_tooltip=yes])
if test x$enable_tooltip = xyes; then
	AC_DEFINE([FEATURE_TOOLTIP], [1],
	[Define to enable menu tooltips.])
fi
#AM_CONDITIONAL([FEATURE_TOOLTIP], [test x$enable_tooltip = xyes])

AC_ARG_ENABLE([variable],
	[AS_HELP_STRING([--enable-variable],
		[Enable menu variables (on)])],
	[enable_variable=$enableval], [enable_variable=yes])
if test x$enable_variable = xyes; then
	AC_DEFINE([FEATURE_VARIABLE], [1],
		[Define to enable menu variables.])
fi
AM_CONDITIONAL([FEATURE_VARIABLE], [test x$enable_variable = xyes])

# Tell automake to create a Makefile.
AC_CONFIG_FILES([Makefile])

# Generate files/dirs.
AC_OUTPUT
