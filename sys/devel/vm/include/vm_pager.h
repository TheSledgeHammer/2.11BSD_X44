/*
 * Copyright (c) 1990 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)vm_pager.h	8.5 (Berkeley) 7/7/94
 */

/*
 * Pager routine interface definition.
 * For BSD we use a cleaner version of the internal pager interface.
 */

#ifndef _VM_PAGER_H_
#define _VM_PAGER_H_

#include <sys/queue.h>

TAILQ_HEAD(pagerlst, pager_struct);
struct pager_struct {
	TAILQ_ENTRY(pager_struct) 	pg_list;	/* links for list management */
	caddr_t			  			pg_handle;	/* ext. handle (vp, dev, fp, ovl) */
	int			  				pg_type;	/* type of pager */
	int			  				pg_flags;	/* flags */
	struct pagerops				*pg_ops;	/* pager operations */
	void			  			*pg_data;	/* private pager data */
};

/* pager types */
#define PG_DFLT			-1
#define	PG_SWAP			0
#define	PG_VNODE		1
#define PG_DEVICE		2
#define PG_OVERLAY		3

/* flags */
#define PG_CLUSTERGET	1
#define PG_CLUSTERPUT	2

struct pagerops {
	/* Initialize pager. */
	void		(*pgo_init)(void);
	/* Allocate pager. */
	vm_pager_t	(*pgo_alloc)(caddr_t, vm_size_t, vm_prot_t, vm_offset_t);
	/* Deallocate pager. */
	void		(*pgo_dealloc)(vm_pager_t);
	/* Get (read) page. */
	int			(*pgo_getpages)(vm_pager_t, vm_page_t *, int, bool_t);
	/* Put (write) page. */
	int			(*pgo_putpages)(vm_pager_t, vm_page_t *, int, bool_t);
	/* Does pager have page? */
	bool_t  	(*pgo_haspage)(vm_pager_t, vm_offset_t);
	/* Return range of cluster. */
	void		(*pgo_cluster)(vm_pager_t, vm_offset_t, vm_offset_t *, vm_offset_t *);

	/* Get (read) segment. */
	int			(*pgo_getsegments)(vm_pager_t, vm_segment_t *, int, bool_t);
	/* Put (write) segment. */
	int			(*pgo_putsegments)(vm_pager_t, vm_segment_t *, int, bool_t);
	/* Does pager have page? */
	bool_t  	(*pgo_hassegment)(vm_pager_t, vm_offset_t);
};

/*
 * get/put return values
 * OK	 operation was successful
 * BAD	 specified data was out of the accepted range
 * FAIL	 specified data was in range, but doesn't exist
 * PEND	 operations was initiated but not completed
 * ERROR error while accessing data that is in range and exists
 * AGAIN temporary resource shortage prevented operation from happening
 */
#define	VM_PAGER_OK			0
#define	VM_PAGER_BAD		1
#define	VM_PAGER_FAIL		2
#define	VM_PAGER_PEND		3
#define	VM_PAGER_ERROR		4
#define VM_PAGER_AGAIN		5

#ifdef _KERNEL
extern struct pagerops *dfltpagerops;

void		 vm_pager_init(void);
vm_pager_t	 vm_pager_allocate(int, caddr_t, vm_size_t, vm_prot_t, vm_offset_t);
void		 vm_pager_deallocate(vm_pager_t);
#endif
#endif /* _VM_PAGER_H_ */
