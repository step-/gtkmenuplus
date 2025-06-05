
## NAME

Gtkmenuplus Scripting Guide

## SYNOPSIS

    /path/to/script_file

## DESCRIPTION

Gtkmenuplus interprets a script that defines the appearance and behavior
of a graphical popup menu. The script consists of directives read
from a text file, standard input (stdin), or command-line arguments.

This document describes the syntax and semantics of the scripting language.
For command invocation, refer to the gtkmenuplus(1) manual page.

## SCRIPT FORMAT

Non-empty lines in the script file are classified as follows:

  * Comment
  * Directive
  * Assignment
  * Absolute pathname
  * Relative pathname
  * URI

Empty lines, lines that consist entirely of white space, and comment lines are
ignored.

### Comments

The text from the first '#' to the end of a line is treated as a comment
and ignored. If the remaining text consists entirely of white space, the
entire line is ignored.

* **Exceptions**:

  * A backslash "\\\\" escapes '#'.

  * Variable assignment with quoted values (e.g., `color_ = "#FAF090"`).

  * _Literal_ text in `cmd=`, `if=` and `elseif=` directives.

  * _Literal_ text in evaluated variable assignment, `variable_name==`.

Examples:

```gtkmenuplus
    item = print 0
    cmd = sh -c "echo $#"

    command_ = 'sh -c "echo $#"'
    item = print 0
    cmd = $command_
```

### Directives

A directive consists of a reserved keyword optionally followed by a simple
or prefixed equal sign to separate optional arguments. The keyword may
be combined with an integer index, which is wrapped by square brackets.

```gtkmenuplus
    keyword         = arguments         # Most directives fall here
    keyword        ?= arguments
    keyword[index]  = arguments
    keyword
    keyword         =
    keyword[index]  =
```

  * White space around the syntactic units is optional.

  * Keywords are reserved words that ignore letter
    case and consist only of alphanumeric characters.

  * The arguments can be empty, and their value may undergo built-in
    textual transformations before the directive will consume them.

#### Overview of supported directives grouped by usage

See § [ALL DIRECTIVES] for the full description of each directive.

General configuration:

```gtkmenuplus
    configure         = option_list
```

Creating a menu item that can run a command:

```gtkmenuplus
    item              = label
    cmd               = command
    icon              = image_file_path
    tooltip           = tooltip_text
```

Creating menu items from pathnames (`=` can be omitted in some cases):

```gtkmenuplus
    = /path/to/file
    = ~/path/to/file
    = ./path/to/file
    =   path/to/file
```

Creating menu items from URIs (`=` can be omitted in some cases):

```gtkmenuplus
    = file:///path/to/file
    = http://www.example.com         (other protocols depend on the system)
```

Creating a submenu that can contain menu items and other submenus:

```gtkmenuplus
    configure endsubmenu
    submenu           = label
    icon              = image_file_path
    tooltip           = tooltip_text

    separator

    endsubmenu
```

Changing appearance:

```gtkmenuplus
    format            = format_section
    tooltipformat     = format_section
    iconsize          = size

    menuposition      = x y
```

Retrieving resources

```gtkmenuplus
    icondirectory     = icon_directory_path
```

Creating menus from [Launcher file]s:

```gtkmenuplus
    launcher          = launcher_file ...
    launcher          = directory_path
    launchersub       = directory_path ...
    launcherdirectory = directory_path
    launcherdir       = directory_path
    launcherargs      = arguments
    launcherdirfile   = launcher_dirfile
    launchersubmenu   = launcher_file
    launchersubmenu   = launcher_dirfile
```

Organizing menu scripts:

```gtkmenuplus
    include           = script_file [paramenter ...]
    include           = directory_path[/file_glob] [directory_glob]
    activationlogfile = logfile_path
    activationlogexclude = regex_pattern
```

Conditional execution:

```gtkmenuplus
    if                = condition
    elseif            = condition
    else
    endif
```

Execution traps:

```gtkmenuplus
    error             = message
    onexit
```

### Parameters

The gtkmenuplus invocation and the [Include] directive can include
optional positional parameters following the script file. Other
directives can reference these internal parameters using a shell-like
syntax, such as `$1`, `$2`, etc. However, be wary that for the [Cmd]
and [Evaluated variable assignment] directives, the `$1` syntax refers
to the **shell's** actual positional parameter.

#### Parameter expansion

An undefined or empty parameter reference yields an empty string.

Referencing `$0` yields the following values:

  * `-` when the program is invoked with standard input redirection.

  * `$0` when invoked with command-line directives.

  * The pathname of the script file currently being read otherwise.

Parameter references can be used in other directives. First references are
expanded to form an expanded line. Then the containing directive is evaluated
on the expanded line.

Nested parameter references are not evaluated. For example, the
following shell command displays two menu items, "$2" and "hello":

```sh
    gtkmenuplus 'item = $1; item = $2' '$2' hello
```

#### Testing empty parameters

The `if ?=` directive can be used to test if a parameter is empty.
Example:

```gtkmenuplus
    if ?= $1
    item = parameter 1 is not empty
    else
    item = parameter 1 is empty
    endif
```

### Variables

```gtkmenuplus
    variable_name  = value          # Direct assignment
    variable_name == expression     # Assignment by output redirection
```

#### Direct assignment

Direct assignment associates the name of a variable with its value.

At the start of any line, a word followed by `=` that is not already
a directive keyword is treated as a `variable_name`, provided that
it begins with an underscore or an alphabetic character, followed by
alphanumeric characters or underscores.

Interior white space in quoted `value` and `expression` is preserved; other
white space on the line is ignored. To preserve all white space after `=`,
wrap the text with single or double quotes; they will be stripped before
storing the value or evaluating the expression.

If `value` starts with `=`, quote the value, e.g.,

```gtkmenuplus
    myvar_ ="=  something nice "
```

**Include a underscore in the variable name to avoid conflicts:**

  * It is an error if `variable_name` matches an existing environment
    variable; no assignment occurs and a warning is printed to the console.

  * If `variable_name` matches a directive keyword, the directive takes precedence.

Naming variables with a final underscore reduces the likelihood of matching
an environment variable and prevents clashes with future directive keywords.

#### Variable expansion

Other directives can reference the value of a variable using `$variable_name`
or `${variable_name}`. The value is made available from the first line after
the assignment. All references on the line are expanded by replacing each
reference with its literal value to create an expanded line. Then the directive
is evaluated passing the expanded value.

Referencing an undefined variable yields the original variable reference.

Variables have global scope. If the same `variable_name` is re-assigned,
its value is globally redefined after the assignment. This holds true also
for variables that are assigned by an included file.

"$" cannot be escaped. To include a literal '$', add a space it.

#### Evaluated variable assignment

The "==" assignment syntax evaluates the `expression` as a shell command, and
saves standard output (stdout) as the variable value. However, if the command
exits non-zero, or the combined lengths of variable name and value exceed 1021
bytes, saving is prevented and the literal `expression` is left unexpanded.

**The evaluated assignment does not replace parameter references:**

  * References `$1`, `$2`, etc. are passed literally to
    the underlying shell that expands the `expression`.

  * You can use a gtkmenuplus parameter in an evaluated assignment via
    an intermediate variable. For instance:

    ```gtkmenuplus
        Name_ = $1
        thisFile_ = $0
        Expression_ == set -- "Hello"; echo "$thisFile_: $1 $Name_"
        item = $myExpression_
    ```

    In the first assignment, `$1` expands to the first command-line argument.
    Instead in the evaluated assignment `$1` is a shell parameter, previously
    `set` to be "Hello", and then utilized by the `echo` command.
    Save the example to file example.txt and run `gtkmenuplus example.txt Jane`
    to display the menu item "example.txt: Hello Jane".

You can use an environment variable in an evaluated assignment this way:

```gtkmenuplus
        home1_ == printenv HOME
        home2_ == echo $HOME
```

The example sets `home1_` and `home2_` to your `$HOME` environment variable.

Utilizing relative pathnames with evaluated variables can be confusing.
Refer to the discussion in § [Directory prefix and the shell].

#### Variables are internal entities

Gtkmenuplus variables are internal program entities and not shell variables.
Consider the example below, which displays a menu item labeled "hello":

```gtkmenuplus
    v1_ =  hello
    v2_ == v1_=world; echo $v1_
    item = $v2_
```

The shell receives as _input_ the string of commands "v1_=world; echo hello".
You can insert `set -x;` after `==` to see the shell execution trace.

#### Testing for empty variables

Directive `if ?=` can be used to test if a defined internal variable is empty.

```gtkmenuplus
    if ?= $varname_
    item = varname_=$varname_
    else
    item = varname_ is empty
    endif
```

### Pathname expansion

Pathname expansion pertains to _absolute
pathnames_, which are defined as follows:

* An _absolute_ pathname starts with "/" or "~/" or "./";
  otherwise, it is considered _relative_ to a directory.

The directives that accept pathnames as arguments
undergo pathname expansion as defined below:

  1. **Leading tilde (`~`) or dot (`.`) replacement**:

     * "~" followed by nothing or "/" expands to the value of the
       `HOME` environment variable followed by "/".

     * "." followed by nothing or "./" expands to the
       "current _base directory_ value followed by "/".

     * Expansion applies when the pattern immediately follows the `=` sign,
       or a space, tab, single/double quote character, or the ':' separator in
       a list of pathnames.

     * Write "`~\/`" to obtain a literal "~/", and "`.\/`" for a literal "./".

  2. **Relative to absolute pathname conversion**:

     * The _directory prefix_ is determined based on the context:

       * For a script file specified on the command line,
         the _directory prefix_ is the _base directory_ of
         the initial script file, where the file is located.

       * For stdin ("-") or a list of command-line directives as the script,
         the _directory prefix_ is the _program's current directory_.[[bd1]]

       * Specific directives may also use a particular
         _directory prefix_ (see [Directory prefix exceptions]).

  3. **Expansion order:**

     * Pathname expansion takes place after
       [Parameter expansion] and [Variable expansion].

  4. **Character limit**:

     * If pathname expansion results in a line longer than
       1023 characters, expansion reverts and reports and error.
       The message abbreviates the _directory prefix_ as "{}".

#### Directory prefix exceptions

When expanding a relative pathname, some directives first check for a
preset directory prefix. If such a prefix is set, it is used; otherwise,
the rules outlined in bullet 2 of § [Pathname expansion] are applied.

| DIRECTIVE        | DIRECTORY PREFIX                         |
|------------------|------------------------------------------|
| [Icon]           | Most-recent [Icondirectory] argument     |
| [Launcher]       | Most-recent [Launcherdirectory] argument |
| [Launchersub]    | Most-recent [Launcherdirectory] argument |
| [Include]        | Directory of the [Include] file argument |

#### Directory prefix and the shell

The argument of a [Cmd] directive or an [Evaluated variable assignment] may
contain relative pathnames, which undergo [Pathname expansion] _before_ being
passed to the shell (`/bin/sh`). Refer to the example in § [Pathnames and
shell execution].

If you find the example confusing, it is advisable to use absolute pathnames
in your shell command argument. For instance, start pathnames with "/", or
prefix relative pathname with [Variables] whose values are the intended
directory prefixes.

## MENU ENTRY

### Menu entry properties

Gtkmenuplus displays a popup menu that consists of menu entries. Each
menu entry can either be a leaf node or a submenu container. Entries are
always visible unless an error in the script prevents their formation. A
leaf entry has four configurable properties: a label, an optional icon, an
optional tooltip, and an optional command that executes when the entry is
selected. If the command property value is empty, the entry is displayed
but cannot be selected. A submenu entry has three configurable properties:
a label, an optional icon, and an optional tooltip. Submenu entries are
always selectable, even if they do not contain any menu entries.

### Directives for creating menu entries

A leaf menu entry corresponds to one of the following directives: [Absolute
pathname], [Relative pathname], [URI], [Item], or [Launcher]. The [Launcher]
directive may also represent a block of leaf entries. In contrast, a submenu
entry corresponds to either the [Submenu] or [Launchersubmenu] directives.
The [Launchersub] directive recursively scans directories, resulting in a
combination of multiple leaf and submenu entries.

## ALL DIRECTIVES

### Item

```gtkmenuplus
    item = label
```

The Item directive begins a new menu entry and concludes the scope of
a preceding [Item] directive.

  * The `label` argument sets the label property of the menu entry.
    The label may be set to an empty string.

  * To select the entry with a key-press add a [Keyboard shortcut].

  * To obtain an indented label, use an intermediate variable as follows:

    ```gtkmenuplus
        label_ = "    indent"
        item   = $label_
    ```

You can place [Cmd], [Icon], and [Tooltip] directives in any order immediately
after the Item directive to set the properties of the menu entry. It is also
possible to conditionally set some properties using the [If] directive.

#### Keyboard shortcut

To create a keyboard shortcut (also known as an accelerator or "mnemonic") for
a menu entry, include an underscore before a label character. This will allow
the corresponding key to select the menu entry. The command associated with the
menu entry is also activated if the selection is unambiguous. Only the first
occurrence of a single underscore in the label marks the shortcut key.

Refer to the [Format mnemonic] directive to set up automatic keyboard shortcuts.

When shortcuts are enabled, GTK transforms all single
occurrences of an underscore followed by a character into an
underlined character. To insert a literal underscore, you can:

  * Disable keyboard shortcuts using the [Configure]`=nomnemonic` option.

  * Enter two consecutive underscores, which will display as a
    single literal underscore. Depending on the context, you may
    need to double the number of underscores multiple times.

### Cmd

```gtkmenuplus
    cmd = command
```

The Cmd directive applies to the preceding [Item] directive,
assigning `command` to the command property of the current menu
entry. The command executes when the menu entry is activated.

  * The [Cmd] directive may also affect multiple entries
    when it follows a [Directory include] directive.

  * An empty `command` argument, or an [Item] without a [Cmd],
    results in a dimmed menu entry that cannot be selected.

The `command` argument undergoes [Pathname expansion].

  * The argument itself may include pathnames. If these start with
    `./` or `~/`, they are also subject to [Pathname expansion], which
    has notable implications (see § [Pathnames in shell commands]).

  * The argument may also include variable
    references, which undergo [Variable expansion].

#### Pathnames in shell commands

Try out this important example:

```sh
    cd /tmp
    gtkmenuplus "item = test; cmd = dash -c 'echo Documents/. ~/Documents ./Documents'"
```

Assuming _joe_ is you login name, activating the "test"
menu item will print the following text to the terminal:

```
    Menu was deactivated.
    Run: dash -c 'echo ~/Documents ./Documents Documents/.'
    /home/joe/Documents /tmp/Documents Documents/.
```

Note that, unlike bash, the dash shell does not expand the leading
tilde; therefore gtkmenuplus did. Note also that the second output
path starts with "/tmp/", and the last relative path is unchanged.

#### Command execution

Gtkmenuplus starts the given `command`, and does not wait
for its termination before closing the menu and exiting.
Command execution follows one of the following three methods:

  * [Shell-like execution]
  * [MIME-type execution]
  * [URI execution]

#### Shell-like Execution

In this method, Gtkmenuplus utilizes a Glib function[[sx1]] to launch the
command. This function implements only a subset[[sx2]] of the syntax available
in `/bin/sh`. While this subset is sufficient for executing simple commands,
more complex commands may require advanced shell features, such as filename
globbing, which are only provided by a real shell, such as `/bin/sh`. In such
cases, the `command` should invoke the shell directly using `sh -c`.

Examples:

```gtkmenuplus
    cmd = date

    cmd = /usr/bin/env date +%F

    cmd = sh -c 'set -- *.jpg; echo first JPG image: $1'

    cmd = sh -c 'set -- `find *.png`; echo $# PNG images found; echo $*'
```

**The Command directive does not replace parameter references:**

  * References `$1`, `$2`, etc. are passed literally to
    the underlying execution method that expands the `command`.

  * You can use a gtkmenuplus parameter inside the `command` via
    an intermediate variable. For instance:

    ```gtkmenuplus
        # ex1 - parameter passed on the gtkmenuplus command line
        param_ = $1
        item   = run gtkmenuplus parameter \"$1\"
        cmd    = echo running \"$param_\" ...

        # ex2 - shell parameter passed directly to sh -c
        item   = run shell parameter "1"
        cmd    = sh -c 'echo running \"$1\"' A B
    ```

    Save the first block above to file "ex1" and the second one to "ex2".
    Run `gtkmenuplus ex1 P`, select the item; the output is "running P".
    Run `gtkmenuplus ex2 P`, select the item; the output is "running B".

#### MIME-type execution

In this method, the `command` argument is a path to a data file:

```gtkmenuplus
    cmd = data_file_path
```

Examples of data files include pictures, plain text files, word processor
documents, and more. These files can be executed as long as their MIME
type is associated with default applications[[mx1]] that can open them.

#### URI execution

In this method, the command is a URI:

```gtkmenuplus
    cmd = http://www.example.com
```

The URI is passed to the default application[[1ux]] that can open it.
In this case the application would typically be your web browser.

### Tooltip

```gtkmenuplus
    tooltip = text
```

The Tooltip directive combines with a preceding [Item] directive
assigning `text` to the tooltip property of the current menu entry.

  * The Tooltip directive may also affect multiple menu
    entries if it follows a [Directory include] directive.

### Icon

```gtkmenuplus
    icon = image_file_path | icon_name | ""
```

The Icon directive combines with a preceding [Item] directive
to assign an image to the icon property of the current menu entry.

  * The Icon directive may also affect multiple menu
    entries if it follows a [Directory include] directive.

Note that, in its default configuration, gtkmenuplus tries to assign icons to
menu entries automatically, when possible. Hence, you only need to use the Icon
directive when gtkmenuplus cannot assign a menu entry icon on its own, or when
you want to use a different icon from the one the program has chosen.

Example of an automatic icon: `gtkmenuplus 'item = a; cmd = file.pdf'`.

  * The menu will display a PDF file type icon.

Example of gtkmenuplus not being able to assign an icon:
`gtkmenuplus 'item = b; cmd = xdg-open file.pdf'`.

  * The menu will not display an icon. Add an Icon directive to the
    script, such as `icon = /path/to/pdf.png`, to fix the empty icon.

To hide an automatically-assigned icon, use an empty argument, literally
"`icon=`". For bulk hiding, refer to § [Hiding item and submenu icons].

#### Image file path vs named icon

The `image_file_path` argument undergoes [Pathname expansion] and [Directory
prefix exceptions]. The path must end with an image file extension from
the following set: ".xpm", ".png", ".svg", ".gif", ".jpg", or ".DirIcon".
Additionally, GTK must support opening the specified image type.

Instead of an image file path you can pass an icon name as the argument
of this directive. Named icons, such as "leafpad" or "org.gnome.Meld",
are looked up in the standard XDG icon directories[[ic1]]. GTK2 and GTK3
also provide stock icons, such as "gtk-ok".

#### Invalid or omitted icon argument

If the [Configure]`=icons` option is enabled, and gtkmenuplus
cannot resolve the directive argument to an actual image, the
menu entry will not display an icon. A warning will be printed
only if the verbose (`-v`) command-line option is enabled.

Passing an empty argument will hide an automatically-assigned icon.

#### Hiding item and submenu icons

While a [Configure]=`noicons` directive is active, gtkmenuplus disables
automatic icon assignment. Thus the [Item] and [Submenu] directives
will not display an icon unless an Icon directive explicitly sets one.

Automatic icons remain disabled until another
[Configure]`=icons` directive is encountered.

  * To hide [Launcher] icons refer to § [Hiding launcher icons].

  * To hide [Directory Include] icons refer to
    § [Directory Include default icon, tooltip and command].

### Format

```gtkmenuplus
    format     = formatting

    formatting = [ format_section [';' format_section ...]]
    formatting = [ format_section [',' format_section ...]]
```

The Format directive applies specific styles to the labels of
subsequent menu entries generated by all directives. These styles
remain in effect until another Format directive is encountered.

  * If only one `format_section` is specified, the formatting applies
    to all subsequent directives, regardless of their menu level.

  * If multiple `format_section`s are provided, they are applied
    cyclically to item and submenu labels at the menu level where the
    Format directive is used. Labels at other levels are not affected.

  * To reset to default styles, use an empty `formatting` argument.

#### Format section syntax

The _Pango Text Attribute Markup Language_ specification[pm1]
defines the syntax of a `format_section`.

  * Gtkmenuplus only supports the `<span>` tag and its attributes. The
    "convenience tags" mentioned in the specification are not available.

  * For Pango Markup attributes whose names are composed of multiple
    words, such as `font-desc`, use hyphens (`-`) to separate the
    components. For example, write `font-desc` instead of `font_desc`.

Syntax is validated directly by the underlying Pango Markup engine, which
reports invalid formats as GTK errors when the menu is _displayed_.

To disable markup processing altogether and display
`<span>` tags and other special characters such as `&` and
`<` literally, use the [Configure]`=nomarkup` directive.

Examples:

```gtkmenuplus
    format= font-desc="Sans Italic 12"       # it's `font-desc', not `font_desc'

    format= style="italic" underline="single"

    format= foreground="blue"  # color names see /usr/share/X11/rgb.txt

    format= weight="bold"      # also possible: "ultralight", "light", "normal",
                               # "ultrabold", "heavy", or a numeric weight

    format= size='12800'       # in 1024ths of a point, or one of 'xx-small',
                               # 'x-small', 'small', 'medium', 'large',
                               # 'x-large', 'xx-large'

    format= color="RoyalBlue";color="DodgerBlue"  # alternate two shades
```

#### Formatting extensions

In addition to accepting Pango Markup attributes, the Format directive also
accepts the following `format_section` values:

  * [Format mnemonic].

Pango Markup can also be inlined. Refer to § [Inline span tag] for details.

#### Format mnemonic

```gtkmenuplus
    format = mnemonic="value"
```

The [Format]`=mnemonic="value"` directive modifies menu entry labels inserting
an underscore (`_`) into the label to mark a [Keyboard shortcut].

  * The mnemonic formatting is invalid inside an [Inline span tag],
    and in combination with the [Tooltipformat] directive.

  * When mnemonic formatting is active, the Pango engine **does not**
    recognize attribute names that include underscores. The solution is
    to replace all underscores with hyphens within composite attribute
    names, e.g., write `font-desc` and not `font_desc`.

The mnemonic `"value"` determines where to insert an underscore:

  * `"1"` inserts the mark before the first non-blank character
    of the label, unless the label already includes an underscore.

  * A quoted string inserts `_<char>` before the label, ignoring whether the
    label already includes an underscore. `<char>` is a character sequentially
    extracted (with recycling) from the `"value"` string. Recycling restarts at
    each submenu level.

Example:

```gtkmenuplus
    format = mnemonic="1"
    launchersub = /usr/share/applications
```

If the label obtained from the launcher's "Name" property does
not include an underscore, one is inserted before the label.

Example:

```gtkmenuplus
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
```

The menu contains two submenus with the following modified labels:

```
    _A England
       _A London, _B Birmingham, _C Liverpool, _A Manchester
    _B Scotland
       _A Glasgow, _B Edingburgh, _C Aberdeen, _A Inverness
```

#### Inline span tag

If the [Configure]`=markup` option is enabled (default), the
[Item], [Submenu] and [Launchersubmenu] directives can include
Pango markup `<span>` syntax directly within the label text.

  * Start the label with `<span format_section>`, and end
    it with `</span>`. Example: `<span fgcolor="blue">the label</span>`

  * To prevent invalid markup errors, use the `&amp;`, `&lt;` and `&gt;`
    entities instead of "&", "<" and ">" for the text inside a `<span>` tag.

  * When a `format=formatting` directive is active, it applies only to
    the text outside the `<span>` tag. Inside the `<span>` the inline
    attributes apply.

#### GTK Themes

The GTK theme can affect global formatting of your menu.
GTK2 and GTK3 theme configuration and application differ vastly.

  * To temporarily change the GTK2 theme invoke gtkmenuplus like so:

    ```sh
        env GTK2_RC_FILES=gtk2_theme_file.rc gtkmenuplus script_file
    ```

  * Gtkmenuplus resets variable `GTK2_RC_FILES` before
    activating a menu item command (since version 1.1.3).

### Tooltipformat

```gtkmenuplus
    tooltipformat = format_section

    formatting    = [ format_section [';' format_section ...]]
    formatting    = [ format_section [',' format_section ...]]
```

The Tooltipformat directive applies a `formatting` to subsequent
menu entries until another Tooltipformat directive is encountered.

For details on `format_section`, refer to the [Format] directive.

  * To reset the tooltip style to default, pass an empty argument.

  * Tooltipformat affects leaf and submenu entries generated by all directives.

### Launcher

```gtkmenuplus
    launcher = launcher_file  [ ':' launcher_file  ...]
    launcher = directory_path [ ':' directory_path ...]
```

The Launcher directive appends leaf entries at the current menu level.

#### Launcher file

Some terminology is in order. The Desktop Entry specification[[la1]]
from freedesktop.org defines the structure and location[[la2]] of
desktop files, which generally describe, by means of _key-value_ pairs,
applications and how to start them.

These files are commonly named "desktop" files because they use the
`.desktop` file name extension. Gtkmenuplus also and often refers to them
as "launchers", and uses the term "property" to mean the _key-value_ pair.

The specification[[la1]] can also describe application collections, aptly
named _directories_, which are stored in "directory" files whose file name
extension is, you guessed, `.directory`. Gtkmenuplus often refers to these
files as `launcher_dirfile`s. We will see them in § [Launcherdirfile].

Launcher and `launcher_dirfile` files employ some of the same keys.
For instance, they can both contain the "Name" key. The property that
distinguishes these two files is "Type=Application" for launchers vs
"Type=Directory" for `launcher_dirfile`s.

#### Launcher paths

The Launcher directive accepts a colon-separated list of
`launcher_file`s or `directory_path`s. List elements undergo
[Pathname expansion] subject to [Directory prefix exceptions].

1. A relative path is based in the directory specified by the
   [Launcherdirectory] directive, defaulting to the base directory.

   * The name of the `launcher_file` must end with ".desktop".

   * The `launcher_file` can be a symbolic link. In this case,
     the name of its final target must end with ".desktop", while
     the link name is unrestricted.

   * The `launcher_file` can be "`*`", which expands to all
     the launcher files in the directory: `launcher = *`.

2. A `directory_path` is scanned non-recursively for desktop files,
   producing a list of file paths, each to be processed as a launcher.

   * For a recursively scanning version see the [Launchersub] directive.

   * If the Launcher argument is a list of `directory_path` elements, they
     are scanned depending on to the `launcherlistfirst` configuration option.

     If `NOlauncherlistfirst` is in effect (default), the list is traversed
     until a path can be scanned successfully; then traversing stops. Otherwise
     the list is traversed entirely, scanning each directory path in turn.

3. A `launcher_file` expanding to the location of a well-formed `.desktop` file
   results in an actual menu entry, unless an exclusion case applies (see §
   [Launcher exclusion cases], and § [Launcherdirfile] for `directory_path`).

   * If the Launcher argument is a list of `launcher_file`
     elements, each is considered for processing as a single launcher
     depending on the `launcherlistfirst` configuration option.

     If `NOlauncherlistfirst` is in effect (default), the list is traversed
     until and element matches an existing file; then traversing stops.
     Otherwise the list is traversed entirely, and each existing file
     is processed as a launcher.

4. Non-existent file paths are reported as warnings if option `--verbose` is
   enabled. Other kinds of file errors (I/O, access, etc.) trigger an error
   dialog. To prevent displaying the dialog use `configure=errorconsoleonly`.

#### Basic Launcher properties

A desktop file includes keywords that define the properties of
its associated launcher. Key to our discussion are the following
property keywords, and the corresponding directives they affect.

| PROPERTY KEYWORD | DIRECTIVE   | MENU ENTRY  |
|------------------|-------------|-------------|
| Name             | [Item]      | Label       |
| Icon             | [Icon]      | Icon        |
| Comment          | [Tooltip]   | Tooltip     |
| Exec             | [Cmd]       | Activation  |

  * The label can include a [Keyboard shortcut].

#### Multi-language Launcher

The Desktop Entry specification[[la1]] enables the creation of multi-language
launcher files files that include multiple translations for each property value.
When gtkmenuplus reads a translated `Name` or `Comment` property that matches
the current system language, it uses the translated value for the menu entry
label or tooltip.

This functionality supports the development of multi-language menus composed
entirely of launcher files. These menus can be organized into submenus using the
[Submenu] or [Launchersubmenu] directives. Note that only the latter supports
creating multi-language submenu entries.

#### Directives affecting Launcher

The following directives affect various properties of the launcher menu item.

  * [Format] and [Tooltipformat] apply formatting to the menu entries.

  * [Launcherdirfile] applies additional properties
    to launchers that derive from a `directory_path`.

  * [Launcherargs] appends a common suffix to
    the `Exec=` property of each launcher.

#### Hiding launcher icons

If a [Configure]`=nolaunchericons` option is enabled,
subsequent launcher menu entries will not display an icon.

Use the [Configure]`=launchericons` option to re-enable icon
visibility for the menu entries that follow the directive.

The `launchersubmenu=` icon is toggled like a `submenu=` icon,
Refer to § [Hiding item and submenu icons] for instructions on
toggling icon visibility for the [Launchersubmenu] directive.

#### Launcher scanner

The Launcher and [Launchersub] directives scan their directory path argument
looking for launcher files. The scanner considers hidden files and directories
and follows symbolic links according to the `ignorehidden` and `followsymlink`
[Configure] directive options. These options are evaluated for all files and
directories encountered during a scan. However, the initial arguments to the
directive are processed regardless of these options.

  * Scanning does not proceed past the maximum menu depth.[[sm1]]

  * The scanner discards empty submenus, and ignores unreadable directories.

#### Launcher exclusion cases

While the scanner looks for launcher files, it excludes a file
from the menu if one of the following exclusion cases applies:

1. The file name starts with "." (is a hidden file)
   and [Configure]`=ignorehidden` is enabled (default).

2. The file is a symbolic link and [Configure]`=followsymlink` is set to
   ignore file targets; or if file targets are allowed, the target name
   does not end with ".desktop":

    ```
        /path/MyEditor -> /path/a -> /path/b/geany.desktop   # included
        /path/MyEditor -> /path/edit_app                     # excluded
    ```

3. The file is a regular file whose name does not end with ".desktop":

    ```
        /path/MyEditor.desktop                               # included
        /path/MyEditor                                       # excluded
    ```

4. The file's "Type=" property value is not "Application".

5. The file includes property "NoDisplay=true" and
   [Configure]=`launchernodisplay` is enabled (default).

6. The file's "Categories=" list does _not_ intersect the
   "Categories=" list of an _applicable_ [Launcherdirfile]
   directive. An _intersection_ is defined as follows.

   1. Given "Categories=`ListA`" and "Categories=`ListB`", the non-empty lists
      `ListA` = "`A1`;`A2`;...;" and `ListB` = "`B1`;`B2`;...;";
      `ListA` and `ListB` intersect, if there exist two integers `i` and `j`
      such that word "`Ai`" equals word "`Bj`".

   2. White space is not allowed in either list. Empty list
      elements generated by `;;` do not affect the outcome.

   3. If either list is empty, the lists intersect when the
      [Configure]`=launchernullcategory` option is enabled (default).
      Conversely, if the option is disabled, the Launcherdirfile's list must
      include the "NULL"[[la3] literal for the lists to intersect.

   Examples:

      - "Categories=ca;cb;cc;" intersects "Categories=da;cb;" on "cb".
      - "Categories=ca;cb;" does not intersect "Categories=da;db;".
      - "Categories=ca;;cb;" intersects "Categories=cb;;" on "cb".
      - "Categories=;;cb;" does not intersect "Categories=;;ca".
      - "Categories=" and "Categories=" intersect.

More precisely, category-based exclusion takes effect when either:

  * The [Launcher] file holds a non-empty "Categories=cat1;cat2;..." list;

    - and a [Launcherdirfile] file is in effect that holds
      a non-empty "Categories=sel1;sel2;..." list;

    - and "cat1;cat2;..." and "sel1;sel2;..." do not intersect, meaning
      there exist no `i` and `j` such that "cat"`i` equals "sel"`j `.

  * Or the "Categories=" property of the [Launcher] file is omitted or empty;

    - and [Configure]=`nolaunchernullcategory` is enabled (default: disabled);

    - and a [Launcherdirfile] file is in effect whose non-empty
      "Categories=" list does not include the literal "NULL".[[la3]]

### Launchersub

```gtkmenuplus
    launchersub = directory_path [ ':' directory_path ...]
```

The Launchersub directive accepts a colon-separated list of `directory_path`s,
which undergo [Pathname expansion] subject to [Directory prefix exceptions].

1. Relative paths are based in the directory specified by the
   [Launcherdirectory] directive, defaulting to the base directory.

2. An existing `directory_path` is scanned recursively for
   desktop files, producing a list of paths. Each path is
   processed as a [Launcher file] or as a `directory_path`.

   * **Menu Organization**:

     - Files immediately under the `directory_path` fill top menu items.

     - Files at deeper levels are automatically arranged in nested
       submenus. Each subdirectory corresponds to an automatic submenu.

   * **Scanning Behavior**:

     - Scanning does not proceed past the maximum menu depth.[[sm1]]

     - Each `directory_path` in the argument list is scanned separately
       according to the `launcherlistfirst` option of the [Configure] directive.

     - If `NOlauncherlistfirst` is in effect (default), the list is traversed
       until an element provides a Launchersub sub-run without errors, at
       which point the scanner and the top Launchersub directory terminate.

     - Otherwise, the argument list is processed entirely,
       scanning all existing directory elements recursively.

     - Error Handling: Non-existent file paths trigger an error dialog;
       to disable it globally use the [Configure]`=errorconsoleonly` option.

   * **Submenu properties**:

     - While the Launchersub directive scans directories looking for
       launcher files, by default, submenu entry labels take the names
       of traversed subdirectories, and submenu entry icons are empty.
       Use the [Launcherdirfile] directive to change the label and icon.

       The [Launcherdirfile] and [Tooltipformat] directives allow changing
       the style of generated launcher and submenu entries, as well as their
       tooltips. Additionally, Launcherdirfile can filter menu entries by
       category. Refer to § [Launcherfilefile properties].

#### Directives affecting Launchersub

  * [Launcherargs] appends a suffix to each launcher's "Exec=" property.

  * [Launcherdirfile] filters entries, and applies additional
    properties and styling to launchers deriving from a `directory_path`.

  * [Tooltipformat] applies styles to the tooltip of launcher entries.

### Launcherdirfile

```gtkmenuplus
    launcherdirfile    = launcher_dirfile
    launcherdirfile[N] = launcher_dirfile
```

The keyword "launcherdirfile" stands for "launcher desktop directory file". A
`launcher_dirfile` file sets menu entry properties and exclusions filters
for launchers obtained with the [Launchersub] directive, and the [Launcher]
directive when its argument is a _directory path_.

  * An empty `launcherdirfile=` directive ends the effect of a previous
    `launcher_dirfile` on subsequent [Launcher] and [Launchersub] directives.

The `launcher_dirfile` path undergoes [Pathname expansion].

  * The file name must end with ".directory".

  * The file must be formatted in accordance with the Desktop
    entry specification,[[la1]] where `Type=Directory`.

  * A `launcher_file` provides a way to create multi-language menus consisting
    of (submenus of) launchers, as discussed in § [Multi-language Launcher].

#### Launchedirfile scoping

By default, an active `launcher_dirfile` binds its properties to
all menu levels. However, you can limit its effect by appending an
integer index to the `launcherdirfile` keyword. For example, setting
`launcherdirfile[1]=/some/file.directory` binds properties from the file
`/some/file.directory` only to menu entries at level 1 (base 0).

#### Launcherdirfile properties

The Launcherdirfile directive considers the following properties:

  - `Icon=`: Sets the icon for submenu entries.
  - `Comment=`: Sets the tooltip for submenu items.
  - `Categories=`: Defines [Launcherdirfile category filters].

The file may also include [Basic launcher properties], but these
are ignored by Launcherdirfile except for the `Name=` property,
which is significant for the [Launchersubmenu] directive.

#### Launcherdirfile category filters

If `launcher_dirfile` includes a "Categories=list" property, the
list of categories defines which launcher files to display in the
menu, as explained in § [Launcher exclusion cases], bullet 4.

  * Filters apply to all levels of the [Launcher] and [Launchersub] directives.

  * To style launcher entries, use the [Format] and [Tooltipformat] directives.

### Launchersubmenu

```gtkmenuplus
    launchersubmenu = launcher_file
    launchersubmenu = launcher_dirfile
```

The [Launchersubmenu] directive creates a submenu entry where
the label, tooltip, and icon are defined using [Basic Launcher
properties] from a `.desktop` or `.directory` file, rather than
through the Submenu, Tooltip, and Icon directives.

The primary advantage of [Launchersubmenu] over the [Submenu] directive is
its ability to create a multi-language menu (see [Multi-language Launcher]).

  * Launchersubmenu requires [Configure]=`endsubmenu` enabled,
    and [Endsubmenu] to close the submenu block.

  * The submenu can contain item- or block-making directives,
    such as [Launcher] and [Item], [Launchersub] and [Submenu].

  * The "Name=" property in the argument file sets the label of the top submenu
    entry only. Contained [Launchersub] directives will ignore that "Name=".

  * **Requirements**: [Launchersubmenu] requires the [Configure]`=endsubmenu`
    option to be enabled and [Endsubmenu] to close the submenu block.

  * **Content**: The submenu can include item- or block-making directives like
      [Launcher], [Item], [Launchersub], and [Submenu].

  * **Properties**:

    - The "Name=", "Icon=" and "Comment=" properties in the argument
      file set the label, icon and tooltip of the top submenu entry
      only. Contained [Launchersub] directives ignore the properties
      set by the Launchersubmenu file argument.

### Launchersubmenu and Launcherdirfile example

The following example demonstrates how to combine the Launchersubmenu
and [Launcherdirfile] directives to create a menu. This menu features an
entry labeled "Desktop," which includes system utilities from various
categories. On a system set to the French locale, the label and tooltip
of the top submenu entry are translated into French; translations may
also be available for some contained items. All tooltips are displayed
in blue, and labels are rendered in bold (`font-weight`).


```gtkmenuplus
    configure        = endsubmenu              # Required for submenu closure
    configure        = nolaunchernullcategory  # No launchers without categories
    tooltipformat    = fgcolor="dodgerblue"    # Style all tooltips
    format           = font-weight="800"       # Bold labels
    launcherdirfile  = Desktop.directory       # Style and filter entries
    launchersubmenu  = Desktop.directory       # Submenu label, icon and tooltip
    launcher         = /usr/share/applications # Scan the directory's top level
    endsubmenu
```

The file "Desktop.directory" in the current directory contains:

```
    [Desktop Entry]
    Type=Directory
    Icon=gtk-ok
    Name=Desktop
    Comment=Desktop utilities
    Name[fr]=Bureau
    Comment[fr]=Utilitaires de bureau
    # Select matching categories
    # (actual category names may vary across OS distributions)
    Categories=;X-Desktop;DesktopSettings;Screensaver;Accessibility;
```

### Launcher-dirfile sample file

The [Launcherdirfile] and [Launchersubmenu] directives may reuse the same
`launcher_dirfile` file. It is important to distinguish which properties
matter to each directive. For more details, refer to the description of each
directive. Below is a commented sample file containing all the standard and
custom properties that the `launcher_dirfile` can affect.

```
    [Desktop Entry]
    Type=Directory

    # In general, properties apply only to the menu levels specified by
    # the zero-based index of the launcherdirfile[index]= directive.

    # Name applies to the top entry generated by the launchersubmenu= directive,
    # and to the submenu entries generated by the launchersub= directive.
    Name=submenu entry label
    Comment=submenu entry tooltip

    # You may also directly apply Pango Markup formatting to:
    #    Name=                                (submenu item label)
    #    Comment=                             (submenu item tooltip)
    # e.g.,
    #    Name=<span>background="green">submenu label</span>

    # Translations can be included in the file, e.g. for Spanish:
    #    Name[es]=etiqueta de submenú
    #    Comment[es]=información sobre herramientas del submenú

    # Icon applies to the top entry generated by the launchersubmenu= directive,
    # and to the submenu entries generated by by the launchersub= directive.
    Icon=submenu item icon (name_no_extension OR icon_full_pathname.extension)

    # Filtering entries: The example below displays only the entries
    # for launcher files whose Categories= includes C1 or C2 or... CN.
    Categories=C1;C2;...;CN;
```

### Launcherargs

```gtkmenuplus
    launcherargs = arguments
```

After the occurrence of the Launcherargs directive, its `arguments`
string is appended to the "Exec=" properties of all subsequent menu
items arising from the [Launcher] and [Launchersub] directive.

  * Quote the `arguments` string as required for [Shell-like execution].

  * `launcherargs=` followed by no text ends appending the `arguments` string.

### Launcherdirectory

```gtkmenuplus
    launcherdirectory = launcher_directory_path
```

After the occurrence of a Launcherdirectory directive, all relative
pathnames of [Launcher file]s are rebased to the `launcher_directory_path`.

  * If `launcher_directory_path` is itself relative,
    it is rebased to the directory of the current script file.

  * Use `launcherdirectory=` followed by no text to reset to default.

### Activationlogfile

```gtkmenuplus
    activationlogfile = logfile_path
```

  1. The `logfile_path` argument undergoes [Pathname expansion],
     and the file is created empty if it does not exist.

  2. Leaf menu entries that can be activated (run, executed)
     are marked "loggable" internally.

  3. Activating a "loggable" entry writes its properties
     (or [Basic Launcher properties]) to the log file.

The log file should not be edited. It can be reused _as is_ directly
or within other script files by means of an [Include] directive.

The Activationlogfile directive has global scope throughout the
menu script; its most recent occurrence applies to all leaf items.

  * An empty `activationlogfile=` directive disables logging globally.

See also § [Activationlogexclude].

### Activationlogexclude

```gtkmenuplus
    activationlogexclude = regex_pattern
```

The `regex_pattern` argument is a POSIX Extended Regular Expression[[al1]].

The Activationlogexclude directive sets an exclusion pattern.
When a menu entry is activated and its command property matches
the pattern, the entry is not added to the activation log.

The Activationlogexclude directive has global scope throughout the
menu script; its most recent occurrence applies to all leaf items.

  * An empty `activationlogexclude=` resets the exclusion filter globally.

  * Activationlogexclude must follow an [Activationlogfile] directive.

### Include

The Include directive is actually a combination of two directives.

File Include:

```gtkmenuplus
    include = script_file [parameter ...]
```

Directory Include:

```gtkmenuplus
    include = directory_path['/'file_glob] [directory_glob]
```

#### File Include

    include = script_file [parameter ...]

The `script_file` argument undergoes [Pathname expansion]
and [Directory prefix exceptions].

The File Include directive inserts a sub-script into the current
script, reading the sub-script from the `script_file`, and
setting [Parameters] from the optional `parameter` arguments.

  * The scope of the parameters is the included file.

  * Quote `parameter` to preserve white space.

Within the included file, relative pathnames are rebased to the
directory of the included file, see § [Directory prefix exceptions].

For backwards compatibility, it is still possible to use indentation
to make the contents of a script file appear as a submenu.

  * This mechanism is **deprecated** and should not be used in new scripts.

  * Indent the [Include] directive as well as all the lines of the included
    file just as if its contents were directly inside the including file.

  * Refer to § [Indentation and submenu nesting].

Pay attention not to recursively include a script file into
itself, directly or indirectly. This will result in a
console error message, and your menu will not be displayed.

   * The maximum include depth is 10, and this limit is enforced.

Parameters can be referenced as `$1`, `$2`, etc. anywhere in the
included script file. Refer to § [Parameter references] for details.

The following rules apply while the included file is being processed:

  * If the [Icondirectory]`=path` or [Launcherdirectory]`=path`
    directives are active in the including file, their
    paths remain in effect in the included file.

  * If the included file itself contains Icondirectory or Launcherdirectory
    directives, their paths remain in effect only for the included
    file and revert to previous path values when the inclusion ends.

  * If the included file contains the [Format] or [Tooltipformat]
    directives, their scope can be restricted to the included
    file by adding the [Configure]`=formattinglocal` option.

#### Directory Include

```gtkmenuplus
    include = directory_path[/file_glob] [directory_glob]
```

The `directory_path` argument undergoes [Pathname expansion]
and [Directory prefix exceptions].

The Directory Include directive scans the `directory_path`, creating menu
entries for readable files that match the extended glob `file_glob`[[di1]].

* If the glob is omitted, an "any file" glob (`*`) is implied; refer
  to [[di2]] for extended globbing rules and examples.

When a file matches the glob, its name becomes the menu entry
label. Activating an entry attempts to launch its full pathname.

A non-empty `directory_glob` argument activates a recursive scanner that
generates submenu entries for directories, and leaf entries for files.

  * If `directory_glob` is omitted, the scanner considers only files directly
    under `directory_path`. Otherwise the scanner recursively descends into
    matching subdirectories, creating submenu entries for all encountered
    directories and leaf entries for files matching `file_glob`.

The scanner considers hidden files and directories and follows symbolic links
according to the `ignorehidden` and `followsymlink` [Configure] directive
options. These options are evaluated for all files and directories encountered
during a scan. However, the initial arguments to the directive are processed
regardless of these options.

  * Scanning does not proceed past the maximum menu depth.[[sm1]]

  * The scanner discards empty submenus, and ignores unreadable directories.

#### Directory Include default icon, tooltip and command

The [Icon], [Tooltip], and [Cmd]`=command` directives can immediately
follow the Directory Include directive. In this case, the same icon and
tooltip apply to each menu entry, while `command` is prepended to each
scanned file path, forming the entry activation command.

  * If an Icon directive is not included, or it is empty, then gtkmenuplus
    will look for the icon associated with the default program that can open
    the matching file, and leave the entry icon empty if no such program exists.

  * Use `Icon=NULL` to hide the icon of included entries (see also [[la3]]).

#### Directory Include performance

If used unwisely, this directive can visit _a lot_ of files on your hard disk,
therefore taking a long time to complete, and creating unwieldy menus that take enormous time to navigate. However, if used knowingly, the Directory Include directive, can create useful menus swiftly.

Here is a real example of how you should _not_ use it:

```sh
gtkmenuplus -v 'configure=noicons; include=~/.cache/thumbnails *'
[0] ok: no file input; assuming directives: configure=noicons; include=~/.cache/thumbnails *
[0] ok: 76,381 ms to process the script
[0] ok: 316,763 ms to display the menu
[0] ok: menu items: 117681; submenus: 1; tracked entries 117682 (bytes: 612,887,856)
```

The `thumbnails/normal` directory holds some 117,000 PNG thumbnails. Gtkmenuplus
parsed and processed the directive in 76 seconds. Then GTK took 316 seconds
to display the menu. Overall, 6 minutes and a half elapsed from program start
before the menu could be navigated. However, navigating was impossible because
GTK took forever to show the next submenu, so much so that killing gtkmenuplus
from another session became necessary.

#### Include directive examples

All the JPEG files in the top level directory:

```gtkmenuplus
    include = /top/level/*.jpeg
```

All the JPEG files in the top level directory
and recursively in its subdirectories:

```gtkmenuplus
    include = /top/level/*.jpeg *
```

All the JPEG files in the top level directory
recursing into subdirectories named "image" something.

```gtkmenuplus
    include = /top/level/*.jpeg image*
```

#### Include directive errors

For backward compatibility, gtkmenuplus uses a slash to separate directory
paths from file globs. However, this design choice leads to ambiguity. For
instance, in the directive `include=/usr/lib/typo *`, gtkmenuplus cannot
determine if "typo" is a mistyped directory name or the file glob "typo" to
be looked for under `/usr/lib`. This ambiguity prevents gtkmenuplus from
reporting genuine typos as errors.

### Absolute pathname

```gtkmenuplus
    [=] /path_to/file
     =  ~/path_to/file
     =  ./path_to/file
```

The Absolute pathname directive creates a menu entry from a pathname,
filling the [Menu entry properties] automatically. Properties cannot be
altered by explicitly adding the [Cmd], [Icon] and [Tooltip] directives.

  * If the pathname ends with ".desktop" and is a valid
    [Launcher file], it is processed as a [Launcher] directive.

To enter an Absolute pathname directive start a line with an equal
sign (`=`) followed by `/`, `~/` or `./` and the rest of the pathname.

  * The leading `=` can be omitted only if the pathname
    starts with `/` and does not itself include `=`.

  * The leading `=` is required if the pathname is
    constructed from variables, as in the following example:

    ```gtkmenuplus
        here == pwd
        = $pwd/file
    ```

By default, the menu entry label is the file name prefixed by its immediately
containing directory name, and the tooltip is the full pathname as read
before [Pathname expansion]. The label can be adjusted as follows.

  * If a [Configure]`=abspathparts N` option is active, the menu
    label includes the last `N` pathname components (the name counts as
    one component). If `N` is zero, the entire pathname is the label.

**Example: the configuration files in and under my home directory**

```sh
    ( echo "configure = abspathparts 3"; find ~ -type f -name \*.conf ) | gtkmenuplus -
```

### Relative pathname

```gtkmenuplus
    = relative_path/file
```

A relative pathname can be a directive if it follows a leading equals sign.
The leading `=` is required.
The pathname follows similar processing to an [Absolute pathname] directive.

**Example: the configuration files in and under the current directory**

```sh
    ( echo "configure = abspathparts 3"; find -type f -name \*.conf | sed s/^/=/ | gtkmenuplus - )
```

### URI

```gtkmenuplus
    = file://absolute_pathname
    = http://...                and other URI schemes
```

A URI can be used as a directive when it follows a leading equals sign.
The leading `=` is required.
The menu entry label is the full URI string.

  * The `file://` URI displays a menu icon.
    It is activated similarly to an [Absolute pathname] directive.

  * Other URI schemes (`http`, `ftp`, etc.) display a blank icon.
    They are activated with the system's default application
    associated with the scheme.

### Submenu

```gtkmenuplus
    submenu = label
```

The Submenu directive begins a new menu entry and concludes the scope
of a preceding [Item] directive. It also ends the scope of a preceding
Submenu directive unless the [Configure]`=endsubmenu` option is enabled.
In that case, the submenu boundaries are determined using the deprecated
indentation mechanism (see below).

  * To select the submenu with a key-press set up a [Keyboard shortcut].

You can place [Icon] and [Tooltip] directives in any order immediately after
the Submenu directive to set the properties of the submenu entry. It is also
possible to conditionally set some properties using the [If] directive.

Empty submenus are discarded; their label will not be shown. To force displaying
the label of a submenu that has no contents, add an empty [Item] to the submenu.

See also § [Launchersubmenu].

#### Indentation and submenu nesting

The maximum menu depth is 5 levels (top menu and four submenu levels).[[sm1]]

#### Submenu indentation

For backwards compatibility, submenu boundaries are determined by line
indentation. Indent directives that belong to the submenu further than
the opening Submenu directive. The submenu ends just before the directive
that is indented at the same level as the opening Submenu.

  * This mechanism is **deprecated** and should not be used in new scripts.

Newly-written menus should use [Configure]`=endsubmenu` option to enable the
[Endsubmenu] directive. Then, a submenu starts with its Submenu directive and
contains all directives until the first occurrence of an Endsubmenu directive,
regardless of indentation.

When deprecated indentation is used to delimit a submenu:

  * The directives comprising the body of the submenu, and
    the submenu entry's [Icon] and [Tooltip] directives, must
    be indented relative to the Submenu directive itself.

  * An indentation stop needs either _four_ spaces or a TAB character.

  * The current submenu ends with the first
    directive having negative hanging indentation.

  * However, the indentation of directives that do not set [Menu entry
    properties], such as [Iconsize], [Menuposition], [Icondirectory],
    [If] and [Format] directives, does not affect the submenu structure.

### Error

```gtkmenuplus
    error = message
```

The Error directive displays the `message` argument in a message
box, and reports the error to the standard error stream (see also
the [Configure]`=errorconsoleonly` option). The error is non-fatal:
the menu will still be displayed.

### Configure

```gtkmenuplus
    configure = option_name...
```

The Configure directive applies multiple configuration options that affect the
current gtkmenuplus run and the menu it produces. Options have global scope,
and apply immediately as they are changed. They can be split into two groups:
[Boolean options] and [String options].

#### Boolean options

Boolean options are toggled by adding the prefix "no" to the
option name, e,g, `Configure = icons`, `Configure = noicons`.

| OPTION NAME            | DEFAULT | REFER TO                        |
|------------------------|---------|---------------------------------|
| `endsubmenu`           | false   | [Endsubmenu]                    |
| `errorconsoleonly`     | false   | note 1                          |
| `formattinglocal`      | false   | [Include]                       |
| `icons`                | true    | [Hiding item and submenu icons] |
| `ignorehidden`         | true    | [Launcher scanner]              |
|                        |         | [Directory Include]  note 2     |
| `launchericons`        | true    | [Hiding launcher icons]         |
| `launchernodisplay`    | true    | [Launcher exclusion cases]      |
| `launchernullcategory` | true    | [Launcher exclusion cases]      |
| `launcherlistfirst`    | false   | [Launcher] and [Launchersub]    |
| `markup`               | true    | [Format section syntax]         |
| `mnemonic`             | true    | [Keyboard shortcut]             |

Notes:

  1. `errorconsoleonly` controls whether the error message box is displayed.
     Error messages are always printed to the standard error stream (stderr).
     Additionally, they are also displayed in a blocking message box when
     the gtkmenuplus standard input stream (stdin) is closed. In this case,
     setting `Configure=errorconsoleonly` disables the message box.
     `Configure=noerrorconsoleonly` reverts to default.

     Compare how these two commands display an error message:

     ```sh
         gtkmenuplus 'word' 0</dev/null

         gtkmenuplus 'configure=errorconsoleonly; word' 0</dev/null
     ```

#### Numeric options

Numeric options set the option value. There is no equal sign `=
between an option name and its value.

| OPTION NAME            | DEFAULT         | REFER TO                     |
|------------------------|-----------------|------------------------------|
| `abspathparts` `N`     | 2               | [Absolute pathname]          |
| `followsymlink` `mask` | 1               | [Launcher scanner]           |
|                        |                 | [Directory Include] note 1   |
| `iconsize` `size`      | GTK button size | [Iconsize] (interchangeable) |
| `menuposition` `x` `y` | N/A             | [Menuposition]               |
| `menuposition` `x` `y` | N/A             | [Menuposition]               |

Notes:

1. The `followsymlink` mask is constructed by
   adding some of the following decimal values:

   - 0 : do not follow symbolic links;
   - 1 : follow symbolic links that target regular files;
   - 2 : follow symbolic links that target directories.

### Onexit

```gtkmenuplus
    onexit = command
```

The Onexit directive establishes the shell `command`
to run after the menu has been deactivated.

The menu script can include multiple Onexit directives but only the last one
will run. Use an empty argument to clear the established command.

  * The `command` is stated via the _system_ C library call (see system(3)).

  * It should always end with "&" to ensure it is detached from gtkmenuplus.

  * It will run whether or not a menu entry is activated.

  * Predicting whether `command` will start or end before or
    after the start or end of the activated menu entry command,
    is not possible. The kernel scheduler controls these events.

### Endsubmenu

```gtkmenuplus
    endsubmenu
```

The Endsubmenu directive allows using line indentation freely, for
instance to clarify an [If]-[Endif] block by indenting its contents.

After a [Configure]`=endsubmenu` directive, line indentation no longer
defines submenu boundaries (see § [Indentation and submenu nesting]).
Instead indentation is ignored, and all lines after a [Submenu] directive
belong to the submenu until the first occurrence of an Endsubmenu directive.

  * The deprecated method to delimit a Submenu by indenting its
    lines could be enabled again with `Configure=noendsubmenu`.

### Separator

```gtkmenuplus
    separator
```

The Separator directive adds a separator line to the menu. Additionally,
it ends the scope of a preceding [Item] or [Submenu] directive.

### Iconsize

```gtkmenuplus
    iconsize = size
```

The Iconsize directive modifies the pixel dimensions of
menu entry icons. It remains effective for subsequent
menu entries until another Iconsize directive appears.

Gtkmenuplus restricts the requested size to a range of 8 to 256 pixels. If
no Iconsize directive is active, the default size for GTK button icons is
used for menu icons, unless this default cannot be determined, in which case
a size of 32 pixels is applied. Not all sizes may be available; themed icon
bitmaps are typically installed in sizes of 16, 24, 48, and 96 pixels. If
the requested size is unavailable, the outcome depends on the GTK version:
GTK2 will scale bitmap icons to the requested size, while GTK3 will use the
closest installed size that is less than or equal to the requested size. SVG
icons can be resized to any requested size.

`iconsize = size` is equivalent to `configure = iconsize size`. However,
only the Configure directive allows expressing the size using a variable.

### Menuposition

```gtkmenuplus
    menuposition = x y
```

By default the menu opens at the mouse cursor position.
However, with a Menuposition directive active, the
menu is pinned at the given (x,y) screen position.

  * The requested position is confined within the default monitor's dimensions.

  * The Menuposition directive ends the scope
    of a preceding [Item] or [Submenu] directive.

The Menuposition directive is equivalent to [Configure]=`menuposition x y`.

### Icondirectory

```gtkmenuplus
    icondirectory = icon_directory_path
```

After the occurrence of an Icondirectory directive, relative
pathnames of [Icon] directives are rebased to the `icon_directory_path`.

  * If `icon_directory_path` is itself relative, it
    is rebased to the script's _base directory_.

  * An empty path argument resets rebasing to the _base directory_.

  * The Icondirectory directive ends the scope of
    a preceding [Item] or [Submenu] directive.

### If

```gtkmenuplus
    if      [?]= expression
      # if-body goes here
    elseif  [?]=  expression
      # elseif-body goes here
    else
      # else-body goes here
    endif
```

The If directive group allows adding entries subject to some condition.

  * Elseif and Endif can also be written as Elif and Fi respectively.

  * Body content can be omitted altogether.

  * Elseif and its body can be repeated.

  * If-Endif blocks can be nested.

Initially the `expression` argument can be one of the following:

  * A positional parameter reference, e.g., `$1`.

  * A variable reference, e.g., `$sunny`.

  * A non-empty string, e.g., `echo true`; the string may
    include [Variables] and [Parameters], e.g., `[ -z "$1" ]`.

  * The empty string, e.g., `""` or simply nothing after `=`.

Expansion of parameters and variables yields a _condition_ to be
evaluated as a _case_ returning the _truth value_ of the `expression`:

| Directive             | _condition_      | _case_         |
|-----------------------|------------------|----------------|
|                       | empty string     | 0              |
| `if ?=` / `elseif ?=` | non-empty string | 1              |
| `if =`  / `elseif =`  | shell command    | command output |

_Cases_ determine the _truth value_, TRUE or FALSE, of the initial `expression`:

  * FALSE (0) for cases "false", "no", "off", 0, and the empty string;

  * TRUE (1) for cases "true", "yes", "on", and non-zero numbers;

  * INVALID (-1) otherwise.

  * Cases are matched irrespective of letter capitalization.

When the If condition is TRUE, directives in the if-body are evaluated,
while remaining directives up to the matching Endif are skipped.

When the If condition is not TRUE, directives in the if-body are skipped,
while remaining directives up to the matching Endif are evaluated.

The evaluation of an elseif-body follows the same rules as an if-body.

#### If scope

The scope of the If group of directives is the file in which they appear. When
an [Include] directive begins reading a new file, a new If scope is created.

Because the If, Elseif, Else and Endif directives _do not_
end the scope of preceding [Item] or [Submenu] directives, it
becomes possible to create icons and tooltips conditionally.

Example:

```gtkmenuplus
    if= [ `date +%H` -le 18 ]; printf $?  # if past 6 PM
      item = evening game
      cmd = mahjongg
    else
     item = daytime game
     cmd = mines
    endif
    tooltip = this item depends on the current time
    icon=games_package.png
```

#### If compatibility

Before version 1.50.0, the behavior of directives within an If-Endif block
was inconsistent, questionable and poorly documented. For instance, some
older versions did not accept empty expressions, failed to report invalid
commands, and evaluate all conditional expressions within the block.

This implementation was corrected in version 1.50.0 as follows:

  * Empty expressions are now accepted and evaluate to FALSE.

  * Invalid commands are reported; however the shell still executes the command.

  * If the `expression` of an If or Elseif directive is invalid, the
  directives within its body are skipped through the closing Endif directive.

  * Evaluation of expressions stops after the
    first TRUE value in an If-Endif block.

#### If examples

**Show menu entries only in PM hours:**

```gtkmenuplus
    if= ! [ `date +%p` = 'PM' ]; printf $?
```

**Use the command line to evaluate a condition:**

```sh
    gtkmenuplus script_file "! [ `date +%p` = 'PM' ]; printf $?"
```

Then use the evaluated value as a parameter, e.g., `if= $1`, in the script file.

**Show menu entries only if a specific physical screen is used:**

```gtkmenuplus
    if= xrandr --current | grep "VGA-0 connected" | wc -l
```

**Show menu entries if firefox is running:**

```gtkmenuplus
    if= pgrep firefox
```

**Show menu entries if a specific memory stick is mounted by label:**

```gtkmenuplus
    if= ! [ -d '/media/VOL_LABEL'  ]; printf $?
```

**Test if the partition your home directory resides on is at 90% full:**

```gtkmenuplus
    if=  df $HOME | awk 'NR==2{split($5,A,/%/);print (A[1]+0>=90)}'
```

## NEAT EXAMPLES

**Log command activation to standard error**

```gtkmenuplus
    format = mnemonic="1"                  # Add shortcuts (optional)
    activationlogfile = /proc/self/fd/2    # The proc file system
    item = Now
    cmd = date
```

**Find absolutely all launchers in your home directory**

The configuration below follows all symbolic links, includes all hidden
files and directories, and displays underscore characters as they appear.
The first run of the script could take considerable time.

```gtkmenuplus
    configure = followsymlink 3 noignorehidden nomnemonic
    launchersub = ~
```

## BUGS

Please report bugs at <https://github.com/step-/gtkmenuplus/issues>.

## AUTHORS

Copyright (C) 2016-2025 step.  
Copyright (C) 2013 Alan Campbell, forked from John Vorthman's myGtkMenu 2004-2011.

## SEE ALSO

gtkmenuplus(1) - usage

Gtkmenuplus home page and project repository (this version):

<https://github.com/step-/gtkmenuplus>

Gtkmenuplus 1.0 home page (old version):

<https://sites.google.com/site/entropyreduction/gtkmenuplus>

myGtkMenu home page (old version):

<https://sites.google.com/site/jvinla/home>

## FOOTNOTES

##### bd1

Before gtkmenuplus version 1.50.0, the base directory was `$HOME` for
a script on standard input, and `undefined` for a command-line script.

##### sx1

The command runs via Glib function `g_spawn_command_line_async`. Refer
to <https://docs.gtk.org/glib/func.spawn_command_line_async.html>.

##### sx2

GLib function `g_spawn_command_line_async` can parse a subset of shell
syntax. Refer to <https://docs.gtk.org/glib/func.shell_parse_argv.html>.

##### mx1

The default application is determined via Glib
function `g_app_info_get_executable`. Refer to
<https://docs.gtk.org/gio/method.AppInfo.get_executable.html>

##### 1ux

The data file opens via GLib function `g_app_info_launch_uris`.
Refer to <https://docs.gtk.org/gio/method.AppInfo.launch_uris.html>

##### pm1

_Pango Text Attribute Markup Language_ specification:
<https://docs.gtk.org/Pango/pango_markup.html>.

##### ic1

The standard icon directories in `$XDG_DATA_DIRS` and
`$XDG_DATA_HOME`, such as `/usr/share/pixmaps,` `/usr/share/icons`,
`~/.local/share/icons` and so on, and their subdirectories. Refer to
<http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html>.

##### ic2

In XDG-compliant Desktop Environments (DE), the MIME-type icon is set by
the `.desktop` file of the default application. That is the highest-ranking
`.desktop` file whose "MimeType=" property includes the MIME-type of the data
file. Refer also the manual page for the _update_desktop_database(1)_ command.

##### la1

The Desktop Entry specification:
  <https://specifications.freedesktop.org/desktop-entry-spec/latest/>

##### la2

The canonical location of a desktop file is the "application"
directory under one of the parts of the `XDG_DATA_DIRS` and
`XDG_DATA_HOME` environment variables. For most systems these are
the `/usr/share/applications`, `/usr/local/share/applications` and
`~/.local/share/applications` directories. Note that gtkmenuplus
can work with launchers located anywhere in the file system.

##### la3

In a [Launcherdirfile], "NULL" is a reserved literal used to match launchers
with an omitted or empty "Categories=" property. This is only applicable when
the [Configure]`=nolaunchernullcategory` option is enabled. Note that this
"NULL" should not be confused with the special [Icon] name `NULL`.

##### sm1

Menu depth is capped at 5. This is a compile-time limit (`MAX_MENU_DEPTH`).
When the depth limit is reached:

  * The [Submenu] directive displays a fatal error
    and exits the program without displaying the menu.

  * The [Directory Include] directive stops scanning the current
    level, and continues in the parent directory. It does not report
    an error; it displays a pruned menu.

  * The [Launchersub] directive stops scanning the current level, and continues
    in the parent directory. It reports a warning; it displays a pruned menu.

##### al1

POSIX Extended Regular Expression: <https://www.gnu.org/software/findutils/manual/html_node/find_html/posix_002dextended-regular-expression-syntax.html>

##### di1

Gtkmenuplus globbing is equivalent to the Bash shell file pattern matching
rules with the `extglob` shell option enabled. The glob implementation utilizes
GNU fnmatch(3): <https://man7.org/linux/man-pages/man3/fnmatch.3.html>.

##### di2

Refer to the article _Introduction to extended globbing patterns_: <http://www.linuxjournal.com/content/bash-extended-globbing>.

