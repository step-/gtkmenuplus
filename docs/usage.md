# gtkmenuplus 1 "WIP 2017-11-26" "version 1.1.8" usage

## NAME

gtkmenuplus - display GTK menus from text files

## SYNOPSIS

    gtkmenuplus [options] [menu_configuration_file] [parameter ...]

    gtkmenuplus [options] directive[;directive...] [parameter ...]

## DESCRIPTION

Gtkmenuplus reads a menu description from configuration directives in a text
file or stdin, and displays the resulting graphical menu. It can be used to
open documents and run applications and scripts.  Menus can be nested. Menu
items comprising label, icon and tooltip, can be formatted each one
independently.

Gtkmenuplus has its own, simple scripting language, with conditionals,
variables, and shell integration, which allow creating dynamic menu entries.

It can read freedesktop.org's .desktop application files, automatically
scanning and displaying entire folder hierarchies with a single command.

Gtkmenuplus is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by the
Free Software Foundation. Gtkmenuplus comes with ABSOLUTELY NO WARRANTY.

See the file COPYING distributed with gtkmenuplus source code for the complete
license.

Gtkmenuplus was forked from myGtkMenu.

## OPTIONS

-c, --gather-comments

> Try specifying `-c` if you are having compatibility issues with a menu
description file designed for a very old version of gtkmenuplus or for
myGtkMenu. Option `-c` disables support for very long comment lines.

-i, --info

> Print more informational messages to `stdout` and `stderr`. Repeat `-i` to
increase verbosity.

-h, --help

> Print a help message to `stdout` or to a graphical window and exit.

-q, --quiet

> Do not print statistics and informational messages to `stdout`.

-v, --version

> Print version string to `stdout` and exit.

--

> Stop recognizing options. All words after `--` are regarded as
`menu_configuration file` and `parameter`s or as `configuration_line`s.

## ARGUMENTS

Without arguments, gtkmenuplus will attempt to read file `test_menu.txt` found
in the same directory as gtkmenuplus.  Otherwise the following are the possible
arguments and their descriptions.

    configuration_file
    -
    directive[;directive...]
    parameter [parameter ...]

The `configuration_file` is a text file that describes the menu according to
the format and `directive`s specified in gtkmenuplus(5).

Use `-` to read the menu description from `stdin`. In this case, any relative
path references are taken as relative to `$HOME`.

Or you can pass configuration directives directly on the command line. Put
semicolons between configuration directives and wrap the entire string with
single quotes.

You can invoke gtkmenuplus with additional `parameter`s.  If you do, their
values can be referred to in any line of your menu configuration file as `$1`
for the first parameter beyond `menu_configuration_file`, `$2` for the second,
and so on. `$0` refers to the path to `menu_configuration_file`.

If `$` isn't followed by an integer or a variable name it's treated as literal
text.

If you provide a parameter but never reference it in the menu configuration
file, gtkmenuplus will print a warning to the terminal window.

## EXAMPLES

    gtkmenuplus 'include=~/bin/*.sh ; cmd=geany; icon=NULL'        

    gtkmenuplus $'submenu=var/run;\tinclude=/var/run/* *'

    gtkmenuplus launchersub=/usr/share/applications

Try appending

    ;/usr/local/share/applications;~/.local/share/applications

to the last command if those directories are populated in your system.

For more examples see gtkmenuplus(5) and visit the gtkmenuplus project home
page (`->`SEE ALSO).

## EXIT STATUS

Gtkmenuplus exits 0 on success, and >0 if an error occurs.

## BUGS

Please report defects in the _Issues_ page of the gtkmenuplus project home.

To troubleshoot your menu configuration files run gtkmenuplus from the shell
prompt.  When in doubt, enter the complete pathname of the files:

    /path/to/gtkmenuplus /path/to/menu_configuration_file

At the shell prompt gtkmenuplus will produce status and error messages,
including reports of errors or problems with the configuration file.

## AUTHORS
 
copyright (C) 2013 Alan Campbell, (C) 2016-2018 step

step is the current maintainer.

**Acknowledgements**

Thanks to John Vorthman for providing myGtkMenu code.

The idea of importing .desktop files was borrowed from popdown.

## SEE ALSO

gtkmenuplus(5) - menu configuration file description

Gtkmenuplus project repository:

https://github.com/step-/gtkmenuplus

myGtkMenu home page:

https://sites.google.com/site/jvinla/home

Popdown home page:

http://www.manatlan.com/page/popdown

