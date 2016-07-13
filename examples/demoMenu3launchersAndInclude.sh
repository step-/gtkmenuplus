#!/usr/bin/env bash

#version 1.00, 2013-04-24
#run demo file demoMenu3launchersAndInclude.txt for gtkMenuPlus

source demo_menus/common.sh

[ -z $THE_EDITOR ] && getDefaultEditor demo_menus/demoMenuLaunchersAndInclude.txt

[ -n $THE_EDITOR ] && $THE_EDITOR demo_menus/demoMenuLaunchersAndInclude.txt demo_menus/demoMenuIncluded.txt &

echo
echo -e ${bldblu}demoMenu3launchersAndInclude.txt, 
echo -e "a demo of include= and launcher= $txtrstback"
echo

#ALL THE SCRIPT ABOVE IS WINDOWDRESSING; 
#ALL YOU NEED TO MAKE A MNEU IS A LINE LIKE THIS:
$GTK_MENU_PLUS demo_menus/demoMenuLaunchersAndInclude.txt
