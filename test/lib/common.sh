#!/bin/sh

# Usage: env ENVIRONMENT list|tree|single-test
# ENVIRONMENT:
# TESTFILE=[/path/to/]SCRIPT.gtkmenuplus       REQUIRED.
# GTKMENUPLUS=[/path/to/]gtkmenuplus
# GTKMENUPLUS_OPTIONS=...               Instead add a special comment to SCRIPT.
# YAD=[/path/to/]yad

# With YAD >= 14.0 (yadu) you can type a regex search pattern in the main
# window. Escape spaces with backslash.

# test/lib/common.gtkmenuplus runs this file as a shell script.

die () {
  echo "$0: $*" >& 2
  if ! [ -t 0 ]; then
    ${YAD:-yad} --title 'ERROR' --window-icon=gtk-no --text="$*"
  fi
  exit 1
}

die_if_no_testfile () {
  # gtkmenuplus script variable mTESTFILE <=> environment variable TESTFILE
  [ -n "$TESTFILE" ] || die "Script doesn't define variable mTESTFILE. Quit."
  [ -e "$TESTFILE" ] || die "$TESTFILE: file not found. Quit."
  [ -d "$TESTFILE" ] && die "$TESTFILE: is a directory. Quit."
}

get_screen_width_height () {
  if [ -e /sys/class/graphics/fb0/virtual_size ]; then
    read screen_width < /sys/class/graphics/fb0/virtual_size
    screen_height="${screen_width#*,}"
    screen_width="${screen_width%,*}"
  else
    set -- $(xwininfo -root | awk -F: '
    /Width/  {w = $2; next}
    /Height/ {h = $2; exit}
    END      {printf "%d %d", w, h}')
    screen_height="${1:-0}" screen_width="${1:-0}"
  fi
}

commit_summary() { # $@-absolute-pathnames
  {
  # awk record 1
  printf "%s\n" "$@"
  echo                # end of awk record

  git log --pretty=format:'%h %ai' --name-only --reverse -- "$@"

  } | awk -v GITROOT="$GITROOT" '
###gawk:  asorti()

BEGIN { RS=""; FS="\n" }

### preload pathnames (from printf)
NR == 1 {
  for (i = 1; i <= NF; i++) {
    P[$i] = $i
  }
  next
}

### load and sort out git information (from git log)
{
  #
  # commit info
  #

  nI = split($1, I, " ")
  hash = I[1]
  date = I[2];     gsub(/-/, "", date)
  time = I[3]I[4]; time =""

  #
  # files in this commit
  #

  # use absolute path as key into arrays P (pathname) F (first commit), L (last commit), C (commit counter), H (first short hash)
  for (i = 2; i <= NF; i++) {
    k = GITROOT "/" $i
    P[k] = k
    if (!(k in C)) {
      F[k] = date" "time
    }
    L[k] = date" "time
    C[k]++
    H[k] = hash
  }
}

END {
  OFS = "\t"
  nK = asorti(P, K)
  for (i = 1; i <= nK; i++) {
    k = K[i]
    print k, C[k]+0, F[k], L[k], H[k]
  }
}
###gawk'
}

cd_to_base_dir () {
  # Make it so that all relative paths of gtkmenuplus
  # directives are based in the "test/" script directory.
  cd "${0%/lib/common.sh}"
}

####### MAIN #######

if [ -n "$GTKMENUPLUS" ]; then
  x="$GTKMENUPLUS"
  if [ "${x#/}" == "$x" ]; then
    die "Error: GTKMENUPLUS='$x' used but GTKMENUPLUS requires an absolute path"
  fi
else
  if ! x="$(command -v gtkmenuplus)"; then
    die "Error: gtkmenuplus program not found"
  fi
fi
export GTKMENUPLUS="$x"

YAD_OPTIONS="--title '$1 - $GTKMENUPLUS $GTKMENUPLUS_OPTIONS $TESTFILE'
--buttons-layout=spread
--window-icon=gtkmenuplus
--regex-search"
export YAD_OPTIONS

GITROOT="$(git -C "${0%/*}" rev-parse --show-toplevel)"

case $1 in

  list)
    yadver="$(${YAD:-yad} --version)"
    if [ -z "$yadver" ] || [ "${yadver%%.*}" -lt 14 ]; then
      m="error: required yad version >= 14 not found"
      [ -f /etc/fatdog-version ] && m="$m
please install the 'yad_ultimate' package then run YAD=yadu test/START"
      die "$m"
    fi
    die_if_no_testfile
    get_screen_width_height
    cd_to_base_dir &&
    commit_summary "$GITROOT/test/"*.gtkmenuplus |
    awk -F '\t' '
###awk
{
  abspath = dirpath = filename = $1
  sub(/^.*\//, "", filename)
  sub(/\/[^/]+$/, "", dirpath)

  ext = name = filename
  sub(/\.[^.]+$/, "", name)
  sub("^"name, "", ext)

  commits  = $2
  first_date = substr($3, 3)
  last_date = substr($4, 3)
  hash = $5

  fixme = skip_json = 0
  while (0 < (getline s < abspath)) {
    if (index(s, "FIXME")) {
      ++fixme
    }
    if (index(s, "SKIP_JSON")) {
      skip_json = 1
    }
  }
  close (abspath)

  jsonf = dirpath "/reference/" name ".json"
  has_jsonf = (0  < (getline s < jsonf))
  close(jsonf)

  size = 0
  desc = ""
  while(0 < (getline s < abspath)) {
    size += length($0)
    if(1 == index(s, "#%")) {
      sub(/^#% */, "", s) # leave <TAB> available for indenting
      desc = desc (desc ? " " : "") s
    }
  }
  close(abspath)
  gsub(/&/,"\\&amp;", desc)
  gsub(/</, "\\&lt;", desc)
  gsub(/>/, "\\&gt;", desc)

  # yad column values
  print abspath    # "OUTPUT" (hidden)
  print fixme ? ("<span color=\"red\">"fixme"</span>") : "" # "fail"
  print skip_json ? "" : (has_jsonf ? "☒" : "☐") # "json"

  # Concatenate all the remaining fields into a single visible column
  # (c). Yad resizes the last visible column automatically, making it
  # as wide as the window can fit. Also assemble a hidden column (t),
  # without Pango Markup, for tooltips and searching.
  c = B(name)
  t = name
  c = c " @d " desc " @s " size
  t = t " @d " desc " @s " size
  c = c " @c " commits " @h " hash " @l " last_date " @f " B(first_date)
  t = t " @c " commits " @h " hash " @l " last_date " @f " first_date
  # c = c " @t " ext
  # t = t " @t " ext
  print c
  print t
}
function B(s) {
  return "<span font-weight=\"800\">" s "</span>"
}
###awk' |
    env YAD_OPTIONS="$YAD_OPTIONS
      --geometry $(($screen_width/2))x$(($screen_height-60))+$(($screen_width/2))+60
      --list --listen --separator=\"\" --simple-tips
      --column=\"OUTPUT:HD\" --print-column=1
      --column=\"<span color='red'>F</span>!number of #FIXME comments in the file:TEXT\"
      --column=\"J!this test menu is serialized as JSON in ./reference/:TEXT\"
      --column=\"(hover)!name @d(escription - in-file '#%' comments) @s(ize) @c(ommits) @h(ash7) @l(ast commit) @f(irst commit):TEXT\"
      --search-column=3 --ellipsize-cols=3 --ellipsize=END
      --column=:HD --tooltip-column=4 --search-column=4
      " ${YAD:-yad} \
        --dclick-action="bash -c 'printf \"\n\033[7m  %40s  \033[0m\n\" \"$(echo "$(printenv GTKMENUPLUS GTKMENUPLUSOPTIONS)")\"; (env TESTFILE= YAD_OPTIONS= $GTKMENUPLUS \$(awk -F= '\''/^# GTKMENUPLUS_OPTIONS=/{\$1=\"\";print}'\'' \$0) $GTKMENUPLUS_OPTIONS -f \$0; echo EXIT_STATUS=\$?) &'"
            # Yad passes the test script file path, which becomes the shell parameter $0 above.
            # We use bash instead of sh because only bash prints the "Segmentation fault" error message.
            # We use option -f to allow running concurrent tests.
    ;;

  single-test)
    die_if_no_testfile
    cd_to_base_dir
    case $TESTFILE in
            /*) testf="$TESTFILE" ;;
      */test/*) testf="${TESTFILE#*/test/}" ;;
        test/*) testf="${TESTFILE#test/}" ;;
    esac
    echo $(printenv GTKMENUPLUS GTKMENUPLUS_OPTIONS) >&2
    exec env TESTFILE= $GTKMENUPLUS $GTKMENUPLUS_OPTIONS -f "$testf"
  ;;

  tree) # $2-DIR      -- only the test scripts call this block
    tree -a "$2" |
    ${YAD:-yad} --geometry 500x500+30+30 --text="tree of '$(realpath "$2")'" \
      --text-info --button="_Open folder!folder:xdg-open $2"
    ;;

  *)
    cat <<- EOF >&2
    usage: env [GTKMENUPLUS_OPTIONS=...] TESTFILE=[path/to/]test.gtkmenuplus $0 list|single-test
    usage: env [YAD=...] $0 tree DIRECTORY
EOF
    ;;
 esac
