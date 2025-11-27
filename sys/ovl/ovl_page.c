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
 */
/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_page.h	8.3 (Berkeley) 1/9/95
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fnv_hash.h>

#include  <ovl/include/ovl.h>
#include  <ovl/include/ovl_page.h>

struct ovl_pglist			*ovl_page_buckets;			/* Array of buckets */
int							ovl_page_bucket_count = 0;	/* How big is array? */
int							ovl_page_hash_mask;			/* Mask for hash function */
simple_lock_data_t			ovl_page_bucket_lock;		/* lock for all page buckets XXX */

struct ovl_pglist			ovl_page_list;
simple_lock_data_t			ovl_page_list_lock;

long						ovl_first_page;
long						ovl_last_page;
vm_offset_t					ovl_first_phys_addr;
vm_offset_t					ovl_last_phys_addr;
vm_offset_t					oentry_data;

struct ovl_vm_page_hash_head 	*ovl_vm_page_hashtable;
long				       	ovl_vm_page_count;
simple_lock_data_t			ovl_vm_page_hash_lock;

/*
 * ovl_pmap_bootstrap:
 *
 * Allocates virtual address space from pmap_bootstrap_alloc.
 */
void
ovl_pmap_bootstrap(void)
{
	extern vm_offset_t	oentry_data;
	vm_size_t oentry_data_size, omap_size, oentry_size;

	omap_size = (MAX_OMAP * sizeof(struct ovl_map));
	oentry_data = (MAX_OMAPENT * sizeof(struct ovl_map_entry));
	oentry_data_size = round_page(omap_size + oentry_size);
	oentry_data = (vm_offset_t)pmap_overlay_bootstrap_alloc(oentry_data_size);
}

/*
 * ovl_pmap_bootinit:
 *
 * Allocates item from space made available by ovl_pmap_bootstrap.
 */
void *
ovl_pmap_bootinit(item, size, nitems)
	void 		*item;
	vm_size_t 	size;
	int 		nitems;
{
	extern vm_offset_t oentry_data;
	vm_size_t free, totsize, result, itemsize;

	free = oentry_data;
	totsize = (size * nitems);
	result = (free - totsize);
	itemsize = (sizeof(item) + size);
	if ((free < totsize) || (itemsize > result)) {
		panic("ovl_pmap_bootinit: not enough space allocated");
	}
	bzero(item, totsize);
	item = (void *)(vm_size_t)itemsize;
	oentry_data = result;
	return (item);
}

void
ovl_page_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register ovl_page_t		 	m;
	register struct ovl_pglist	*bucket;
	vm_size_t				 	npages;
	vm_offset_t				 	pa;
	int i;

	simple_lock_init(&ovl_page_list_lock, "ovl_page_list_lock");
	simple_lock_init(&ovl_vm_page_hash_lock, "ovl_vm_page_hash_lock");

	TAILQ_INIT(&ovl_page_list);

	if (ovl_page_bucket_count == 0) {
		ovl_page_bucket_count = 1;
		while (ovl_page_bucket_count < atop(*end - *start)) {
			ovl_page_bucket_count <<= 1;
		}
	}

	ovl_page_hash_mask = ovl_page_bucket_count - 1;

	ovl_page_buckets = (struct ovl_pglist *)pmap_overlay_bootstrap_alloc(ovl_page_bucket_count * sizeof(struct ovl_pglist));
	bucket = ovl_page_buckets;

	for (i = ovl_page_bucket_count; i--;) {
			TAILQ_INIT(bucket);
			bucket++;
	}

	for (i = 0; i < ovl_page_hash_mask; i++) {
		TAILQ_INIT(&ovl_page_buckets[i]);
		TAILQ_INIT(&ovl_vm_page_hashtable[i]);
	}
	simple_lock_init(&ovl_page_bucket_lock, "ovl_page_bucket_lock");

	*end = trunc_page(*end);

	ovl_pmap_bootstrap();

	npages = (*end - *start + sizeof(struct ovl_page)) / (PAGE_SIZE + sizeof(struct ovl_page));

	ovl_first_page = *start;
	ovl_first_page += npages * sizeof(struct ovl_page);
	ovl_first_page = atop(round_page(ovl_first_page));
	ovl_last_page  = ovl_first_page + npages - 1;

	ovl_first_phys_addr = ptoa(ovl_first_page);
	ovl_last_phys_addr  = ptoa(ovl_last_page) + PAGE_MASK;

	m = (ovl_page_t)pmap_overlay_bootstrap_alloc(npages * sizeof(struct ovl_page));

	pa = ovl_first_phys_addr;
	while (npages--) {
		m->flags = 0;
		m->segment = NULL;
		m->phys_addr = pa;
		m++;
		pa += PAGE_SIZE;
	}
}

unsigned long
ovl_page_hash(segment, offset)
    ovl_segment_t    segment;
    vm_offset_t     offset;
{
    Fnv32_t hash1 = fnv_32_buf(&segment, (sizeof(&segment) + offset)&ovl_page_hash_mask, FNV1_32_INIT)%ovl_page_hash_mask;
    Fnv32_t hash2 = (((unsigned long)segment+(unsigned long)offset)&ovl_page_hash_mask);
    return (hash1^hash2);
}

void
ovl_page_insert(mem, segment, offset)
	register ovl_page_t		mem;
	register ovl_segment_t	segment;
	register vm_offset_t	offset;
{
	register struct ovl_pglist	*bucket;

	mem->segment = segment;
	mem->offset = offset;

	bucket = &ovl_page_buckets[ovl_page_hash(segment, offset)];
	simple_lock(&ovl_page_bucket_lock);
	TAILQ_INSERT_TAIL(bucket, mem, hashq);
	simple_unlock(&ovl_page_bucket_lock);

	TAILQ_INSERT_TAIL(&segment->pglist, mem, listq);
	mem->flags |= PG_TABLED;

	segment->object->page_count++;
}

/*
 *	vm_page_remove:		[ internal use only ]
 *				NOTE: used by device pager as well -wfj
 *
 *	Removes the given mem entry from the segment/offset-page
 *	table and the segment page list.
 *
 *	The object, segment and page must be locked.
 */

void
ovl_page_remove(mem)
	register ovl_page_t	mem;
{
	register struct ovl_pglist  *bucket;

	if (!(mem->flags & PG_TABLED))
		return;

	/*
	 *	Remove from the object_segment/offset hash table
	 */

	bucket = &ovl_page_buckets[ovl_page_hash(mem->segment, mem->offset)];
	simple_lock(&ovl_page_bucket_lock);
	TAILQ_REMOVE(bucket, mem, hashq);
	simple_unlock(&ovl_page_bucket_lock);

	/*
	 *	Now remove from the segment's list of backed pages.
	 */

	TAILQ_REMOVE(&mem->segment->pglist, mem, listq);

	/*
	 *	And show that the segment has one fewer resident
	 *	page.
	 */

	mem->segment->object->page_count--;

	mem->flags &= ~PG_TABLED;
}

/*
 *	vm_page_lookup:
 *
 *	Returns the page associated with the segment/offset
 *	pair specified; if none is found, NULL is returned.
 *
 *	The object must be locked.  No side effects.
 */

ovl_page_t
ovl_page_lookup(segment, offset)
	register ovl_segment_t	segment;
	register vm_offset_t	offset;
{
	register ovl_page_t			mem;
	register struct ovl_pglist	*bucket;

	/*
	 *	Search the hash table for this segment/offset pair
	 */

	bucket = &ovl_page_buckets[ovl_page_hash(segment, offset)];

	simple_lock(&ovl_page_bucket_lock);
	TAILQ_FOREACH(mem, bucket, hashq) {
		VM_PAGE_CHECK(mem);
		if ((mem->segment == segment) && (mem->offset == offset)) {
			simple_unlock(&ovl_page_bucket_lock);
			return(mem);
		}
	}

	simple_unlock(&ovl_page_bucket_lock);
	return(NULL);
}

/* vm page */
u_long
ovl_vpage_hash(opage, vpage)
	ovl_page_t 	opage;
	vm_page_t 	vpage;
{
	Fnv32_t hash1 = fnv_32_buf(&opage, sizeof(&opage), FNV1_32_INIT) % ovl_page_hash_mask;
	Fnv32_t hash2 = fnv_32_buf(&vpage, sizeof(&vpage), FNV1_32_INIT) % ovl_page_hash_mask;
	return (hash1 ^ hash2);
}

void
ovl_page_insert_vm_page(opage, vpage)
	ovl_page_t 	opage;
	vm_page_t 	vpage;
{
    struct ovl_vm_page_hash_head 	*vbucket;

    if(vpage == NULL) {
        return;
    }

    vbucket = &ovl_vm_page_hashtable[ovl_vpage_hash(opage, vpage)];
    opage->vm_page = vpage;
    opage->flags |= OVL_PG_VM_PG;

    ovl_vm_page_hash_lock();
    TAILQ_INSERT_HEAD(vbucket, opage, vm_page_hlist);
    ovl_vm_page_hash_unlock();

    ovl_vm_page_count++;
}

vm_page_t
ovl_page_lookup_vm_page(opage)
	ovl_page_t 	opage;
{
	register vm_page_t 			vpage;
    struct ovl_vm_page_hash_head 	*vbucket;

    vbucket = &ovl_vm_page_hashtable[ovl_vpage_hash(opage, vpage)];
    ovl_vm_page_hash_lock();
    TAILQ_FOREACH(opage, vbucket, vm_page_hlist) {
    	 if(vpage == TAILQ_NEXT(opage, vm_page_hlist)->vm_page) {
    		 vpage = TAILQ_NEXT(opage, vm_page_hlist)->vm_page;
    		 ovl_vm_page_hash_unlock();
    		 return (vpage);
    	 }
    }
    ovl_vm_page_hash_unlock();
    return (NULL);
}

void
ovl_page_remove_vm_page(vpage)
	vm_page_t 	vpage;
{
	register ovl_page_t 		opage;
    struct ovl_vm_page_hash_head 	*vbucket;

    vbucket = &ovl_vm_page_hashtable[ovl_vpage_hash(opage, vpage)];

    TAILQ_FOREACH(opage, vbucket, vm_page_hlist) {
    	 if(vpage == TAILQ_NEXT(opage, vm_page_hlist)->vm_page) {
    		 vpage = TAILQ_NEXT(opage, vm_page_hlist)->vm_page;
    		 if(vpage != NULL) {
    			 TAILQ_REMOVE(vbucket, opage, vm_page_hlist);
    	         ovl_vm_page_count--;
    		 }
    	 }
    }
}
