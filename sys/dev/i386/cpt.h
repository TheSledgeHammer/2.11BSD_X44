/*
 * cpt.h
 *
 *  Created on: 13 Mar 2020
 *      Author: marti
 */

/* Clustered Page Tables */

#ifndef MACHINE_CPT_H_
#define MACHINE_CPT_H_

#include <vm/include/vm.h>
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
    RB_ENTRY(cpt)                 cpt_entry;
    unsigned int                  cpt_hindex;	/* VPBN Hash Result */
    vm_offset_t                   cpt_addr; 	/* Address */
    unsigned long                 cpt_pad;      /* Partial-Subblock & Superpage Mapping */
    unsigned long                 cpt_sz;       /* Superpage size: power of 2 */
    struct cpte                   *cpt_cpte;    /* pointer to cpte */
};
RB_HEAD(cpt_rbtree, cpt) cpt_root;

struct cpte {
    RB_ENTRY(cpte)    cpte_entry;
    int               cpte_boff;        /* Block Offset */
    struct pte        *cpte_pte;        /* pointer to pte */
};
RB_HEAD(cpte_rbtree, cpte) cpte_root;

extern struct cpt cpt_base[];
extern struct cpte cpte_base[];

#define NCPTE       16              /* Number of PTE's per CPTE */
#define CPTSBLKF    16              /* CPT's sub-block factor */
#define NBPG        4096            /* bytes/page (temp in i386) */

/* Clustered Page Table */
extern void         cpt_add(struct cpt *cpt, struct cpte *cpte, vm_offset_t vpbn);
extern struct cpt   *cpt_lookup(struct cpt *cpt, vm_offset_t vpbn);
extern void         cpt_remove(struct cpt *cpt, vm_offset_t vpbn);
struct cpte         *cpt_lookup_cpte(struct cpt *cpt, vm_offset_t vpbn);
extern void         cpt_add_superpage(struct cpt *cpt, struct cpte *cpte, vm_offset_t vpbn,
						unsigned long sz, unsigned long pad);
extern void         cpt_add_partial_subblock(struct cpt *cpt, struct cpte *cpte, vm_offset_t vpbn,
						unsigned long pad);

/* Clustered Page Table Entries */
extern void         cpte_add(struct cpte *cpte, struct pte *pte, int boff);
extern struct cpte  *cpte_lookup(struct cpte *cpte, int boff);
extern void         cpte_remove(struct cpte *cpte, int boff);
extern struct pte   *cpte_lookup_pte(struct cpte *cpte, int boff);

unsigned int 		VPBN(vm_offset_t entry);

#endif /* MACHINE_CPT_H_ */
