#version 1.1.0, 2016-07-25

.PHONY: all clean test

GTK_CFLAGS=$(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS=$(shell pkg-config --libs gtk+-2.0)

DEBUG=-O0 -g
DEBUG=-s -O3

CC = gcc

CFLAGS = $(GTK_CFLAGS) $(DEBUG) -Wall \
	-DG_DISABLE_DEPRECATED \
	-DGDK_DISABLE_DEPRECATED \
	-DGDK_PIXBUF_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED \
#	-D_GTKMENUPLUS_NO_LAUNCHERS_ \
#	-D_GTKMENUPLUS_NO_FORMAT_  \
#	-D_GTKMENUPLUS_NO_TOOLTIPS_ \
#	-D_GTKMENUPLUS_NO_IF_ \
#	-D_GTKMENUPLUS_NO_PARAMS_ \
#	-D_GTKMENUPLUS_NO_VARIABLES_ \

LIBS=$(GTK_LIBS)
FILES=../gtkmenuplus.c ../common.c ../launcher.c ../menuInput.c

all: gtkmenuplus

gtkmenuplus: $(FILES)
	$(CC) $(FILES) -o gtkmenuplus $(CFLAGS) $(LIBS)

clean:
	rm -f *.o gtkmenuplus

test: all
	./gtkmenuplus ../../test/1.gtkmenuplus
