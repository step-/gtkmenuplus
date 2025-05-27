enum Result
entry_add_icon (struct Entry *entry,
                GtkWidget *menu_item);

enum Result
a_activationlogfile (struct Entry *entry);

enum Result
a_activationlogexclude (struct Entry *entry);

enum Result
a_cmd (struct Entry *entry);

enum Result
a_icon (struct Entry *entry);

enum Result
a_tooltip (struct Entry *entry);

enum Result
entry_append_leaf_node (struct Entry *entry,
                        const gchar *label,
                        const gchar *cmd,
                        GtkWidget **widgetptr,
                        struct Entry **trackedptr);

enum Result
node_attach_tracked_entry (GtkWidget *node,
                           struct Entry **tracked,
                           struct Entry *entry);

enum Result
enter_item (struct Entry *entry);

enum Result
leave_item (struct Entry *entry);

enum Result
a_endsubmenu (struct Entry *entry);

enum Result
enter_submenu_internal (struct Entry *entry,
                        const enum EntryFlags feat);

enum Result
enter_submenu (struct Entry *entry);

enum Result
leave_submenu (struct Entry *entry);

enum Result
a_separator (struct Entry *entry);

enum Result
a_iconsize (struct Entry *entry);

enum Result
a_menuposition (struct Entry *entry);

enum Result
a_icondir (struct Entry *entry);

enum Result
make_rooted_dirpath (struct Entry *entry);

enum Result
a_error (struct Entry *entry);

enum Result
a_configure (struct Entry *entry);

enum Result
a_onexit (struct Entry *entry);

enum Result
a_absolutepath (struct Entry *entry);

enum Result
enter_include (struct Entry *entry);

enum Result
leave_include (struct Entry *entry);

enum Result
a_eof (struct Entry *entry);

GtkMenu *
core_get_menu (void);

int
core_module_init (void);

/***********************************************************/

static const gchar *
path_rcomponents (gchar *path,
                  guint n);

static enum Result
include_file (const gint argc,
              const gchar **argv,
              struct Entry *entry);

static enum Result
include_directory (const gint argc,
                   const gchar **argv,
                   struct Entry *entry);

static gint
include_scan_filter (const struct dirent *d);

static enum Result
include_directory_scan (const gchar *dir,
                        const gchar *subdir,
                        const gchar *file_glob,
                        const gchar *dir_glob,
                        struct Entry *entry);

static gboolean
include_directory_match_glob (const gchar *dir,
                              const gchar *subdir,
                              const gchar *file_glob,
                              const gchar *dir_glob,
                              guint depth,
                              struct Entry *entry);

static enum Result
include_directory_enter_subdir (const gchar *subdir,
                                struct Entry *entry);

