# Gtkmenuplus

This project was forked from Gtkmenuplus 1.00 found at
https://sites.google.com/site/entropyreduction/gtkmenuplus
retrieved on 17-Apr-2016. Version 1.00 was released on 24-Apr-2013.

Thanks to Alan Campbell, gtkmenuplus author, for making his source code
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

 * An _onexit=_ hook simplifies cleaning up after a menu

 * Up-to-date documentation.

Each release introduces new features, please see the [commit history](https://github.com/gtkmenuplus/commits) or browse the list of [releases](https://github.com/gtkmenuplus/releases).

Some errors or omissions in the original 1.00 source code are fixed, for example:

 * Core dumps on deeply nested sub-menus
 
 * `Format=`_single-format-string_ is honored when exporting formatting
   to cascaded sub-menus

 * More icon specifications are loaded correctly

 * Nested 'if='/'else' work in all cases

 * Unterminated `if=` no longer hangs the program

Etcetera. For a full list of fixes please refer to the commit history.

I wrote some scripts that apply the new features. They can be found in my [scripts-to-go](https://github.com/step-/scripts-to-go/) repository:

 * **gmenu2** displays a System menu that resembles the standard
   Fatdog64 and Puppy Linux System menu. An extension script,
   **gmenu2-fdcp**, adds a sub-menu of Fatdog64 Control Panel items.

 * **quicklaunch** is a customizable user menu

 * **roxmm** displays a ROX-Filer SendTo menu look-alike for a given
   file or directory

## Contributing

Feel free to submit pull requests! This is a short list of desiderata if
you are looking for ideas on how to contribute to the project:

 * Markdown documentation - Review and improve docs/usage.txt
   and docs/menu_configuration_file_format.txt. Format as markdown text
   ([mdview](http://chiselapp.com/user/jamesbond/repository/mdview3/index)
   markdown subset preferred)

 * Test suite - Submit new test scripts. Improve existing ones. Automate
   the test suite.

 * i18n - Edit source code to enable translation with the GNU GetText tools.

 * Bug fixing, of course.
