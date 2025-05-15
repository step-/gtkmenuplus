/* test harness for "sharp isn't comment" regex in source/comment.c */

/*
#define TEST_ALL_SHARPS
#ifndef TEST_ALL_SHARPS
#define TEST_LEFTMOST_SHARP
#endif
*/

/*
Compile in top directory:

cc -DTEST_ALL_SHARPS -g -Isource -o test/regex_comment test/regex_comment.c source/comment.c

# OR

cc -DTEST_LEFTMOST_SHARP -g -Isource -o test/regex_comment test/regex_comment.c source/comment.c
*/

static char * sTestCase =
"cmd=/bin/echo #four (test) [2,yad --text=:mode=exe]"
"\ncmd = echo a#b"
"\n\\##\\##:comment \\#:not-comment #:comment '#ffffff':not-comment"
"\ncmd = #:not-comment"
"\nname == #:not-comment"
"\nname = '#:not-comment'"
"\ncolor = \"#ffffff\" '#eee'"
"\nif = [ -x '#not-comment' ]"
"\n#:comment1 #:comment2 \\#:not-comment #:comment \"#ffffff\":not-comment #last-comment"
;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "comment.h"

int main (int argc, char ** argv)
{
	regex_t r;
	const char *pattern = comment_get_pattern_sharp_isnt_comment ();
	const char *subject = sTestCase;
	const char *comment;
	printf ("=====\nMatching pattern:\n%s\nIn subject:\n%s\n=====\n", pattern, subject);
	printf (" 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789\n");
	comment_module_init (REG_NEWLINE);
	comment = comment_find (subject);
#ifdef TEST_LEFTMOST_SHARP
	if (comment) {
		printf ("# COMMENT FOUND (((%s)))\n", comment);
	}
#endif
	comment_module_finalize ();
	return 0;
}
