/* FBSD kern environment */
/* Belongs in sys/systm.h */

char	*kern_getenv(const char *name);
void	freeenv(char *env);
int		getenv_int(const char *name, int *data);
int		getenv_uint(const char *name, unsigned int *data);
int		getenv_long(const char *name, long *data);
int		getenv_ulong(const char *name, unsigned long *data);
int		getenv_string(const char *name, char *data, int size);
int		getenv_int64(const char *name, int64_t *data);
int		getenv_uint64(const char *name, uint64_t *data);
int		getenv_quad(const char *name, quad_t *data);
int		kern_setenv(const char *name, const char *value);
int		kern_unsetenv(const char *name);
int		testenv(const char *name);

int		getenv_array(const char *name, void *data, int size, int *psize, int type_size, bool allow_signed);
#define	GETENV_UNSIGNED	false	/* negative numbers not allowed */
#define	GETENV_SIGNED	true	/* negative numbers allowed */
