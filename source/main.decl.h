int
main (int argc, gchar *argv[]);

enum Result
main_loop (struct InputSource *isrc,
           const gint argc,
           const gchar *argv[],
           const guint menu_depth_base,
           const struct Entry *overrides);

void
menu_position_cb (GtkMenu *menu,
                  gint *x,
                  gint *y,
                  gboolean *push_in,
                  gpointer data);

void
entry_activate (struct Entry *entry);

enum Result
main_finale (const struct Entry *entry);

/***********************************************************/

static void
disable_popup_menu ();

static void
menu_deactivate_cb (gpointer data);

static guint
lockfile_test_set (void);

static gint
lockfile_unlock (void);

static enum Result
entry_expand_params_vars (struct Entry *entry,
                          const struct Params *params,
                          const gboolean no_expand_param_refs);

static enum Result
entry_leave (struct Entry *entry,
             const struct Entry *overrides);

static gint
do_onexit ();

static void
prune_empty_branches (GtkMenu *menu);

static gint
main_options (int argc,
              char *argv[]);

static unsigned
opt_delimiter (const char *arg);

static void
opt_force (void);

static void
opt_help (void);

static void
opt_z__1 (void);

static void
opt_license (void);

static void
opt_verbose (void);

static void
opt_quiet (void);

static void
opt_version (void);

