/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYS_MPX_H_
#define SYS_MPX_H_

#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/tree.h>

#define	NINDEX				15 	/* comes from ufs inode */
#define	NGROUPS				10	/* number of group structures */
#define	NCHANS				20	/* number of channel structures */
#define	NLEVELS				4

#ifndef MPX_SIZE
#define MPX_SIZE			16384
#endif

#ifndef BIG_MPX_SIZE
#define BIG_MPX_SIZE		(64*1024)
#endif

#ifndef SMALL_MPX_SIZE
#define SMALL_MPX_SIZE		PAGE_SIZE
#endif

/*
 * MPX_MINDIRECT MUST be smaller than MPX_SIZE and MUST be bigger
 * than MPX_BUF.
 */
#ifndef MPX_MINDIRECT
#define MPX_MINDIRECT		8192
#endif

#define MPXNPAGES			(BIG_MPX_SIZE / PAGE_SIZE + 1)

struct mpxbuf {
	u_int					cnt;		/* number of chars currently in buffer */
	u_int					in;			/* in pointer */
	u_int					out;		/* out pointer */
	u_int					size;		/* size of buffer */
	caddr_t					buffer;		/* kva of buffer */
};

/*
 * Bits in mpx_state.
 */
#define MPX_ASYNC		0x004	/* Async? I/O. */
#define MPX_WANTR		0x008	/* Reader wants some characters. */
#define MPX_WANTW		0x010	/* Writer wants space to put characters. */
#define MPX_WANT		0x020	/* Pipe is wanted to be run-down. */
#define MPX_SEL			0x040	/* Pipe has a select active. */
#define MPX_EOF			0x080	/* Pipe is in EOF condition. */
#define MPX_LOCKFL		0x100	/* Process has exclusive access to pointers/data. */
#define MPX_LWANT		0x200	/* Process wants exclusive access to pointers/data. */
#define MPX_DIRECTW		0x400	/* Pipe direct write active. */
#define MPX_DIRECTOK	0x800	/* Direct mode ok. */

struct channellist;
LIST_HEAD(channellist, mpx_channel);
struct mpx_channel {
    LIST_ENTRY(mpx_channel) mpc_node;       /* channel node in list */
    struct mpx_group	    *mpc_group;     /* this channels group */
    int 	                mpc_index;      /* channel index */
    int 			        mpc_flags;      /* channel flags */
};

struct grouplist;
LIST_HEAD(grouplist, mpx_group);
struct mpx_group {
    LIST_ENTRY(mpx_group)    mpg_node;     /* group node in list */
    struct mpx_channel      *mpg_channel;  /* channels in this group */
    int 					 mpg_index;    /* group index */
};

struct mpx {
	struct mpxbuf			mpx_buffer;
    struct mpx_group		*mpx_group;
    struct mpx_channel	    *mpx_channel;
    struct mpxpair			*mpx_pair;

	int 				    mpx_state;      /* state */
	struct selinfo 			mpx_sel;		/* for compat with select */
	struct timespec	 		mpx_atime;		/* time of last access */
	struct timespec 		mpx_mtime;		/* time of last modify */
	struct timespec 		mpx_ctime;		/* time of status change */

    struct file				*mpx_file;
};

struct mpxpair {
	struct mpx				mpp_wmpx;
	struct mpx				mpp_rmpx;
    struct lock_object		mpp_lock;
};

#define MPX_LOCK_INIT(mpp)	simple_lock_init((mpp)->mpp_lock, "mpxpair_lock")
#define MPX_LOCK(mpp)		simple_lock((mpp)->mpp_lock)
#define MPX_UNLOCK(mpp)		simple_unlock((mpp)->mpp_lock)

extern struct grouplist     mpx_groups[];
extern struct channellist   mpx_channels[];
extern long					maxmpxkva;

void                		mpx_init(void);
void                		mpx_create_group(int);
struct mpx_group    		*mpx_get_group(int);
void						mpx_remove_group(struct mpx_group *, int);
void                		mpx_create_channel(struct mpx_group *, int, int);
struct mpx_channel     		*mpx_get_channel(int);
void						mpx_remove_channel(struct mpx_channel *, int);
void						mpx_attach(struct mpx_channel *, struct mpx_group *);
void						mpx_detach(struct mpx_channel *, struct mpx_group *);
int							mpx_connect(struct mpx_channel *, struct mpx_channel *);

#endif /* SYS_MPX_H_ */
