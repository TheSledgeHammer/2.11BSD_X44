#ifndef _MP_H_
#define _MP_H_

#include <sys/types.h>
#include <sys/cdefs.h>

struct mint {
	int		len;
	short	*val;
};
typedef struct mint MINT;

struct half {
	short	high;
	short	low;
};

#define	FREE(x)	{ 				\
	if (x.len != 0) { 			\
		free((char *)x.val);	\
		x.len=0;				\
	}							\
}

#ifndef	DBG
#define	shfree(u)	free((char *)u)
#else
#include <stdio.h>

extern	int	dbg;

#define	shfree(u) {				\
	if (dbg) {					\
		fprintf(stderr, "free %o\n", u); \
	}							\
	free((char *)u);			\
}
#endif

extern	MINT	*itom(int);
extern	short	*xalloc(int, char *);

#endif /* _MP_H_ */
