enum Result
entry_var (struct Entry *entry);

enum Result
entry_var_expand (struct Entry *entry,
                  const gchar **iptr,
                  gchar **optr,
                  const guint max_size,
                  gboolean *found);

gboolean
var_name_test (const gchar *p);

enum Result
a_variable (struct Entry *entry);

int
entry_var_module_init (void);

void
entry_var_module_finalize (void);

/***********************************************************/

static enum Result
entry_var_add (struct Entry *entry,
               struct Variable **varptr);

static
void var_table_destroy ();

static struct Variable*
var_find (const gchar *name,
          const guint length);

static enum Result
entry_var_eval (struct Entry *entry,
                struct Variable *var);

