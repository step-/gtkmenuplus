# gtkmenuplus 5 "2018-09-16" "version 1.1.8" "menu configuration file"

## NAME

Format of menu configuration files for gtkmenuplus(1).

## SYNOPSIS

    menu_configuration_file

## DESCRIPTION

Gtkmenuplus takes a `menu_configuration_file` as its primary argument, and
constructs a menu from the directives that it reads in the file.  This document
describes the format of the menu configuration file, and the set of valid
directives.

## FORMAT

### Comments

Anything on a line after the "#" character is considered a comment and is
ignored, - "#" included.

The following cases are exceptions in which "#" and the characters that follow
it till the end of the line aren't comments:

* directives `cmd=`, `if=`, `elseif=`
* variable evaluation `variable_name==`
* variable assignment of quoted strings that include #, such as quoted HTML
  colors, i.e., "#FAF090"
* valid shell syntax following the above directives, i.e.,
  `if=test ${y#prefix} = abc`

Any line whose first non-whitespace character is a "#" is ignored.

### Lines

Blank lines are ignored.

Each meaningful line in a menu configuration file contains a directive (which
may be preceded by whitespace).  The case of directives is ignored.

### Directives

Seven directives are inherited from myGtkMenu, the ancestor to gtkmenuplus.
**(M)** marks directives whose behaviour in gtkmenuplus differs from that in
myGtkMenu.

    item=item_description
    cmd=command                (M) (command may be null)
    icon=path_to_image_file    (M) (path may be null)
    separator
    submenu=submenu_description
    iconsize=size
    menupos=x y                (M)
    menuposition=x y           (synonym for menupos)

The remaining directives are new in gtkmenuplus:

    icondirectory=path_to_icon_directory

    format=format_string

    tooltip=tooltip_text
    tooltipformat=format_string

    launcher=path_to_launcher(s)
    launchersub=path_to_directory
    launcherdirectory=path_to_launcher_directory
    launcherdir=path_to_launcher_directory (synonym for launcherdirectory)
    launcherargs=arguments
    launcherdirfile=launcher_dirfile
    launchersubmenu=launcher_dirfile

    activationlogfile=logfile_path

    include=menu_configuration_file or
    include=path_to_directory[/file_glob] [directory_glob] 

    if=condition
    elseif=condition or elif=condition
    else
    endif or fi

    error=message

    /path_to/file
    ~/path_to/file

    configure=option_list
    onexit
    endsubmenu

**Notes**

For `directive=value` there may be whitespace between `directive` and "=".
Whitespace between "=" and `value` is ignored, as is trailing whitespace after
`value`.

A line of `item=` may be followed by a line of `cmd=`, `icon=` and/or
`tooltip=`, in any order.

### Variables

    variable_name=value
    variable_name==expression

Any string preceding "=", aside from the directives listed above, will be taken
to be a declaration of a variable, providing it meets the following conditions.

A `variable_name` may not be the same as any existing environmental variable.

A `variable_name` must begin with an alphabetic character.  All other
characters in a `variable_name` must be alphabetic, numeric or the underscore.

Whitespace between a `variable_name` and "=" is ignored.

Whitespace following the "=", and trailing whitespace after `value` or
`expression` is ignored.  If you want to include leading or trailing whitespace
in a value or expression, enclose it all in single or double quotes, which will
be stripped before the value is stored or the expression is evaluated.  You can
also use quotes if you want a value beginning with "=", e.g.

    myvar="  something nice "

If a `variable_name` is followed by "==", the text after the "==" is
interpreted as an expression which is passed to the shell for evaluation;
whatever ends up in stdout becomes the variable value.

The value of a `variable_name` is referenced as `$variable_name` in _any_
subsequent line of the same file as well as of included files. For instance
`$variable_name` can be all or part of a command (in a `cmd=` line); of a menu
item's `item_description` (in an `item=` line); or of the condition on `if=` or
`elseif=` lines.

If the same `variable_name` is re-assigned, including in included files, its
value is redefined.

### Parameters

Additional arguments can optionally follow `menu_configuration_file` on the
gtkmenuplus command line.  Such arguments are called _positional parameters_,
and their value can be referenced by `$1`, `$2`,... etc, in any line in the
`menu_configuration_file` (except `cmd=` lines, since `$1`, `$2`... may occur
in shell one-liners and be confused with gtkmenuplus command line parameter
references).

Referencing an unassigned (null) parameter is allowed in an evaluation context,
such as `if=`, `elseif=` or `variable_name==`, and produces the value 0
('false', 'no').

`$0` references the `menu_configuration_file` itself unless gtkmenuplus gets
its input from stdin.  Reference `$0` is invalid in included files.

### Paths

The following lines may contain a path or paths:

    cmd=command                
    icondirectory=path_to_icon_directory
    icon=path_to_image_file    
    launcherdirectory=path_to_launcher_directory
    launcher=path_to_launcher(s)
    launchersub=path_to_directory
    include=menu_configuration_file 
    include=path_to_directory 

Paths may be absolute (beginning with "/") or relative.  They may begin with
the tilde ("~"), which in all cases will be expanded into `$HOME`, as it would
be by the shell.

Relative paths may begin with "./" and/or include "../", begin with the name of
a directory or simply name a file.  With some expections noted below, such
paths will be taken to be relative to the path of the directory that contains
the menu configuration file as specified on the gtkmenuplus command line.

**Note** Unlike what the shell does, gtkmenuplus resolves relative paths from
the path of the directory that contains `$0` rather than from the current
working directory.  This can be confusing. For that reason it is recommended to
invoke gtkmenplus with the full path of the `menu_configuration_file`.  This
note applies to the remainder of this section.

**Exceptions** the following directives resolve relative paths as noted:

    icon=         directory in the last non-null icondirectory= line, if any
    launcher=     directory in the last non-null launcherdirectory= line, if any
    launchersub=  directory in the last non-null launcherdirectory= line, if any
    cmd=          assumed to be on the system's PATH.
  
The command on a `cmd=command` line in particular may contain multiple paths
requiring expansion (typically multiple arguments to the specified executable).
After expansion the entire command must be no longer than 1024 (?) characters.

## DIRECTIVES

### Item

    item=item_description

Denotes the `item_description` to show in the menu. An underscore as part of
item description indicates that the following letter is the mnemonic (the
keyboard accelerator) for the menu item.

A mnemonic can also be added via global formatting, cf. `format=`.

If you want to include an underscore in the item description but not use it to
indicate a mnemonic, use two consecutive underscores.

An `item=` line may be immediately followed by any or all of `cmd=`, `icon=`
and `tooltip=` lines, in any order.

An `item=` line marks the end of any menu item or submenu preceding it.

### Cmd

    cmd=command

Optional.  Denotes the command to run.

Must be preceded by an `item=` line, and possibly by `icon=` or `tooltip=`
lines.  It applies to the menu entry begun by the preceding `item=` line.

The command that follows `cmd=` on the line must be a valid (syntax error free)
shell command, or nothing.

`cmd=`, on its own, or an `item=` not followed by a `cmd=`, will create a
disabled menu item (possibly to use as a menu or section title).

You can use "~" to refer to your home directory, e.g. ~/bin/myScript.sh.
  
A `cmd=` line is the only kind of line in which you can't use parameters
originating on the gtkmenuplus command line, or as part of an include line,
since `$1`, `$2`... may occur in shell one-liners and be confused with
gtkmenuplus command line parameter references.  If you want to use a parameter
in a command, set a variable to the parameter e.g.
  
    myParam=$1
    
and use the variable ($myParam) in the command.   
  
Not everything that can work at a shell prompt will work in `cmd=`:  

* You can't specify more than one command on a line (using ;, && or |).
* You can't use environmental variables (e.g. $WINEPREFIX, $HOME).

However, you _can_ get the shell to do stuff like that for gtkmenuplus.  Either
you can make a small script containing the commands you need, or you can make
your command a shell invocation with `sh -c`, e.g.:

     # start two instances of freecell
     cmd=sh -l -c "( sol --freecell &) ; (sol --freecell &)"

You also can have:

     cmd=path_to_a_non_executable_file [path_to_other_non_executable_file ...]

A `non_executable_file` could for instance be a doc, html, xls or plain text
file.  `path_to_a_non_executable_file` can begin with a tilde (for the home
directory), or be a relative or absolute path.

If a `cmd=` begins with a `non_executable_file`, its MIME type is used to
determine which application will be used to execute that file (and any
`path_to_other_non_executable_files` on the same line). 

### Tooltip

    tooltip=tooltip_text

Optional. Adds a tooltip to a menu item or submenu.

Must be preceded by an `item=`, and possibly by an `icon=` and/or (if there's a
preceding `item=` line) a `cmd=` line.  It applies to the menu entry begun by
the preceding `item=` line or submenu begun by the preceding `submenu=` line.

### Icon

    icon=path_to_image_file | icon_name | NULL

Optional.  Denotes an image to show with the menu item or submenu. 
  
Must be preceded by an `item=`, or `submenu=` line, and possibly by an `icon=`
and/or (if there's a preceding `item=` line) a `cmd=` line.

It applies to the menu entry begun by the preceding `item=` line or submenu
begun by the preceding `submenu=` line.

If a menu item lacks an icon line, or has an `icon=` line with nothing
following the "=" sign, gtkmenuplus will attempt to find an icon associated
with the executable named in the menu item's `cmd=` line; or, if the `cmd=`
line specifies only a non-executable file, an attempt will be made to locate an
icon associated with the default program used to open that file.

There are situations in which gtkmenplus can't automatically determine the icon
image for an `item=` without an `icon=`. In such cases you need specify the
icon explicitly:

* any submenu
* a menu item where the command on the `cmd=command` involves `sh -c` to run
  multiple shell commands
* a menu item where `cmd=` involves a terminal emulator to run a shell command
* a menu item where `cmd=` involves gtksu, gksudo or equivalent to run a shell
  command 
* successive menu items (e.g. ones opening text files) which, based on command
  or file type would all have the same icon
* a `cmd=` consisting of a URL to something on the net or on another machine.
  If the net isn't accessible, gtkmenuplus will block while trying to get
  information about the target file type.  It might be better to use a named
icon like, .e.g., text-html or applications-internet.

If you do not want an image on your menu item, use the line `icon=NULL`, or the
method described below.

If the most recently encountered "configure=" line in the menu configuration
file included the word `noicons`, any item without an `icon=path_to_image_file`
or `icon=icon_name` line will not be assigned an image.
    
A subsequent `configure=` line containing the word `icons` will cause
gtkmenuplus to revert to its default behaviour of finding icons based on the
application or filetype specified on the `cmd=` line.

The `path_to_image_file` includes a dotted file extension and follows the rules
for paths referred to in menu configuration files (see above):

* A `path_to_image_file` can begin with a tilde, which will be expanded as in
  bash to `$HOME`.
* It can be absolute.
* Or it can be relative.  If it doesn't begin with a dot, and the most recent
  `icondirectory=path_to_icon_directory` line has a non-null
  `path_to_icon_directory`, the path is relative to that.  Otherwise it's
  relative to the path in which the configuration file was found (as specified
on the gtkmenuplus command line, unless gtkmenuplus is reading from stdin). 

The dotted file extension indicates one of the supported image types: png, svg,
xpm or gif.

Tip: To speed execution, all icon files associated with a menu configuration
file should be of the same image size.

Instead of a `path_to_image_file` you can use an `icon_name`, which  is
distinguished by not including an extension for the image type.

An `icon_name` will be recognised if icons matching it are in one of the
standard sets of icon directories (e.g. /usr/share/pixmaps/, subdirectories of
/usr/share/icons, etc); in particular the icon names listed in
freedesktop.org's Icon Naming Specification: 

http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html

### Format

    format=formatting 

    formatting=[ format_string [;|, format_string [;|, format_string... ]]]

Menu items and submenu labels following a `format` line have the given
`format_string`(s) applied, until the occurrence of the next
`format=formatting` line.  

If more than one `format_string` occurs on a `format=` line, each
`format_string` is applied in turn to successive following items or submenu
labels at the same level as the menu level in which the `format=` line occurs.
Items or submenu labels at any other level in the menu hierachy are _not_
subject to the `format_string` sequence.

If `formatting` contains only one `format_string`, that `format_string` applies
to everything following, no matter where it is in the menu hierarchy.

A `format_string` consists of a string of whitespace-separated
attribute="value" pairs, attributes and their values must be appropriate for
placement within a `<span>` tag in the Pango Text Attribute Markup Language,
see https://developer.gnome.org/pango/stable/PangoMarkupFormat.html for details
(the "convenience tags" mentioned aren't supported).

An additional non-Pango attribute="value" pair is supported, `mnemonic`, see
further down for details.

Examples:

    format= font_desc="Sans Italic 12"
    format= style="bold" underline="single"
    format= foreground="blue"  # color names see /usr/share/X11/rgb.txt
    format= weight="bold"      # also possible: "ultralight", "light", "normal",
                               # "ultrabold", "heavy", or a numeric weight
    format= size='12800'       # in 1024ths of a point, or one of 'xx-small',
                               # 'x-small', # 'small', 'medium', 'large',
                               # 'x-large', 'xx-large'
    format= color="RoyalBlue";color="DodgerBlue"  # alternate two shades

A `format=` with a null `format_string` causes all subsequent menu and submenu
items to revert to default formatting.

As well as using `format=` lines to modify some menu and submenu entries,
global changes (background color, font, etc.) can be made to menus using the
built-in "GTK theme" mechanism.

GTK2 and GTK3 differ in the way themes are defined and applied for specific
applications. For GTK2 only you can invoke gtkmenuplus as such:

    env GTK2_RC_FILES=gtk2_rc_file gtkmenuplus menu_configuration_file

Note: Since version 1.1.3 gtkmenplus unexports variable `GTK2_RC_FILES` to
avoid changing the default theme of any GTK2 application that is being
executed.

As yet another formatting method, the text of any menu item or submenu label
can be formatted by wrapping it in `<span format_string>some text</span>` tags,
e.g.

    <span color="white">some text</span>

Menu items or submenus formatted by inclusion of `<span...>...</span>` tags or
by preceding `format=` lines mustn't contain "<" or ">" characters.  Use
"`&lt;`"  or "`&gt;`" instead.

If a `format=` line is in force, that will apply to all parts of a line
containing `<span...>...</span>` tags not within those tags.

`mnemonic=value` is a semantic, non-Pango attribute="value" that modifies each
formatted item label by inserting a keyboard accelerator key mark (`_`) before
the character that is to act as accelerator.  The key is detected only while
the menu is being displayed.  Menus display mnemonic keys as underlined
characters.

`Value` can be either `"1"` or an arbitrary non-null quoted string.

* `"1"` inserts `_` before the label, unless the label already includes its own
  mnemonic.
* A quoted string inserts `_<char> ` (notice the space) before the label, also
  when then label already includes its own mnemonic. `<char>` represents a
  character extracted (sequentially with recycling) from `value` The sequence is
  recycled separately for each submenu level.

Examples:

    format = mnemonic="1"
    launchersub = /usr/share/applications

Turns the first letter of all application menu item labels into a mnemonic,
unless the label already includes its own mnemonic.

    format = mnemonic="ABC"
    submenu = England
      item = London
      item = Birmingham
      item = Liverpool
      item = Manchester
    submenu = Scotland
      item = Glasgow
      item = Edingburgh
      item = Aberdeen
      item = Inverness

expands into two submenus with the following labels

    _A England
       _A London, _B Birmingham, _C Liverpool, _A Manchester
    _B Scotland
       _A Glasgow, _B Edingburgh, _C Aberdeen, _A Inverness

The rules for applying mnemonic="value" are the same rules as for applying
global label formatting.  menmonic="value" can't be used within `<span>` tags
and with directive `tooltipformat=`.

### Tooltipformat

    tooltipformat=format_string

The text of all tooltips encountered in menu items and submenus is formatted by
the preceding `tooltipformat=format_string` line.

`format_string` is as for `format=>format_string` lines.

A null `format_string` turns off formatting for tooltips in subsequent menu
items and submenus.

### Launcher

    launcher=path_to_launcher(s)

A launcher is a freedesktop.org's `.desktop` file used to launch an
application. It usually includes a name, executable, comment (tooltip) and
icon.  System desktop files can be located in /usr/share/applications, and
/usr/local/share/applications. User's application files can be located in
~/.local/share/applications, or any other directory.

If `path_to_launcher` is the path of a .desktop file, it will be used to create
a menu entry, unless an exclusion case applies (see section _Launcher Exclusion
Cases_).

Any preceding `format=format_string` line will apply to that entry.

Any preceding `launcherargs=arguments` line will apply to that entry, that is,
the `arguments` string will be appended to the command entry for the shell to
execute. Quote `arguments` as needed.

If `path_to_launcher(s)` is a directory path (dirpath), it will be scanned for
.desktop files, which will all be used to create successive menu entries.

Any preceding `launcherdirfile=launcher_dirfile` line will apply to the menu
entry of each scanned dirpath.

`path_to_launcher(s)` can also be a colon-separated list of paths. In this case
a single `launcher=` line effectively expands to multiple
`launcher=member_path` lines, where `member_path` represents each successive
member of `path_to_launcher(s)`.  Expansion stops at the end of the list if
`configure=nolauncherlistfirst` is enabled (by default it is). If
`configure=launcherlistfirst` is enabled, expansion stops after the first
successful file hit in the list.

Note that each unsuccessful expansion is likely to produce a "File not found"
error message, which in turn will display an error box. To prevent such error
box from appearing use `configure=errorconsoleonly`.

`path_to_launcher(s)` follows the rules for paths referred to in menu
configuration files (see above):

* It can begin with a tilde, which will be expanded as in bash to $HOME.
* It can be absolute.
* Or it can be relative.  If `path_to_launcher(s)` doesn't begin with a dot,
  and the most recent `launcherdirectory=path_to_launcher_directory` line has a
  non-null `path_to_launcher_directory`, it's relative to that.
* Otherwise a relative `path_to_launcher(s)` is relative to the path in which
  the configuration file was found (as specified on the gtkmenuplus command
  line, unless gtkmenuplus is reading from stdin).

If you want to refer to all the .desktop files in the directory specified by
`launcherdirectory=` use

    launcher=.

or

    launcher=*

### Launcher Exclusion Cases

A .desktop file is displayed in the menu unless one or more of the following
exclusion cases apply:

* The file is a regular file and its name doesn't end with ".desktop", i.e.,
  /path/MyEditor.desktop is included; /path/MyEditor is exluded.
* The file is a link and the name of its ultimate target doesn't end with
  ".desktop", i.e.,

    /path/MyEditor -> /path/a -> /path/b/geany.desktop   # included
    /path/MyEditor -> /path/edit_app                     # excluded

* The file includes entry "NoDisplay=true" and `configure=launchernodisplay` is
  enabled (by default it is).
* The file includes a "Categories=List" entry and List isn't empty, and an
  applicable `launcherdirfile=` `Categories=` entry excludes List.
* The file doesn't include a "Category=List" entry or List is empty, and
  `configure=launchernullcategory` is disabled (by default it's enabled), and a
  "Category=" list of an applicable `launcherdirfile=` `dirfile` doesn't
  include special category "NULL" (verbatim).
* The "Category=" entries of the .desktop file and of an applicable
  `launcherdir=` `dirfile` are defined, and the intersection between their
  list values is empty. Note that null list elements, such as the null item
  found between two semicolons in e.g. "Desktop;;System", don't count towards
  finding an intersection.

### Launchersub

    launchersub=path_to_directory

It is a recursive version of `launcher=`. It displays all the .desktop files
that it can find in `path_to_directory` and in the subdirectories under it.
Menu entries are created in nested submenus according to the subdirectory
level. More information follows further down in this section.

`path_to_directory` can also be a colon-separated list of paths. In this case a
single `launchersub=` line effectively expands into multiple
`launchersub=member_path` lines, where `member_path` represents each successive
member of `path_to_directory`.  Expansion stops at the end of the list if
`configure=nolauncherlistfirst` is enabled (by default it is). If
`configure=launcherlistfirst` is enabled, expansion stops after the first
successful recursive directory hit in the list.

Note that each unsuccessful expansion is likely to produce a "File not found"
error message, which in turn will display an error box. To prevent such error
box from appearing use `configure=errorconsoleonly`.

Rules for relative paths, the directives `launcherdirfile=` and `launcherargs=`,
and _Launcher Exclusion Cases_ all apply to `launchersub=` as they do to
`launcher=`. Each topic is explained elsewhere in this document.

When `launchersub=dirpath` is encounted submenus are created automatically for
`dirpath` and each scanned subdirectory.

Up to 5 menu levels are automatically nested (see `MAX_SUBMENU_DEPTH`).

By default the submenu label is the name of the subdirectory that includes its
.desktop files, and the submenu icon is undefined. To specify different values
and other properties use directive `launcherdirfile=`.

If the maximum allowed submenu depth is exceeded, `launchersub=dirpath` reports
a warning and displays the menu. Contrast that with the `submenu=` directive,
which exits with a fatal error if submenu depth is exceeded.

By default subdirectory scanning depth is set to fill at most 5 submenu levels.
If launcher files exist in lower subdirectories they will be ignored without
warnings.

For menu testing purposes you can force printing warnings by telling
gtkmenuplus to scan for launcher files at deeper levels. Then if such files
exist and they can't be displayed within the `MAX_SUBMENU_DEPTH` hard limit, a
warning message is printed to the console. To increase the scan depth set
environment variable `GTKMENUPLUS_SCAN_DEPTH=5` or higher.

Item formatting for the items in `dirpath` of `launchersub=dirpath` is set by
the most recent `format=` and `tooltipformat=` directives that precede
`launchersub=dirpath`. For nested subdirectories, you can control item
formatting by specifying `format_strings` in a file named `.desktop.directory`.
See section _Format_ about `format_strings`. Several example menus are included
in directory "test" of the project repository.

### Launcherdirfile

    launcherdirfile=launcher_dirfile

After this line is encountered, properties of `dirpath` in all subsequent
`launcher=dirpath` and `launchersub=dirpath` lines are read from
`launcher_dirfile`, which stands of "launcher desktop directory file".

A `launcher_dirfile` is a .desktop file that doesn't include an "Exec=" line,
and may include lines "Type=Directory" and "Format=formatting".

It sets the menu entry label, icon, and tooltip for each scanned `dirpath`.

Formatting is applied to all contained items and cascades to subdirectories of
`dirpath`.

.desktop file entry "Categories=List", if any, is used to filter which .desktop
files to display in the menu, as explained in section _Launcher Exclusion
Cases_.

`launcherdirfile=` followed by no text clears out the `launcher_dirpath` string
for all subsequent `launcher=dirpath` lines.

There can be multiple `launcherdirfiles` lines; each one sets the
`launcher_dirfile` for all `launcher=dirpath` lines that follow, until the next
`launcherdirfile=` line.

`launcher_dirfile` follows the rules for paths referred to in menu
configuration files (see above): tilde expansion and relative paths.

An alternative method to provide settings for `launcher{sub}=dirpath` lines is
to place a hidden file named `.desktop.directory` in each subdirectory. If this
file exists, it overrides the `launcher_dirfile` specified by
`launcherdirfile=launcher_dirfile`.

Example of `launcher_dirfile`:

    # Note: This file is ignored if its dirpath is used in "launcher=dirpath".
    [Desktop Entry]
    Encoding=UTF-8
    Name=submenu label
    Comment=redirected from .desktop.directory (tooltip)
    Name[es]=localized label example
    Comment[es]=localized tooltip example
    Icon=icon_name_no_extension or full_path_to_icon_file_with_extension
    Type=Directory
    Categories=
    # Format applies to contained items, and cascades.
    Format=background="purple" etc.
    # You can also apply direct (local) formatting to Name= and Comment=
    # (label and tooltip), i.e.
    # Name=<span>background="green">submenu name</span>

### Launchersubmenu

    launchersubmenu=launcher_dirfile

`launchersubmenu=` describes a submenu as an alternative to `submenu=`.

Label, icon, and tooltip are read from `launcher_dirfile` instead of being
specified through `item=`, `icon=`, etc.  In all other aspects
`launchersubmenu` works like `submenu=`.

### Launcherargs

    launcherargs=arguments

After this line is encountered, in all subsequent `launcher{sub}=` lines, the
`arguments` string will be appended to the launcher command entry for the shell
to execute. Quote `arguments` as needed.

`launcherargs=` followed by no text clears out the arguments string for all
subsequent `launcher=` lines.

There can be multiple `launcherargs` lines; each one sets the arguments for all
`launcher{sub}=` lines that follow, until the next `launcherargs=` line.

### Launcherdir, Launcherdirectory

    launcherdirectory=path_to_launcher_directory

    launcherdir=path_to_launcher_directory

After this line is encountered, in all subsequent
`launcher=path_to_launcher(s)` lines, if `path_to_launcher(s)`  doesn't begin
with a tilde or forward slash, it's assumed to be relative to
`path_to_launcher_directory`.

`path_to_launcher_directory` follows the rules for paths referred to in menu
configuration files (see above). 

If `path_to_launcher_directory` doesn't begin with a tilde or forward slash,
it's assumed to be relative to the path in which the configuration file was
found (as specified on command line).

`launcherdirectory=` followed by no text reverts the base path for icons to the
path in which the configuration file was found (as specified on command line).

There can be multiple `launcherdirectory` lines; each one sets the base
directory for all `launcher=` that follow, until the next `launcherdirectory=`
line.

### Activationlogfile

    activationlogfile=logfile_path

After this line is encountered and `logfile_path` specifies a valid file path,
three things happen:

1. File `logfile_path` is created if it doesn't exist.
2. All parsed menu items and launchers encountered after this line and before
   an `activationlogfile=` (null path) line are flagged as "loggable".
3. Activating a "loggable" entry writes its attributes (`item=`, `cmd=`,
   `icon=`, `tooltip=` or, for launchers, "Name=", "Exec=", "Icon=",
   "Comment=") to the log file `logfile_path`.

The log file is formatted as a gtkmenuplus `menu_configuration_file` and can be
included in other menu configuration files with `include=logfile_path`.

If `logfile_path` doesn't begin with a tilde or forward slash, it's assumed to
be relative to the path in which the configuration file was found (as specified
on command line).
  
Generally speaking the log file shouldn't be edited, although some changes are
allowed within the limits explained in the project repository (see git commit
message 8bd8abf, which documents log file format and application development
policies).

### Include

First form:

    include=menu_configuration_file [parameter1 [parameter2 ...]]

Second form (explained further down):

    include=path_to_directory[/file_glob] [directory_glob] 

The first form inserts the contents of a `menu_configuration_file` into the one
in which the line occurs, at the point at which it occurs.

`menu_configuration_file` follows the rules for paths referred to in menu
configuration files (see above). 

If you want the contents of a `menu_configuration_file` to appear in a submenu,
indent the `include=` line as well as all the lines of the
`menu_configuration_file` just as you would if the contents of the file were
found in the including file.

Be careful not to include recursively, directly or indirectly, a
`menu_configuration_file` in itself.

Parameters can be referred to as `$1`, `$2`, etc. anywhere in the included
`menu_configuration_file`.  See section _Parameter references_ above for more
detail.

The following rules apply as the included `menu_configuration_file` is
processed:

Any paths (see section _Paths_ above) beginning with a dot are taken to be
relative to the directory in which the included file lives; this will of course
change nothing if the including and included file are in the same directory.

If `icondirectory=path_to_icon_directory` and/or
`launcherdirectory=path_to_launcher_directory` directives are in force in the
including file, the `path_to_icon_directory` or `path_to_launcher_directory`
remain in force within the included file.

If `icondirectory=path_to_icon_directory` and/or
`launcherdirectory=path_to_launcher_directory` lines are encountered in an
included file, the `path_to_icon_directory` or `path_to_launcher_directory`
remain in force only within the included file; they revert to the values set in
the including file once the included file is processed.

If the most recently encountered `configure=` line in the menu configuration
file included the word `formattinglocal`, the effects of any `format=` or
`tooltipformat=` lines that occur within the included menu configuration file
will persist only until the end of that included file.  Formatting then reverts
to that specified by the last encountered `format=` and `tooltipformat=` lines
in the including file.

This behaviour can be turned off with a `configure=` line containing the word
`formattinglocal`.

Second form:

    include=path_to_directory[/file_glob] [directory_glob] 

`path_to_directory` follows the rules for paths referred to in menu
configuration files.

The second form inserts a series of menu entries, one per file, including only
those files to which the user has read access matching the `file_glob`
specified (e.g. `*.txt`, `d?t*`, `[a-f]*.txt`).  

(??) Extended globbing patterns can be used: see

http://www.linuxjournal.com/content/bash-extended-globbing

The generated menu item name will be the file name; if chosen the command
executed will be the full path to the file.

There is no recursion into subdirectories under `path_to_directory` unless
there's a `directory_glob`.  If one exists it's applied only to subdirectories
within `path_to_directory`, not to the matching of subdirectories further down
the directory tree.

Only subdirectories containing a file matching `file_glob` appear in the
generated menu.  Subdirectories to which the user doesn't have read access are
ignored.

The second form may be immediately followed by any or all of `icon=`,
`tooltip=` and `cmd=` lines, in any order.  If it is, the icon and tooltip will
be applied to each of the menu entries created; if there's a command, it will
be prepended to the path associated with the chosen item in the menu generated
by the `include=` line.

### Absolute Path

    /path_to/file, ~/path_to/file

A line in a menu configuration file can be an absolute path to a file,
beginning with a forward slash or tilde.  No directive is expected or required,
nor is it to be followed by `icon=`, `tooltip=` or `cmd=` lines.  

By default, menu items generated from such lines will display the file name
prefixed by its immediately containing subdirectory.

Each generated item's tooltip will display the full path to the file, as
provided in the menu configuration file, before tilde expansion.

If a previously encountered `configure=` line includes `abspathparts n`, the
lowest n elements of the path (the filename counts as one element) will be
displayed.  If `n` is zero, the whole path will be displayed.

The most likely use of such lines in a menu configuration file is to make it
possible to generate a configuration file on the fly and pipe it into
gtkmenuplus, with e.g. something like:

    { echo "configure abspathparts 3" ; find ~ -name *.conf } | gtkmenuplus -

### Submenu

    submenu=submenu_description

It denotes a `submenu_description` to show in the menu listing. See also
`launchersubmenu=`.

It may be followed by `icon=` and/or `tooltip=` lines, which, if they are to
relate to a given `submenu=`, must precede lines with any other directive except
`if=`, `elseif=`, `else` or `endif`.

By default, (but see `configure=endsubmenu`, below):

* The `icon=` and/or `tooltip=` must be indented using the tab character.  They
  must be indented by one more tabs than the `submenu=` line, as must all menu
entries in the submenu.
* The first line that is not indented with the same number of tabs signals the
  end of this submenu.
* The indentation of lines with directives like `iconsize=`, `menupos=`,
  `icondirectory=`, `format=`, `tooltipformat=`, `if=`, etc, don't make up part
  of the definition of a menu item or submenu definition, and therefore is
  ignored and has no effect on when a submenu ends.

Submenus can be nested up to a maximum of 5 levels. Changing this limit
requires recompiling the source code: look for and change the value of
`MAX_SUBMENU_DEPTH`.

A `submenu=` line marks the end of any menu item or submenu that precede it.

### Configure

    configure= keywords

Any of the keywords `endsubmenu`, `noendsubmenu`, `icons`, `noicons`,
`formattinglocal`, `noformattinglocal`, `launchernodisplay`,
`nolaunchernodisplay`, `launchernullcategory`, `nolaunchernullcategory`,
`launcherlistfirst`, `nolauncherlistfirst`, `errormsgbox` , `noerrormsgbox`,
`abspathparts`, `menuposition`, and `iconsize` can occur on this line.  

`abspathparts` and `iconsize` must be immediately followed by whitespace, then
an integer; `menuposition` must be followed by whitespace, then two
whitespace-separated integers.

For the effects of `endsubmenu`/`noendsubmenu`, see the `endsubmenu` line.

For the effects of `icons`/`noicons`, see the `icon=` line.

For the effects of `formattinglocal`/`noformattinglocal`, see the
`include=menu_configuration_file` line.

For the effects of `launchernodisplay`/`nolaunchernodisplay` and
`launchernullcategory` / `nolaunchernullcategory`, see _Launcher Exclusion
Cases_ in section _Launcher_, which also applies to the `launchersub=` line.

For the effects of `launcherlistfirst`/`nolauncherlistfirst` see the
`launcher=` and `launchersub=` lines.

For the effects of `abspathparts n`, see section _Plain File Path_.

`menuposition x y` has the same effect as the `menuposition=x y` line.  Only
one x y menu position, specified by either method, may occur in a menu
configuration file.
   
`iconsize n` has the same effect as the `iconsize=size` line, overrides the
effect of that line, and is overridden by any such following line.

By default when gtkmenuplus is _not_ launched via a CLI, fatal errors are
displayed in a message box.  `errorconsoleonly` prevents such message boxes
from appearing. `noerrorconsoleonly` reverts to the default behaviour.
  

### Onexit

    onexit=command

Shell command `command` is executed after the menu gets deactivated.  onexit is
a hook for a menu script to clean up after the menu ends.

A script can include `onexit=command` multiple times.  Only the last `command`
will be executed. Use `onexit=` to clear an established `command`.

If you need to run multiple shell commands, wrap them in a "sh -c" invocation.
Note that `command` is executed regardless of a menu entry being selected, and
it isn't synchronized with the execution/termination of an item or launcher.

### Endsubmenu

    endsubmenu

Once `endsubmenu` is encountered on a `configure=` line, indentation of lines
no longer signals which menu entries belong to which submenu.  Instead
indentation is ignored, and everything after a `submenu=` line belongs to that
submenu until the occurrence of an `endsubmenu` line.  Behaviour reverts to
default when `noendsubmenu` occurs on a subsequent `configure=` line.

Ignoring indentation means that leading whitespace can be used cosmetically,
e.g.  to mark lines within `if=`/`elseif=`/`else`/`endif` blocks (and of course
to continue to clarify what belongs to which submenu).

### Separator

    separator

It displays a line in the menu.

A separator marks the end of any menu item or submenu preceding it.

### Iconsize

    iconsize=size

An optional line that changes the dimensions of the image used for succeeding
menu items.  There can be multiple `iconsize=` lines; each one sets the icon
size for all menu entries that follow, until the next `iconsize=` line.

Size must be between 20 and 200.

Standard icons are typically 16, 24, 48 or 96 pixels square.

If no `iconsize=` is in force size will be 30 unless the gtk framework returns
a different size.

To speed execution, all icon files associated with a menu configuration file
should be of the image size specified by the most recent `iconsize=` line.

An `iconsize=` line marks the end of any menu item or submenu preceding it.

You can get the same result by putting `iconsize size` on a `configure=` line.

### Menupos, Menuposition

    menupos=x y

    menuposition=x y

An optional line to force the menu to open at a given x-y position (the program
xev can help you find coordinates - see its man page).  If no `menupos=` in
encountered, the menu is shown at the mouse cursor position.  Only one
`menupos=` is allowed per configuration file.

An `menupos=` line marks the end of any menu item or submenu preceding it.

You can get the same result by putting `menuposition x y` on a `configure=`
line.

### Icondirectory

    icondirectory=path_to_icon_directory

After this line is encountered, in all subsequent `icon=path_to_image_file`
lines, if `path_to_image_file`, doesn't begin with a tilde or forward slash
it's assumed to be relative to  `path_to_icon_directory`.

`path_to_icon_directory` follows the rules for paths referred to in menu
configuration files (see above). 

`icondirectory=` followed by no text reverts the base path for icons to the
path in which the configuration file was found (as specified on command line).

There can be multiple `icondirectory=` lines; each one sets the icon directory
for all menu entries that follow, until the next `icondirectory=` line.

An `icondirectory=` line marks the end of any menu item or submenu preceding
it.

### If, Elseif, Else, Endif, Fi

    if=condition
    elseif=condition or elif=condition
    else
    endif or fi

`condition` may be either

* A reference to an argument following the menu configuration file on the
  command line when gtkmenuplus was called, the arguments referred to by the
  reference `$1`, `$2`,... etc, e.g.

    if= $2  # referring to the third argument on the gtkmenuplus command line

* A valid command that the shell can execute that produces a value on `stdout`.

* A variable previously defined by a previous `var=` line in the menu
  configuration file.

In either case the value is expected to be an integer, "yes", "true", "no" or
"false", all case insensitive.

If the value (either the result of command execution sent to `stdout` or
received as a parameter) is non-zero, "true" or "yes", the menu entries
following the `if=` up to the following `else` or `endif` will be displayed.

If that value is zero, "false" or "no" , the menu entries following the `if=`
up to the following `else` or `endif` will _not_ be displayed, but any after a
following `else` line will be.

An `if=/[elseif=]/[else]endif` block can be embedded within another.

If `if=$n` or `elseif=$n` lines are read when there are less than `n`
parameters on the gtkmenuplus command line, all lines from the line up to the
matching `elseif` or `endif` will be processed into the menu.

If you want to test some condition requiring a call to the shell, and you want
to use the same condition in various `if=` lines in your menu configuration
file, you might be best to invoke the shell command within an argument on the
command line; that way the shell needs to be invoked only once, instead of
multiple times for multiple `if=` statements.

`if=`, `elseif=`, `else` and `endif` lines do _not_ mark the end of any menu
item or submenu preceding it.  So you can have `tooltip=` or `icon=` lines
apply to any of several `item=`s that might appear conditionally before them
e.g.


    if= [ `date +%H` -lt 18 ]; printf $?  # if past 18:00 hours
      item = evening game
      cmd = mahjongg
    else
     item = daytime game
     cmd = mines
    endif
    tooltip = the item you see here depends on the time of day
    icon=games_package.png

`if=`, `elseif=`, `else` and `endif` lines are scoped to each menu
configuration file.  If you `include=` a menu configuration file, an `endif`
line must follow an `if=` line within that file, and won't relate to a `if=`
line  in the including file.

`error=message` can be used to stop menu configuration file processing, the
need for which would generally be detected by `if=`, `elseif=`, `else` and
`endif` lines.

Sample conditions in `if=condition`, `elseif=condition` or command line
parameters:

Show menu entries following the if= line only in PM hours:

    if= ! [ `date +%p` = 'PM' ]; printf $?

On the command line:

    gtkmenuplus path_to_configuration_file "! [ `date +%p` = 'PM' ]; printf $?"

and then use `if= $1` inside the configuration file.

The date command can be used to show menu items on certain days of week, days
of the month, week of the year, etc.

Show menu entries following the `if=` line only if using a particular physical
screen:

    if= xrandr --current | grep "VGA-0 connected" | wc -l

Show menu entries following the `if=` line only if firefox is running:

    if= xdotool search --name Firefox  | wc -l

Test if a particular memory stick is mounted:

    if= ! [ -d '/media/VOL_LABEL'  ]; printf $?

Test if the partition `$HOME` resides on is more than 90% full:

    if=  df $HOME | awk 'NR==2{split($5,A,/%/);print (A[1]+0>90)}'

## BUGS

Please report defects in the _Issues_ page of the gtkmenuplus project home.

## AUTHOR
 
copyright (C) 2013 Alan Campbell, (C) 2016-2018 step

step is the current maintainer.

**Acknowledgements**

Thanks to John Vorthman for providing myGtkMenu code.

The idea of importing .desktop files was borrowed from popdown

## SEE ALSO

gtkmenuplus(1) - usage

Gtkmenuplus home page and project repository (current version):

https://github.com/step-/gtkmenuplus

Gtkmenuplus 1.0 home page (old version):

https://sites.google.com/site/entropyreduction/gtkmenuplus

myGtkMenu home page (old version):

https://sites.google.com/site/jvinla/home

Popdown home page:

http://www.manatlan.com/page/popdown
