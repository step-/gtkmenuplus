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
    find . -perm +111 -type f -name \*.gtkmenuplus |
    sed 's/..//' |
    yad --geometry 500x500+60+60 \
      --list --listen --column="Test file:TEXT" \
      --dclick-action="sh -c '>&2 which $GTKMENUPLUS; unset TESTFILE; \"$GTKMENUPLUS\" \"\$0\"'"
    ;;
  tree) # $2-DIR
    tree -a "$2" |
    yad --geometry 500x500+30+30 \
      --text-info
    ;;
 esac
