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
 * @(#)ovl_.c	1.00
 */
/* Current Kernel Overlays:
 * - Akin to a more advanced & versatile overlay from original 2.11BSD
 */

#ifndef _OVL_IO_H_
#define _OVL_IO_H_

#include <sys/queue.h>
/*
//#ifdef OVL
vm_offset_t						overlay_start;
vm_offset_t 					overlay_end;
extern struct pmap				overlay_pmap_store;
#define overlay_pmap 			(&overlay_pmap_store)
//#endif
 */

/* OVL Space Address Layout */
#define OVL_MIN_ADDRESS 		((vm_offset_t)0)	/* put ovlspace before vmspace in memory stack */
#define OVL_MAX_ADDRESS			((/*xxx*//100)*10)	/* Total Size of Overlay Address Space (Roughly 10% of PGSIZE) */
#define VM_MIN_ADDRESS			OVL_MAX_ADDRESS

#define OVL_MIN_KERNEL_ADDRESS	OVL_MIN_ADDRESS
#define OVL_MAX_KERNEL_ADDRESS
#define OVL_MIN_VM_ADDRESS
#define OVL_MAX_VM_ADDRESS

#define OVL_MEM_SIZE			OVL_MAX_ADDRESS		/* total ovlspace for allocation */
#define NBOVL 										/* number of overlays */

/* memory management definitions */
ovl_map_t 						omem_map;
ovl_map_t 						overlay_map;

#ifndef OVL_MAP
/* as defined in ovl_map.h */
#define MAX_OMAP				(64)
#define	NOVL					(32)				/* number of kernel overlay entries */
#endif

#endif /* _OVL_IO_H_ */
