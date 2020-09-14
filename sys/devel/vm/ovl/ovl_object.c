/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <devel/vm/include/vm_page.h>
#include <devel/vm/ovl/ovl_object.h>

struct ovl_object	kernel_ovl_object_store;
struct ovl_object	vm_ovl_object_store;

#define	OVL_OBJECT_HASH_COUNT	157

int		ovl_cache_max = 100;	/* can patch if necessary */
struct	ovl_object_hash_head 	ovl_object_hashtable[OVL_OBJECT_HASH_COUNT];

long	object_collapses = 0;
long	object_bypasses  = 0;

static void _ovl_object_allocate (vm_size_t, ovl_object_t);

/*
 *	vm_object_init:
 *
 *	Initialize the VM objects module.
 */
void
ovl_object_init(size)
	vm_size_t	size;
{
	register int	i;

	TAILQ_INIT(&ovl_object_cached_list);
	TAILQ_INIT(&ovl_object_list);
	ovl_object_count = 0;
	simple_lock_init(&ovl_cache_lock);
	simple_lock_init(&ovl_object_list_lock);

	for (i = 0; i < OVL_OBJECT_HASH_COUNT; i++)
		TAILQ_INIT(&ovl_object_hashtable[i]);

	kern_ovl_object = &kernel_object_store;
	_ovl_object_allocate(size, kern_ovl_object);

	vm_ovl_object = &vm_ovl_object_store;
	_ovl_object_allocate(size, vm_ovl_object);
}

/*
 *	ovl_object_allocate:
 *
 *	Returns a new object with the given size.
 */

ovl_object_t
ovl_object_allocate(size)
	vm_size_t	size;
{
	register ovl_object_t	result;

	result = (ovl_object_t)
		malloc((u_long)sizeof *result, M_OVLOBJ, M_WAITOK);

	_ovl_object_allocate(size, result);

	return(result);
}

static void
_ovl_object_allocate(size, object)
	vm_size_t				size;
	register ovl_object_t	object;
{
	//TAILQ_INIT(&object->memq);
	ovl_object_lock_init(object);
	object->ref_count = 1;
	object->size = size;
	object->flags = OBJ_INTERNAL;	/* vm_allocate_with_pager will reset */

	/*
	 *	Object starts out read-write.
	 */

	simple_lock(&ovl_object_list_lock);
	TAILQ_INSERT_TAIL(&ovl_object_list, object, object_list);
	ovl_object_count++;
	//cnt.v_nzfod += atop(size);
	simple_unlock(&ovl_object_list_lock);
}

/*
 *	ovl_object_reference:
 *
 *	Gets another reference to the given object.
 */
void
ovl_object_reference(object)
	register ovl_object_t	object;
{
	if (object == NULL)
		return;

	ovl_object_lock(object);
	object->ref_count++;
	ovl_object_unlock(object);
}

