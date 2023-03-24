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

#include <sys/bufq.h>
#include <sys/queue.h>
#include <sys/tree.h>

#define NOLIST ((struct buf *)0x87654321)

struct iodone_chain {
	long				ic_prev_flags;
	void				(*ic_prev_iodone)(struct buf *);
	void				*ic_prev_iodone_chain;
	struct {
		long			ia_long;
		void			*ia_ptr;
	} ic_args[5];
};

/*
 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
struct bufhd {
	short				b_flags;			/* see defines below */
	struct	buf 		*b_forw, *b_back;	/* fwd/bkwd pointer in chain */
};

/*
 * The buffer header describes an I/O operation in the kernel.
 */
struct buf {
	/* 2.11BSD Orig (Deprecated) */
	//struct	buf 		*b_forw, *b_back;	/* hash chain (2 way street) */
	//struct	buf 		*av_forw, *av_back;	/* position on free list if not BUSY */
	//struct	buf 		*b_actf, **b_actb;	/* Device driver queue when active. */

	/* 2.11BSD New */
	TAILQ_ENTRY(buf) 	b_actq;				/* Device driver queue when active. */
	RB_ENTRY(buf)	 	b_rbnode;			/* Cyclical scan (CSCAN) */

	LIST_ENTRY(buf) 	b_hash;				/* Hash chain. */
	LIST_ENTRY(buf) 	b_vnbufs;			/* Buffer's associated vnode. */
	TAILQ_ENTRY(buf) 	b_freelist;			/* Free list position if not active. */
	struct  proc 		*b_proc;			/* Associated proc; NULL if kernel. */
	volatile long		b_flags;			/* B_* flags. */
	int					b_qindex;			/* buffer queue index */
	int					b_prio;				/* Hint for buffer queue discipline. */
	int					b_error;			/* returned after I/O */
	long				b_bufsize;			/* Allocated buffer size. */
	long				b_bcount;			/* transfer count */
	long				b_resid;			/* words not transferred after error */
	char				b_xmem;				/* high order core address */
	dev_t				b_dev;				/* major+minor device name */
	struct {
	    caddr_t 		b_addr;				/* low order core address */
	} b_un;
	void				*b_saveaddr;		/* Original b_addr for physio. */
	daddr_t				b_lblkno;			/* Logical block number. */
	daddr_t				b_blkno;			/* Underlying physical block number. (partition relative) */
	daddr_t				b_pblkno;           /* physical block number */
	daddr_t				b_rawblkno;			/* Raw underlying physical block number. (not partition relative) */

	void				(*b_iodone)(struct buf *);
	struct	iodone_chain *b_iodone_chain;
	struct	vnode 		*b_vp;				/* Device vnode. */
	int					b_pfcent;			/* Center page when swapping cluster. */
	int					b_dirtyoff;			/* Offset in buffer of dirty region. */
	int					b_dirtyend;			/* Offset of end of dirty region. */
	struct	ucred 		*b_rcred;			/* Read credentials reference. */
	struct	ucred 		*b_wcred;			/* Write credentials reference. */
	int					b_validoff;			/* Offset in buffer of valid region. */
	int					b_validend;			/* Offset of end of valid region. */
	//void				*b_fsdata;			/* fs private data */
	struct swapbuf		*b_swbuf;			/* swapbuf back pointer */
	void				*b_private;

	struct lock_object	b_lnterlock;		/* buf lock */
};

#define	b_active 		b_bcount			/* driver queue head: drive active */
#define	b_data	 		b_un.b_addr			/* b_un.b_addr is not changeable. */
#define	b_cylin 		b_resid				/* disksort */
#define	b_errcnt 		b_resid				/* while i/o in progress: # retries */
#define	iodone	 		biodone				/* Old name for biodone. */
#define	iowait	 		biowait				/* Old name for biowait. */

/*
 * Definitions for the buffer free lists.
 */
#define	BQUEUES			4		/* number of free buffer queues */

#define	BQ_LOCKED		0		/* super-blocks &c */
#define	BQ_LRU			1		/* lru, useful buffers */
#define	BQ_AGE			2		/* rubbish */
#define	BQ_EMPTY		3		/* buffer headers with no memory */

struct bufhashhdr;
LIST_HEAD(bufhashhdr, buf);
extern struct bufhashhdr *bufhashtbl, invalhash;

struct bqueues;
TAILQ_HEAD(bqueues, buf);
extern struct bqueues 	bufqueues[];

#define	bftopaddr(bp)	((u_int)(bp)->b_un.b_addr >> 6 | (bp)->b_xmem << 10)
#define	bfree(bp)		(bp)->b_bcount = 0

/*
 * Zero out the buffer's data area.
 */
#define	clrbuf(bp) {													\
	bzero((bp)->b_data, (u_int)(bp)->b_bcount);							\
	(bp)->b_resid = 0;													\
}

/* Flags to low-level allocation routines. */
#define B_CLRBUF		0x01	/* Request allocated buffer be cleared. */
#define B_SYNC			0x02	/* Do all allocations synchronously. */

/*
 * number of buffer hash entries
 */
#define	BUFHSZ			512		/* must be power of 2 */

/* These flags are kept in b_flags. */
#define	B_WRITE			0x00000000		/* non-read pseudo-flag */
#define	B_READ			0x00000001		/* read when I/O occurs */
#define	B_DONE			0x00000002		/* transaction finished */
#define	B_ERROR			0x00000004		/* transaction aborted */
#define	B_BUSY			0x00000008		/* not on av_forw/back list */
#define	B_PHYS			0x00000010		/* physical IO */
#define	B_MAP			0x00000020		/* alloc UNIBUS */
#define	B_WANTED 		0x00000040		/* issue wakeup when BUSY goes off */
#define	B_AGE			0x00000080		/* delayed write for correct aging */
#define	B_ASYNC			0x00000100		/* don't wait for I/O completion */
#define	B_DELWRI 		0x00000200		/* write at exit of avail list */
#define	B_TAPE 			0x00000400		/* this is a magtape (no bdwrite) */
#define	B_INVAL			0x00000800		/* does not contain valid info */
#define	B_BAD			0x00001000		/* bad block revectoring in progress */
#define	B_LOCKED		0x00002000		/* locked in core (not reusable) */
#define	B_UBAREMAP		0x00004000		/* addr UNIBUS virtual, not physical */
#define	B_RAMREMAP		0x00008000		/* remapped into ramdisk */
#define	B_CACHE			0x00010000		/* Bread found us in the cache. */
#define	B_CALL			0x00020000		/* Call b_iodone from biodone. */
#define	B_WRITEINPROG	0x00040000		/* Write in progress. */
#define	B_NOCACHE		0x00080000		/* Do not cache block after use. */
#define	B_DIRTY			0x00100000		/* Dirty page to be pushed out async. */
#define	B_RAW			0x00200000		/* I/O to user memory. */
#define	B_PAGET			0x00400000		/* Page in/out of page table space. */
#define	B_PGIN			0x00800000		/* Pagein op, so swap() can count it. */
#define	B_UAREA			0x01000000		/* Buffer describes Uarea I/O. */
#define	B_NEEDCOMMIT	0x02000000		/* Append-write in progress. */
#define	B_EINTR			0x04000000		/* I/O was interrupted */
#define	B_GATHERED		0x08000000		/* LFS: already in a segment. */
#define	B_VFLUSH		0x10000000		/* Buffer is being synced. */
#define	B_XXX			0x20000000		/* Debugging flag. */

//simple_lock_init(&(bp)->b_interlock);	\

#define	BUF_INIT(bp) do {					\
	(bp)->b_dev = NODEV;					\
	BIO_SETPRIO((bp), BPRIO_DEFAULT);		\
} while (0)

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	bremhash(bp) { 						\
	LIST_REMOVE(bp, b_hash);				\
}

#define	binshash(bp, dp) { 					\
	LIST_INSERT_HEAD(dp, bp, b_hash);		\
}

/*
 * Insq/Remq for the buffer free lists.
 */
#define	binsheadfree(bp, dp) { 				\
	TAILQ_INSERT_HEAD(dp, bp, b_freelist);	\
}

#define	binstailfree(bp, dp) {				\
	TAILQ_INSERT_TAIL(dp, bp, b_freelist);	\
}

/*
 * Take a buffer off the free list it's on and
 * mark it as being use (B_BUSY) by a device.
 */
#define	notavail(bp, dp, field) { 			\
	KASSERT(LIST_FIRST(dp) != NULL); 		\
	KASSERT(LIST_PREV(bp, field) != NULL); 	\
	KASSERT(LIST_FIRST(dp) != (bp)); 		\
	LIST_REMOVE(bp, field);					\
	(bp)->b_flags |= B_BUSY; 				\
	LIST_FIRST(dp) = NULL;					\
	LIST_PREV(bp, field) = NULL;			\
}

/*
 * This structure describes a clustered I/O.  It is stored in the b_saveaddr
 * field of the buffer on which I/O is done.  At I/O completion, cluster
 * callback uses the structure to parcel I/O's to individual buffers, and
 * then free's this structure.
 */
struct cluster_save {
	long		bs_bcount;				/* Saved b_bcount. */
	long		bs_bufsize;				/* Saved b_bufsize. */
	void		*bs_saveaddr;			/* Saved b_addr. */
	int			bs_nchildren;			/* Number of associated buffers. */
	struct buf 	**bs_children;			/* List of associated buffers. */
};

#ifdef _KERNEL
TAILQ_HEAD(swqueue, buf); 			/* Head of swap I/O buffer headers free list. */
extern struct 	swqueue bswlist;
extern int		nbuf;				/* number of buffer headers */
extern struct	buf *buf;			/* the buffer pool itself */
extern char		*buffers;			/* The buffer contents. */
extern int		bufpages;			/* Number of memory pages in the buffer pool. */
extern struct	buf *swbuf;			/* Swap I/O buffer headers. */
extern int		nswbuf;				/* Number of swap I/O buffer headers. */

__BEGIN_DECLS
void		bufinit(void);
void		bremfree(struct buf *);
int			bread(struct vnode *, daddr_t, int, struct ucred *, struct buf **);
int			breadn(struct vnode *, daddr_t, int, daddr_t *, int *, int, struct ucred *, struct buf **);
int			breada(struct vnode *, daddr_t, daddr_t, int, struct ucred *);
int			bwrite(struct buf *);
void		bdwrite(struct buf *);
void		bawrite(struct buf *);
void		brelse(struct buf *);
struct 	buf *getnewbuf(int, int);
struct 	buf *getpbuf(void);
struct 	buf *incore(struct vnode *, daddr_t);
struct 	buf *getblk(struct vnode *, daddr_t, int, int, int);
struct 	buf *geteblk(int);
void		allocbuf(struct buf *, int);
int			biowait(struct buf *);
void		biodone(struct buf *);
void		blkflush(struct vnode *, daddr_t, dev_t);
void		bflush(struct vnode *, daddr_t, dev_t);
int			geterror(struct buf *);

void		cluster_callback(struct buf *);
int			cluster_read(struct vnode *, u_quad_t, daddr_t, long, struct ucred *, struct buf **);
void		cluster_write(struct buf *, u_quad_t);

int 		physio(void (*)(struct buf *), struct buf *, dev_t, int, void (*)(struct buf *), struct uio *);
void		minphys(struct buf *);
int			rawrw(dev_t, struct uio *, int);
int			rawread(dev_t, struct uio *);
int			rawwrite(dev_t, struct uio *);

void		vwakeup(struct buf *);
void		vmapbuf(struct buf *);
void		vunmapbuf(struct buf *);
void		relpbuf(struct buf *);
void		brelvp(struct buf *);
void		bgetvp(struct vnode *, struct buf *);
void		reassignbuf(struct buf *, struct vnode *);
__END_DECLS
#endif

#endif /* !_SYS_BUF_H_ */
