/*
gtkmenuplus - Generate a GTK popup menu from text directives.

Copyright (C) 2016-2025 step https://github.com/step-/gtkmenuplus

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

/* fatal error (out of memory,etc) */
#define uthash_fatal(msg) do { \
 fprintf (stderr, "uthash: %s\n", (msg)); exit (-1); \
} while (0);

#include "launcher_cache.h"
#include "entry.h"
#include "uthash.h"

struct mec
{
    char *key;
    struct Entry *value;
    UT_hash_handle hh;
};

struct sdc
{
    char *key;
    struct Scandir *value;
    UT_hash_handle hh;
};

static struct mec *gl_mec = NULL;    /* cache of Entry entries */
static struct sdc *gl_sdc = NULL;    /* cache of scandir lists */
static guint gl_sdc_signature = 0;

/**
*/
struct Entry *
launcher_cache_find_entry (const gchar *key)
{
 struct mec *cached;

 HASH_FIND_STR (gl_mec, key, cached);
 if (cached)
 {
  return cached->value;
 }
 return NULL;
}

/**
launcher_cache_add_entry:
Cache a struct #Entry pointer.
*/
void
launcher_cache_add_entry (const gchar *key,
                          struct Entry *value)
{
 struct mec *cached = NULL;
 struct Entry *me = NULL;

 if (!(cached = calloc (1, sizeof (*cached))) || !(cached->key = strdup (key))
     || !(me = entry_new ()))
 {
  perror ("malloc me cache");
  free (me);
  free (cached->key);
  free (cached);
  return;
 }
 memcpy (me, value, sizeof (*me));
 cached->value = me;
 HASH_ADD_KEYPTR (hh, gl_mec, cached->key, strlen (cached->key), cached);
}

/**
launcher_cache_entry_count:
*/
guint
launcher_cache_entry_count ()
{
 return HASH_COUNT (gl_mec);
}

/**
cache_purge_scandir_all:
Delete all cached scandir lists, and clear the scandir cache.
*/
static void
cache_purge_scandir_all ()
{
 struct sdc *cached = NULL, *tmp;
 HASH_ITER (hh, gl_sdc, cached, tmp)
 {
  for (gint i = cached->value->n - 1; i >= 0; i--)
  {
   free (cached->value->namelist[i]);
  }
  free (cached->value->namelist);
  HASH_DEL (gl_sdc, cached);
  free (cached->value);
  free (cached->key);
  free (cached);
 }
 HASH_CLEAR (hh, gl_sdc);
}

/**
launcher_cache_find_scandir:
Find a cached scandir list.

@key: directory path.
@signature: if @signature does not match the global signature the whole
cache is invalidated and emptied, and the new signature is recorded.
*/
struct Scandir *
launcher_cache_find_scandir (const gchar *key,
                             const guint signature)
{
 struct sdc *cached;

 if (gl_sdc_signature != signature)
 {
  cache_purge_scandir_all ();
  gl_sdc_signature = signature;
 }
 else
 {
  HASH_FIND_STR (gl_sdc, key, cached);
  if (cached)
  {
   return cached->value;
  }
 }
 return NULL;
}

/**
launcher_cache_add_scandir:
Cache a scandir list.

@key: directory path.
@value: struct %Scandir pointer holding the scandir namelist.
@signature: if @signature does not match the global signature the whole
cache is invalidated and emptied, and the new signature is recorded.
*/
void
launcher_cache_add_scandir (const gchar *key,
                            struct Scandir *value,
                            const guint signature)
{
 struct sdc *cached = NULL;
 struct Scandir *sd = NULL;

 if (gl_sdc_signature != signature)
 {
  cache_purge_scandir_all ();
  gl_sdc_signature = signature;
 }

 if (!(cached = calloc (1, sizeof (*cached))) || !(cached->key = strdup (key))
     || !(sd = malloc (sizeof (*sd))))
 {
  perror ("malloc sd cache");
  free (sd);
  free (cached->key);
  free (cached);
  return;
 }
 memcpy (sd, value, sizeof (*value));
 cached->value = sd;
 HASH_ADD_KEYPTR (hh, gl_sdc, cached->key, strlen (cached->key), cached);
}

/**
launcher_cache_scandir_count:
*/
guint
launcher_cache_scandir_count ()
{
 return HASH_COUNT (gl_sdc);
}

/* vim: set et ts=1 sts=1 sw=1 tw=0 fdm=syntax */
/* *INDENT-ON* */
