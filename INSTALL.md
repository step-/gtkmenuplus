## DEPENDENCIES

**Optional run-time dependencies**

  * If the program was built with `--enable-variable` or
    `--enable-conditional` (the default build options), `/bin/sh` is required.

**Optional build-time dependencies**

_The source distribution archive includes pre-formatted manual
pages. To regenerate them, you will need the following tools:_

  * GNU help2man to build the man1 page.
  * gawk and [lowdown] to build the man5 page.
  * [pandoc] to convert the man1 page to Markdown.

[lowdown]: <https://kristaps.bsd.lv/lowdown/>
[pandoc]: <https://pandoc.org>

**Optional development dependencies**

_These are only needed if you build from the Git worktree,
and only for specific changes (you will know which)._

  * [ragel] to modify regular expressions.
  * [fccf] to update `.decl.h` files.

[fccf]: <https://github.com/p-ranav/fccf>
[ragel]: <http://www.colm.net/open-source/ragel>

## INSTALLING

Gtkmenuplus builds for GTK-3 using GNU Autotools.
Building for GTK-2 is also supported.

### Build types

1. **Release build**: installs the binary and accompanying documentation:

   ```sh
   ( mkdir -p build && cd build && ../configure CFLAGS= &&
     make all install-strip install-man )
   ```

2. **Debug build**: installs into the build directory:

   ```sh
   ( mkdir -p build-debug && cd build-debug &&
     ../configure --prefix=`pwd` CFLAGS='-ggdb3 -Og -UNDEBUG' &&
     make all install install-man )
   ```

3. **GTK-2 release build**:

   ```sh
   ( mkdir build-gtk2 && cd build-gtk2 &&
     ../configure --with-gtk=gtk2 CFLAGS= &&
     make all install-strip install-man )
   ```

4. **Multilib build**: on a 64-bit system with multilib
   support and 32-bit libraries in `/usr/lib`, this builds
   a 32-bit binary and installs into the build directory:

   ```sh
   ( mkdir build-32 && cd build-32 &&
     env PKG_CONFIG_PATH=/usr/lib/pkgconfig:$PKG_CONFIG_PATH \
     ../configure --prefix=`pwd` --build=i686-linux-gnu --libdir=/usr/lib "CFLAGS=-m32 -L/usr/lib -O2 -DNDEBUG=1" &&
     make all install-strip install-man )
   ```

## Running tests

Refer to [test/README.md].

## Building from git source

When building from the Git source, you must first
generate the configuration files by running:

```sh
autoreconf -fiv
```

### Configuration options

```
Usage: ./configure [OPTION]... [VAR=VALUE]...

```

Run `./configure --help` for further help.

### Salient options

```
  --disable-dependency-tracking
                          speeds up one-time build
  --enable-debug-trace    Output raw and cooked lines and launcherpaths
  --enable-debug-if-states
                          Output states of `if=` directives
  --enable-deprecated     Enable deprecated GTK functions
                          (for GTK-3 deprecated functions are always enabled)
  --enable-activation-log Enable activation log directive group (on)
  --enable-conditional    Enable conditional directive group (on)
  --enable-formatting     Enable menu formatting (on)
  --enable-icons-always-on
                          Force the display of icons, ignoring
                          GtkSettings:gtk-menu-images setting (off)
  --enable-launcher       Enable launcher directive group (on)
  --enable-parameter      Enable menu parameters (on)
  --enable-serialization  Enable json serialization (off)
  --enable-tooltip        Enable menu tooltips (on)
  --enable-variable       Enable menu variables (on)

  --with-gtk=gtk2|gtk3    set the GTK+ version to use (default - gtk3)
```

[test/README.md]: <https://github.com/step-/gtkmenuplus/blob/master/test/README.md>
