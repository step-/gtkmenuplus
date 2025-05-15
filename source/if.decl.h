gboolean
if_context_push_new ();

gboolean
if_context_pop ();

gboolean
if_unit_push_new ();

void
if_unit_pop ();

gboolean
if_get_has_else ();

void
if_set_has_else (gboolean state);

gboolean
if_get_is_active ();

void
if_set_is_active (gboolean state);

gboolean
if_get_evaluates_body ();

gboolean
if_get_container_evaluates_body ();

gboolean
if_get_is_contained ();

void
if_set_evaluates_body (gboolean state);

if_truth_value
if_get_truth_value ();

void
if_set_truth_value (if_truth_value state);

void
if_set_lineno (guint lineno);

enum Result
if_evaluate_expression (struct Entry *entry,
                        const gchar *label);

enum Result
a_if (struct Entry *entry);

enum Result
a_else (struct Entry *entry);

enum Result
a_elseif (struct Entry *entry);

enum Result
a_endif (struct Entry *entry);

int
if_module_init ();

void
if_module_finalize (void);

/***********************************************************/

static struct _if_unit *
if_fetch_top_unit ();

