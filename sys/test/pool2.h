/*
 * pool2.h
 *
 *  Created on: 14 Nov 2019
 *      Author: marti
 */

#ifndef SYS_POOL_H_
#define SYS_POOL_H

#include <sys/param.h>

struct Pool {
    struct Pool     *pool;
    char 			*name;

    /* VPool Stats */
    struct vPool    *vPool;
    int             vpools_max;             /* Maximum Number of vpools per pool zone */
    int             vpools_allocated;       /* Number of vpools currently allocated per pool zone */
    unsigned long	pool_min_zone_size;     /* Minimum Pool Zone allocation size (minimum vpool block allocation) */

    /* Pool Bucket Zone */
    int16_t	        pool_cnt;              /* Pools Allocated. */
    int16_t         pool_entries;          /* Max entries in vpool. */
    void            *pool_bucket[];        /* Actual allocation storage */

    struct Pool    	*next;
    struct Pool    	*prev;
    struct Pool 	*head;
    struct Pool 	*tail;
};
typedef struct Pool Pool;

struct vPool {
    struct vPool    *vpool;

    char	        *name;
    unsigned long	maxsize;

    unsigned long	cursize;
    unsigned long	curfree;
    unsigned long	curalloc;

    unsigned long	minarena;	        /* smallest size of new arena */
    unsigned long 	quantum;          	/* allocated blocks should be multiple of */
    unsigned long 	minblock;         	/* smallest newly allocated block */

    void*	        freeroot;	        /* actually Free* */
    void*	        arenalist;	        /* actually Arena* */

    void*	        (*alloc)(unsigned long);
    int	            (*merge)(void*, void*);
    void	        (*move)(void* from, void* to);

	void			(*lock)(vPool*);
	void			(*unlock)(vPool*);
	void			(*print)(vPool*, char*, ...);
	void			(*panic)(vPool*, char*, ...);
	void			(*logstack)(vPool*);

    int	            flags;
    int	            nfree;
    int	            lastcompact;
};
typedef struct vPool vPool;

struct Arena {
	Bhdr 			Bhdr;
    struct Arena*   aup;
    struct Arena*   down;
	unsigned long	asize;
	unsigned long	pad;			/* to a multiple of 8 bytes */
	unsigned long	magic;
};
typedef struct Arena Arena;

struct Alloc {
    struct Bhdr 	Bhdr;
	unsigned long	magic;
	unsigned long	size; 			/* same as Bhdr->size */
};
typedef struct Alloc Alloc;

/* Block structure: defines block attributes */
struct Block {
    unsigned long b_quantum;          /* allocated blocks should be multiple of */
    unsigned long b_minblock;         /* smallest newly allocated block */

    struct block_header {
        unsigned long magic;
        unsigned long size;
    } Bhdr;

    struct block_tail {
        unsigned char magic0;
        unsigned char datasize[2];
        unsigned char magic1;
        unsigned long size;
    } Btail;

    struct block_free {
        struct block_free 	*left;
        struct block_free 	*right;
        struct block_free 	*next;
        struct block_free 	*prev;
    	unsigned long		size; 	/* same as Bhdr->size */
    	unsigned long		magic;
    } Bfree;
};
typedef struct Block Block;
typedef struct block_header Bhdr;
typedef struct block_tail Btail;
typedef struct block_free Bfree;

/* Pool's: VPool Management */
extern void*	        vpoolalloc(vPool*, unsigned long);
extern void*	        vpoolallocalign(vPool*, unsigned long, unsigned long, long, unsigned long);
extern void	            vpoolfree(vPool*, void*);
extern unsigned long	vpoolmsize(vPool*, void*);
extern void*	        vpoolrealloc(vPool*, void*, unsigned long);
extern void	            vpoolcheck(vPool*);
extern int	            vpoolcompact(vPool*);
extern void	            vpoolblockcheck(vPool*, void*);

/* VPool's: Arena & Block Management */
static void*	    	B2D(vPool*, Alloc*);
static Alloc*	    	D2B(vPool*, void*);
static Arena*	    	arenamerge(vPool*, Arena*, Arena*);
static void		    	blockcheck(vPool*, Bhdr*);
static Alloc*	    	blockmerge(vPool*, Bhdr*, Bhdr*);
static Alloc*	    	blocksetdsize(vPool*, Alloc*, unsigned long);
static Bhdr*	    	blocksetsize(Bhdr*, unsigned long);
static unsigned long	bsize2asize(vPool*, unsigned long);
static unsigned long	dsize2bsize(vPool*, unsigned long);
static unsigned long	getdsize(Alloc*);
static Alloc*	    	trim(vPool*, Alloc*, unsigned long);
static Bfree*	    	listadd(Bfree*, Bfree*);
static void		    	logstack(vPool*);
static Bfree**	    	ltreewalk(Bfree**, unsigned long);
static void		    	memmark(void*, int, unsigned long);
static Bfree*	    	vpooladd(vPool*, Alloc*);
static void*	    	vpoolallocl(vPool*, unsigned long);
static void		    	vpoolcheckl(vPool*);
static void		    	vpoolcheckarena(vPool*, Arena*);
static int		    	vpoolcompactl(vPool*);
static Alloc*	    	vpooldel(vPool*, Bfree*);
static void		    	vpooldumpl(vPool*);
static void		    	vpooldumparena(vPool*, Arena*);
static void		    	vpoolfreel(vPool*, void*);
static void		    	vpoolnewarena(vPool*, unsigned long);
static void*	    	vpoolreallocl(vPool*, void*, unsigned long);
static Bfree*	    	treedelete(Bfree*, Bfree*);
static Bfree*	    	treeinsert(Bfree*, Bfree*);
static Bfree*	    	treelookup(Bfree*, unsigned long);
static Bfree*	    	treelookupgt(Bfree*, unsigned long);

/* Block Magic */
#define ARENA_MAGIC 	0xC0A1E5CE + 1
#define ARENATAIL_MAGIC 0xEC5E1A0C + 1
#define ALLOC_MAGIC 	0x0A110C09
#define UNALLOC_MAGIC  	0xCAB00D1E + 1
#define TAIL_MAGIC0 	0xBE
#define TAIL_MAGIC1  	0xEF
#define FREE_MAGIC  	0xBA5EBA11
#define NOT_MAGIC 		0xdeadfa11
#define DEAD_MAGIC 		0xdeaddead
#define ALIGN_MAGIC  	0xA1F1D1C1
#define UNALLOC_MAGIC 	0xCAB00D1E
#define ARENA_MAGIC 	0xC0A1E5CE
#define ARENATAIL_MAGIC 0xEC5E1A0C

/* Flags */
#define POOL_ANTAGONISM	1<<0
#define POOL_PARANOIA	1<<1
#define POOL_VERBOSITY	1<<2
#define POOL_DEBUGGING	1<<3
#define POOL_LOGGING	1<<4
#define POOL_TOLERANCE	1<<5
#define POOL_NOREUSE	1<<6

#define MINBLOCKSIZE \
	sizeof(Bfree) + sizeof(Btail)

#define B2NB(b) (Bhdr*)(char*)(b)+(b)
#define SHORT(x) (((x)[0] << 8) | (x)[1])
#define PSHORT(p, x) \
	(((unsigned long*)(p))[0] = ((x)>>8)&0xFF, \
	((unsigned long*)(p))[1] = (x)&0xFF)

#define B2T(b)	((Btail*)((unsigned long*)(b)+(b)->size-sizeof(Btail)))
#define B2PT(b) ((Btail*)((unsigned long*)(b)-sizeof(Btail)))
#define T2HDR(t) ((Bhdr*)((unsigned long*)(t)+sizeof(Btail)-(t)->size))

#define A2TB(a)	\
	((Bhdr*)((unsigned char*)(a)+(a)->asize-sizeof(Bhdr)))
#define A2B(a)	B2NB(a)

static char datamagic[] = { 0xFE, 0xF1, 0xF0, 0xFA };

#define	Poison	(void*)0xCafeBabe

#define _B2D(a)	((void*)((unsigned char*)a+sizeof(Bhdr)))
#define _D2B(v)	((Alloc*)((unsigned char*)v-sizeof(Bhdr)))

/* Pool Debugging
 * Antagonism causes blocks to always be filled with garbage if their
 * contents are undefined.  This tickles both programs and the library.
 * It's a linear time hit but not so noticeable during nondegenerate use.
 * It would be worth leaving in except that it negates the benefits of the
 * kernel's demand-paging.  The tail magic and end-of-data magic
 * provide most of the user-visible benefit that antagonism does anyway.
 *
 * Paranoia causes the library to recheck the entire pool on each lock
 * or unlock.  A failed check on unlock means we tripped over ourselves,
 * while a failed check on lock tends to implicate the user.  Paranoia has
 * the potential to slow things down a fair amount for pools with large
 * numbers of allocated blocks.  It completely negates all benefits won
 * by the binary tree.  Turning on paranoia in the kernel makes it painfully
 * slow.
 *
 * Verbosity induces the dumping of the pool via p->print at each lock operation.
 * By default, only one line is logged for each alloc, free, and realloc.
 */
#define antagonism	if(!(p->flags & POOL_ANTAGONISM)){}else
#define paranoia	if(!(p->flags & POOL_PARANOIA)){}else
#define verbosity	if(!(p->flags & POOL_VERBOSITY)){}else

#define DPRINT		if(!(p->flags & POOL_DEBUGGING)){}else p->print
#define LOG			if(!(p->flags & POOL_LOGGING)){}else p->print

/* Pool Bucket Zones & VPool Zones */
#define	BUCKET_SIZE(n)						\
    (((sizeof(void *) * (n)) - sizeof(struct Pool)) / sizeof(void *))

#define	BUCKET_MAX	BUCKET_SIZE(256)
#define	BUCKET_MIN	BUCKET_SIZE(4)

struct Pool pools[] = {
		{ &pools[0], "Pool_4" },
		{ &pools[1], "Pool_6" },
		{ &pools[2], "Pool_8" },
		{ &pools[3], "Pool_12" },
		{ &pools[4], "Pool_16" },
		{ &pools[5], "Pool_32" },
		{ &pools[6], "Pool_64" },
		{ &pools[7], "Pool_128" },
		{ &pools[8], "Pool_256" },
};

struct vPool vpools[] = {
		{ &vpools[0], "vPool_4" },
		{ &vpools[1], "vPool_6" },
		{ &vpools[2], "vPool_8" },
		{ &vpools[3], "vPool_12" },
		{ &vpools[4], "vPool_16" },
		{ &vpools[5], "vPool_32" },
		{ &vpools[6], "vPool_64" },
		{ &vpools[7], "vPool_128" },
		{ &vpools[8], "vPool_256" },
};

struct Pool_Bucket_Zone {
	Pool            pool;
	vPool			vpool;
    unsigned long   pool_entries;          /* Number of Pool Zones Allocated. */
    unsigned long   pool_maxsize;          /* Maximum allocation size per Pool. */
};

struct Pool_Bucket_Zone pool_zone[] = {
		{ &pools[0], &vpools[0], BUCKET_SIZE(4), 4096 },
        { &pools[1], &vpools[1], BUCKET_SIZE(6), 3072 },
        { &pools[2], &vpools[2], BUCKET_SIZE(8), 2048 },
        { &pools[3], &vpools[3], BUCKET_SIZE(12), 1536 },
        { &pools[4], &vpools[4], BUCKET_SIZE(16), 1024 },
        { &pools[5], &vpools[5], BUCKET_SIZE(32), 512 },
        { &pools[6], &vpools[6], BUCKET_SIZE(64), 256 },
        { &pools[7], &vpools[7], BUCKET_SIZE(128), 128 },
        { &pools[8], &vpools[8], BUCKET_SIZE(256), 64 }
};
#endif /* SYS_POOL_H_ */
