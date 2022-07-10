/*	$NetBSD: pmap.c,v 1.171.2.1.2.2 2005/08/24 04:08:09 riz Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _I386_PMAP_TLB_H_
#define _I386_PMAP_TLB_H_

#include <sys/lock.h>
#include <sys/queue.h>

/* TLB Shootdown job queue */
struct pmap_tlb_shootdown_job {
	TAILQ_ENTRY(pmap_tlb_shootdown_job) 	pj_list;
	vm_offset_t 							pj_va;			/* virtual address */
	vm_offset_t 							pj_sva;			/* virtual address start */
	vm_offset_t 							pj_eva;			/* virtual address end */
	pt_entry_t 								pj_pte;			/* the PTE bits */
	pmap_t 									pj_pmap;		/* the pmap which maps the address */
	struct pmap_tlb_shootdown_job 			*pj_nextfree;
};

#define PMAP_TLB_SHOOTDOWN_JOB_ALIGN 32
union pmap_tlb_shootdown_job_al {
	struct pmap_tlb_shootdown_job 			pja_job;
	char 									pja_align[PMAP_TLB_SHOOTDOWN_JOB_ALIGN];
};

struct pmap_tlb_shootdown_q {
	TAILQ_HEAD(, pmap_tlb_shootdown_job) 	pq_head;
	int 									pq_count;		/* number of pending requests */
	struct lock_object						pq_slock;		/* spin lock on queue */
	int 									pq_flushg;		/* pending flush global */
	int 									pq_flushu;		/* pending flush user */
} pmap_tlb_shootdown_q[NCPUS];

#define	PMAP_TLB_MAXJOBS					16

struct lock_object 							pmap_tlb_shootdown_job_lock;
union pmap_tlb_shootdown_job_al 			*pj_page, *pj_free;

#endif /* _I386_PMAP_TLB_H_ */
