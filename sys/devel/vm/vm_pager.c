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

#include <vm/include/vm_kern.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_pager.h>

#ifdef SWAPPAGER
extern struct pagerops swappagerops;
#endif

#ifdef VNODEPAGER
extern struct pagerops vnodepagerops;
#endif

#ifdef DEVPAGER
extern struct pagerops devicepagerops;
#endif

#ifdef OVERLAYPAGER
extern struct pagerops overlaypagerops;
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
#ifdef OVERLAYPAGER
		&overlaypagerops, 	/* PG_OVERLAY */
#else
		NULL
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
bool_t pager_map_wanted;
vm_offset_t pager_sva, pager_eva;

void
vm_pager_init(void)
{
	struct pagerops **pgops;

	/*
	 * Allocate a kernel submap for tracking get/put page mappings
	 */
	pager_map = kmem_suballoc(kernel_map, &pager_sva, &pager_eva, PAGER_MAP_SIZE, FALSE);

	/*
	 * Initialize known pagers
	 */
	for (pgops = pagertab; pgops < &pagertab[npagers]; pgops++) {
		if (pgops) {
			(*(*pgops)->pgo_init)();
		}
	if (dfltpagerops == NULL) {
		panic("no default pager");
	}
}

vm_pager_t
vm_pager_allocate(type, handle, size, prot, off)
	int type;
	caddr_t handle;
	vm_size_t size;
	vm_prot_t prot;
	vm_offset_t off;
{
	struct pagerops *ops;

	ops = (type == PG_DFLT) ? dfltpagerops : pagertab[type];
	if (ops) {
		return ((*ops->pgo_alloc)(handle, size, prot, off));
	}
	return (NULL);
}

void
vm_pager_deallocate(pager)
	vm_pager_t	pager;
{
	if (pager == NULL) {
		panic("vm_pager_deallocate: null pager");
	}
	(*pager->pg_ops->pgo_dealloc)(pager);
}

int
vm_pager_put_segments(pager, slist, nsegments, sync)
	vm_pager_t	pager;
	vm_segment_t *slist;
	int			nsegments;
	bool_t		sync;
{
	return ((*pager->pg_ops->pgo_putsegments)(pager, slist, nsegments, sync));
}

vm_offset_t
vm_pager_map_segments(slist, nsegments, canwait)
	vm_segment_t	*slist;
	int			nsegments;
	bool_t		canwait;
{
	vm_offset_t kva, va;
	vm_size_t size;
	vm_segment_t s;

	size = nsegments * SEGMENT_SIZE;
	while (vm_map_findspace(pager_map, 0, size, &kva)) {

	}
	for (va = kva; nsegments--; va += SEGMENT_SIZE) {
		s = *slist++;
	}
	return (kva);
}
