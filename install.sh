#!/bin/bash
#
# Install gtkmenuplus gtkmenuplus.svg gtkmenuplus.desktop
#
# version 1.00, 2013-04-24
#

IS_GTK3=$(pkg-config --modversion gtk+-3.0 2> /dev/null | wc -l)

if [ $IS_GTK3 -eq 1 ]; then
	THE_PLATFORM_DIR=gdk3
else
	THE_PLATFORM_DIR=gdk2
fi

RESULT=0
MACHINE_TYPE=$(uname -m)
if [ ${MACHINE_TYPE} == 'x86_64' ] ; then
	if [ $(command -v gcc make | wc -l) -eq 2 ] ; then
		pushd $THE_PLATFORM_DIR  > /dev/null
		make
		RESULT=$?
		popd  > /dev/null
	else
		echo " "
		echo 64 bit compilation required, build tools not available
		echo " "
		exit 1
	fi 
fi

if [[ $RESULT -ne 0 ]] ; then
	echo " "
	echo make failed
	echo " "
	exit 1
fi

# this script must be run as root
if [ ${UID} != 0 ]; then
	echo " "
	echo "This script must be run as root (su -or- sudo first...)."
	echo " "
	exit 1
fi


DIR_1="/usr/bin"                               # gtkmenuplus
DIR_2="/usr/share/icons/hicolor/scalable/apps" # gtkmenuplus.svg
DIR_3="/usr/share/applications"                # gtkmenuplus.desktop

for d in $DIR_1 $DIR_2  $DIR_3
	do
		#echo $d
		if [ ! -d $d ]; then
			echo " "
			echo Error, cannot find $d
			echo " "
			exit 1
		fi
	done



DIR=$(pwd)
FILE_1="$DIR/$THE_PLATFORM_DIR/gtkmenuplus"
FILE_2="$DIR/gtkmenuplus.svg"
FILE_3="$DIR/gtkmenuplus.desktop"

for f in $FILE_1 $FILE_2 $FILE_3
	do
		#echo $f
		if [ ! -r $f ]; then
			echo Error, cannot read $f
			exit 1
		fi
	done

if [ ! -x $FILE_1 ]; then
	chmod +x $FILE_1
fi

echo Running: cp $DIR/gtkMenuPlus $DIR_1/gtkMenuPlus
cp $DIR/$THE_PLATFORM_DIR/gtkmenuplus $DIR_1/gtkmenuplus

echo Running: cp $DIR/gtkmenuplus.svg $DIR_2/gtkmenuplus.svg
cp $DIR/gtkmenuplus.svg $DIR_2/gtkmenuplus.svg
echo Running: gtk-update-icon-cache -f -t  /usr/share/icons/hicolor
gtk-update-icon-cache -f -t  /usr/share/icons/hicolor

echo Running: cp $DIR/gtkmenuplus.desktop $DIR_3/gtkmenuplus.desktop
cp $DIR/gtkmenuplus.desktop $DIR_3/gtkmenuplus.desktop
desktop-file-install --delete-original --dir $DIR_3 $DIR/gtkmenuplus.desktop

echo
echo All done.
echo

exit 0

