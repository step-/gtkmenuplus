#!/bin/sh

pandoc=${PANDOC:-pandoc}
if ! command -v $pandoc >/dev/null; then
	echo "${0##/*}: please install pandoc" >&2
	exit 1
fi
if [ "x$1" = x-h -o "x$1" = x--help -o -z "$1" ]; then
	echo "usage: $0 man1_manpage"
	echo "convert troff(1) manpage to markdown on stdout."
	exit
fi

$pandoc -fman -tcommonmark "$1" |

	# Fix links within angular brackets
	sed '
		/\\<[a-z].*[^ *]\\>/ { s/\\</</g; s/\\>/>/g }
		/\\<\*[a-z].*[^ *]\*\\>/ { s/\\<\*/</g; s/\*\\>/>/g }
	'
