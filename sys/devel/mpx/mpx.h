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

#ifndef _KERNEL
#include <sys/select.h>			/* for struct selinfo */
#endif

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


struct mpxmapping {
	caddr_t					kva;		/* kernel virtual address */
	vm_size_t				cnt;		/* number of chars in buffer */
	vm_size_t				pos;		/* current position within page */
	int						npages;		/* how many pages allocated */
	vm_page_t				pgs[MPXNPAGES];		/* pointers to the pages */
};

/*
 * Bits in mpx_state.
 */
#define MPX_ASYNC			0x0004	/* Async? I/O. */
#define MPX_WANTR			0x0008	/* Reader wants some characters. */
#define MPX_WANTW			0x0010	/* Writer wants space to put characters. */
#define MPX_WANT			0x0020	/* Pipe is wanted to be run-down. */
#define MPX_SEL				0x0040	/* Pipe has a select active. */
#define MPX_EOF				0x0080	/* Pipe is in EOF condition. */
#define MPX_LOCKFL			0x0100	/* Process has exclusive access to pointers/data. */
#define MPX_LWANT			0x0200	/* Process wants exclusive access to pointers/data. */
#define MPX_DIRECTW			0x0400	/* Pipe direct write active. */
#define MPX_DIRECTR			0x0800	/* Pipe direct read request (setup complete) */
#define MPX_DIRECTOK		0x1000	/* Direct mode ok. */
#define MPX_SIGNALR			0x2000	/* Do selwakeup() on read(2) */

struct channellist;
LIST_HEAD(channellist, mpx_channel);
struct mpx_channel {
    LIST_ENTRY(mpx_channel) mpc_node;       /* channel node in list */
    struct mpx_group	    *mpc_group;     /* this channels group */
    int 	                mpc_index;      /* channel index */
};

struct grouplist;
LIST_HEAD(grouplist, mpx_group);
struct mpx_group {
    LIST_ENTRY(mpx_group)    mpg_node;     	/* group node in list */
    struct mpx_channel      *mpg_channel;  	/* channels in this group */
    int 					 mpg_index;    	/* group index */
    struct pgrp				*mpg_pgrp;		/* proc group association (Optional) */
};

struct mpx {
    struct lock_object		mpx_slock;		/* mpx mutex */
    struct lock				mpx_lock;		/* long-term mpx lock */
	struct mpxbuf			mpx_buffer;		/* data storage */
    struct mpxmapping		mpx_map;		/* mpx mapping for direct I/O */
    struct selinfo 			mpx_sel;		/* for compat with select */
	struct timeval 	 		mpx_atime;		/* time of last access */
	struct timeval  		mpx_mtime;		/* time of last modify */
	struct timeval  		mpx_ctime;		/* time of status change */
	pid_t					mpx_pgid;		/* process group for sigio */
	struct mpxpair			*mpx_pair;
	struct mpx				*mpx_peer;		/* link with other direction */
	int 				    mpx_state;      /* mpx status info */
	int 					mpx_busy;		/* busy flag, mostly to handle rundown sanely */

    struct mpx_group		*mpx_group;
    struct mpx_channel	    *mpx_channel;
    struct file				*mpx_file;
};

struct mpxpair {
	struct mpx				mpp_wmpx;
	struct mpx				mpp_rmpx;
};

#define MPX_LOCK(mpx)		simple_lock((mpx)->mpx_slock)
#define MPX_UNLOCK(mpx)		simple_unlock((mpx)->mpx_slock)

extern struct grouplist     mpx_groups[];
extern struct channellist   mpx_channels[];
extern int groupcount;
extern int channelcount;

/* groups */
struct mpx_group 			*mpx_allocate_groups(struct mpx *, int);
void						mpx_add_group(struct mpx_group *, int);
void                		mpx_create_group(struct mpx *, int, int);
struct mpx_group    		*mpx_get_group(int);
struct mpx_group 			*mpx_get_group_from_channel(int);
void						mpx_remove_group(struct mpx_group *, int);
/* channels */
struct mpx_channel 			*mpx_allocate_channels(struct mpx *, int);
void						mpx_add_channel(struct mpx_channel *, int);
void                		mpx_create_channel(struct mpx *, int, int);
struct mpx_channel     		*mpx_get_channel(int);
struct mpx_channel 			*mpx_get_channel_from_group(int);
void						mpx_remove_channel(struct mpx_channel *, int);
/* common routines */
void                		mpx_init(void);
void						mpx_set_channelgroup(struct mpx_channel *, struct mpx_group *);
void						mpx_set_grouppgrp(struct mpx_group *, struct pgrp *);
void						mpx_attach(struct mpx_channel *, struct mpx_group *);
void						mpx_detach(struct mpx_channel *, struct mpx_group *);
int							mpx_connect(struct mpx_channel *, struct mpx_channel *);
struct mpx_channel 			*mpx_disconnect(struct mpx_channel *, int);

#define MPXIOATTACH			_IO('m', 1)
#define MPXIODETACH			_IO('m', 2)
#define MPXIOCONNECT		_IOW('m', 130, struct mpx_channel)
#define MPXIODISCONNECT		_IOW('m', 131, int)

#endif /* SYS_MPX_H_ */
