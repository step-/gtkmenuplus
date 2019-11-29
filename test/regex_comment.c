/* test harness for ../source/regex_comment.c */

/*
#define TEST_ALL_SHARPS
#ifndef TEST_ALL_SHARPS
#define TEST_LEFTMOST_SHARP
#endif
*/

/*
cc -DTEST_ALL_SHARPS -g -c -o source-regex_comment.o ../source/regex_comment.c
cc -DTEST_ALL_SHARPS -g -c -o regex_comment.o regex_comment.c
cc -g -o regex_comment regex_comment.o source-regex_comment.o

# OR

cc -DTEST_LEFTMOST_SHARP -g -c -o source-regex_comment.o ../source/regex_comment.c
cc -DTEST_LEFTMOST_SHARP -g -c -o regex_comment.o regex_comment.c
cc -g -o regex_comment regex_comment.o source-regex_comment.o
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

extern char * gl_sSharpIsntComment;
extern int compile_regex (regex_t *, const char *, int);
extern const char * find_comment (const char *, regex_t *);

int main (int argc, char ** argv)
{
	regex_t r;
	const char * regex_text = gl_sSharpIsntComment;
	const char * find_text  = sTestCase;
	const char * comment;
	printf ("Trying to find\n%s\nin\n%s\n", regex_text, find_text);
	printf (" 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789\n");
	compile_regex (& r, regex_text, REG_EXTENDED|REG_ICASE|REG_NEWLINE);
	comment = find_comment (find_text, & r);
#ifdef TEST_LEFTMOST_SHARP
	if (comment) {
		printf ("# COMMENT FOUND (((%s)))\n", comment);
	}
#endif
	regfree (& r);
	return 0;
}
