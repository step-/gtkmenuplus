#!/usr/bin/env bash

#displayManPage.sh
#called via menu generated by manpageMenu.sh
#version 1.00, 2013-04-24
#by Alan Campbell

readonly sNameInDisp=$1
readonly nSectionInDisp=$2
readonly sDisplayWithDisp=$3
readonly bTryInfoDisp=$4


###########################################

#assume manpageCommon.sh in same folder as displayManPage.sh
readonly sDir="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )" # http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in

source "$sDir/manpageCommon.sh" # doDisplay

###########################################

if [ -n "$bTryInfoDisp" ] && [ $bTryInfoDisp -eq 1 ] && [ "$(info -w $sNameInDisp)" != '*manpages*' ]; then
 
   yad --title "Display info page for \"$sNameInDisp\"" --form \
                 --field="There's an info page for \"$sNameInDisp\".\nIt can only be displayed as text or in terminal.:LBL"  \
                 --button="Display as text:3"  \
                 --button="Display in terminal using info:2" \
                 --button="neither:1"

   PROG_EXIT=$?

   case $PROG_EXIT in

    0)
     exit
     ;;

    2)
     doDisplay "man" "info" "$nSectionInDisp" "$sNameInDisp"
     ;;

    3)
     doDisplay "text" "info" "$nSectionInDisp" "$sNameInDisp"
     ;;

    *)
    ;;
    
   esac

fi # if [ "$bTryInfoDisp" -eq "1" ] && [ "$(info -w $sNameInDisp)" != '*manpages*' ]; then

sCmd="man "
[ "$nSectionInDisp" -ne "0" ] &&  sCmd="$sCmd -s $nSectionInDisp "

doDisplay "$sDisplayWithDisp" "$sCmd" "$nSectionInDisp" "$sNameInDisp"

echo "$sDisplayWithDisp" "$sCmd" "$nSectionInDisp" "$sNameInDisp"

exit

