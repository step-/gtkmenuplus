# Gtkmenuplus

This project was forked from Gtkmenuplus 1.00 found at
https://sites.google.com/site/entropyreduction/gtkmenuplus
retrieved on 17-Apr-2016. Version 1.00 was released on 24-Apr-2013.

Thanks to Alan Campbell, gtkmenuplus author, for making his source code
available under the GLPv2 FOSS license.

Version 1.10 adds:

 * Nested launcher menus. Keyword `launcher=`_directory_ automatically
   produces nested sub-menus if _directory_ includes sub-directories that
   contain launcher (.desktop) files.

 * Keyword `submenu=` followed by keyword `launcher=` produces a nested
   sub-menu.

 * New keyword `launcherargs=` appends its argument to the command that
   `launcher=` generates. This makes it possible to pass parameters to
   menu entries that derive from launcher files.

 Some issues of original 1.00 source code are fixed. Notably:

 * No more core dumps on deeply nested sub-menus. There was and there is
   a hard limit of 5 on sub-menu depth.

For an application of the launcherargs feature see my shell script
[roxmm](https://github.com/step-/rox-filer-trove/tree/master/usr/bin/roxmm),
which generates a ROX-Filer right-click menu look-alike for a given file
or directory.
