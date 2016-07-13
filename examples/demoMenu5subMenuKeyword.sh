#!/usr/bin/env bash

#version 1.00, 2013-04-24
#run demo file demoMenu5subMenuKeyword.txt for gtkMenuPlus

source demo_menus/common.sh

[ -z $THE_EDITOR ] && getDefaultEditor demo_menus/demoMenuSubMenuKeyword.txt

[ -n $THE_EDITOR ] && $THE_EDITOR demo_menus/demoMenuSubMenuKeyword.txt  &

echo
echo -e ${bldblu}demoMenu5subMenuKeyword.txt, a demo of endsubmenu$txtrstback
echo -e "${normblu}as alternative to indentation to define submenus $txtrstback"
echo

#ALL THE SCRIPT ABOVE IS WINDOWDRESSING; 
#ALL YOU NEED TO MAKE A MNEU IS A LINE LIKE THIS:
$GTK_MENU_PLUS demo_menus/demoMenuSubMenuKeyword.txt
