const activation_log *
activation_log_fetch (const gchar *path);

int
activation_log_exclude (const gchar *pattern);

guint
entry_activationlog_write (struct Entry *entry,
                           const struct Entry *overrides);

int
activation_log_module_init (void);

int
activation_log_module_finalize (void);

/***********************************************************/

