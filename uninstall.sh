#!/bin/bash
#
# Install gtkmenuplus gtkmenuplus.svg gtkmenuplus.desktop
#
# version 1.00, 2013-04-24
#

# this script must be run as root
if [ ${UID} != 0 ]; then
	echo " "
	echo "This script must be run as root (su -or- sudo first...)."
	echo " "
	exit 1
fi


DIR_1="/usr/bin"                               # gtkmenuplus
DIR_2="/usr/share/icons/hicolor/scalable/apps" # gtkmenuplus.svg
#DIR_3="/usr/share/applications"               # gtkmenuplus.desktop

FILE_1="$DIR_1/gtkmenuplus"
FILE_2="$DIR_2/gtkmenuplus.svg"
#FILE_3="$DIR_3/gtkmenuplus.desktop"

for f in $FILE_1 $FILE_2  # $FILE_3
	do
		if [ ! -f $f ]; then
			echo Cannot find $f
		else
			rm -fv $f
		fi
	done

gtk-update-icon-cache -f -t  /usr/share/icons/hicolor

echo
echo All done.
echo

exit 0

