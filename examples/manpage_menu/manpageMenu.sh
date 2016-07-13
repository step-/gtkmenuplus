#!/usr/bin/env bash

#manpageMenu.sh
#version 1.00, 2013-04-24
#by Alan Campbell

#bar="$( IFS=! ; echo "${AgeArray[*]}" )"


#Place this script, manpageDisplay.sh and manpageCommon.sh in the same folder.  
#They don't have to be on the path.

#Create a launcher to manpageMenu.sh anywhere, or run from gtkmenuplus or myGtkMenu, 
#or add to main menu, or run from command line.  


###################################################
#globals used by getTempFile
gsTempFile=

#globals used by checkProgs
gnProgOption=
gsProgComboTest=''

#assume manpageCommon.sh in same folder as manpageMenu.sh
sDir="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )" # http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in

source "$sDir/manpageCommon.sh"  # getTempFile, checkProgs, bIsTerminal, doMsgBox, checkVariable, getMsgBoxProg, msgTiming

#readonly sDisplayProg=( "yelp"  "html" "pdf"     "text" "man in terminal")
#OLDIFS="$IFS"
#IFS="|"
#echo "${sDisplayProgNames[*]}"
#registerComboPick html "${sDisplayProg[*]}"   "$sConfigFile" "nDisplayWith" 
#IFS="$OLDIFS"
#exit

###################################################
#global used by getMsgBoxProg
gsMsgBoxProg=
getMsgBoxProg

###################################################
#globals used by checkProgs
gnProgOption=$nProgChooseWith
gsProgComboTest=

readonly sChooseProgs=( "gtkmenuplus" "myGtkMenu" "yad" "zenity"  "pdmenu")
nProgs=${#sChooseProgs[*]}

for (( i=0; i<$nProgs; i++ )); do
 [ $($PROG_EXISTS ${sChooseProgs[$i]}) ] && checkProgs $(( $i +1 )) ${sChooseProgs[$i]}  # checkProgs exported from manpageCommon.sh
done

[ $gnProgOption -eq $NO_OPTION_CHOSEN ]  && ( doMsgBox $gsMsgBoxProg "manpageMenu.sh" "can't find a programme to show choices";  exit 1)

readonly sProgChooseComboText=${gsProgComboTest:1} # strip first !
nProgChooseWith=$gnProgOption

#overridden by yad call (f zenity not used instead)
sChooseProg=${sChooseProgs[$nProgChooseWith]}

###################################################
#some globals used by checkProgs
gnProgOption=$nDisplayWith
gsProgComboTest=''

readonly sDisplayProgNames=( "yelp"  "html" "pdf"     "text" "man in terminal")
readonly sDisplayProgs=(     "yelp"  ""     "ps2pdf"  ""     "man") # man2html

#sDisplayProgs
nProgs=${#sDisplayProgNames[*]}

for (( i=0; i<$nProgs; i++ )); do
  [ -z ${sDisplayProgs[$i]} ] || [ $($PROG_EXISTS ${sDisplayProgs[$i]}) ] && checkProgs $(( $i +1)) ${sDisplayProgNames[$i]}   # checkProgs exported from manpageCommon.sh
done

[ $gnProgOption -eq $NO_OPTION_CHOSEN ]  && (doMsgBox $gsMsgBoxProg "manpageMenu.sh" "can't find a programme to display a man page" ; exit 1)

sProgDisplayComboText=${gsProgComboTest:1} # strip first !
nDisplayWith=$gnProgOption

#overridden by yad call (f zenity not used instead)
sDisplayProg=$(echo "${sDisplayProgNames[$nDisplayWith]}" | cut -f1 -d ' ')

 
###################################################
#set max allowed menu sizes

readonly sSectionsArray=( "1 - General commands"             "2 - System calls" \
                          "3 - Library functions"            "4 - Special files and drivers" \
                          "5 - File formats and conventions" "6 - Games and screensavers" \
                          "7 - Miscellanea"                  "8 - Sysadmin commands, daemons" "All man sections")

nSections=${#sSectionsArray[*]}
 
for (( i=0; i<$nSections; i++ )); do
 sSec=${sSectionsArray[$i]} 
 cLet=${sSec:0:1}
 if [ "$nSection" = "$cLet" ]; then
  sSections="!${sSectionsArray[$i]}$sSections"
 else 
  sSections="$sSections!${sSectionsArray[$i]}"  
 fi 

done

sSections=${sSections:1} # strip first !


sGetInfo=TRUE
bSectionMakeDefault=TRUE
bChooseProgMakeDefault=TRUE
bDisplayProgMakeDefault=TRUE

###################################################
# can be overridden by yad or zenity dialogs

sFilter=$1

###########################################
# use yad 

if [ $($PROG_EXISTS yad) ]; then 
 
 frmdata=$(yad --title "find man pages" --form  --columns 2 \
               --field "regex filter\nleave empty to see everything:" \
               --field="section:CB" \
               --field="choose with:CB" \
               --field="display man page with:CB" \
               --field="try for info page if available\n\(you can only see info output using man or text display options\):CHK" \
               --field=" :LBL" --field="make default:CHK"  --field="make default:CHK" --field="make default:CHK" \
               "$1"  "$sSections"  "$sProgChooseComboText" "$sProgDisplayComboText" "$sGetInfo" ''  "$bSectionMakeDefault" "$bChooseProgMakeDefault" "$bDisplayProgMakeDefault")
               
#TO DO use "${sChooseProgs[@]}" ??

 PROG_EXIT=$?

 sFilter=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $1 }')
 nSection=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $2 }')
 sChooseProg=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $3 }')
 sDisplayProg=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $4 }')
 sGetInfo=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $5 }')
 bSectionMakeDefault=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $7 }')
 bChooseProgMakeDefault=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $8 }')
 bDisplayProgMakeDefault=$(echo $frmdata | awk 'BEGIN {FS="|" } { print $9 }')


###########################################
# use zenity

elif [ $($PROG_EXISTS zenity) ]; then 

 sFilter=$(zenity  --title="find man pages" --entry \
  --text="regex filter to apply to man page names and descriptions\nleave empty to see everything:" \
  --entry-text "$1")

 PROG_EXIT=$?
 if [ $PROG_EXIT -ne 0 ]; then
   exit 0
 fi
  
 if [ $ONE_ZENITY_DIALOG -eq 0 ]; then

#  sSections='"'$sSections'"'
  sSections=$(echo $sSections | tr \! \\n ) 
  nSection=$(echo "$sSections" | zenity  --title="find man pages: in which section" --list \
    --column "Section:") 

  if [ $? -ne 0 ]; then
   exit 0
  fi
  nSection=${nSection:0:1}
  
  sComboText=$(echo $sProgChooseComboText | tr \! \\n ) 
  sChooseProg=$(echo "$sComboText" | zenity  --title="find man pages: use which programme to choose" --list \
    --column "Programme:")

  if [ $? -ne 0 ]; then
   exit 0
  fi

  sComboText=$(echo $sProgDisplayComboText | tr \! \\n ) 
  sDisplayProg=$(echo  "$sComboText" | zenity  --title="find man pages: use which programme to display results" --list \
    --column "Programme:")
  PROG_EXIT=$?

 fi #  if [ $ONE_ZENITY_DIALOG -eq 0 ]; then
fi  # if [ $($PROG_EXISTS yad/zenity) ]

[ $PROG_EXIT -ne 0 ] && exit 0

OLDIFS="$IFS"
IFS="|"
[ "$bDisplayProgMakeDefault" = "TRUE" ] && registerComboPick $sDisplayProg "${sDisplayProgNames[*]}"  "$sConfigFile" "nDisplayWith" 
[ "$bChooseProgMakeDefault"  = "TRUE" ] && registerComboPick $sChooseProg  "${sChooseProgs[*]}"       "$sConfigFile" "nProgChooseWith"
[ "$bSectionMakeDefault"     = "TRUE" ] && registerComboPick $nSection     "${sSectionsArray[*]}"     "$sConfigFile" "nSection"
IFS="$OLDIFS"

#gnComboPick=0
#getComboPick $sDisplayProg sDisplayProgNames[@]

sDisplayProg=$(echo "$sDisplayProg" | cut -f1 -d ' ') # deal with "man in terminal"
nSection=${nSection:0:1} # first char

###################################################
#get manpages for section and filter

if [ "$nSection" != "A" ]; then
 sSection="-s $nSection"
else
 nSection=0
 sSection=
fi

bTryInfo=0
[ "$sGetInfo" =  "TRUE" ] && bTryInfo=1

nMaxPages=0 
[ "$sChooseProg" = "myGtkMenu" ] && nMaxPages=2000

[ -z $sFilter ] && sFilter="."

nSecsStart=$(($(date +%s%N)/1000000))

readonly sManpages=$(apropos $sSection -r $sFilter |  tr -s '[:blank:]' ' ' | sed '/^ *$/d' | sort ) #  sed  -e 's/- //' -e 's/ -//' | -e 's/ (1)/: /'

msgTiming $nSecsStart "time to accumulate man page data from apropos" # exported from manpageCommon.sh

readonly nManpages=$(echo -n "$sManpages" | wc -l)

if [ "$nManpages" -eq 0 ]; then
 doMsgBox $gsMsgBoxProg "manpageMenu.sh" "No pages found for filter \"$sFilter\"" # <span color='red'></span>
 exit
elif  [ "$nMaxPages" -gt 0 ] && [ "$nManpages" -ge "$nMaxPages" ]; then
 doMsgBox $gsMsgBoxProg "manpageMenu.sh" "Too many pages to display using $sChooseProg"
fi # if [ "$nManpages" -eq 0 ]; then



###################################################

#assume manpageDisplay.sh is in same folder as this script
readonly sDisplayScript="$sDir/manpageDisplay.sh"

###################################################
#set up and run for each possible choice programme

#globals used by prepareItem
gsCurrentLetter=
gnPrefixed=0

gsEntryName=
gsEntryDesc=
gsEntryDesc1=
gsPrefix=

sTitle="man pages"
[ $nSection -ne 0 ]   &&  sTitle="section $nSection $sTitle"
[ "$sFilter" != "." ] &&  sTitle="$sTitle, filtered by \"$sFilter\""

nSecsStart=$(($(date +%s%N)/1000000))

case "$sChooseProg" in 

###################################################
#display men in gtkmenuplus

 gtkmenuplus)
  {
   while read line # -r  If this option is given, backslash does not act as an escape character
    do
     gnSection=$nSection  # get section number from prepareItem in case more than one $sName in same section
     prepareItem "$line"  # exported from manpageCommon.sh
     echo "item=$gsPrefix$gsEntryDesc" 
     echo "cmd=$sDisplayScript $gsEntryName $gnSection $sDisplayProg $bTryInfo"
     echo 
   done <<< "$sManpages" #  here document 
  } | gtkmenuplus -
 ;;

###################################################
#display men in myGtkMenu

 myGtkMenu) # |gtkmenuplus

   gsTempFile=''
   getTempFile ".txt"  # exported from manpageCommon.sh
   sTmpMenuFile=$gsTempFile
   trap "rm $sTmpMenuFile" 1 2 3 15

   while read line # -r  If this option is given, backslash does not act as an escape character
    do 
     gnSection=$nSection   # get section number from prepareItem in case more than one $sName in same section
     prepareItem "$line"   # exported from manpageCommon.sh
     echo "item=$gsPrefix$gsEntryDesc"       >> $sTmpMenuFile
     echo "cmd=$sDisplayScript $gsEntryName $gnSection $sDisplayProg $bTryInfo" >> $sTmpMenuFile
     echo "icon=NULL" >> $sTmpMenuFile;  
     echo  >> $sTmpMenuFile
    done <<< "$sManpages" #  here document 

   msgTiming $nSecsStart "time to send data to $sChooseProg" # exported from manpageCommon.sh

   $sChooseProg $sTmpMenuFile
   rm $sTmpMenuFile
  ;;

###################################################
#display men in pdmenu

  pdmenu)

   gsTempFile=''
   getTempFile ".pdmenu" # exported from manpageCommon.sh
   sTmpMenuFile=$gsTempFile
   trap "rm $sTmpMenuFile" 1 2 3 15

   echo "menu:main:Main Menu:$sTitle" >> $sTmpMenuFile;  

   while read line # -r  If this option is given, backslash does not act as an escape character
    do 
     gnSection=$nSection  # get section number from prepareItem in case more than one $sName in same section
     prepareItem "$line"  # exported from manpageCommon.sh
     echo "   exec:$gsPrefix$gsEntryDesc::./manpageDisplay.sh $gsEntryName $gnSection $sDisplayProg $bTryInfo" >> $sTmpMenuFile
   done <<< "$sManpages" #  here document 

   msgTiming $nSecsStart "time to send data to $sChooseProg"  # exported from manpageCommon.sh

   bIsTerminal # function in manpageCommon.sh
   if [ $? -eq 0 ]; then
    checkVariable "TERMINAL_PROG" $TERMINAL_PROG 
    $TERMINAL_PROG "-x pdmenu $sTmpMenuFile" #  ; read -s -n1 -p 'any key to continue' keyp
   else
    pdmenu  $sTmpMenuFile
   fi # if [ $? -eq 0 ]; then

   rm $sTmpMenuFile
  ;;

###################################################
#display listboxes in yad or zenity 

 yad|zenity)
 
   sProg=$(echo -n "$sManpages" | $sChooseProg --list  --width=800 --height=500 --title "man page search results" --column="$sTitle")
   PROG_EXIT=$?

#  msgTiming $nSecsStart "time to send data to $sChooseProg"  # nah, includes time for user to ponder and click

   [ ! "$PROG_EXIT" = "0" ] &&  exit 0

   sName=$(echo -n "$sProg" | cut -f1 -d ' ')
   gnSection=$nSection 
   if [ $gnSection -eq 0 ]; then
    gnSection=$(echo "$sProg" | sed -r -e  's|^.*\((.*)\).*$|\1|') # get section number in case more than one $sName in same section
   fi
   $sDisplayScript $sName $gnSection $sDisplayProg $bTryInfo
  ;;
  
esac # case "$sChooseProg" in 

exit
