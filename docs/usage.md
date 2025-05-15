# NAME

gtkmenuplus - Generate a GTK menu from text directives

# SYNOPSIS

**gtkmenuplus** \[*OPTIONS*\] *FILE* \[*PARAMETER*...\]  
**gtkmenuplus** \[*OPTIONS*\] *DIRECTIVE*\[*';'DIRECTIVE*...\]
\[*PARAMETER*...\]

# DESCRIPTION

Generate a GTK popup menu from text directives. Home page:
<https://github.com/step-/gtkmenuplus>. Refer to the gtkmenuplus(5)
manual page for menu scripting help.

Gtkmenuplus interprets a script that defines the appearance and behavior
of a graphical popup menu. The script consists of directives read from a
text file or standard input (stdin) or as command-line arguments.

The scripting language includes conditionals, variables, and shell
integration directives that enable the creation of dynamic menu entries
and allow for independent formatting of individual menu items.

Gtkmenuplus can directly read \`.desktop\` application files from
freedesktop.org and display folder hierarchies as a menu with just one
directive.

# OPTIONS

Use "-" as the FILE name to read from stdin.

**-d**, **--delimiter** C  
Use character C as the DIRECTIVE delimiter.

**-f**, **--force**  
Show the menu even if another instance is showing. Normally gtkmenuplus
prevents two menus from popping up simultaneously to avoid keyboard and
mouse interference. Only the first menu is allowed showing unless this
option is used.

**-h**, **--help**  
Display this help text and exit. The text is printed to stdout and, if
stdin is closed, also shown in a dialog.

**-j**, **--json**  
Dump the menu as a JSON object and exit.

**-L**, **--license**  
Display license information and exit.

**-q**, **--quiet**  
Repeat **-q** to tweak verbose messages.

· **-q** hides the source lines that would be printed after a verbose
message

· **-qq** disables message dialogs, like *configure=errorconsoleonly*

· **-qqq** hides all verbose messages except for fatal errors.

**-v**, **--verbose**  
Repeat **-v** to increase verbosity. Message verbosity levels:

· **-v** '\[0\] ok'

· **-vv** '\[1\] info'

· **-vvv** '\[2\] warning'.

Levels '\[3\] error' and '\[4\] fatal error' are always visible unless
tweaked by option **--quiet**.

**--version**  
Display version and exit.

**--**  
End of options marker.

# EXIT STATUS

Gtkmenuplus exits 0 on success, and non-zero otherwise.

# AUTHORS

Copyright (C) 2016-2025 step

Copyright (C) 2013 Alan Campbell

From a fork of myGtkMenu by John Vorthman (2004-2011).

# EXAMPLES

A menu item labeled "hello" that prints "hello world" to the terminal.  
**gtkmenuplus 'item=hello; tooltip="world"; icon=gtkmenuplus; cmd=echo
hello world'**

A menu list of the shell scripts in /usr/bin; select to edit the script.  
**gtkmenuplus 'include=/usr/bin/\*.sh; cmd=leadpad; icon=NULL'**

A menu list comprising the system applications; select an application to run it.  
**gtkmenuplus launchersub=/usr/share/applications**

# REPORTING BUGS

Please open a ticket at
<https://github.com/step-/gtkmenuplus/issues>.

To troubleshoot your menu script, run gtkmenuplus from a shell prompt in
a terminal window. Gtkmenuplus outputs diagnostic messages when it runs
in a terminal.

# SEE ALSO

gtkmenuplus(5) Menu Scripting Guide:
<https://github.com/step-/gtkmenuplus/blob/master/docs/scripting_guide.md>.

Project Wiki: <https://github.com/step-/gtkmenuplus/wiki>.
