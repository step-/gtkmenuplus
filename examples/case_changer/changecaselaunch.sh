#!/usr/bin/env bash
# comment out if you are ok changing pasting (case-changed) clipboard contents if no selection made
# version 1.00, 2013-01-15

REQUIRE_SELECTION=1

MENU_DEF_FILE=./clip_menu.tmp

FILE_MANAGER="nautilus"                                     # change if you want to use another file manager 

function addToMenu()
{
 echo "item=$1" >> "$MENU_DEF_FILE"
 echo "cmd=caseChanger.sh --$2" >> "$MENU_DEF_FILE"
 echo "icon=NULL" >> "$MENU_DEF_FILE"
 echo "" >> "$MENU_DEF_FILE"
}

if [! -n "$DISPLAY" ] || [! -x /usr/bin/xclip ] || [! -x /usr/bin/xdotool ]; then  # -n string non-null
 if [ -x /usr/bin/zenity ]; then
  zenity --info --text $"$0 requires an x-display and the utilities xclip and xdotool" 
 else	
  echo $"$0 requires an x-display and the utilities xclip and xdotool"
 fi	 
 exit 1
fi

aPid=$(xdotool getactivewindow getwindowpid)
theExe=$(ps -p $aPid -o comm | tail -1 ) # | wc -w 1
bIsFileManager=$( [ "$theExe" == "$FILE_MANAGER" ]; echo $?)

#save clipboard
theClip=$(xclip -out -selection clip)
#echo "" | xclip -selection clip         
echo -n | xclip -selection clipboard 


#xsel -p -o | ($1)  | xclip -selection clip ; xdotool key ctrl+v
#xsel not reliable, e.g. doesn't work for jEdit

if [ "$bIsFileManager" -eq 0 ]; then
  xdotool key F2 ctrl+c Escape                             # maybe change if you want to use another file manager 
else	
  xdotool key ctrl+c
fi

theNewClip=$(xclip -out -selection clip)
if [! -n "$theNewClip" ] && [ ! -z "$REQUIRE_SELECTION" ] ; then  # -n string non-null
 if [ -x /usr/bin/zenity ]; then
  zenity --info --text $"Usage: $0 requires a selection" 
 else	
  echo $"Usage: $0 requires a selection"
 fi	 
 exit 1
fi

rm "$MENU_DEF_FILE"
#if [[ "$VARIABLE" =~ "Let's.*ring" ]]

#bIsWS=$([[ "${theNewClip//[ \t]/}" != "" ]]; echo $?)
#bIsUpper=$([[ "${theNewClip//[A-Z]/}" != "" ]]; echo $?)
#bIsLower=$([[ "${theNewClip//[a-z]/}" != "" ]]; echo $?)
#bIsUnderS=$([[ "${theNewClip//_/}" != "" ]]; echo $?)

if [[ "${theNewClip//[a-z]/}" != "" ]]; then  addToMenu "to UPPER CASE" toUpper; fi
if [[ "${theNewClip//[A-Z]/}" != "" ]]; then  addToMenu "to lower case" toLower; fi
if [[ "${theNewClip// [a-z]/}" != "" ]]; then  addToMenu "to Title Case" toTitle; fi  
if [[ "${theNewClip// /}" != "" ]]; then  addToMenu "to SquashedTitleCase" squashedTitleCase; fi 
if [[ "${theNewClip// /}" != "" ]]; then  addToMenu "to slugged__lower__case" slugged_lower_case; fi
if [[ "${theNewClip//_/}" != "" ]]; then  addToMenu "to white space from__slugged" slugged_to_spaced; fi

exit 1

gtkmenuplus "$MENU_DEF_FILE"

rm "$MENU_DEF_FILE"

