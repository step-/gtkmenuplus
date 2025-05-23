#!/bin/sh

# Run by: The "write reference file" menu entry in ./common.gtkmenuplus.
# Purpose: clean up the JSON serialization of a menu.

usage="gtkmenuplus -j script.gtkmenuplus > ref.json && $0 ref.json"
set -e
RF="${1:?"$usage"}" # the reference file - a JSON file
[ -f "$RF" ]
gtkmenuplus="${GTKMENUPLUS:-$(which gtkmenuplus)}"

# Awk helper to allow redirecting jq output to the input file.
# The jq script is passed as variable JQ. The script deletes the "test tools"
# submenu and the .cmdline array to enhance portability of the reference file.
#
awk '###awk
{
	R[NR] = $0

	# Make the reference file portable by replacing some absolute
	# paths specific to the developer`s system with generic templates.
	gsub (PWD, "{pwd}", R[NR])
	gsub (GTKMENUPLUS, "{gtkmenuplus}", R[NR])
}
END {
	for (i=1; i<= NR; i++) print R[i] | JQ
	exit close(JQ) # empty input and invalid JSON are errors
}
###awk' \
	"PWD=$PWD" "GTKMENUPLUS=${gtkmenuplus:?}" \
	\
	"JQ=jq 'del(.children[] | select(.label == \"_test tools\"))
	| .count -= 1 | del(.cmdline)' > '$RF'" "$RF"
