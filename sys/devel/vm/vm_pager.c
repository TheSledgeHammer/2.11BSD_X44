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
 *	@(#)vm_pager.c	8.7 (Berkeley) 7/7/94
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
 *	Paging space routine stubs.  Emulates a matchmaker-like interface
 *	for builtin pagers.
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm_segment.h>
#include <vm/include/vm_kern.h>

#ifdef SWAPPAGER
extern struct pagerops swappagerops;
#endif

#ifdef VNODEPAGER
extern struct pagerops vnodepagerops;
#endif

#ifdef DEVPAGER
extern struct pagerops devicepagerops;
#endif

struct pagerops *pagertab[] = {
#ifdef SWAPPAGER
	&swappagerops,		/* PG_SWAP */
#else
	NULL,
#endif
#ifdef VNODEPAGER
	&vnodepagerops,		/* PG_VNODE */
#else
	NULL,
#endif
#ifdef DEVPAGER
	&devicepagerops,	/* PG_DEV */
#else
	NULL,
#endif
};
int npagers = sizeof (pagertab) / sizeof (pagertab[0]);

struct pagerops *dfltpagerops = NULL;	/* default pager */

/*
 * Kernel address space for mapping pages.
 * Used by pagers where KVAs are needed for IO.
 *
 * XXX needs to be large enough to support the number of pending async
 * cleaning requests (NPENDINGIO == 64) * the maximum swap cluster size
 * (MAXPHYS == 64k) if you want to get the most efficiency.
 */
#define PAGER_MAP_SIZE	(4 * 1024 * 1024)

vm_map_t pager_map;
boolean_t pager_map_wanted;
vm_offset_t pager_sva, pager_eva;

int
vm_pager_get_segments(pager, slist, nsegments, sync)
        vm_pager_t	    pager;
        vm_segment_t	*slist;
        int		        nsegments;
        bool	        sync;
{
    int rv;

    if (pager == NULL) {
        rv = VM_PAGER_OK;
        while (nsegments--) {
            if(!vm_segment_zero_fill(*slist)) {
                rv = VM_PAGER_FAIL;
                break;
            } else {
                slist++;
            }
        }
        return (rv);
    }
    return ((*pager->pg_ops->pgo_getsegments)(pager, slist, nsegments, sync));
}

int
vm_pager_put_segments(pager, slist, nsegments, sync)
	vm_pager_t		pager;
	vm_segment_t	*slist;
	int				nsegments;
	boolean_t		sync;
{
	if (pager == NULL)
		panic("vm_pager_put_segments: null pager");
	return ((*pager->pg_ops->pgo_putsegments)(pager, slist, nsegments, sync));
}

boolean_t
vm_pager_has_segment(pager, offset)
	vm_pager_t	pager;
	vm_offset_t	offset;
{
	if (pager == NULL)
		panic("vm_pager_has_segment: null pager");
	return ((*pager->pg_ops->pgo_hassegment)(pager, offset));
}

vm_segment_t
vm_pager_atos(kva)
	vm_offset_t	kva;
{
	vm_offset_t pa;

	pa = pmap_extract(vm_map_pmap(pager_map), kva);
	if (pa == 0)
		panic("vm_pager_stop");
	return (PHYS_TO_VM_SEGMENT(pa));
}

vm_offset_t
vm_pager_map_segments(slist, nsegments, canwait)
	vm_segment_t	*slist;
	int				nsegments;
	boolean_t		canwait;
{
	vm_offset_t kva, va;
	vm_size_t size;
	vm_segment_t s;
	vm_segment_lookup(s->sg_object, s->sg_offset)->sg_logical_addr;
	/*
	 * Allocate space in the pager map, if none available return 0.
	 * This is basically an expansion of kmem_alloc_wait with optional
	 * blocking on no space.
	 */
	size = nsegments * SEGMENT_SIZE;
	vm_map_lock(pager_map);
	while (vm_map_findspace(pager_map, 0, size, &kva)) {
		if (!canwait) {
			vm_map_unlock(pager_map);
			return (0);
		}
		pager_map_wanted = TRUE;
		vm_map_unlock(pager_map);
		(void) tsleep(pager_map, PVM, "pager_map", 0);
		vm_map_lock(pager_map);
	}
	vm_map_insert(pager_map, NULL, 0, kva, kva + size);
	vm_map_unlock(pager_map);
	for (va = kva; nsegments--; va += SEGMENT_SIZE) {
		s = *slist++;
#ifdef DEBUG
		if ((s->sg_flags & SEG_BUSY) == 0)
			panic("vm_pager_map_segments: segment not busy");
		if (s->sg_flags & PG_PAGEROWNED)
			panic("vm_pager_map_segments: segment already in pager");
#endif
#ifdef DEBUG
		s->sg_flags |=  PG_PAGEROWNED;
#endif
		pmap_enter(vm_map_pmap(pager_map), va, VM_SEGMENT_TO_PHYS(s), VM_PROT_DEFAULT, TRUE);
	}
	return (kva);
}

void
vm_pager_unmap_segments(kva, nsegments)
	vm_offset_t	kva;
	int			nsegments;
{
	vm_size_t size = nsegments * SEGMENT_SIZE;

#ifdef DEBUG
	vm_offset_t va;
	vm_segment_t s;
	int np = nsegments;

	for (va = kva; np--; va += SEGMENT_SIZE) {
		s = vm_pager_atos(va);
		if (s->sg_flags & PG_PAGEROWNED)
			s->sg_flags &= ~PG_PAGEROWNED;
		else
			printf("vm_pager_unmap_segments: %x(%x/%x) not owned\n", s, va, VM_SEGMENT_TO_PHYS(s));
	}
#endif

	pmap_remove(vm_map_pmap(pager_map), kva, kva + size);
	vm_map_lock(pager_map);
	(void) vm_map_delete(pager_map, kva, kva + size);
	if (pager_map_wanted)
		wakeup(pager_map);
	vm_map_unlock(pager_map);
}
