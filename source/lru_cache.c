#include "lru_cache.h"
// Modified from http://jehiah.cz/a/uthash, retrieved 2016-08-12:

// this is an example of how to do a LRU cache in C using uthash
// http://uthash.sourceforge.net/
// by Jehiah Czebotar 2011 - jehiah@gmail.com
// this code is in the public domain http://unlicense.org/

#define MAX_CACHE_SIZE 100000

struct CacheEntry *cache = NULL;

struct MenuEntry* find_in_cache(const gchar *key)
{
    struct CacheEntry *cache_entry;
    HASH_FIND_STR(cache, key, cache_entry);
    if (cache_entry) {
        // remove it (so the subsequent add will throw it on the front of the list)
        HASH_DELETE(hh, cache, cache_entry);
        HASH_ADD_KEYPTR(hh, cache, cache_entry->key, strlen(cache_entry->key), cache_entry);
        return cache_entry->value;
    }
    return NULL;
}

void add_to_cache(const gchar *key, struct MenuEntry *value)
{
    struct CacheEntry *cache_entry, *tmp_entry;
    struct MenuEntry *menu_entry = NULL;
    if ( !(cache_entry = malloc(sizeof(struct CacheEntry)))
      || !( menu_entry = malloc(sizeof(struct MenuEntry )))
      || !( cache_entry->key = strdup(key)))
    {
      perror("malloc");
      if (cache_entry)
      {
        if(cache_entry->key) free(cache_entry->key);
        free(cache_entry);
      }
      if (menu_entry) free(menu_entry);
      return;
    }
    memcpy(menu_entry, value, sizeof(struct MenuEntry));
    cache_entry->value = menu_entry;
    HASH_ADD_KEYPTR(hh, cache, cache_entry->key, strlen(cache_entry->key), cache_entry);
    
    // prune the cache to MAX_CACHE_SIZE
    if (HASH_COUNT(cache) >= MAX_CACHE_SIZE) {
        HASH_ITER(hh, cache, cache_entry, tmp_entry) {
            // prune the first cache_entry (loop is based on insertion order so this deletes the oldest item)
            HASH_DELETE(hh, cache, cache_entry);
            free(cache_entry->key);
            free(cache_entry);
            free(menu_entry);
            break;
        }
    }    
}
