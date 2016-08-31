#version 1.1.0, 2016-08-27

.PHONY: all clean test

GTK_CFLAGS=$(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS=$(shell pkg-config --libs gtk+-2.0)

DEBUG=-s -O3
DEBUG=-O0 -g

CC = gcc

CFLAGS = $(GTK_CFLAGS) $(DEBUG) -Wall \
	-DG_DISABLE_DEPRECATED \
	-DGDK_DISABLE_DEPRECATED \
	-DGDK_PIXBUF_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED \
#	-D_GTKMENUPLUS_NO_CACHE_ \
#	-D_GTKMENUPLUS_NO_LAUNCHERS_ \
#	-D_GTKMENUPLUS_NO_FORMAT_  \
#	-D_GTKMENUPLUS_NO_TOOLTIPS_ \
#	-D_GTKMENUPLUS_NO_IF_ \
#	-D_GTKMENUPLUS_NO_PARAMS_ \
#	-D_GTKMENUPLUS_NO_VARIABLES_ \

LIBS=$(GTK_LIBS)
INCLUDES0=../common.h ../menuInput.h
FILES0=../common.c ../menuInput.c ../gtkmenuplus.c

ifeq (, $(findstring _GTKMENUPLUS_NO_LAUNCHERS_,$(CFLAGS)))
FILES1=$(FILES0) ../launcher.c
INCLUDES1=$(INCLUDES0) ../launcher.h
else
FILES1=$(FILES0)
INCLUDES1=$(INCLUDES0)
endif

ifeq (, $(findstring _GTKMENUPLUS_NO_CACHE_,$(CFLAGS)))
FILES2=$(FILES1) ../lru_cache.c
INCLUDES2=$(INCLUDES1) ../lru_cache.h ../uthash.h
else
FILES2=$(FILES1)
INCLUDES2=$(INCLUDES1)
endif

FILES=$(FILES2)
INCLUDES=$(INCLUDES2)

all: gtkmenuplus makefile

gtkmenuplus: $(FILES) $(INCLUDES) makefile
	$(CC) $(FILES) -o gtkmenuplus $(CFLAGS) $(LIBS)

clean:
	rm -f *.o gtkmenuplus

test: all
	@PWD=`pwd`; \
	if ! cd ../../test; then \
		echo &>2 "Please run make from the directory that contains 'makefile'."; \
		exit 1; \
	fi; \
	"${PWD}/gtkmenuplus" START
