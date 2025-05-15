gboolean
param_array_init (struct Params *param,
                  const guint argc,
                  const gchar **argv,
                  const gchar *par0);

void
param_array_destroy (struct Params *param);

enum Result
expand_param (const gchar **iptr,
              gchar **optr,
              const guint max_size,
              gboolean *found,
              const struct Params *param,
              struct Entry *entry);

/***********************************************************/

