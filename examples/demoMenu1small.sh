#!/usr/bin/env bash

#version 1.00, 2013-04-24
#run demo file demoMenu1small.txt for gtkMenuPlus

source demoMenus/common.sh

[ -z $THE_EDITOR ] && getDefaultEditor demoMenus/demoMenuSmall.txt

[ -n $THE_EDITOR ] && $THE_EDITOR demoMenus/demoMenuSmall.txt  &

echo
echo -e ${bldblu} demoMenu1small.txt, using format= to produce a very small menu $txtrstback
echo

#ALL THE SCRIPT ABOVE IS WINDOWDRESSING; 
#ALL YOU NEED TO MAKE A MNEU IS A LINE LIKE THIS:
$GTK_MENU_PLUS demoMenus/demoMenuSmall.txt
