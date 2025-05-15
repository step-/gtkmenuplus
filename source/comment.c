/*
Copyright (C) 2019,2024-2025 step https://github.com/step-/gtkmenuplus

Licensed under the GNU General Public License Version 2
-------------------------------------------------------------------------

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License, version 2, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 59
Temple Place, Suite 330, Boston, MA 02111-1307 USA
-------------------------------------------------------------------------
*/
/* *INDENT-OFF* */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "autoconfig.h"
#include "comment.h"
#include "common.h"

/* Maximum number of matches returned by match_regex. */
#define MAX_MATCHES 20

/* POSIX ERE pattern for directives that include '#' but aren't comments */
static const char *gl_pat_sharp_isnt_comment =
""  "^[ \t]*cmd[ \t]*=.+$"                               /* cmd= */
#ifdef FEATURE_CONDITIONAL
"|" "^[ \t]*(else)?if[ \t]*=.+$"                         /* if= | elseif= */
#endif
#ifdef FEATURE_VARIABLE
"|" "^[ \t]*[[:alnum:]_]+[ \t]*==.+$"                    /* variable_name== */
"|" "^[ \t]*[[:alnum:]_]+[ \t]*=[ \t]*\"[^\"]*#[^\"]*\"" /* variable_name="...#..." */
"|" "^[ \t]*[[:alnum:]_]+[ \t]*=[ \t]*'[^']*#[^']*'"     /* variable_name='...#...' */
#endif
"|" "\\\\#"                                              /* escaped # */
"|" "\"#([[:xdigit:]]{3}){1,2}\""                        /* " quoted HTML color spec - short and long forms */
"|" "'#([[:xdigit:]]{3}){1,2}'"                          /* ' quoted HTML color spec - short and long forms */

"|" "&#[[:digit:]]{1,6};"                                /* decimal HTML entity */
"|" "&#x[[:xdigit:]]{1,4};"                              /* hex HTML entity */
;
static regex_t gl_rex_sharp_isnt_comment;
#if defined(TEST_ALL_SHARPS) || defined(TEST_LEFTMOST_SHARP)
const char*
comment_get_pattern_sharp_isnt_comment (void)
{
 return gl_pat_sharp_isnt_comment;
}
#endif

#ifdef TEST_ALL_SHARPS
/* Find all sharps within positions ["start"..."end"] of string "s" */
static char*
match_sharp (const char *s, const int start, const int end)
{
 printf ("match_sharp[%d:%d]: ", start, end);
 for (char *p = (char *)s + start; p <= s + end; p++)
 {
  printf ("%c", *p);
  if ('#' == *p)
  {
   printf ("[%d]", (p - s));
  }
 }
 printf ("\n");
 return NULL;
}
#else
/* Find the leftmost sharp within positions ["start"..."end"] of string "s" */
static char*
match_sharp (const char *s, const int start, const int end)
{
 for (char *p = (char *)s + start; p <= s + end; p++)
 {
  if ('#' == *p)
  {
   return p;
  }
 }
 return NULL;
}
#endif

/**
match_regex:
Match @to_match against the compiled regular expression @r.
Then match '#' inside the non-matched segments of @to_match.

@r: pointer to POSIX ERE regex_t.
@to_match: subject string.

Returns: pointer to the match or NULL pointer if no match.
*/
static char*
match_regex (regex_t *r, const char *to_match)
{
 const char *p = to_match;         /* end of previous match */
 const int  n_matches = MAX_MATCHES;
 regmatch_t m[n_matches];

 while (1)
 {
  int i = 0;
  int nomatch = regexec (r, p, n_matches, m, 0);
  if (nomatch)
  {
#if defined(TEST_ALL_SHARPS) || defined(TEST_LEFTMOST_SHARP)
   printf ("No more matches.\n");
#endif

   /* look for # inside the string tail */
   const char *q = p;
   while (*q)
   {
    ++q;
   }
   return match_sharp (to_match, p - to_match, q - to_match - 1);

  }
  for (i = 0; i < n_matches; i++)
  {
   if (m[i].rm_so == -1)
   {
    break;
   }
   int             start = m[i].rm_so + (p - to_match);
#if defined(TEST_ALL_SHARPS) || defined(TEST_LEFTMOST_SHARP)
   int finish = m[i].rm_eo + (p - to_match);
   if (i == 0)
   {
    printf ("NOT COMMENT is ");
   }
   else
   {
    printf ("$%d is ", i);
   }
   printf ("'%.*s' (bytes %d:%d) (look in %ld:%d)\n", (finish - start),
           to_match + start, start, (finish - 1),
           (p - to_match), (start - 1));
#endif

   /* look for # inside range (previous end... current start) */
   char *sharp = match_sharp (to_match, p - to_match, start - 1);
   if (sharp)
   {
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

/**
comment_find_suffix:
Return a pointer to an inline comment suffix (the line ends with the comment).
*/
const gchar *
comment_find_suffix (const gchar *subject)
{
 while (' ' == *subject || '\t' == *subject)
 {
  ++subject;
 }
 switch (*subject)
 {
  case '\0': return NULL;
  case '#':  return subject;
 }
 if (strchr (subject, '#') == NULL)
 {
  return NULL;
 }
 return match_regex (&gl_rex_sharp_isnt_comment, subject);
}

/**
comment_module_init:
Initialize this module.

@cflags: additional cflags passed to regcomp(3).

Returns: 0(OK) -1(Error).
*/
int
comment_module_init (const int cflags)
{
 if (compile_regex (&gl_rex_sharp_isnt_comment, gl_pat_sharp_isnt_comment,
                    REG_EXTENDED | REG_ICASE | cflags) != 0)
 {
  return -1;
 }
 return 0;
}

/**
comment_module_finalize:
Clean up this module. Call before exit(2).
*/
void
comment_module_finalize (void)
{
 regfree (&gl_rex_sharp_isnt_comment);
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
