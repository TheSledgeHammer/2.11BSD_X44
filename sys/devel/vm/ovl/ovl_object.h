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

#include <devel/vm/ovl/ovl.h>
#include <sys/tree.h>

struct vobject_hash_head;
TAILQ_HEAD(vobject_hash_head, ovl_object);
struct object_t;
RB_HEAD(object_t, ovl_object);
struct ovl_object {
	struct ovseglist					ovo_ovseglist;		/* list of segments */

	RB_ENTRY(ovl_object)				ovo_object_tree;	/* list of all objects */

	vm_pager_t							ovo_pager;			/* where to get data */
	vm_offset_t							ovo_paging_offset;	/* Offset into paging space */

	u_short								ovo_flags;			/* see below */
	simple_lock_data_t					ovo_lock;			/* Synchronization */
	int									ovo_ref_count;		/* How many refs?? */
	vm_size_t							ovo_size;			/* Object size */

	TAILQ_ENTRY(ovl_object)				ovo_vobject_hlist; 	/* list of all my associated vm_objects */
	union {
		vm_object_t 					vm_object;			/* a vm_object being held */
		vm_segment_t 					vm_segment;			/* a vm_segment being held */
		vm_page_t 						vm_page;			/* a vm_page being held */
	} ovo_vm;

#define ovo_vm_object					ovo_vm.vm_object
#define ovo_vm_segment					ovo_vm.vm_segment
#define ovo_vm_page						ovo_vm.vm_page
};

/* Flags */
#define OVL_OBJ_VM_OBJ		0x01	/* overlay object holds vm_object */

RB_HEAD(ovl_object_hash_head, ovl_object_hash_entry);
struct ovl_object_hash_entry {
	RB_ENTRY(ovl_object_hash_entry)  	ovoe_hlinks;		/* hash chain links */
	ovl_object_t						ovoe_object;
};
typedef struct ovl_object_hash_entry	*ovl_object_hash_entry_t;

//#ifdef KERNEL
struct object_t				ovl_object_tree;		/* list of allocated objects */
long						ovl_object_count;		/* count of all objects */
simple_lock_data_t			ovl_object_tree_lock;	/* lock for object list and count */

ovl_object_t				overlay_object;			/* single overlay object */
ovl_object_t				omem_object;

extern
struct vobject_hash_head 	ovl_vobject_hashtable;
long						ovl_vobject_count;
simple_lock_data_t			ovl_vobject_hash_lock;

//#endif /* KERNEL */

#define	ovl_object_lock_init(object)	simple_lock_init(&(object)->ovo_lock)
#define	ovl_object_lock(object)			simple_lock(&(object)->ovo_lock)
#define	ovl_object_unlock(object)		simple_unlock(&(object)->ovo_lock)

//#ifdef KERNEL
ovl_object_t	ovl_object_allocate (vm_size_t);
void		 	ovl_object_enter (ovl_object_t, vm_pager_t);
void		 	ovl_object_init (vm_size_t);
ovl_object_t	ovl_object_lookup (vm_pager_t);
void		 	ovl_object_reference (ovl_object_t);
void			ovl_object_remove (vm_pager_t);

void			ovl_object_insert_vm_object (ovl_object_t, vm_object_t);
vm_object_t		ovl_object_lookup_vm_object (ovl_object_t, vm_object_t);
void			ovl_object_remove_vm_object (ovl_object_t, vm_object_t);

//#endif /* KERNEL */
#endif /* _OVL_OBJECT_H_ */
