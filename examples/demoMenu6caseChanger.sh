#!/usr/bin/env bash

#version 1.00, 2013-04-24
#run demo file demoMenu7caseChanger.txt for gtkMenuPlus

source demo_menus/common.sh

echo
echo -e "${bldblu}caseChanger is a menu configuration file (and script invoked by it)"
echo -e "to change case of text in editable controls.$txtrstback"
echo

if [ ! -f ./case_changer/casechange_menu ]; then 
 echo -e "${bldred}caseChanger/casechange_menu bit found.$txtrstback"
 read -s -n1 -p "hit any key to finish this script." keypress ; echo
 echo -e $txtrst
 exit 1
fi

[ -z $THE_EDITOR ] && getDefaultEditor case_changer/casechange_menu

[ -n $THE_EDITOR ] && $THE_EDITOR case_changer/casechange_menu  &


if [ ! -n "$DISPLAY" ] || [ ! -x /usr/bin/xclip ] || [ ! -x /usr/bin/xdotool ]; then  # -n string non-null
 echo -e ${bldred}caseChanger requires an x-display and the utilities xclip and xdotool, 
 echo -e "which you can probably get from your friendly local repository.$txtrstback"
 echo
 read -s -n1 -p "hit any key to finish this script." keypress ; echo
 echo -e $txtrst
 exit 1
fi	 

nSecs=20
 
echo -e ${bldred}If you want to use it: within the next $nSecs seconds, start your
echo -e "favourite text editor or word processor, type something in it,"
echo -e "highlight the text, and make sure the window containing"
echo -e "the typed text has focus. $txtrstback"
echo
 
keypress=
shopt -s nocasematch
while [ "$keypress" != "y" ] && [ "$keypress" != "n" ] ; do  echo -e "${bldblu}run casechanger?$txtrstback (${bldblu}y${txtrstback}/${bldblu}n${txtrstback})"; read -s -n1  keypress ; echo ; done
[ "$keypress"  ==  "n" ] && (shopt -u nocasematch; exit 0 )
shopt -u nocasematch

echo casechanger menu will appear in:
for (( i=$nSecs; i>0; i--)); do sleep 1 ;  printf "$i seconds\r" ; wait ;  done

#ALL THE SCRIPT ABOVE IS WINDOWDRESSING; 
#ALL YOU NEED TO MAKE A MNEU IS A LINE LIKE THIS:
$GTK_MENU_PLUS ./case_changer/casechange_menu &

echo
read -s -n1 -p "hit any key to finish this script." keypress ; echo
echo -e $txtrst
