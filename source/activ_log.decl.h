const activation_log *
activation_log_fetch (const gchar *path);

int
activation_log_exclude (const gchar *pattern);

int
activationlog_write_entry (struct Entry *entry);

int
activation_log_module_init (void);

int
activation_log_module_finalize (void);

/***********************************************************/

