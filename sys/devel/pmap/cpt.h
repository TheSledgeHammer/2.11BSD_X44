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
 *
 *	@(#)cpt.h 1.0 	1/05/20
 */

/*
 * TODO:
 * 	- Add Resize
 * 	- Make it so, CPT aligns to each page directory. (Can do by borrowing from FreeBSD) superpages & subblocks
 *  - change cpt_vpn from a char* to unsigned long (vm_offset)
 *  - create lookup function that returns pte by boff/tag
 *  - cpte_tag creation: vpbn?
 */

/* Clustered Page Tables (Trees) */

#ifndef CPT_H_
#define CPT_H_

#include <sys/tree.h>

struct cpt {
    RB_ENTRY(cpt)                 	cpt_entry;
    u_long							cpt_hindex;
    u_long                   		cpt_pa_addr; 	/* Physical Address */
    u_long                   		cpt_va_addr; 	/* Virtual Address */
    u_long                 			cpt_pad;      	/* Partial-Subblock & Superpage Mapping */
    u_long 							cpt_sz;       	/* Superpage size: power of 2 */
    struct cpte                   	*cpt_cpte;    	/* pointer to cpte */
};
struct cpte_rbtree;
RB_HEAD(cpt_rbtree, cpt) cpt_root;

struct cpte {
    RB_ENTRY(cpte)    				cpte_entry;
    int               				cpte_boff;        /* Block Offset */
};
struct cpte_rbtree;
RB_HEAD(cpte_rbtree, cpte) cpte_root;

extern struct cpt cpt_base[];
extern struct cpte cpte_base[];

#define NKPT		30					/* Number of Kernel Page Table Pages (No PAE) Temp */

#define NCPT		NKPT				/* Number of Buckets in Clustered Page Table (+ Overhead) */
#define NCPTE       16              	/* Number of PTE's per Clustered Page Table Entry */

typedef struct cpt 	cpt_entry_t;		/* clustered page table */
typedef struct cpte cpte_entry_t;		/* clustered page table entry */

/* Virtual Page Block Number */
unsigned int 		VPBN(vm_offset_t entry);

extern cpt_entry_t 	*KCPTmap;			/* Kernel Clustered Page Table Mapping */
extern cpte_entry_t *KCPTEmap;			/* Kernel Clustered Page Table Entry Mapping */

/* Clustered Page Table */
extern void         cpt_add(struct cpt *cpt, struct cpte *cpte, u_long vpbn);
extern struct cpt   *cpt_lookup(struct cpt *cpt, u_long vpbn);
extern struct cpt   *cpt_traversal(struct cpt *cpt, u_long addr);
extern void         cpt_remove(struct cpt *cpt, u_long vpbn);
extern void         cpt_add_superpage(struct cpt *cpt, struct cpte *cpte, u_long vpbn, u_long sz, u_long pad);
extern void         cpt_add_partial_subblock(struct cpt *cpt, struct cpte *cpte, u_long vpbn, u_long pad);
struct cpte         *cpt_lookup_cpte(struct cpt *cpt, u_long vpbn);

/* Clustered Page Table Entries */
extern void         cpte_add(struct cpte *cpte, struct pte *pte, int boff);
extern struct cpte  *cpte_lookup(struct cpte *cpte, int boff);
extern void         cpte_remove(struct cpte *cpte, int boff);

/* PDE & PTE Compatibility */
extern struct pde	*cpt_to_pde(struct cpt *cpt, u_long vpbn);
extern struct cpt	*pde_to_cpt(struct pde *pde, u_long vpbn);
extern struct pte   *cpte_to_pte(struct cpte *cpte, int boff);
extern struct cpte	*pte_to_cpte(struct pte *pte, int boff);

#endif /* _CPT_H_ */
