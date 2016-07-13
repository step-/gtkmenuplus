#!/usr/bin/env bash

#caseChanger.sh
#version 1.00, 2013-04-24
#invoked by gtkmenuplus path_to/casechange_menu

# comment out if you are ok changing pasting (case-changed) clipboard contents if no selection made
REQUIRE_SELECTION=1                                       

FILE_MANAGER="nautilus"                                   # change if you want to use another file manager, perhaps also two other commented lines below 

if [ ! -n "$DISPLAY" ] || [ ! -x /usr/bin/xclip ] || [ ! -x /usr/bin/xdotool ]; then  # -n string non-null
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

if [ ! -n "$theNewClip" ] && [ ! -z "$REQUIRE_SELECTION" ]; then  # -n string non-null
 if [ -x /usr/bin/zenity ]; then
  zenity --info --text $"Usage: $0 requires a selection" 
 else	
  echo $"Usage: $0 requires a selection"
 fi	 
 exit 1
fi

case "$1" in

 --toUpper)
   xclip -out -selection clip | tr '[:lower:]' '[:upper:]' | xclip -selection clip
   ;;
         
 --toLower)
   xclip -out -selection clip | tr '[:upper:]' '[:lower:]' | xclip -selection clip
   ;;
         
 --toTitle)
   xclip -out -selection clip | tr '[:upper:]' '[:lower:]' | sed -e 's/[ _]./\U&/g' -e 's/^./\U&/' | xclip -selection clip
   ;;

 --squashedTitleCase)
   xclip -out -selection clip | tr '[:upper:]' '[:lower:]' | sed -e 's/[ _]./\U&/g' -e 's/^./\U&/' -e 's/[ _]//g' | xclip -selection clip            
   ;;

 --slugged_lower_case)
   xclip -out -selection clip | tr '[:upper:]' '[:lower:]' | tr --squeeze-repeats  ' \t' '_' | xclip -selection clip         
   ;;

 --slugged_to_spaced)
   xclip -out -selection clip | sed -e 's/_/ /g' | xclip -selection clip         
   ;;

 *)
  if [ -x /usr/bin/zenity ]; then
   zenity --info --text $"Usage: $0 menu not sending comprehensible option, one of:\n  --toUpper\n--  toLower\n  --toTitle\n  --squashedTitleCase\n  --slugged_lower_case\n  --slugged_to_spaced" 
  else	
   echo $"Usage: $0 {--toUpper|--toLower|--toTitle|--squashedTitleCase|--slugged_lower_case|--slugged_to_spaced}"
  fi	 
  exit 1
 
esac

if [ "$bIsFileManager" -eq 0 ]; then
 xdotool key --delay 50 F2 ctrl+v                             # maybe change if you want to use another file manager
 xdotool key KP_Enter
else	
 xdotool key ctrl+v
fi

#restore clipboard
echo $theClip | xclip -selection clip 


