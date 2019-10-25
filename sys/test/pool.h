/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
#define KiB		1024u			/* Kibi 0x0000000000000400 */
#define MiB		1048576u		/* Mebi 0x0000000000100000 */
#define GiB		1073741824u		/* Gibi 000000000040000000 */

#include <stdint.h>

struct Pool {
	char*	 name;
	unsigned long	maxsize;

	unsigned long	cursize;
	unsigned long	curfree;
	unsigned long	curalloc;

	unsigned long	minarena;						/* smallest size of new arena */
	unsigned long	quantum;						/* allocated blocks should be multiple of */
	unsigned long	minblock;						/* smallest newly allocated block */

	void*			freeroot;						/* actually Free* */
	void*			arenalist;						/* actually Arena* */

	void*			(*alloc)(unsigned long);
	int	    		(*merge)(void*, void*);
	void			(*move)(void* from, void* to);

	int				flags;
	int				nfree;
	int				lastcompact;

	void			(*lock)(Pool*);					/* Unused */
	void			(*unlock)(Pool*);				/* Unused */
	void			(*print)(Pool*, char*, ...);	/* Unused */
	void			(*panic)(Pool*, char*, ...);	/* Unused */
	void			(*logstack)(Pool*);

	void*			private;						/* Unused */
};

typedef struct Pool Pool;

extern Pool*		 mainmem;
extern Pool*		 virtmem;

extern void*	     poolalloc(Pool*, unsigned long);
extern void*	     poolallocalign(Pool*, unsigned long, unsigned long, long, unsigned long);
extern void	         poolfree(Pool*, void*);
extern unsigned long poolmsize(Pool*, void*);
extern void*	     poolrealloc(Pool*, void*, unsigned long);
extern void	         poolcheck(Pool*);
extern int	         poolcompact(Pool*);
extern void	         poolblockcheck(Pool*, void*);

extern	void*		 malloc(u_long);
extern	void*		 mallocz(u_long, int);
extern	void		 free(void*);
extern	u_long		 msize(void*);
extern	void*		 mallocalign(u_long, u_long, long, u_long);
extern	void*		 calloc(u_long, u_long);
extern	void*		 realloc(void*, u_long);
extern	void		 setmalloctag(void*, u_long);
extern	void		 setrealloctag(void*, u_long);
extern	u_long		 getmalloctag(void*);
extern	u_long		 getrealloctag(void*);
extern	void*		 malloctopoolblock(void*);


enum {	/* flags */
	POOL_ANTAGONISM	= 1<<0,
	POOL_PARANOIA	= 1<<1,
	POOL_VERBOSITY	= 1<<2,
	POOL_DEBUGGING	= 1<<3,
	POOL_LOGGING	= 1<<4,
	POOL_TOLERANCE	= 1<<5,
	POOL_NOREUSE	= 1<<6,
};
