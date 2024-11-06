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
 *	@(#)vm_page.h	8.3 (Berkeley) 1/9/95
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

#ifndef OVL_PAGE_H_
#define OVL_PAGE_H_

#include <vm/include/vm_page.h>

struct ovl_vm_page_hash_head;
TAILQ_HEAD(ovl_vm_page_hash_head, ovl_page);
struct ovl_pglist;
TAILQ_HEAD(ovl_pglist, ovl_page);
struct ovl_page {
	TAILQ_ENTRY(ovl_page)	hashq;					/* hash table links (S)*/
	TAILQ_ENTRY(ovl_page)	listq;					/* pages in same segment (S)*/

	ovl_segment_t			segment;				/* which segment am I in (O,(S,P))*/
	vm_offset_t				offset;					/* offset into segment (O,(S,P)) */

	u_short					flags;					/* see below */
	vm_offset_t				phys_addr;				/* physical address of page */

	vm_page_t				vm_page;				/* a vm_page being held */
	TAILQ_ENTRY(ovl_page) 	vm_page_hlist;			/* list of all my associated vm_pages */
};

/* flags */
#define OVL_PG_VM_PG			0x16				/* overlay page holds vm_page */

#ifdef _KERNEL

extern
struct ovl_pglist				ovl_page_list;
extern
simple_lock_data_t				ovl_page_list_lock;
extern
struct ovl_vm_page_hash_head   	*ovl_vm_page_hashtable;
extern
long				       		ovl_vm_page_count;
extern
simple_lock_data_t				ovl_vm_page_hash_lock;

extern
long							ovl_first_page;
extern
long							ovl_last_page;

extern
vm_offset_t						ovl_first_phys_addr;
extern
vm_offset_t						ovl_last_phys_addr;

#define	ovl_page_lock_lists()		simple_lock(&ovl_page_list_lock)
#define	ovl_page_unlock_lists()		simple_unlock(&ovl_page_list_lock)

#define	ovl_vm_page_hash_lock()		simple_lock(&ovl_vm_page_hash_lock)
#define	ovl_vm_page_hash_unlock()	simple_unlock(&ovl_vm_page_hash_lock)

void 		*ovl_pmap_bootinit(void *, vm_size_t, int);
void		ovl_page_startup(vm_offset_t *, vm_offset_t *);
void		ovl_page_insert(ovl_page_t, ovl_segment_t, vm_offset_t);
void		ovl_page_remove(ovl_page_t);
ovl_page_t	ovl_page_lookup(ovl_segment_t, vm_offset_t);

//vm_page_copy_to_ovl_page		/* inserts into ovl_page hash list */
//vm_page_copy_from_ovl_page	/* removes from ovl_page hash list */
#endif /* KERNEL */
#endif /* _OVL_PAGE_H_ */
