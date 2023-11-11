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

#ifdef notyet
#define	NINDEX				15 	/* comes from ufs inode */
#define	NGROUPS				10	/* number of group structures */
#define	NCHANS				20	/* number of channel structures */
#define	NLEVELS				4
#endif

struct channellist;
LIST_HEAD(channellist, mpx_channel);
struct mpx_channel {
    LIST_ENTRY(mpx_channel) mpc_node;       /* channel node in list */
    struct mpx_group	    *mpc_group;     /* this channels group */
    int 	                mpc_index;      /* channel index */
    void                    *mpc_data;		/* channel data */
    u_long					mpc_size;		/* channel size */
    int                     mpc_refcnt;		/* channel reference count */
};

#ifdef notyet
struct grouprbtree;
RB_HEAD(grouprbtree, mpx_group);
struct mpx_group {
	RB_ENTRY(mpx_group)    	mpg_node;     	/* group node in tree */
    struct mpx_channel      *mpg_channel;  	/* channels in this group */
    int 					 mpg_index;    	/* group index */
    struct pgrp				*mpg_pgrp;		/* proc group association (Optional) */
};
#endif

struct mpx {
    struct lock_object		mpx_slock;		/* mpx mutex */
    struct lock				mpx_lock;		/* long-term mpx lock */
    struct file				*mpx_file;
    int						mpx_nentries;	/* mpx number of entries */
    int 					mpx_refcnt;		/* mpx reference count */

    /* channels */
    struct channellist		mpx_chanlist;	/* list of channels */
    struct mpx_channel	    *mpx_channel;	/* channel back pointer */
    int 					mpx_nchans;		/* max number channels for this mpx */
#ifdef notyet
    /* groups */
    struct grouprbtree		mpx_grouptree;
    struct mpx_group		*mpx_group;
    int 					mpx_ngroups;	/* max number groups for this mpx */
#endif
};

#ifdef notyet
/* mpx type */
#define MPXCHANNEL 			0x00
#define MPXGROUP 			0x01
#endif

/* mpx args: mpxcall */
#define MPXCREATE			0
#define MPXDESTROY			1
#define MPXPUT				2
#define MPXREMOVE			3
#define MPXGET				4

#define MPX_LOCK(mpx)		simple_lock((mpx)->mpx_slock)
#define MPX_UNLOCK(mpx)		simple_unlock((mpx)->mpx_slock)

/* common routines */
void                		mpx_init(void);
struct mpx 					*mpx_allocate(int);
void						mpx_deallocate(struct mpx *);

/* channel functions */
struct mpx_channel 			*mpx_channel_create(int, u_long, void *, int);
void						mpx_channel_destroy(struct mpx_channel *);
void						mpx_channel_insert(struct mpx *, int, void *);
void						mpx_channel_insert_with_size(struct mpx *, int, u_long, void *);
void						mpx_channel_remove(struct mpx *, int, void *);
struct mpx_channel 			*mpx_channel_lookup(struct mpx *, int, void *);

/* mpx channel routines via mpxcall(aka syscall) */
int							mpx_create(struct mpx *, int, void *);
int							mpx_put(struct mpx *, int, void *);
int							mpx_get(struct mpx *, int, void *);
int							mpx_destroy(struct mpx *, int, void *);
int							mpx_remove(struct mpx *, int, void *);

#ifdef notyet
struct mpx_channel 			*mpx_get_channel_from_group(int);

/* groups */
struct mpx_group 			*mpx_allocate_groups(struct mpx *, int);
void						mpx_deallocate_groups(struct mpx *, struct mpx_group *);
void						mpx_add_group(struct mpx_group *, int);
void                		mpx_create_group(struct mpx *, int, int);
void						mpx_destroy_group(struct mpx *, int);
struct mpx_group    		*mpx_get_group(int);
struct mpx_group 			*mpx_get_group_from_channel(int);
void						mpx_remove_group(struct mpx_group *, int);

/* common routines */
void                		mpx_init(void);
struct mpx 					*mpx_alloc(void);
void						mpx_free(struct mpx *);
void						mpx_set_channelgroup(struct mpx_channel *, struct mpx_group *);
void						mpx_set_grouppgrp(struct mpx_group *, struct pgrp *);
void						mpx_attach(struct mpx_channel *, struct mpx_group *);
void						mpx_detach(struct mpx_channel *, struct mpx_group *);
int							mpx_connect(struct mpx_channel *, struct mpx_channel *);
struct mpx_channel 			*mpx_disconnect(struct mpx_channel *, int);
#endif

#endif /* SYS_MPX_H_ */
