# Gtkmenuplus

This project was forked from Gtkmenuplus 1.00 found at
https://sites.google.com/site/entropyreduction/gtkmenuplus
retrieved on 17-apr-2016.

Thanks to Alan Campbell, gtkmenuplus author, for making his source code
available under the GLPv2 FOSS license.

This fork adds:

 * Nested launcher menus. Keyword `launcher=directory` produces nested
   sub-menus if `directory` includes sub-directories that contain .desktop
   files.

 * Keyword `launcherargs=`, which appends its argument to the command
   that the `launcher=` keyword generates. This makes possible passing
   parameters to menu entries that derive from .desktop files.

 * Some bug fixes of the legacy 1.00 code.

For an application of the launcherargs feature see my shell script
[roxmm](https://github.com/step-/rox-filer-trove/tree/master/usr/bin/roxmm),
which generates a ROX-Filer right-click menu look-alike for a given file
or directory.
