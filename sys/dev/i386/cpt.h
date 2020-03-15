/*
 * cpt.h
 *
 *  Created on: 13 Mar 2020
 *      Author: marti
 */

/* Clustered Page Tables */

#ifndef MACHINE_CPT_H_
#define MACHINE_CPT_H_

#include <sys/tree.h>

/*
 * Add Resize
 * Make it so, CPT aligns to each page directory. (Can do by borrowing from FreeBSD)
 * superpages & subblocks
*/

/*
 * TODO:
 *  - change cpt_vpn from a char* to unsigned long (vm_offset)
 *  - Add pointer to struct pte into struct cpte
 *  - add struct pte to cpte_add
 *  - create lookup function that returns pte by boff/tag
 *  - cpte_tag creation: vpbn?
 */

struct cpt {
    RB_ENTRY(cpt)                 	cpt_entry;
    unsigned int                  	cpt_hindex;		/* VPBN Hash Result */
    u_long                   		cpt_pa_addr; 	/* Physical Address */
    u_long                   		cpt_va_addr; 	/* Virtual Address */
    u_long                 			cpt_pad;      	/* Partial-Subblock & Superpage Mapping */
    u_long 							cpt_sz;       	/* Superpage size: power of 2 */
    struct cpte                   	*cpt_cpte;    	/* pointer to cpte */
    struct pde						*cpt_pde;		/* pointer to pde (For Compatability) */
};
RB_HEAD(cpt_rbtree, cpt) cpt_root;

struct cpte {
    RB_ENTRY(cpte)    				cpte_entry;
    int               				cpte_boff;        /* Block Offset */
    struct pte        				*cpte_pte;        /* pointer to pte */
};
RB_HEAD(cpte_rbtree, cpte) cpte_root;

extern struct cpt cpt_base[];
extern struct cpte cpte_base[];

#define NKPT		30				/* Number of Kernel Page Table Pages (No PAE) Temp */

#define NCPT		(NKPT * 2)		/* Number of Buckets in Clustered Page Table (+ Overhead) */
#define NCPTE       16              /* Number of PTE's per Clustered Page Table Entry */

typedef struct cpt 	cpt_entry_t;		/* clustered page table */
typedef struct cpte cpte_entry_t;		/* clustered page table entry */

/* Clustered Page Table */
extern void         cpt_add(struct cpt *cpt, struct cpte *cpte, u_long vpbn);
extern struct cpt   *cpt_lookup(struct cpt *cpt, u_long vpbn);
extern void         cpt_remove(struct cpt *cpt, u_long vpbn);
struct cpte         *cpt_lookup_cpte(struct cpt *cpt, u_long vpbn);
extern void         cpt_add_superpage(struct cpt *cpt, struct cpte *cpte, u_long vpbn, u_long sz, u_long pad);
extern void         cpt_add_partial_subblock(struct cpt *cpt, struct cpte *cpte, u_long vpbn, u_long pad);
void 				cpt_to_pde(struct cpt *cpt, struct pde *pde);

/* Clustered Page Table Entries */
extern void         cpte_add(struct cpte *cpte, struct pte *pte, int boff);
extern struct cpte  *cpte_lookup(struct cpte *cpte, int boff);
extern void         cpte_remove(struct cpte *cpte, int boff);
extern struct pte   *cpte_lookup_pte(struct cpte *cpte, int boff);

unsigned int 		VPBN(vm_offset_t entry);

#endif /* MACHINE_CPT_H_ */
