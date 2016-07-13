#!/usr/bin/env bash

#version 1.00, 2013-04-24
#run demo file demoMenu2large.txt for gtkMenuPlus

source demo_menus/common.sh

[ -z $THE_EDITOR ] && getDefaultEditor demo_menus/demoMenuLarge.txt

[ -n $THE_EDITOR ] && $THE_EDITOR demo_menus/demoMenuLarge.txt  &

echo
echo  -e ${bldblu}demoMenu2large.txt, using format= to produce a large menu $txtrstback
echo

#ALL THE SCRIPT ABOVE IS WINDOWDRESSING; 
#ALL YOU NEED TO MAKE A MNEU IS A LINE LIKE THIS:
$GTK_MENU_PLUS demo_menus/demoMenuLarge.txt
