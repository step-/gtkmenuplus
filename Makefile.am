
bin_PROGRAMS = gtkmenuplus

#
# https://autotools.io/automake/nonrecursive.html
# 
gtkmenuplus_SOURCES = \
	source/gtkmenuplus.c \
	source/common.c source/common.h \
	source/menuInput.c source/menuInput.h

if FEATURE_LAUNCHERS
gtkmenuplus_SOURCES += source/launcher.h source/launcher.c
endif
if FEATURE_CACHE
gtkmenuplus_SOURCES += source/lru_cache.c source/lru_cache.h source/uthash.h
endif

gtkmenuplus_CFLAGS = $(GTK_CFLAGS) $(DISABLE_DEPRECATED) -Wall
# by default build an unstripped optimized binary
gtkmenuplus_CFLAGS += -O2 -fomit-frame-pointer

gtkmenuplus_LDADD = $(GTK_LIBS)

# Install manual pages
man1_MANS = man/gtkmenuplus.1
man5_MANS = man/gtkmenuplus.5

# Distribution archive
EXTRA_DIST = \
	$(man1_MANS) $(man5_MANS) \
	docs/usage.md docs/menu_configuration_file_format.md \
	docs/COPYING docs/changelog \
	package AUTHORS

