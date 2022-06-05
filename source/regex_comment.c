
/* The following POSIX ERE regular expression matches constructs that include
the sharp character but aren't comments. */

const char * gl_sSharpIsntComment =
""  "^[ \t]*cmd[ \t]*=.+$"                               // cmd=
#if                                                      !defined(_GTKMENUPLUS_NO_IF_)
"|" "^[ \t]*(else)?if[ \t]*=.+$"                         // if= | elseif=
#endif
#if                                                      !defined(_GTKMENUPLUS_NO_VARIABLES_)
"|" "^[ \t]*[[:alnum:]_]+[ \t]*==.+$"                    // variable_name==
"|" "^[ \t]*[[:alnum:]_]+[ \t]*=[ \t]*\"[^\"]*#[^\"]*\"" // variable_name="...#..."
"|" "^[ \t]*[[:alnum:]_]+[ \t]*=[ \t]*'[^']*#[^']*'"     // variable_name='...#...'
#endif
"|" "\\\\#"                                              // escaped #
"|" "\"#([[:xdigit:]]{3}){1,2}\""                        // " quoted HTML color spec - short and long forms
"|" "'#([[:xdigit:]]{3}){1,2}'"                          // ' quoted HTML color spec - short and long forms

"|" "&#[[:digit:]]{1,6};"                                // decimal HTML entity
"|" "&#x[[:xdigit:]]{1,4};"                              // hex HTML entity
;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

/* The following is the size of a buffer to contain any error messages
encountered when the regular expression is compiled. */

#define MAX_ERROR_MSG 0x1000

/* This is the maximum number of matches that we can handle */

#define MAX_MATCHES 20

#ifdef TEST_ALL_SHARPS

/* Find all sharps within positions ["start"..."end"] of string "s" */

static char * match_sharp (const char * s, const int start, const int end)
{
	printf ("match_sharp[%d:%d]: ", start, end);
	for (char * p = (char*) s + start; p <= s + end; p++) {
		//printf (" %d(%c)", p - s, *p);
		printf ("%c", *p);
		if ('#' == *p) {
			printf ("[%d]", (p - s));
		}
	}
	printf("\n");
	return NULL;
}

#else /* TEST_ALL_SHARPS */

/* Find the leftmost sharp within positions ["start"..."end"] of string "s" */

static char * match_sharp (const char * s, const int start, const int end)
{
	for (char * p = (char*) s + start; p <= s + end; p++) {
		if ('#' == *p) {
			return p;
		}
	}
	return NULL;
}

#endif /* TEST_ALL_SHARPS */

/* Compile the regular expression described by "regex_text" into "r". */

int compile_regex (regex_t * r, const char * regex_text, int cflags)
{
	int status = regcomp (r, regex_text, cflags);
	if (status != 0) {
		char error_message[MAX_ERROR_MSG];
		regerror (status, r, error_message, MAX_ERROR_MSG);
		fprintf (stderr, "Regex error compiling '%s': %s\n", regex_text, error_message);
		return status;
	}
	return 0;
}

/* Match the string in "to_match" against the compiled regular expression in "r".
Then look for sharps within the non-matched segments of "to_match". */

static char * match_regex (regex_t * r, const char * to_match)
{
	/* "p" is a pointer into the string which points to the end of the previous match. */
	const char * p = to_match;
	/* "n_matches" is the maximum number of matches allowed. */
	const int n_matches = MAX_MATCHES;
	/* "m" contains the matches found. */
	regmatch_t m[n_matches];

	while (1) {
		int i = 0;
		int nomatch = regexec (r, p, n_matches, m, 0);
		if (nomatch) {
#if defined(TEST_ALL_SHARPS) || defined(TEST_LEFTMOST_SHARP)
			printf ("No more matches.\n");
#endif

			/* look for sharp within the string tail */

			const char * q = p;
			while (*q) {
				++q;
			}
			return match_sharp (to_match, p - to_match, q - to_match - 1);

		}
		for (i = 0; i < n_matches; i++) {
			if (m[i].rm_so == -1) {
				break;
			}
			int start = m[i].rm_so + (p - to_match);
#if defined(TEST_ALL_SHARPS) || defined(TEST_LEFTMOST_SHARP)
			int finish = m[i].rm_eo + (p - to_match);
			if (i == 0) {
				printf ("NOT COMMENT is ");
			}
			else {
				printf ("$%d is ", i);
			}
			printf ("'%.*s' (bytes %d:%d) (look in %ld:%d)\n", (finish - start),
				to_match + start, start, (finish - 1),
				(p - to_match), (start - 1));
#endif

			/* look for sharp within position range (previous end... current start) */
			char *sharp =  match_sharp (to_match, p - to_match, start - 1);
			if (sharp) {
#ifdef TEST_LEFTMOST_SHARP
				printf ("# COMMENT FOUND [%ld]\n", sharp - to_match);
#else
				return sharp;
#endif
			}
		}
		p += m[0].rm_eo;
	}
	return NULL;
}

const char * find_comment (const char * s, regex_t * rp)
{
	/* trivial cases */
	while (' ' == *s || '\t' == *s) {
		++s;
	}
	switch(*s) {
		case '\0': return NULL;
		case '#':  return s;
	}
	if (NULL == strchr(s, '#')) {
		return(NULL);
	}

	/* big gun */
	return match_regex(rp, s);
}

