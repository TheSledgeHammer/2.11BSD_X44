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
	vm_offset_t	     orig_vaddr;
    vm_size_t        orig_size;
    unsigned int     version;
    vm_size_t        size;

    /* map */
   vm_map_t         orig_map;
   vm_map_t         map;
   vm_map_entry_t   entry;

   /* object */
   vm_object_t      object;
   vm_offset_t      offset;

   /* segment */
   vm_segment_t     segment;
   vm_size_t		segment_size;

   /* page */
   vm_page_t        page;
   vm_size_t		page_size;
};

struct vm_advice {
    int 			advice;
	int 			nback;
	int 			nforw;
};

/*
 * TODO:
 * - Should not allow freeing or releasing of a segment without first checking pages
 */
/*
 *	Recovery actions
 */
#define	FREE_SEGMENT(s)	{				\
	SEGMENT_WAKEUP(s);					\
	vm_segment_lock_lists();			\
	vm_segment_free(s);					\
	vm_segment_unlock_lists();			\
}

#define	RELEASE_SEGMENT(s)	{			\
	SEGMENT_WAKEUP(s);					\
	vm_segment_lock_lists();			\
	vm_segment_activate(s);				\
	vm_segment_unlock_lists();			\
}

#define	FREE_PAGE(m)	{				\
	PAGE_WAKEUP(m);						\
	vm_page_lock_queues();				\
	vm_page_free(m);					\
	vm_page_unlock_queues();			\
}

#define	RELEASE_PAGE(m)	{				\
	PAGE_WAKEUP(m);						\
	vm_page_lock_queues();				\
	vm_page_activate(m);				\
	vm_page_unlock_queues();			\
}

#define	UNLOCK_MAP	{					\
	if (lookup_still_valid) {			\
		vm_map_lookup_done(map, entry);	\
		lookup_still_valid = FALSE;		\
	}									\
}

#define	UNLOCK_THINGS	{				\
	object->paging_in_progress--;		\
	vm_object_unlock(object);			\
	if (object != first_object) {		\
		vm_object_lock(first_object);	\
		FREE_PAGE(first_m);				\
		first_object->paging_in_progress--;	\
		vm_object_unlock(first_object);	\
	}									\
	UNLOCK_MAP;							\
}

#define	UNLOCK_AND_DEALLOCATE	{		\
	UNLOCK_THINGS;						\
	vm_object_deallocate(first_object);	\
}

#endif /* _VM_FAULT_H_ */
