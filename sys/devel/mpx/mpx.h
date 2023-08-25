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

struct channellist;
LIST_HEAD(channellist, mpx_channel);
struct mpx_channel {
    LIST_ENTRY(mpx_channel) mpc_node;       /* channel node in list */
    struct mpx_group	    *mpc_group;     /* this channels group */
    int 	                mpc_index;      /* channel index */
    void                    *mpc_data;		/* channel data */
    int                     mpc_refcnt;		/* channel reference count */
};

struct grouprbtree;
RB_HEAD(grouprbtree, mpx_group);
struct mpx_group {
	RB_ENTRY(mpx_group)    	mpg_node;     	/* group node in tree */
    struct mpx_channel      *mpg_channel;  	/* channels in this group */
    int 					 mpg_index;    	/* group index */
    struct pgrp				*mpg_pgrp;		/* proc group association (Optional) */
};

struct mpx {
    struct lock_object		mpx_slock;		/* mpx mutex */
    struct lock				mpx_lock;		/* long-term mpx lock */

    struct mpx_group		*mpx_group;
    struct mpx_channel	    *mpx_channel;
    struct file				*mpx_file;
};

/* mpx type */
#define MPXCHANNEL 			0x00
#define MPXGROUP 			0x01

/* mpx args */
#define MPXCREATE			0
#define MPXDESTROY			1
#define MPXPUT				2
#define MPXREMOVE			3
#define MPXGET				4

#define MPX_LOCK(mpx)		simple_lock((mpx)->mpx_slock)
#define MPX_UNLOCK(mpx)		simple_unlock((mpx)->mpx_slock)

extern struct grouprbtree   mpx_groups[];
extern struct channellist   mpx_channels[];
extern int groupcount;
extern int channelcount;

/* common routines */
void                		mpx_init(void);
struct mpx 					*mpx_alloc(void);
void						mpx_free(struct mpx *);

/* syscall callback */
int 						mpxcall(int, int, struct mpx *, int, void *);

/* channels */
struct mpx_channel 			*mpx_allocate_channels(struct mpx *, int);
void						mpx_deallocate_channels(struct mpx *, struct mpx_channel *);
void						mpx_add_channel(struct mpx_channel *, int, void *);
void						mpx_remove_channel(struct mpx_channel *, int, void *);
void                		mpx_create_channel(struct mpx *, int, void *, int);
void						mpx_destroy_channel(struct mpx *, int, void *);
struct mpx_channel     		*mpx_get_channel(int, void *);

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
