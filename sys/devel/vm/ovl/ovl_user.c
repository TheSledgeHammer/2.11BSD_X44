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
 *	@(#)vm_user.c	8.2 (Berkeley) 1/12/94
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

/*
 *	User-exported overlay memory functions.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <devel/vm/ovl/ovl.h>

int
ovl_allocate(map, addr, size, anywhere)
	register ovl_map_t		map;
	register vm_offset_t	*addr;
	register vm_size_t		size;
	boolean_t				anywhere;
{
	int	result;

	if(map == NULL) {
		return (KERN_INVALID_ARGUMENT);
	}
	if (size == 0) {
		*addr = 0;
		return (KERN_SUCCESS);
	}
	if (anywhere) {
		*addr = ovl_map_min(map);
	}
	result = ovl_map_find(map, NULL, (vm_offset_t) 0, addr, size, anywhere);

	return (result);
}

int
ovl_deallocate(map, start, size)
	register ovl_map_t	map;
	vm_offset_t			start;
	vm_size_t			size;
{
	if (map == NULL)
		return(KERN_INVALID_ARGUMENT);

	if (size == (vm_offset_t) 0)
		return(KERN_SUCCESS);

	return (ovl_map_remove(map, start, (start+size)));
}

/*
 * Similar to ovl_allocate but assigns an explicit overlayer.
 */
int
ovl_allocate_with_overlayer(map, addr, size, anywhere, overlayer, poffset, internal)
	register ovl_map_t		map;
	register vm_offset_t	*addr;
	register vm_size_t		size;
	boolean_t				anywhere;
	ovl_overlay_t			overlayer;
	vm_offset_t				poffset;
	boolean_t				internal;
{
	register ovl_object_t	object;
	register int			result;

	if (map == NULL)
		return(KERN_INVALID_ARGUMENT);

	/*
	 *	Lookup the pager/paging-space in the object cache.
	 *	If it's not there, then create a new object and cache
	 *	it.
	 */
	object = ovl_object_lookup(overlayer);

	if (object == NULL) {
		object = ovl_object_allocate(size);
		/*
		 * From Mike Hibler: "unnamed anonymous objects should never
		 * be on the hash list ... For now you can just change
		 * vm_allocate_with_pager to not do vm_object_enter if this
		 * is an internal object ..."
		 */
		if (!internal)
			ovl_object_enter(object, overlayer);
	}

	if (internal)
		object->ovo_flags |= OBJ_INTERNAL;
	else {
		object->ovo_flags &= ~OBJ_INTERNAL;
	}

	result = ovl_map_find(map, object, poffset, addr, size, anywhere);
	if (result != KERN_SUCCESS)
		ovl_object_deallocate(object);
	else if (overlayer != NULL)
		ovl_object_setoverlayer(object, overlayer, (vm_offset_t) 0);
	return(result);
}
