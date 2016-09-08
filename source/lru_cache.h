#include <string.h>
#include "uthash.h"
#include "menuInput.h"

struct CacheEntry {
    char *key;
    struct MenuEntry *value;
    UT_hash_handle hh;
};

struct MenuEntry *find_in_cache(const gchar *key);
void add_to_cache(const gchar *key, struct MenuEntry *value);

extern struct CacheEntry *cache;

//---------------------------------------------------------------------
#include <dirent.h>

struct ScanDirCacheEntry {
    char *key;
    struct ScanDir *value;
    UT_hash_handle hh;
};

struct ScanDir {
    uint n;
    struct dirent **namelist;
};

struct ScanDir *find_in_scandir_cache(const gchar *key);
void add_to_scandir_cache(const gchar *key, struct ScanDir *value);

extern struct ScanDirCacheEntry *scanDirCache;
