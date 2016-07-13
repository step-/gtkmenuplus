
#version 1.00, 2013-04-24

CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DGDK_DISABLE_DEPRECATED 	\
	-DGDK_PIXBUF_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED \
#	-D_GTKMENUPLUS_NO_LAUNCHERS_ \
#	-D_GTKMENUPLUS_NO_FORMAT_  \
#	-D_GTKMENUPLUS_NO_TOOLTIPS_ \
#	-D_GTKMENUPLUS_NO_IF_ \
#	-D_GTKMENUPLUS_NO_PARAMS_ \
#	-D_GTKMENUPLUS_NO_VARIABLES_ \
	-s -O3

GKT2 =  `pkg-config gtk+-2.0 --cflags --libs`

all: gtkmenuplus

gtkmenuplus: ../gtkmenuplus.c ../common.c ../launcher.c ../menuInput.c
	$(CC) ../gtkmenuplus.c ../common.c ../launcher.c ../menuInput.c -o gtkmenuplus $(CFLAGS) $(GKT2)

clean:
	rm -f *.o gtkmenuplus
