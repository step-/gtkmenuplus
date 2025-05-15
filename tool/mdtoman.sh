#!/bin/sh

lowdown=${LOWDOWN:-lowdown}
if ! command -v $lowdown >/dev/null; then
	echo "${0##/*}: please install the lowdown package (>= 1.4.0)" >&2
	exit 1
fi
ver="$($lowdown --version)"
ver=${ver##* }
case "$ver" in 0*|1.[0123]*)
	echo "${0##/*}: please upgrade the lowdown package (>= 1.4.0)" >&2
	exit 1
esac
if [ "x$1" = x-h -o "x$1" = x--help -o -z "$1" ]; then
	echo "usage: $0 [lowdown_options] markdown_file"
	echo "convert markdown_file to troff(1) manpage on stdout."
	exit
fi

$lowdown -st man "$@" | gawk '

###gawk
BEGIN {
	OUTPUT_FOR_TERM = 0
}

### Make code spans stand out
# Wrap Courier-Roman text within backticks (a la Markdown) to better
# distinguish code spans on terminals that cannot change the font.
OUTPUT_FOR_TERM && /\\f\(CR/ {
	s = $0
	while (sub(/\\f\(CR/, ":<{", s) && sub(/\\fR/, "}>:", s))
		;
	gsub (/:<{/, "`\\f(CR", s)
	gsub (/}>:/, "\\fR`", s)
	$0 = s
}

# ### Fix sections for lowdown 0.4.1
# 1 == NR && /^\.SH / { sub(/S/, "T"); print; next }
# /^\.SS / {
# 	sub(/SS/, "SH"); print; next
# }
# /^\.ft CR$/ {
# 	print "\\fC"; ftCR = 1; next
# }
# /^\.ft$/ && ftCR {
# 	print "\\fR"; ftCR = 0; next
# }
# ftCR {
# 	print; next # preformatted
# }
#
# {
# 	# gsub(/\\f\[CR\]/, "\\fC")
# 	# gsub(/\\f\[R\]/, "\\fR")
# 	# gsub(/\\f\[B\]/, "\\fB")
# 	# gsub(/\\f\[I\]/, "\\fI")
# 	# gsub(/\\\(en/, "--")
# }

### Sanity fallback for Markdown tables.
# This block is only entered if Markdown tables are indented,
# which prevents lowdown from parsing a table. You should not
# indent Markdown tables.
#
# All lines that start with /^\|/ are considered table rows,
# and are rendered as a code block (.EX).
/^\|/ {
	if (!within_table) {  # table header
		print ".EX"
		print
		len = split ($0, ch, "")
		for (i = 1; i <= len; i++) {
			if (ch[i] == "|") {
				printf "%s", ch[i]
			} else {
				printf "\\-"
			}
		}
		print ""
		getline      # chomp table header underline
		within_table = 1
		next
	}
}
within_table && !/^\|/ {
	print ".EE"
	within_table = 0
}

{
	print
}

###gawk'

