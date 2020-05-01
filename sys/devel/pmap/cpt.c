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
 *	@(#)cpt.c 1.0 	1/05/20
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <pmap/cpt.h>
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

/* Walk CPT RB-Tree by physical address */
struct cpt *
cpt_traversal(cpt, addr)
    struct cpt *cpt;
	u_long addr;
{
    struct cpt *result;
    for(int i = 0; i < NCPT; i++) {
        result = cpt_lookup(cpt, addr);
        if(result->cpt_pa_addr == i) {
            return (result);
        }
    }
    return (NULL);
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

/* Retrieve a PDE from a CPT */
struct pde *
cpt_to_pde(cpt, vpbn)
	struct cpt *cpt;
	u_long vpbn;
{
	struct pde *result;
	if (cpt_lookup(cpt, vpbn) != NULL) {
		result = cpt_lookup(cpt, vpbn)->cpt_pde;
		return (result);
	}
	return (NULL);
}

/* Retrieve a CPT from PDE */
struct cpt *
pde_to_cpt(pde, vpbn)
	struct pde *pde;
	u_long vpbn;
{
	struct cpt *result;
	if (pde != NULL) {
		if (&cpt_base[VPBN(vpbn)].cpt_pde == pde) {
			result =  &cpt_base[VPBN(vpbn)];
			return (result);
		}
	}
	return (NULL);
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
    if(&cpte_base[boff] != NULL) {
        result = &cpte_base[boff];
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

/* Retrieve a PTE from a CPTE */
struct pte *
cpte_to_pte(cpte, boff)
    struct cpte *cpte;
    int boff;
{
    struct pte *result;
    if(cpte_lookup(cpte, boff) != NULL) {
    	result = cpte_lookup(cpte, boff)->cpte_pte;
    	return (result);
    }
    return (NULL);
}

/* Retrieve a CPTE from a PTE */
struct cpte *
pte_to_cpte(pte, boff)
    struct pte *pte;
    int boff;
{
    struct cpte *result;
    if (pte != NULL) {
    	if (&cpte_base[boff].cpte_pte == pte) {
    		result = &cpte_base[boff];
    		return (result);
    	}
    }
    return (NULL);
}
