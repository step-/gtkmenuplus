#!/usr/bin/env bash

# runDemos.sh
# version 1.00, 2013-04-24
# run demos for gtkmenuplus
# run runDemos.sh from examples folder 

function getDefaultEditor
{
 sSampleName=$1
 sCurMine=$(xdg-mime query filetype $sSampleName)
 sCurDsk=$(xdg-mime query default $sCurMine)

 if [ -f "$HOME/.local/share/applications/$sCurDsk" ]; then
  sTrueDsk="$HOME/.local/share/applications/$sCurDsk"
 elif [ -f "/usr/local/share/applications/$sCurDsk" ]; then
  sTrueDsk="/usr/local/share/applications/$sCurDsk"
 elif [ -f  "/usr/share/applications/$sCurDsk" ]; then
  sTrueDsk="/usr/share/applications/$sCurDsk"
 else 
  echo "Sorry no executable found for $1"
 fi

 THE_EDITOR=$(grep "^Exec" "$sTrueDsk" | head -1 | sed -e 's|Exec=||' -e 's| .*$||' )
}

#http://stackoverflow.com/questions/592620/check-if-a-program-exists-from-a-bash-script
readonly PROG_EXISTS="command -v "


#https://linuxtidbits.wordpress.com/2008/08/11/output-color-on-bash-scripts/
txtbld=$(tput bold)  
backwhite=$(tput setab 7)
bldred=${txtbld}$(tput setaf 1)
bldblu=${txtbld}$(tput setaf 4)
normblu=$(tput setaf 4)
txtrstback="\e[0m\e[107m" # high intensity white background
txtrst=$(tput sgr0)

if [ $($PROG_EXISTS ../gtkmenuplus) ]; then 
 GTK_MENU_PLUS=../gtkmenuplus
elif [ $($PROG_EXISTS gtkmenuplus) ]; then 
 GTK_MENU_PLUS=gtkmenuplus
else
 echo gtkmenuplus not found
 exit
fi

echo -e $txtrstback
clear

############################################################

