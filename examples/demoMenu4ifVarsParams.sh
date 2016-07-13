#!/usr/bin/env bash

#version 1.00, 2013-04-24
#run demo file demoMenu4ifVarsParams.txt for gtkMenuPlus

source demo_menus/common.sh

[ -z $THE_EDITOR ] && getDefaultEditor demo_menus/demoMenuIfVarsParams.txt

[ -n $THE_EDITOR ] && $THE_EDITOR demo_menus/demoMenuIfVarsParams.txt  &

echo
echo  -e ${bldblu}demoMenu4ifVarsParams.txt, a demo of if=, 
echo  -e "a parameter on command line, and variables $txtrstback"
echo

#ALL THE SCRIPT ABOVE IS WINDOWDRESSING; 
#ALL YOU NEED TO MAKE A MNEU IS A LINE LIKE THIS:
$GTK_MENU_PLUS demo_menus/demoMenuIfVarsParams.txt "hi-ho, hi-ho"
