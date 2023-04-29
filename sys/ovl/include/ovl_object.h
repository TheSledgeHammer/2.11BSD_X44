/*
 * Copyright (c) 1991, 1993
 * The Regents of the University of California.  All rights reserved.
 *
 *	Copyright (c) 2020
 *	Martin Kelly. All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_object.h	8.4 (Berkeley) 1/9/95
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _OVL_OBJECT_H_
#define _OVL_OBJECT_H_

#include <ovl/include/ovl_segment.h>

#include <vm/include/vm_object.h>
#include <vm/include/vm_pager.h>

struct ovl_object {
	struct ovl_seglist					seglist;			/* list of segments */

	RB_ENTRY(ovl_object)				object_tree;		/* list of all objects */

	vm_pager_t							pager;				/* where to get data */
	vm_offset_t							paging_offset;		/* Offset into paging space */
	u_short								paging_in_progress;

	u_short								flags;				/* see below */
	simple_lock_data_t					lock;				/* Synchronization */
	int									ref_count;			/* How many refs?? */
	vm_size_t							size;				/* Object size */

	int									segment_count;		/* number of resident segments */
	int									page_count;			/* number of resident pages */

	vm_object_t 						vm_object;			/* a vm_object being held */
	TAILQ_ENTRY(ovl_object)				vm_object_hlist; 	/* list of all my associated vm_objects */
};

/* Flags */
#define OVL_OBJ_VM_OBJ					0x01	/* overlay object holds vm_object */

RB_HEAD(ovl_object_hash_head, ovl_object_hash_entry);
struct ovl_object_hash_entry {
	RB_ENTRY(ovl_object_hash_entry)  	hlinks;			/* hash chain links */
	ovl_object_t						object;
};
typedef struct ovl_object_hash_entry	*ovl_object_hash_entry_t;

#ifdef _KERNEL
struct ovl_vm_object_hash_head;
struct ovl_object_rbt;
TAILQ_HEAD(ovl_vm_object_hash_head, ovl_object);
RB_HEAD(ovl_object_rbt, ovl_object);

extern
struct ovl_object_rbt		ovl_object_tree;				/* list of allocated objects */
extern
long						ovl_object_count;				/* count of all objects */
extern
simple_lock_data_t			ovl_object_tree_lock;			/* lock for object list and count */

extern
struct ovl_vm_object_hash_head 	ovl_vm_object_hashtable;
extern
long						ovl_vm_object_count;
extern
simple_lock_data_t			ovl_vm_object_hash_lock;

extern
ovl_object_t				overlay_object;				/* single overlay object */
extern
ovl_object_t				omem_object;


#define	ovl_object_tree_lock()			simple_lock(&ovl_object_tree_lock)
#define	ovl_object_tree_unlock()		simple_unlock(&ovl_object_tree_lock)

#define	ovl_vm_object_hash_lock()		simple_lock(&ovl_vm_object_hash_lock)
#define	ovl_vm_object_hash_unlock()		simple_unlock(&ovl_vm_object_hash_lock)

#endif /* KERNEL */

#define	ovl_object_lock_init(object)	simple_lock_init(&(object)->lock)
#define	ovl_object_lock(object)			simple_lock(&(object)->lock)
#define	ovl_object_unlock(object)		simple_unlock(&(object)->lock)

#ifdef _KERNEL
ovl_object_t	ovl_object_allocate(vm_size_t);
void		 	ovl_object_enter(ovl_object_t, vm_pager_t);
void		 	ovl_object_init(vm_size_t);
ovl_object_t	ovl_object_lookup(vm_pager_t);
void		 	ovl_object_reference(ovl_object_t);
void			ovl_object_deallocate(ovl_object_t);
void			ovl_object_terminate(ovl_object_t);
void			ovl_object_remove(vm_pager_t);
void		 ovl_object_setpager(ovl_object_t, vm_pager_t, vm_offset_t, bool_t);

#endif /* KERNEL */
#endif /* _OVL_OBJECT_H_ */
