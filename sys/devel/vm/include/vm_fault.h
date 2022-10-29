/*	$NetBSD: uvm_fault.h,v 1.16 2001/12/31 22:34:39 chs Exp $	*/

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
 *
 * from: Id: uvm_fault.h,v 1.1.2.2 1997/12/08 16:07:12 chuck Exp
 */

#ifndef _VM_FAULT_H_
#define _VM_FAULT_H_

struct vm_faultinfo {
	/* faultinfo */
	vm_offset_t	     orig_rvaddr;
    vm_size_t        orig_size;
    unsigned int	 mapv;
    vm_size_t        size;
    bool_t 			 lookup_still_valid;
    bool_t			 wired;

    /* map */
   vm_map_t         orig_map;
   vm_map_entry_t   orig_entry;
   vm_map_t         map;
   vm_map_entry_t   entry;

   /* object */
   vm_object_t      object;
   vm_offset_t      offset;

   /* segment */
   vm_segment_t     segment;

   /* page */
   vm_page_t        page;

   /* first object, segment & page */
   vm_object_t      first_object;
   vm_offset_t      first_offset;
   vm_segment_t     first_segment;
   vm_page_t        first_page;
};

struct vm_advice {
    int 			advice;
	int 			nback;
	int 			nforw;
};

void	vm_fault_free(vm_segment_t, vm_page_t);
void	vm_fault_release(vm_segment_t, vm_page_t);

#endif /* _VM_FAULT_H_ */
