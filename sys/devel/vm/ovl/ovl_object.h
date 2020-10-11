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

struct ovl_object {
	RB_ENTRY(ovl_object)				ovo_object_tree;	/* list of all objects */
	u_long								ovo_index;
	u_short								ovo_flags;			/* see below */
	simple_lock_data_t					ovo_lock;			/* Synchronization */
	int									ovo_ref_count;		/* How many refs?? */
	vm_size_t							ovo_size;			/* Object size */
};

/*
 * Flags
 */
#define OVL_OBJ_CANPERSIST	0x0001	/* allow to persist */
#define OVL_OBJ_INTERNAL	0x0002	/* internally created object */
#define OVL_OBJ_ACTIVE		0x0004	/* used to mark active objects */
#define OVL_OBJ_KERNEL		0x0006	/* kernel overlay object */
#define OVL_OBJ_VM			0x0008	/* vm overlay object */

RB_HEAD(ovl_object_hash_head, ovl_object_hash_entry);
struct ovl_object_hash_entry {
	RB_ENTRY(ovl_object_hash_entry)  	ovoe_hlinks;		/* hash chain links */
	ovl_object_t						ovoe_object;
};
typedef struct ovl_object_hash_entry	*ovl_object_hash_entry_t;

#ifdef KERNEL
struct object_t;
RB_HEAD(object_t, ovl_object);

struct object_t		ovl_object_cached_tree;	/* list of objects persisting */
int					ovl_object_cached;		/* size of cached list */
simple_lock_data_t	ovl_cache_lock;			/* lock for object cache */

struct object_t		ovl_object_tree;		/* list of allocated objects */
long				ovl_object_count;		/* count of all objects */
simple_lock_data_t	ovl_object_tree_lock;
											/* lock for object list and count */

ovl_object_t		kern_ovl_object;		/* single kernel overlay object */
ovl_object_t		vm_ovl_object;			/* single vm overlay object */

#define	ovl_object_cache_lock()			simple_lock(&ovl_cache_lock)
#define	ovl_object_cache_unlock()		simple_unlock(&ovl_cache_lock)
#endif /* KERNEL */

#define	ovl_object_lock_init(object)	simple_lock_init(&(object)->ovo_lock)
#define	ovl_object_lock(object)			simple_lock(&(object)->ovo_lock)
#define	ovl_object_unlock(object)		simple_unlock(&(object)->ovo_lock)
#define	ovl_object_lock_try(object)		simple_lock_try(&(object)->ovo_lock)
#define	ovl_object_sleep(event, object, interruptible) \
			thread_sleep((event), &(object)->ovo_lock, (interruptible))

//#ifdef KERNEL
ovl_object_t	ovl_object_allocate (vm_size_t);
void		 	ovl_object_cache_clear (void);
void			ovl_object_cache_trim (void);
void		 	ovl_object_deallocate (ovl_object_t);
void		 	ovl_object_enter (ovl_object_t, u_long);
void		 	ovl_object_init (vm_size_t);
ovl_object_t	ovl_object_lookup (u_long);
void		 	ovl_object_reference (ovl_object_t);
void			ovl_object_remove(u_long);
void		 	ovl_object_terminate (ovl_object_t);

void		 	ovl_object_collapse (ovl_object_t);
void		 	ovl_object_copy_vm_object (ovl_object_t, vm_offset_t, vm_size_t, vm_object_t *, vm_offset_t *, boolean_t *);
void		 	ovl_object_copy_avm_object (ovl_object_t, vm_offset_t, vm_size_t, avm_object_t *, vm_offset_t *, boolean_t *);
void			ovl_object_copy_to();
void			ovl_object_copy_from();
void			ovl_object_shadow(ovl_object_t *, vm_offset_t *, vm_size_t);
/* Place in segment? */
void			ovl_object_coalesce_segment();
void			ovl_object_split_segment();
void			ovl_object_shrink_segment();
void			ovl_object_expand_segment();

//#endif /* KERNEL */
#endif /* _OVL_OBJECT_H_ */
