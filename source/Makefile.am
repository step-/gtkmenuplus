
bin_PROGRAMS = gtkmenuplus

gtkmenuplus_SOURCES = gtkmenuplus.c common.c common.h menuInput.c menuInput.h
if FEATURE_LAUNCHERS
gtkmenuplus_SOURCES += launcher.h launcher.c
endif
if FEATURE_CACHE
gtkmenuplus_SOURCES += lru_cache.c lru_cache.h uthash.h
endif

gtkmenuplus_CFLAGS = $(GTK_CFLAGS) $(DISABLE_DEPRECATED) -Wall

gtkmenuplus_LDADD = $(GTK_LIBS)
