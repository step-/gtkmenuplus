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
