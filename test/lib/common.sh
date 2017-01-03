#!/bin/sh

# All relative paths of gtkmenuplus directives are based in the
# configuration file directory.
if [ -z "$TESTFILE" ]; then
  yad --title 'ERROR' --window-icon=gtk-no \
    --text "Script doesn't define variable mTESTFILE. Quitting."
  exit 1
fi

GTKMENUPLUS=${GTKMENUPLUS:-../source/gtk2/gtkmenuplus}
YAD_OPTIONS="--title '$1 - $TESTFILE' --window-icon=/usr/share/icons/hicolor/scalable/apps/gtkmenuplus.svg"
export YAD_OPTIONS

case $1 in
  list)
    stat --printf="%n\t%y\t%a\n" *.gtkmenuplus |
    awk -F '\t' '
$3 ~ /^[75]/ {
  filename = $1
  print filename # OUTPUT
  sub(/\.[^.]+$/, "\n&", $1)
  print $1 # NAME & TYPE
  print substr($2,1,index($2,".")-1) # MODIFIED

  # GIT
  cmd = "git log --pretty=%cdX%h --date=format:%Y-%m-%d\" \"%H:%M:%S --reverse --abbrev-commit -- " filename
  cmd | getline gitlog
  close(cmd)
  if("" == gitlog) gitlog = "X"
  sub(/X/, "\n", gitlog)
  print gitlog # GIT FIRST COMMIT DATE & HASH
  
  # DESCRIPTION
  desc = ""
  while(0 < (getline s < filename)) {
    if(s ~ /^#%/) {
      sub(/^#% */, "", s) # leave <TAB> available for indenting
      desc = desc (desc ? " " : "") s
    }
  }
  close(filename)
  gsub(/</, "\\&lt;", desc)
  print desc # DESCRIPTION
}' |
    yad --geometry 800x500+60+60 \
      --list --listen \
      --column="OUTPUT:HD" --print-column=1 --separator="" \
      --column="NAME" \
      --column="TYPE:HD" \
      --column="MODIFIED" \
      --column="FIRST COMMIT" \
      --column="HASH" \
      --column "DESCRIPTION:TIP" --tooltip-column=7 \
      --dclick-action="sh -c '>&2 which $GTKMENUPLUS; unset TESTFILE; \"$GTKMENUPLUS\" \"\$0\"'"
    ;;
  tree) # $2-DIR
    tree -a "$2" |
    yad --geometry 500x500+30+30 \
      --text-info
    ;;
 esac
