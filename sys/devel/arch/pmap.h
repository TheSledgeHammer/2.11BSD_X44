/*
 * The 3-Clause BSD License:
 * Copyright (c) 2023 Martin Kelly
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
 * Possible Issues: (OVLBASE = OVL_MAX_ADDRESS)
 * - Adding OVLBASE onto KERNBASE may cause:
 * - A conflict with the KERNLOAD address
 * - Other unforeseen stack issues and conflicts
 *
 * Potential Alterations & Fixes to above:
 * - Having the current KERNBASE = KERNBASE+OVLBASE i.e. 512mb (higher)
 * Inverting the below to the following point:
 * - i.e. Subtracting the OVLBASE from KERNBASE if OVERLAY have not been configured
 * and allowing for that space to still be used
 */
#ifndef _I386_PMAP_H_
#define _I386_PMAP_H_

#define OVL_MAX_ADDRESS         ((vm_offset_t)(VM_MAX_ADDRESS/10))

#ifdef OVERLAY
#define OVLBASE		 			OVL_MAX_ADDRESS
#define SYSTEMBASE	 			KERNBASE+OVLBASE
#else
#define SYSTEMBASE	 			KERNBASE
#endif

#ifndef PMAP_PAE_COMP /* PMAP_NOPAE */
#define L2_SLOT_PTE				(SYSTEMBASE/NBPD_L2-1)		/* 843: for recursive PDP map */
#define L2_SLOT_KERN			(SYSTEMBASE/NBPD_L2)		/* 844: start of kernel space */
#else /* PMAP_PAE */

#define L2_SLOT_PTE				(SYSTEMBASE/NBPD_L2-4) 		/* 1685: for recursive PDP map */
#define L2_SLOT_KERN			(SYSTEMBASE/NBPD_L2)   		/* 1689: start of kernel space */
#endif

#define NKL2_MAX_ENTRIES		(KVA_PAGES - (SYSTEMBASE/NBPD_L2) - 1)

typedef uint64_t	ovl_entry_t;

struct pmap {
	ovl_entry_t		*pm_ovltab;		/* OVA Address- Direct access OVL mapped memory */
};
typedef struct pmap *pmap_t;
#endif /* _I386_PMAP_H_ */
