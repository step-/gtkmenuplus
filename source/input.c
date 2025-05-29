/*
Copyright (C) 2024-2025 step https://github.com/step-/gtkmenuplus

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

#include <ctype.h>
#include <gdk/gdk.h>
#include <sys/stat.h>
#include "autoconfig.h"
#include "comment.h"
#include "directive.h"
#include "entry.h"
#include "input.h"

#define WHITESPACE_CHARS  " \t\n\r\f\v"

static gchar gl_directives_sep = ';';

/**
input_set_directive_separator:
Change the character that separates command-line directives.

@sep: separator character.
*/
void
input_set_directive_separator (const gchar sep)
{
 gl_directives_sep = sep;
}

/**
input_set_get_directive_separator:
Get the character that separates command-line directives.

Returns: separator character.
*/
gchar
input_get_directive_separator ()
{
 return gl_directives_sep;
}

/**
input_fetch_directive_from_linetype:
*/
const struct Directive *
input_fetch_directive_from_linetype (const enum LineType type)
{
 for (const struct Directive *d = directive_get_table (); d->keyword; d++)
 {
  if (d->type == type)
  {
   return d;
  }
 }
#ifdef FEATURE_VARIABLE
 assert (FALSE);
#endif
 return NULL;
}

/**
input_fetch_next_directive:
Parse the next directive from the input source.

@isrc: Pointer to struct #InputSource returning the input line.
@entry: pointer to #Entry structure to return the .flags and .index members.
@menu_depth: pointer to integer returning the current menu depth.
@skip_if_body: whether to return directives from `if=`-`endif=` bodies.

Return: pointer to struct #Directive. Invalid directives are silently passed
to the caller (see the #LineType .type member). The raw and cooked input lines
are returned via the @isrc pointer. Empty and whole comment lines are discarded.
*/
const struct Directive *
input_fetch_next_directive (struct InputSource *isrc,
                            struct Entry *entry,
                            guint *menu_depth,
                            gboolean skip_if_body __attribute__((unused)))
{
 gchar *p;
 gint count, r;
 enum LineType type = LINE_UNDEFINED;
 gchar cooked[SIZEOF_COOKED];
 gchar *str1, *str2;
 gsize i, len = 0;
 gchar directive_separator[2] = { 0 };
 const struct Directive *d;

 directive_separator[0] = input_get_directive_separator ();

 while (len == 0)
 {
  if (isrc->fp != NULL)
  {
   /*********************
   *  Read a raw line   *
   *********************/
   isrc->raw[0] = isrc->cooked[0] = '\0';
   if (fgets (isrc->raw, SIZEOF_RAW, isrc->fp) == NULL)
   {
    goto eof;
   }

   /* If the input line overflows the buffer, and the overflow consists
   of an inline comment, chomp the overflowing comment to EOL. */
   {
    for (p = isrc->raw; isspace (*p); p++)
     ;
    if (*p == '#')          /* found a comment */
    {
     p = strchr (p, '\0');
     if (p[-1] != '\n')     /* the comment is inline hence overflowing */
     {
      int c;

      p[-1] = '\n';
      while ((c = fgetc (isrc->fp)) != EOF && c != '\n')
       ;                     /* chomp to EOL */
      if (c == EOF)
      {
       goto eof;
      }
     }
    }
   }

   isrc->lineno++;

   p = strchr (isrc->raw, '\0');
   if (p[-1] != '\n')
   {
    return input_fetch_directive_from_linetype (LINE_BAD_LEN);
   }
   p[-1] = '\0';
  }
  else
  {
   /**********************************************************
   * Or extract the next section of command-line directives  *
   **********************************************************/
   static gchar *pos;

   if (isrc->token == NULL)
   {
    isrc->token = strtok_r (isrc->_buf, directive_separator, &pos);
   }
   else
   {
    isrc->token = strtok_r (NULL, directive_separator, &pos);
   }
   if (isrc->token == NULL)
   {
    goto eof;
   }
   r = snprintf (isrc->raw, SIZEOF_RAW, "%s", isrc->token);
   if (r < 0 || (gsize) r >= SIZEOF_RAW)
   {
    /* the line input buffer would overflow */
    return input_fetch_directive_from_linetype (LINE_BAD_LEN);
   }
  }

  strcpy (cooked, isrc->raw);
  p = (gchar *) comment_find_suffix (cooked);
  if (p != NULL)
  {
   /* Chop the comment suffix from the cooked buffer.
   If needed, the raw buffer still carries it. */
   *p = '\0';
  }

  /* Mark loop termination */
  len = strlen (cooked);

  /* Right trim white space */
  if (len > 0)
  {
   for (p = cooked + len - 1; p >= cooked && isspace (*p); p--, len--)
   {
    *p = '\0';
   }
  }
 }
 if (len >= SIZEOF_RAW)
 {
  snprintf (isrc->cooked, SIZEOF_COOKED, "%s", cooked);
  return input_fetch_directive_from_linetype (LINE_BAD_LEN);
 }

 count = 0;

 {
  /****************
  *  DEPRECATED  *
  ****************/
  /*
  Calculate menu depth from tab indentation.
  4 spaces or 1 tab increase menu depth by 1 level.
  */
  for (i = 0; i < len; i++)
  {
   if (cooked[i] == ' ')
   {
    count += 1;
   }
   else if (cooked[i] == '\t')
   {
    count += 4;
   }
   else
   {
    break;
   }
  }
  *menu_depth = count / 4;
 }

 /*
 Trim leading whitespace by shifting non-whitespace backwards.
 */
 str1 = str2 = cooked;
 while (isspace (*str2))
 {
  str2++, len--;
 }
 for (i = 0; i <= len; i++)
 {
  *str1++ = *str2++;
 }

 /*
 Match valid directives.
 */
 type = LINE_UNDEFINED;
 if (*cooked == G_DIR_SEPARATOR || *cooked == '~')
 {
  strcpy (isrc->cooked, cooked);
  return input_fetch_directive_from_linetype (LINE_ABSOLUTE_PATH);
 }
 {
  for (d = directive_get_table (); d->keyword; d++)
  {
   if (strncasecmp (cooked, d->keyword, d->length) == 0)
   {
    for (p = cooked + d->length; isspace (*p); p++)
     ;
    if (*p == '=' || *p == '\0')
    {
     type = d->type;
     break;
    }
    if (*p == '?' && p[1] == '=')
    {
     entry->flags |= ENTRY_FLAG_QEQ;
     type = d->type;
     break;
    }
    else if (*p == '[')
    {
     gchar *end;
     glong num = strtol (++p, &end, 0);
     if (p < end && *end == ']')
     {
      for (++end; *end && isspace (*end); end++)
       ;
      if (*end == '=')
      {
       type = d->type;
       entry->index = num;
       entry->flags |= ENTRY_FLAG_ARRAY;
       break;
      }
     }
    }
   }
  }
  if (type == LINE_UNDEFINED)
  {
   if (*cooked == '=')
   {
    /* empty LHS */
    gchar *p = cooked;
    type = LINE_ABSOLUTE_PATH;
    do
    {
     p++;
    }
    while (*p && isspace (*p));
    strcpy (isrc->cooked, p);
   }
   else
   {
    type = LINE_SET_VARNAME;
    strcpy (isrc->cooked, cooked);
   }
   return input_fetch_directive_from_linetype (type);
  }
  else
  {
   if (!d->niladic && strchr (cooked, '=') == NULL)
   {
    strcpy (isrc->cooked, cooked);
    return input_fetch_directive_from_linetype (LINE_BAD_NO_EQ);
   }
  }
 }

#ifdef FEATURE_CONDITIONAL
 if (skip_if_body && type != LINE_ELSEIF)
 {
  /* for debugging, redundant otherwise */
  strcpy (isrc->cooked, "(`IF=` BRANCH NOT ENTERED)");
  return input_fetch_directive_from_linetype (type);
 }

#endif
 /*
 Chomp LHS up to `=` and trim both sides
 */
 input_trim_from_end (cooked, cooked + strlen (cooked) - 1);
 isrc->cooked[0] = '\0';
 str1 = strchr (cooked, '=');
 if (str1 == NULL)
 {
  return input_fetch_directive_from_linetype (type);
 }
 str2 = str1 + 1;
 while (isspace (*str2))
 {
  str2++;
 }
 strcpy (isrc->cooked, str2);
 return input_fetch_directive_from_linetype (type);

eof:
 d = input_fetch_directive_from_linetype (LINE_EOF);
 strcpy (isrc->raw, d->keyword);
 return d;
}

/**
input_fetch_current_line:
Return the current, module-owned raw line or token (for directive-list input).

@src: input source.
@lineno: pointer to return the line number.
*/
const gchar *
input_fetch_current_line (const struct InputSource *src,
                          guint *lineno)
{
 assert (src);
 *lineno = src->lineno;
 return src->fp ? src->raw : src->token;
}

/**
input_copy_first_word:
Copy the first (quoted) word to the output buffer:

@in: input string
@out: output buffer

Return: null-terminated, unquoted first word to @out.
*/
void
input_copy_first_word (gchar *out,
                       const gchar *in)
{
 for (; isspace (*in); in++)
  ;
 if ((*in == '\'' || *in == '"'))
 {
  gchar *o, *p, *q = strchr (in + 1, *in);
  if (q != NULL)
  {
   for (o = out, p = (gchar *) in + 1; p < q; o++, p++)
   {
    *o = *p;
   }
   *o = '\0';
  }
 }
 else
 {
  strcpy (out, in);
  strtok (out, WHITESPACE_CHARS);
 }
}

/**
input_source_open:
Open file or stdin or command line directives.

@isrc: pointer to struct #InputSource.
@entry: pointer to return errors.

Return: TRUE/FALSE (error); pointer to #FILE or #stdin
for streams; NULL for command line directives or error.
*/
gboolean
input_source_open (struct InputSource *isrc,
                   struct Entry *entry)
{
 gboolean retval = TRUE;
 int errnum = 0;
 isrc->token = isrc->_buf = NULL;
 isrc->lineno = isrc->raw[0] = 0;
 if (isrc->fname[0] == '-' && isrc->fname[1] == '\0')
 {
  isrc->fp = stdin;
  entry_push_error (entry, ROK, "%s", "reading stdin");
 }
 else
 {
  isrc->fp = fopen (isrc->fname, "r");
  if (isrc->fp == NULL)
  {
   if (strchr (isrc->fname, gl_directives_sep) ||
       (isrc->fname[0] != '.' && strchr (isrc->fname, G_DIR_SEPARATOR) == NULL))
   {
    entry_push_error (entry, ROK, "no file input; assuming directives: %s",
                      isrc->fname);
    isrc->_buf = strdup (isrc->fname);
   }
   else
   {
    errnum = errno;
   }
  }
  else
  {
   struct stat sb;
   if (stat (isrc->fname, &sb) == 0 && S_ISDIR (sb.st_mode))
   {
    fclose (isrc->fp);
    isrc->fp = NULL;
    errnum = EISDIR;
   }
   else
   {
    entry_push_error (entry, ROK, "reading file '%s'", isrc->fname);
   }
  }
 }
 if (errnum)
 {
  retval = FALSE;
  entry_push_error (entry, RFAIL, "%s: %s", isrc->fname, strerror (errnum));
 }
 return retval;
}

/**
input_trim_from_end:
Trim string in-place.
*/
void
input_trim_from_end (gchar *start,
                     gchar *end)
{
 while ((end >= start) && isspace (*end))
 {
  *end = '\0';
  end--;
 }
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
