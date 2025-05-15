enum Result
a_launcher (struct Entry *entry);

enum Result
a_launchersub (struct Entry *entry);

enum Result
a_launcherargs (struct Entry *entry);

enum Result
a_launcherdirfile (struct Entry *entry);

enum Result
enter_launchersubmenu (struct Entry *entry);

enum Result
leave_launchersubmenu (struct Entry *entry);

enum Result
a_launcherdir (struct Entry *entry);

const gchar *
launcher_get_launcherdirectory ();

void
launcher_set_launcherdirectory (const gchar *value);

int
launcher_module_init (const int cflags);

void
launcher_module_finalize (void);

/***********************************************************/

static void
launcher_prop_free_values ();

static void
launcher_exec_data_whiteout_code (gchar *data);

static void
launcher_copy_properties (struct Entry *dst,
                          const struct Entry *src);

static enum Result
launcher_entry_fill_cached (const gchar *filepath,
                            struct Entry *entry,
                            const gchar *label,
                            const gboolean required,
                            const enum LineType callerid);

static enum Result
launcher_entry_cache_props (const gchar *filepath,
                            struct Entry *entry,
                            const gchar *label,
                            const gboolean required);

static enum Result
launcher_submenu_fill_entry (const gchar *dirpath,
                             struct Entry *entry);

static enum Result
map_token_list (const fnEntry func,
                const gboolean return_on_first_ok,
                const gchar *delim,
                const gchar *list,
                struct Entry *entry);

static gint
launcher_scan_filter (const struct dirent *d);

static gboolean
check_launcher_path_ext (const gchar *path);

static gboolean
build_launcher_path_from_dirent (const struct dirent *e,
                                 const gchar *dirpath,
                                 gchar *path,
                                 struct Entry *entry);

static int
launcher_label_cmp (const void *a,
                    const void *b);

static gboolean
launcher_cache_namelist_sorted (struct dirent **namelist,
                                const gint length,
                                const gchar *dirpath,
                                const enum LineType callerid,
                                struct Entry *entry);

inline static void
squeeze_slash_dot (const gchar *src,
                   gchar *dst);

static void
launcher_find_cb (const gchar *path,
                  struct lfpod *pod);

static enum Result
launcher_scan (struct Entry *entry,
               const gchar *caller,
               const enum LineType callerid);

static gboolean
intersectQ (gchar *a,
            gchar *b);

static gboolean
are_categories_intersecting (const gchar *a,
                             const gchar *b);

static enum Result
launcher_app (gchar *launcherpath,
              struct Entry *entry,
              const enum LineType callerid);

