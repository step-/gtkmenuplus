# gtkmenuplus

This project was forked from gtkmenuplus 1.00 found at
https://sites.google.com/site/entropyreduction/gtkmenuplus
retrieved on 17-Apr-2016. Version 1.00 was released on 24-Apr-2013.

Thanks to Alan Campbell, gtkmenuplus 1.00 author, for making his source code
available under the GLPv2 FOSS license.

This fork is actively developed. It significantly improves `.desktop`
file processing (launchers) by adding:

* Recursive directory traversal, which automatically builds sub-menus of
  launchers

* Regular sub-menus and sub-menus of launchers can inherit their visual
  properties from _Type=Directory_ .desktop files

* Launchers can be excluded from sub-menus by filtering their
  _Categories=_ and _NoDisplay=_ property values

* Sub-menus of launchers can be sorted alphabetically on their _Name=_
  property value

* Passing parameters to menu items that derive from launcher files is
  now possible

* Logging command activations to a file, and the file can itself become
  a sub-menu

* An _onexit=_ hook simplifies cleaning up when closing the menu

* Automatic label mnemonics.

* Up-to-date documentation (markdown and Unix manual pages).

Each release introduces new features, please see the [commit
history](https://github.com/gtkmenuplus/commits) or browse the list of
[releases](https://github.com/gtkmenuplus/releases).

Some errors or omissions found in the original (legacy) 1.00 source code are
fixed, for example:

* Core dumps on deeply nested sub-menus
 
* `Format=`_single-format-string_ is honored when exporting formatting
  to cascaded sub-menus

* More icon specifications are loaded correctly

* Nested 'if='/'else' work in all cases

* Unterminated `if=` no longer hangs the program

* `#` in shell commands and similar cases does work, i.e., `[ $# = 0 ]`

For the full list of fixed bugs, please run

    git log --grep '[Ll]egacy'

in the repository directory.

## Examples

Shell scripts that showcase some of the new features:

* [gmenu2](https://github.com/step-/scripts-to-go/blob/master/README.md#gmenu2)
  System menu that resembles the standard Fatdog64 and Puppy Linux System menu,
  with some added zest.
* [gmenu2-fdcp](https://github.com/step-/scripts-to-go/blob/master/README.md#gmenu2-fdcp)
  Fatdog64 Control Panel items as a menu.
* [quicklaunch](https://github.com/step-/scripts-to-go/blob/master/README.md#quicklaunch)
  Customizable user menu.
* [roxmm](https://github.com/step-/scripts-to-go/blob/master/README.md#roxmm)
  ROX-Filer SendTo menu look-alike for a given file or directory.
* [tray-radio](https://github.com/step-/scripts-to-go/blob/master/README.md#tray-radio)
  System tray icon that springs a customizable menu of your Internet radio
  stations and media files.

## Contributing

Feel free to submit pull requests! This is a short list of desiderata if
you are looking for ideas on how to contribute to the project:

* Test suite - Submit new test scripts. Improve existing ones. Automate
  the test suite.

* i18n - Edit source code to enable translation with the GNU GetText tools.

* Reporting bugs.

* Fixing bugs, of course.
