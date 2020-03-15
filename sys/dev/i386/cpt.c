/*
 * cpt.c
 *
 *  Created on: 13 Mar 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <dev/i386/cpt.h>
#include <lib/libkern/libkern.h>

struct cpt cpt_base[NCPT];
struct cpte cpte_base[NCPTE];

/*
 * Clustered Page Table & Entries (Red-Black Tree) Comparisons & Hash Function
 */
int
cpt_cmp(cpt1, cpt2)
    struct cpt *cpt1, *cpt2;
{
    if(cpt1->cpt_hindex > cpt2->cpt_hindex) {
        return (1);
    } else if(cpt1->cpt_hindex < cpt2->cpt_hindex) {
        return (-1);
    } else {
        return (0);
    }
}

int
cpte_cmp(cpte1, cpte2)
    struct cpte *cpte1, *cpte2;
{
    if(cpte1->cpte_boff > cpte2->cpte_boff) {
        return (1);
    } else if(cpte1->cpte_boff < cpte2->cpte_boff) {
        return (-1);
    } else {
        return (0);
    }
}

/* Virtual Page Block Number */
unsigned int
VPBN(entry)
	u_long entry;
{
	u_long hash1 = (prospector32(entry) % NCPT);
	u_long hash2 = (lowbias32(entry) % NCPT);

    if(hash1 != hash2) {
        return (hash1);
    } else if(hash1 == hash2) {
        return ((hash1 + hash2) % NCPT);
    } else {
        return (hash2);
    }
}

RB_PROTOTYPE(cpt_rbtree, cpt, cpt_entry, cpt_cmp);
RB_GENERATE(cpt_rbtree, cpt, cpt_entry, cpt_cmp);
RB_PROTOTYPE(cpte_rbtree, cpte, cpte_entry, cpte_cmp);
RB_GENERATE(cpte_rbtree, cpte, cpte_entry, cpte_cmp);

/*
 * Clustered Page Table (Red-Black Tree) Functions
 */
/* Add to the Clustered Page Table */
void
cpt_add(cpt, cpte, vpbn)
    struct cpt *cpt;
    struct cpte *cpte;
    u_long vpbn;
{
    cpt = &cpt_base[VPBN(vpbn)];
    cpt->cpt_pa_addr = vpbn;
    cpt->cpt_va_addr = vpbn;
    cpt->cpt_hindex = VPBN(vpbn);
    cpt->cpt_cpte = cpte;
    RB_INSERT(cpt_rbtree, &cpt_root, cpt);
}

/* Search Clustered Page Table */
struct cpt *
cpt_lookup(cpt, vpbn)
    struct cpt *cpt;
	u_long vpbn;
{
    struct cpt *result;
    if(&cpt[VPBN(vpbn)] != NULL) {
        result = &cpt[VPBN(vpbn)];
        return (RB_FIND(cpt_rbtree, &cpt_root, result));
    } else {
        return (NULL);
    }
}

/* Remove from the Clustered Page Table */
void
cpt_remove(cpt, vpbn)
    struct cpt *cpt;
	u_long vpbn;
{
    struct cpt *result = cpt_lookup(cpt, vpbn);
    RB_REMOVE(cpt_rbtree, &cpt_root, result);
}

/* Search Clustered Page Table Entry from The Clustered Page Table */
struct cpte *
cpt_lookup_cpte(cpt, vpbn)
    struct cpt *cpt;
	u_long vpbn;
{
    return (cpt_lookup(cpt, vpbn)->cpt_cpte);
}

/* Map CPT's into PDE's or vice-versa (Compatibility) */
void
cpt_to_pde(cpt, pde)
	struct pde *pde;
	struct cpt *cpt;
{
	if(pde == NULL && cpt->cpt_pde != NULL) {
		pde = cpt->cpt_pde;
	} else {
		cpt->cpt_pde = pde;
	}
}

/* (WIP) Clustered Page Table: Superpage Support */
void
cpt_add_superpage(cpt, cpte, vpbn, sz, pad)
    struct cpt *cpt;
    struct cpte *cpte;
    u_long vpbn;
    u_long sz, pad;
{
    cpt = &cpt_base[VPBN(vpbn)];
    cpt->cpt_sz = sz;
    cpt->cpt_pad = pad;

    cpt_add(cpt, cpte, vpbn);
}

/* (WIP) Clustered Page Table: Partial-Subblock Support */
void
cpt_add_partial_subblock(cpt, cpte, vpbn, pad)
    struct cpt *cpt;
    struct cpte *cpte;
    u_long vpbn;
    u_long pad;
{
    cpt = &cpt_base[VPBN(vpbn)];
    cpt->cpt_pad = pad;

    cpt_add(cpt, cpte, vpbn);
}

/*
 * Clustered Page Table (Red-Black Tree) Entry Functions
 */
/* Add Clustered Page Table Entries */
void
cpte_add(cpte, pte, boff)
    struct cpte *cpte;
    struct pte *pte;
    int boff;
{
    if(boff <= NCPTE) {
        cpte = &cpte_base[boff];
        cpte->cpte_boff = boff;
        cpte->cpte_pte = pte;
        RB_INSERT(cpte_rbtree, &cpte_root, cpte);
    } else {
        printf("%s\n", "This Clustered Page Table Entry is Full");
    }
}

/* Search Clustered Page Table Entries */
struct cpte *
cpte_lookup(cpte, boff)
    struct cpte *cpte;
    int boff;
{
    struct cpte *result;
    if(&cpte[boff] != NULL) {
        result = &cpte[boff];
        return (RB_FIND(cpte_rbtree, &cpte_root, result));
    } else {
        return (NULL);
    }
}

/* Remove entries from Clustered Page Table Entry */
void
cpte_remove(cpte, boff)
    struct cpte *cpte;
    int boff;
{
    struct cpte *result = cpte_lookup(cpte, boff);
    RB_REMOVE(cpte_rbtree, &cpte_root, result);
}

/* Search a Page Table Entry from a Clustered Page Table Entry */
struct pte *
cpte_lookup_pte(cpte, boff)
    struct cpte *cpte;
    int boff;
{
    return (cpte_lookup(cpte, boff)->cpte_pte);
}
