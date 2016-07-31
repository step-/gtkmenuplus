#!/bin/sh -x

# All relative paths are based in the configuration file directory.
if ! cd "${TESTFILE%/*}"; then
  yad --title 'ERROR' --window-icon=gtk-no \
    --text "Script doesn't define variable mTESTFILE. Quitting."
  exit 1
fi

GTKMENUPLUS=${GTKMENUPLUS:-gtkmenuplus}
YAD_OPTIONS="--title '$1 - $TESTFILE' --window-icon=/usr/share/icons/hicolor/scalable/apps/gtkmenuplus.svg"
export YAD_OPTIONS

case $1 in
  list)
    find . -perm +111 -type f -name \*.gtkmenuplus |
    sed 's/..//' |
    yad --geometry 500x500+60+60 \
      --list --listen --column="Test file:TEXT" \
      --dclick-action="sh -c 'set -x; unset TESTFILE; \"$GTKMENUPLUS\" \"\$0\"'"
    ;;
  tree) # $2-DIR
    tree -a "$2" |
    yad --geometry 500x500+30+30 \
      --text-info
    ;;
 esac