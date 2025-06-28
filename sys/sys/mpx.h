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

struct channellist;
LIST_HEAD(channellist, mpx_channel);
struct mpx_channel {
    LIST_ENTRY(mpx_channel) mpc_node;       /* channel node in list */
    //struct mpx_group	    *mpc_group;     /* this channels group */
    int 	                mpc_index;      /* channel index */
    void                    *mpc_data;		/* channel data */
    u_long					mpc_size;		/* channel size */
    int                     mpc_refcnt;		/* channel reference count */
};

struct mpx {
    struct lock_object		mpx_slock;		/* mpx mutex */
    struct lock				mpx_lock;		/* long-term mpx lock */
    struct file				*mpx_file;
    int						mpx_nentries;	/* mpx number of entries */
    int 					mpx_refcnt;		/* mpx reference count */

    /* channels */
    struct channellist		*mpx_chanlist;	/* list of channels */
    struct mpx_channel	    *mpx_channel;	/* channel back pointer */
    int 					mpx_nchans;		/* max number channels for this mpx */
};

/*
 * cmd options for mpx syscalls
 * - create, destroy, put, remove, get
 */
enum mpx_cmdops {
	MPXCREATE,
	MPXDESTROY,
	MPXPUT,
	MPXREMOVE,
	MPXGET
};

#define MPX_LOCK_INIT(mpx, name)  simple_lock_init(&(mpx)->mpx_slock, name)
#define MPX_LOCK(mpx)		simple_lock(&(mpx)->mpx_slock)
#define MPX_UNLOCK(mpx)		simple_unlock(&(mpx)->mpx_slock)

/*
extern struct lock_holder mpx_loholder;
#define MPX_MUTEX_INIT(mpx, name, data, pid)	mtx_init(mpx, &mpx_loholder, name, data, pid)
#define MPX_MUTEX_ENTER(mpx) 					mtx_lock(mpx, &mpx_loholder)
#define MPX_MUTEX_EXIT(mpx)	 					mtx_unlock(mpx, &mpx_loholder)
*/
#ifdef _KERNEL
struct mpx *mpx_allocate(int);
void mpx_deallocate(struct mpx *);
int mpx_channel_create(struct mpx *, int, u_long, void *);
int mpx_channel_put(struct mpx *, int, u_long, void *);
int mpx_channel_get(struct mpx *, int, void *);
int mpx_channel_destroy(struct mpx *, int, void *);
int mpx_channel_remove(struct mpx *, int, void *);

#else
#include <sys/cdefs.h>

__BEGIN_DECLS
int mpx(int, struct mpx *, int, void *);
int mpx_create(struct mpx *, int, void *);
int mpx_put(struct mpx *, int, void *);
int mpx_get(struct mpx *, int, void *);
int mpx_destroy(struct mpx *, int, void *);
int mpx_remove(struct mpx *, int, void *);
__END_DECLS
#endif /* !_KERNEL */
#endif /* SYS_MPX_H_ */
