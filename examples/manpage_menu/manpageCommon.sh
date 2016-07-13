#!/usr/bin/env bash

#displayManPage.sh
#called by manPageMenu.sh, manPageDisplay.sh
#version 1.00, 2013-04-24
#by Alan Campbell

###########################################

#assume .manpageMenu in same folder as manpageCommon.sh
if [ -z $sDir ]; then
 sDir="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )" # http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
fi

sConfigFile=
if [ -f "$HOME/.manpageMenu" ]; then
 sConfigFile="$HOME/.manpageMenu"
elif [ -f "$sDir/.manpageMenu" ]; then
 sConfigFile="$sDir/.manpageMenu"
else
 sConfigFile="$HOME/.manpageMenu"
 if [ -f "$sDir/manpageMenuDefaultConfig" ]; then 
  mv "$sDir/manpageMenuDefaultConfig" "$sConfigFile" # initialise .manpageMenu
 else
  echo "can't find file .manpageMenu or manpageMenuDefaultConfig"
  exit 1
 fi
fi # if [ -f "$sDir/.manpageMenu" ]; then

source "$sConfigFile"  

#http://stackoverflow.com/questions/592620/check-if-a-program-exists-from-a-bash-script
readonly PROG_EXISTS="command -v "
#foo >/dev/null 2>&1 "

###########################################

gsMsgBoxProg=''

function getMsgBoxProg
{
 if [ $($PROG_EXISTS yad) ]; then 
  gsMsgBoxProg=yad
 elif [ $($PROG_EXISTS zenity) ]; then 
  gsMsgBoxProg=zenity
 else
  gsMsgBoxProg=''
 fi
}

###########################################

function doMsgBox
{
 local readonly sMsgBoxProg="$1"
 local readonly sTitle="$2"
 local readonly sText="$3"
 if [ -z $sMsgBoxProg ]; then
  echo "$sTitle=: $sText"   # fallback if neither zenity or yad available
 else
  $sMsgBoxProg --title="$sTitle" --width=250 --height=150  --info --text="$sText"
 fi # if [ -z $sMsgBoxProg ]; then
}

###########################################

function checkVariable 
{
 gsMsgBoxProg=''
 getMsgBoxProg
 
 if [ -z $2 ]; then
  doMsgBox $gsMsgBoxProg "manPagesMenu" "You must define the variable $1 near line 15 in manPagesMenu.sh" 	
  exit
 fi 
}

###########################################

function bIsTerminal
{
#The $$ variable will hold your current process ID, and $PPID contains the parent PID.
#    ps -o stat= -p $PPID wil contain lower case s and
#    ps -o stat= -p $$ wil contain +
#http://stackoverflow.com/questions/4261876/check-if-bash-script-was-invoked-from-a-shell-or-another-script-application

#[ -t 0 ] && terminal=1
#http://ubuntuforums.org/showthread.php?p=9573745
	
 [ -t 0 ] && return 1
 return 0
}

###########################################
#get temp file 
#globals used by getTempFile
#gsTempFile=

function getTempFile()
{
 if [ $($PROG_EXISTS tempfile) ]; then
  gsTempFile=$(tempfile --suffix $1)
 elif [ $($PROG_EXISTS mktemp) ]; then
  gsTempFile=$(mktemp --suffix $1)
 else
  gsTempFile=$TMPDIR/$0.$RANDOM$1
 fi 
}

###################################################
#globals used by checkProgs
#gnProgOption=
#gsProgComboTest=''

function checkProgs()
{
 if [ $gnProgOption -eq $1 ]; then
  gsProgComboTest="!$2$gsProgComboTest"
 else 
  gsProgComboTest="$gsProgComboTest!$2"  
 fi 

 if [ $gnProgOption -eq $NO_OPTION_CHOSEN ]; then
  gnProgOption=$1
 fi 
 return
}

###########################################

function registerComboPick 
{
 local sFind="$1"
#declare -a arItems=("${!2}")
# local arItems=(${2})
 local arItems=( $2 )
# local arItems=( $2 )
# local sConstNamePattern="$4"
 local sConfigFile="$3"
 local sConfigVariable="$4"
 
echo "count ${#arItems[@]}"
echo "${arItems[@]}"
 
 for (( i = 0 ; i < ${#arItems[@]} ; i++ )) do 
   if [ "${arItems[$i]}" = "$sFind" ]; then 
   echo "${arItems[$i]}"
    i=$(($i + 1))
#    if [ -z "$sConstNamePattern" ]; then
#     sConstName="$i"
#    else 
#     sConstName=$(grep "sConstNamePattern.*=$i" "$sConfigFile")
#     sConstName=${sConstName##$sConstNamePattern.*=}
#	 sConstName="\$$sConstName"
#    fi # if [ -z "$sConstNamePattern" ]; then
#echo sConfigVariable "$sConfigVariable"
    sed -i "s/$sConfigVariable=.*/$sConfigVariable=$i/" "$sConfigFile"
    return
   fi
 done
# for arItem in "${arItems[@]}"
# do 
#  if [ "$arItem" = "$sFind" ]; then 
#  fi
# done
 return
}
###########################################

function prepareItem 
{
 local sLine="$1"
 gsEntryName=$(echo $sLine | cut -f1 -d ' ')   # entry name for man
 gsEntryDesc=$(echo $sLine | sed  's/_/-/g')   # get rid of underscores for display

 gsEntryDesc1=${gsEntryDesc:0:1}                        #set up for underscore first two items beginning with a given letter
 if [ "$gsEntryDesc1" != "$gsCurrentLetter" ]; then
  gsPrefix=_
  gsCurrentLetter="$gsEntryDesc1"
  gnPrefixed=1
 else
  if [ $gnPrefixed -ge 2 ]; then
   gsPrefix=
   gnPrefixed=0
  else
   gnPrefixed=2
  fi  # if [ $gnPrefixed -ge 2 ]; then
 fi # if [ "$gsEntryDesc1" != "$gsCurrentLetter" ]; then
 
 if [ $gnSection -eq 0 ]; then  # if all exectuion extract (section no)
  gnSection=$(echo "$gsEntryDesc" | sed -r -e  's|^.*\((.*)\).*$|\1|')
 fi
}

###########################################
#http://askubuntu.com/questions/159369/script-to-find-executable-based-on-extension-of-a-file
#more or less
#have mimeopen installed?
function getDefaultApplication
{
 sExt=$1
 sSampleName=$HOME/~temp.$sExt
 touch $sSampleName
 sCurMine=$(xdg-mime query filetype $sSampleName)
 rm $sSampleName
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

 gsDefaultApplication=$(grep "^Exec" "$sTrueDsk" | head -1 | sed -e 's|Exec=||' -e 's| .*$||' )
}

###########################################

function msgTiming
{
 nSecsStart=$1
 sMsg="$2"
 nSecsEnd=$(($(date +%s%N)/1000000))
#http://serverfault.com/questions/151109/how-do-i-get-current-unix-time-in-milliseconds-using-bash
 echo "$sMsg: $(expr $nSecsEnd - $nSecsStart) msecs"
}

###########################################

function doDisplay
{
 local readonly sDisplayWithDoDisplay="$1"
 local sCmdDoDisplay="$2"
 local readonly nSectionDoDisplay="$3"
 local readonly sNameDoDisplay="$4"
 
 case "$sDisplayWithDoDisplay" in 

  text)

    gsDefaultApplication=
    getDefaultApplication "txt"   # function defined above
    readonly sEditor="$gsDefaultApplication"

    gsTempFile=''
    getTempFile  ".txt"     # function defined above
    sTmpTxtFile=$gsTempFile
    trap "rm $sTmpTxtFile" 1 2 3 15

    if [ "$sCmdDoDisplay" = "info" ]; then
     info --subnodes -o $sTmpTxtFile $sNameDoDisplay 2>/dev/null
     [[ "$EDITORS_NOT_HANDLING_NON_PRINT" =~ ":$sEditor:" ]]  && sed -i 's/[^[:print:]]//g' $sTmpTxtFile  # geany gets in a muddle with non printing characters
    else
#    http://ubuntuforums.org/showthread.php?t=880044
     $( $sCmdDoDisplay  "$sNameDoDisplay" 2>/dev/null > $sTmpTxtFile) 
    fi 
    ( $sEditor $sTmpTxtFile; rm -f $sTmpTxtFile) &
  ;;
    
  yelp)

    sCmdDoDisplay="yelp man:$sNameDoDisplay"
    [ "$nSectionDoDisplay" -ne "0" ] &&  sCmdDoDisplay="$sCmdDoDisplay\($nSectionDoDisplay\)"
    eval $sCmdDoDisplay &
   ;;

  html)

    gsDefaultApplication=
    getDefaultApplication "html"   # function defined above
    readonly sBrowser="$gsDefaultApplication"

    if [ $($PROG_EXISTS man2html) ]; then
     url="http://localhost/cgi-bin/man/man2html?query="
     [ "$nSectionDoDisplay" -ne "0" ] && url="$url$nSectionDoDisplay+"
     $sBrowser "$url$sNameDoDisplay" &
    else
      sCmdDoDisplay="$sCmdDoDisplay --html=$sBrowser "
      $($sCmdDoDisplay $sNameDoDisplay &) 
    fi  # if [ $($PROG_EXISTS man2html) ]; then
   ;;

  pdf)

    gsDefaultApplication=
    getDefaultApplication "pdf"  # function defined above
    readonly sPdfReader="$gsDefaultApplication"
    
    gsTempFile=''
    getTempFile ".pdf"     # function defined above
    sTmpPdfFile=$gsTempFile
    trap "rm $sTmpPdfFile" 1 2 3 15
    $( $sCmdDoDisplay -t "$sNameDoDisplay" | ps2pdf - $sTmpPdfFile ) 
    ( $sPdfReader $sTmpPdfFile;  rm -f $sTmpPdfFile ) &
  ;;
 
  man)

    bIsTerminal # function in manpageCommon.sh
    PROG_EXIT=$?

    if [ $PROG_EXIT -eq 1 ]; then 
     eval "$sCmdDoDisplay $sNameDoDisplay | less" # only happens if pdmenu used for choice; otherwise this script run from a GTK+ app
    else 
     checkVariable "TERMINAL_PROG" $TERMINAL_PROG 
     $TERMINAL_PROG -x $sCmdDoDisplay $sNameDoDisplay  | less # $TERMINAL_PROG exported from displayManPage.sh
    fi 
  ;;
 
 esac
}
