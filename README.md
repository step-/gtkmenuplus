# Gtkmenuplus

This project was forked from Gtkmenuplus 1.00 found at
https://sites.google.com/site/entropyreduction/gtkmenuplus
retrieved on 17-Apr-2016. Version 1.00 was released on 24-Apr-2013.

Thanks to Alan Campbell, gtkmenuplus author, for making his source code
available under the GLPv2 FOSS license.

Version 1.1.0 significantly improves `.desktop` file processing
(launchers) by adding nested launcher menus.

 * New keyword `launchersub=`_directory_ displays a nested sub-menu of
   all the .desktop launcher files that it can find while descending
   into _directory_.

   * Nested sub-menu labels, tooltips, icons _and formatting_ can be
     controlled via _Type=Directory_ .desktop files.

   * Items can be excluded from view by their _Categories=_ and
     _NoDisplay=_ property values.

   * `launchersub=` can be nested in `submenu=` keywords.

   * A `launchersub=` entry list is sorted alphabetically on the `Name=`
     property value, rather than on .desktop file names.

 * New keyword `launchersubmenu=` reads its settings from a .desktop
   file, and displays a sub-menu of the items that follow it in the
   configuration file.

 * New keyword `launcherargs=` appends its argument to the command that
   `launchersub=` and `launcher=` generate. This makes it possible to
   `pass parameters to menu entries that derive from launcher files.

 * New menu configuration options `launchernodisplay` and
   `launchernullcategory`.

Version 1.1.0 also adds a logging feature that creates a sub-menu of
menu item and launcher activations.

 * New keyword `activationlogfile=`_logfile\_path_ specifies the log
   file and enables this feature.

Some errors or omissions of the original 1.00 source code are fixed. Notably:

 * No more core dumps on deeply nested sub-menus. There was and there is
   a hard limit of 5 on sub-menu depth.
 
 * `Format=`_single-format-string_ works to export formatting to
   cascaded sub-menus (nested with `submenu=`).

 * Some icon specifications that couldn't be loaded now can.

 * A `launcher=` entry list is sorted alphabetically on the `Name=`
   property value, rather than on .desktop file names.

 * Program hanging on certain input cases involving unterminated `if=`.

 * And more.

Documentation is up-to-date with the new development.

For sample applications of the new features see my shell scripts:

 * [gmenu2](https://github.com/step-/scripts-to-go/), which displays an
   application menu that resembles the typical Puppy Linux menu.
   Showcases launchers and the activation log.

 * [roxmm](https://github.com/step-/scripts-to-go/), which displays a
   ROX-Filer SendTo menu look-alike for a given file or directory.
   Showcases launchers.

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
