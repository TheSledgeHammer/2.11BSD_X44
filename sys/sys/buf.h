/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)buf.h	1.4 (2.11BSD GTE) 1996/9/13
 */

/*
 * The header for buffers in the buffer pool and otherwise used
 * to describe a block i/o request is given here.
 *
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * hashed into a chain by <dev,blkno> so it can be located in the cache,
 * and (usually) on (one of several) queues.  These lists are circular and
 * doubly linked for easy removal.
 *
 * There are currently two queues for buffers:
 * 	one for buffers containing ``useful'' information (the cache)
 *	one for buffers containing ``non-useful'' information
 *		(and empty buffers, pushed onto the front)
 * These queues contain the buffers which are available for
 * reallocation, are kept in lru order.  When not on one of these queues,
 * the buffers are ``checked out'' to drivers which use the available list
 * pointers to keep track of them in their i/o active queues.
 */

/*
 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
#ifndef _SYS_BUF_H_
#define	_SYS_BUF_H_
#include <sys/queue.h>

#define NOLIST ((struct buf *)0x87654321)

struct buf
{
	LIST_ENTRY(buf) b_hash;			/* Hash chain. */
	LIST_ENTRY(buf) b_vnbufs;		/* Buffer's associated vnode. */
	TAILQ_ENTRY(buf) b_freelist;	/* Free list position if not active. */
	struct	buf 	*b_actf, **b_actb;	/* Device driver queue when active. */
	struct  proc 	*b_proc;		/* Associated proc; NULL if kernel. */
	volatile long	b_flags;		/* B_* flags. */
	int				b_qindex;		/* buffer queue index */
	int				b_error;		/* returned after I/O */
	long			b_bufsize;		/* Allocated buffer size. */
	long			b_bcount;		/* transfer count */
	long			b_resid;		/* words not transferred after error */
	char			b_xmem;			/* high order core address */
	dev_t			b_dev;			/* major+minor device name */
	struct {
	    caddr_t b_addr;				/* low order core address */
	} b_un;
	void			*b_saveaddr;	/* Original b_addr for physio. */
	daddr_t			b_lblkno;		/* Logical block number. */
	daddr_t			b_blkno;		/* Underlying physical block number. */

	void	(*b_iodone)(struct buf *);
	struct	vnode 	*b_vp;			/* Device vnode. */
	int				b_pfcent;		/* Center page when swapping cluster. */
	int				b_dirtyoff;		/* Offset in buffer of dirty region. */
	int				b_dirtyend;		/* Offset of end of dirty region. */
	struct	ucred 	*b_rcred;		/* Read credentials reference. */
	struct	ucred 	*b_wcred;		/* Write credentials reference. */
	int				b_validoff;		/* Offset in buffer of valid region. */
	int				b_validend;		/* Offset of end of valid region. */
	daddr_t			b_pblkno;       /* physical block number */
	caddr_t			b_savekva;      /* saved kva for transfer while bouncing */

#ifndef VMIO
	void	*b_pages[(MAXBSIZE + PAGE_SIZE - 1)/PAGE_SIZE];
#else
	vm_page_t	b_pages[(MAXBSIZE + PAGE_SIZE - 1)/PAGE_SIZE];
#endif
	int		b_npages;
};

/* Device driver compatibility definitions. */
//struct	buf 		*av_forw, *av_back;	/* position on free list if not BUSY */
//#define	b_actf		av_forw				/* alternate names for driver queue */
//#define	b_actl		av_back				/* head - isn't history wonderful */

#define	b_active 	b_bcount			/* driver queue head: drive active */
#define	b_data	 	b_un.b_addr			/* b_un.b_addr is not changeable. */
#define	b_cylin 	b_resid				/* disksort */
#define	b_errcnt 	b_resid				/* while i/o in progress: # retries */
#define	iodone	 	biodone				/* Old name for biodone. */
#define	iowait	 	biowait				/* Old name for biowait. */

/*
 * This structure describes a clustered I/O.  It is stored in the b_saveaddr
 * field of the buffer on which I/O is done.  At I/O completion, cluster
 * callback uses the structure to parcel I/O's to individual buffers, and
 * then free's this structure.
 */
struct cluster_save {
	long		bs_bcount;			/* Saved b_bcount. */
	long		bs_bufsize;			/* Saved b_bufsize. */
	void		*bs_saveaddr;		/* Saved b_addr. */
	int			bs_nchildren;		/* Number of associated buffers. */
	struct buf 	**bs_children;		/* List of associated buffers. */
};


/* These flags are kept in b_flags. */
#define	B_WRITE			0x00000		/* non-read pseudo-flag */
#define	B_READ			0x00001		/* read when I/O occurs */
#define	B_DONE			0x00002		/* transaction finished */
#define	B_ERROR			0x00004		/* transaction aborted */
#define	B_BUSY			0x00008		/* not on av_forw/back list */
#define	B_PHYS			0x00010		/* physical IO */
#define	B_MAP			0x00020		/* alloc UNIBUS */
#define	B_WANTED 		0x00040		/* issue wakeup when BUSY goes off */
#define	B_AGE			0x00080		/* delayed write for correct aging */
#define	B_ASYNC			0x00100		/* don't wait for I/O completion */
#define	B_DELWRI 		0x00200		/* write at exit of avail list */
#define	B_TAPE 			0x00400		/* this is a magtape (no bdwrite) */
#define	B_INVAL			0x00800		/* does not contain valid info */
#define	B_BAD			0x01000		/* bad block revectoring in progress */
#define	B_LOCKED		0x02000		/* locked in core (not reusable) */
#define	B_UBAREMAP		0x04000		/* addr UNIBUS virtual, not physical */
#define	B_RAMREMAP		0x08000		/* remapped into ramdisk */

#define	B_CACHE			0x10000		/* Bread found us in the cache. */
#define	B_CALL			0x20000		/* Call b_iodone from biodone. */
#define	B_WRITEINPROG	0x40000		/* Write in progress. */
#define	B_NOCACHE		0x80000		/* Do not cache block after use. */
#define B_VMIO			0x100000	/* VMIO flag */

/*
 * number of buffer hash entries
 */
#define	BUFHSZ	512	/* must be power of 2 */
//#define	BUFHASH(dev, blkno)	((struct buf *)&bufhash[((long)(dev) + blkno) & ((long)(BUFHSZ - 1))])
/*
 * buffer hash table calculation, originally by David Greenman
 */
#define BUFHASH(vnp, bn)        \
	(&bufhashtbl[(((int)(vnp) / sizeof(struct vnode))+(int)(bn)) % BUFHSZ])

/*
 * We never use BQ_LOCKED or BQ_EMPTY, but if you want the 4.X block I/O
 * code to drop in, you have to have BQ_AGE and BQ_LRU *after* the first
 * queue, and it only costs 6 bytes of data space.
 */
#define	BQUEUES			5		/* number of free buffer queues */

#define	BQ_NONE			0		/* on no queue */
#define BQ_LOCKED		1		/* locked buffers */
#define	BQ_LRU			2		/* lru, useful buffers */
#define	BQ_AGE			3		/* rubbish */
#define	BQ_EMPTY		4		/* buffer headers with no memory */

LIST_HEAD(bufhashhdr, buf) bufhashtbl[BUFHSZ], invalhash;
TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];

/*
 * Zero out the buffer's data area.
 */
#define	clrbuf(bp) {										\
	blkclr((bp)->b_data, (u_int)(bp)->b_bcount);			\
	(bp)->b_resid = 0;										\
}

/* Flags to low-level allocation routines. */
#define B_CLRBUF	0x01	/* Request allocated buffer be cleared. */
#define B_SYNC		0x02	/* Do all allocations synchronously. */

#ifdef KERNEL
extern int		nbuf;				/* number of buffer headers */
extern struct	buf *buf;			/* the buffer pool itself */
extern char		*buffers;			/* The buffer contents. */
extern int		bufpages;			/* Number of memory pages in the buffer pool. */
extern struct	buf *swbuf;			/* Swap I/O buffer headers. */
extern int		nswbuf;				/* Number of swap I/O buffer headers. */
extern TAILQ_HEAD(swqueue, buf) bswlist;


__BEGIN_DECLS
void	bufinit (void);
void	bremfree (struct buf *);
int		bread (struct vnode *, daddr_t, int, struct ucred *, struct buf **);
int		breadn (struct vnode *, daddr_t, int, daddr_t *, int *, int, struct ucred *, struct buf **);
int		bwrite (struct buf *);
void	bdwrite (struct buf *);
void	bawrite (struct buf *);
void	brelse (struct buf *);
struct 	buf *getnewbuf (int slpflag, int slptimeo);
struct 	buf *getpbuf (void);
struct 	buf *incore (struct vnode *, daddr_t);
struct 	buf *getblk (struct vnode *, daddr_t, int, int, int);
struct 	buf *geteblk (int);
void	allocbuf (struct buf *, int);
int		biowait (struct buf *);
void	biodone (struct buf *);

void	cluster_callback (struct buf *);
int		cluster_read (struct vnode *, u_quad_t, daddr_t, long, struct ucred *, struct buf **);
void	cluster_write (struct buf *, u_quad_t);
//u_int	minphys (struct buf *);
void	vwakeup (struct buf *);
void	vmapbuf (struct buf *);
void	vunmapbuf (struct buf *);
void	relpbuf (struct buf *);
void	brelvp (struct buf *);
void	bgetvp (struct vnode *, struct buf *);
void	reassignbuf (struct buf *, struct vnode *);
__END_DECLS
#endif

#endif /* !_SYS_BUF_H_ */
