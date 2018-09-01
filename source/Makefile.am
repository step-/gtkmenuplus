
bin_PROGRAMS = gtkmenuplus

gtkmenuplus_SOURCES = gtkmenuplus.c common.c common.h menuInput.c menuInput.h
if FEATURE_LAUNCHERS
gtkmenuplus_SOURCES += launcher.h launcher.c
endif
if FEATURE_CACHE
gtkmenuplus_SOURCES += lru_cache.c lru_cache.h uthash.h
endif

gtkmenuplus_CFLAGS = $(GTK_CFLAGS) \
	-Wall \
	-DG_DISABLE_DEPRECATED \
	-DGDK_DISABLE_DEPRECATED \
	-DGDK_PIXBUF_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED

gtkmenuplus_LDADD = $(GTK_LIBS)
