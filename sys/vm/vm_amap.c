/*	$NetBSD: uvm_amap.c,v 1.19.2.2 1999/07/07 00:07:13 perry Exp $	*/

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
 */

/*
 * uvm_amap.c: amap operations
 */

/*
 * this file contains functions that perform operations on amaps.  see
 * uvm_amap.h for a brief explanation of the role of amaps in uvm.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <vm/include/vm_kern.h>

#include <vm/include/vm.h>
#include <vm/include/vm_map.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>

#include <vm/include/vm_pageout.h>
//#include <vm/include/vm_amap.h>

static vm_amap_t vm_amap_alloc1(int, int, int);

struct lock_object amap_list_lock;
static LIST_HEAD(, vm_amap) amap_list;

/*
 * local functions
 */
static inline void
vm_amap_list_insert(amap)
	vm_amap_t	amap;
{
	simple_lock(&amap_list_lock);
	LIST_INSERT_HEAD(&amap_list, amap, am_list);
	simple_unlock(&amap_list_lock);
}

static inline void
vm_amap_list_remove(amap)
	vm_amap_t	amap;
{
	simple_lock(&amap_list_lock);
	LIST_REMOVE(amap, am_list);
	simple_unlock(&amap_list_lock);
}

#ifdef VM_AMAP_PPREF

/*
 * what is ppref?   ppref is an _optional_ amap feature which is used
 * to keep track of reference counts on a per-page basis.  it is enabled
 * when UVM_AMAP_PPREF is defined.
 *
 * when enabled, an array of ints is allocated for the pprefs.  this
 * array is allocated only when a partial reference is added to the
 * map (either by unmapping part of the amap, or gaining a reference
 * to only a part of an amap).  if the malloc of the array fails
 * (M_NOWAIT), then we set the array pointer to PPREF_NONE to indicate
 * that we tried to do ppref's but couldn't alloc the array so just
 * give up (after all, this is an optional feature!).
 *
 * the array is divided into page sized "chunks."   for chunks of length 1,
 * the chunk reference count plus one is stored in that chunk's slot.
 * for chunks of length > 1 the first slot contains (the reference count
 * plus one) * -1.    [the negative value indicates that the length is
 * greater than one.]   the second slot of the chunk contains the length
 * of the chunk.   here is an example:
 *
 * actual REFS:  2  2  2  2  3  1  1  0  0  0  4  4  0  1  1  1
 *       ppref: -3  4  x  x  4 -2  2 -1  3  x -5  2  1 -2  3  x
 *              <----------><-><----><-------><----><-><------->
 * (x = don't care)
 *
 * this allows us to allow one int to contain the ref count for the whole
 * chunk.    note that the "plus one" part is needed because a reference
 * count of zero is neither positive or negative (need a way to tell
 * if we've got one zero or a bunch of them).
 *
 * here are some in-line functions to help us.
 */

/*
 * pp_getreflen: get the reference and length for a specific offset
 *
 * => ppref's amap must be locked
 */
static __inline void
pp_getreflen(ppref, offset, refp, lenp)
	int *ppref, offset, *refp, *lenp;
{
	if (ppref[offset] > 0) {		/* chunk size must be 1 */
		*refp = ppref[offset] - 1;	/* don't forget to adjust */
		*lenp = 1;
	} else {
		*refp = (ppref[offset] * -1) - 1;
		*lenp = ppref[offset+1];
	}
}

/*
 * pp_setreflen: set the reference and length for a specific offset
 *
 * => ppref's amap must be locked
 */
static __inline void
pp_setreflen(ppref, offset, ref, len)
	int *ppref, offset, ref, len;
{
	if (len == 0)
		return;
	if (len == 1) {
		ppref[offset] = ref + 1;
	} else {
		ppref[offset] = (ref + 1) * -1;
		ppref[offset+1] = len;
	}
}
#endif

void
vm_amap_init(void)
{
	register vm_amap_t amap;

	MALLOC(amap, struct vm_amap *, sizeof(struct vm_amap), M_VMAMAP, M_WAITOK);

	LIST_INIT(&amap_list);
	amap->am_ref = 1;
	simple_lock_init(&amap->am_lock, "amap_lock");
	simple_lock_init(&amap_list_lock, "amap_list_lock");
}

/*
 * amap_alloc1: internal function that allocates an amap, but does not
 *	init the overlay.
 *
 * => lock on returned amap is init'd
 */
static inline vm_amap_t
vm_amap_alloc1(slots, padslots, waitf)
	int slots, padslots, waitf;
{
	vm_amap_t amap;
	int totalslots;

	amap = (struct vm_amap *)malloc(sizeof(struct vm_amap *), M_VMAMAP, M_WAITOK);
	if (amap == NULL) {
		return (NULL);
	}
	totalslots = slots + padslots;
	amap->am_ref = 1;
	amap->am_flags = 0;
#ifdef VM_AMAP_PPREF
	amap->am_ppref = NULL;
#endif
	amap->am_maxslot = totalslots;
	amap->am_nslot = slots;
	amap->am_nused = 0;

	amap->am_slots = calloc(totalslots, sizeof(int), M_VMAMAP, waitf);
	if (amap->am_slots == NULL)
		goto fail1;

	amap->am_bckptr = calloc(totalslots, sizeof(int), M_VMAMAP, waitf);
	if (amap->am_bckptr == NULL)
		goto fail2;

	amap->am_anon = calloc(totalslots, sizeof(struct vm_anon *), M_VMAMAP, waitf);
	if (amap->am_anon == NULL)
		goto fail3;

	return(amap);

fail3:
	free(amap->am_bckptr, M_VMAMAP);
fail2:
	free(amap->am_slots, M_VMAMAP);
fail1:
	free(amap, M_VMAMAP);
	return (NULL);
}

/*
 * amap_alloc: allocate an amap to manage "sz" bytes of anonymous VM
 *
 * => caller should ensure sz is a multiple of PAGE_SIZE
 * => reference count to new amap is set to one
 * => new amap is returned unlocked
 */

vm_amap_t
vm_amap_alloc(sz, padsz, waitf)
	vm_offset_t sz, padsz;
	int waitf;
{
	vm_amap_t amap;
	int slots, padslots;

	AMAP_B2SLOT(slots, sz); /* load slots */
	AMAP_B2SLOT(padslots, padsz);

	amap = vm_amap_alloc1(slots, padslots, waitf);
	if (amap) {
		bzero(amap->am_anon, (slots + padslots) * sizeof(struct vm_anon*));
		vm_amap_list_insert(amap);
	}
	return (amap);
}

/*
 * amap_free: free an amap
 *
 * => the amap must be locked (mainly for simplelock accounting)
 * => the amap should have a zero reference count and be empty
 */
void
vm_amap_free(amap)
	vm_amap_t amap;
{
#ifdef DIAGNOSTIC
	if (amap->am_ref || amap->am_nused)
		panic("amap_free");
#endif

	FREE(amap->am_slots, M_VMAMAP);
	FREE(amap->am_bckptr, M_VMAMAP);
	FREE(amap->am_anon, M_VMAMAP);
#ifdef VM_AMAP_PPREF
	if (amap->am_ppref && amap->am_ppref != PPREF_NONE)
		FREE(amap->am_ppref, M_VMAMAP);
#endif
	amap_unlock(amap);	/* mainly for lock debugging */
}

/*
 * amap_extend: extend the size of an amap (if needed)
 *
 * => called from uvm_map when we want to extend an amap to cover
 *    a new mapping (rather than allocate a new one)
 * => amap should be unlocked (we will lock it)
 * => to safely extend an amap it should have a reference count of
 *    one (thus it can't be shared)
 * => XXXCDC: needs a waitflag or failure return value?
 * => XXXCDC: support padding at this level?
 */
void
vm_amap_extend(entry, addsize)
	vm_map_entry_t entry;
	vm_size_t addsize;
{
	vm_amap_t amap;
	int slotoff;
	int slotmapped, slotadd, slotneed;
#ifdef VM_AMAP_PPREF
	int *newppref, *oldppref;
#endif
	u_int *newsl, *newbck, *oldsl, *oldbck;
	struct vm_anon **newover, **oldover;
	int slotadded;

	amap = entry->aref.ar_amap;
	slotoff = entry->aref.ar_pageoff;

	/*
	 * first, determine how many slots we need in the amap.  don't
	 * forget that ar_pageoff could be non-zero: this means that
	 * there are some unused slots before us in the amap.
	 */

	amap_lock(amap); /* lock! */

	AMAP_B2SLOT(slotmapped, entry->end - entry->start); /* slots mapped */
	AMAP_B2SLOT(slotadd, addsize); 						/* slots to add */
	slotneed = slotoff + slotmapped + slotadd;

	/*
	 * case 1: we already have enough slots in the map and thus
	 * only need to bump the reference counts on the slots we are
	 * adding.
	 */

	if (amap->am_nslot >= slotneed) {
#ifdef VM_AMAP_PPREF
		if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
			vm_amap_pp_adjref(amap, slotoff + slotmapped, addsize, 1);
		}
#endif
		amap_unlock(amap);
		return; /* done! */
	}

	/*
	 * case 2: we pre-allocated slots for use and we just need to
	 * bump nslot up to take account for these slots.
	 */
	if (amap->am_maxslot >= slotneed) {
#ifdef VM_AMAP_PPREF
		if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
			if ((slotoff + slotmapped) < amap->am_nslot)
				vm_amap_pp_adjref(amap, slotoff + slotmapped,
				    (amap->am_nslot - (slotoff + slotmapped)) <<
				    PAGE_SHIFT, 1);
			pp_setreflen(amap->am_ppref, amap->am_nslot, 1, slotneed - amap->am_nslot);
		}
#endif
		amap->am_nslot = slotneed;
		amap_unlock(amap);
		/*
		 * no need to zero am_anon since that was done at
		 * alloc time and we never shrink an allocation.
		 */
		return;
	}

	/*
	 * case 3: we need to malloc a new amap and copy all the amap
	 * data over from old amap to the new one.
	 *
	 * XXXCDC: could we take advantage of a kernel realloc()?
	 */

	amap_unlock(amap); /* unlock in case we sleep in malloc */
#ifdef VM_AMAP_PPREF
	newppref = NULL;
	if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
		MALLOC(newppref, int *, slotneed * sizeof(int), M_VMAMAP, M_NOWAIT);
		if (newppref == NULL) {
			/* give up if malloc fails */
			FREE(amap->am_ppref, M_VMAMAP);
			    amap->am_ppref = PPREF_NONE;
		}
	}
#endif
	MALLOC(newsl, int *, slotneed * sizeof(int), M_VMAMAP, M_WAITOK);
	MALLOC(newbck, int *, slotneed * sizeof(int), M_VMAMAP, M_WAITOK);
	MALLOC(newover, struct vm_anon **, slotneed * sizeof(struct vm_anon *), M_VMAMAP, M_WAITOK);
	amap_lock(amap); /* re-lock! */

#ifdef DIAGNOSTIC
	if (amap->am_maxslot >= slotneed)
		panic("amap_extend: amap changed during malloc");
#endif

	/*
	 * now copy everything over to new malloc'd areas...
	 */

	slotadded = slotneed - amap->am_nslot;

	/* do am_slots */
	oldsl = amap->am_slots;
	bcopy(oldsl, newsl, sizeof(int) * amap->am_nused);
	amap->am_slots = newsl;

	/* do am_anon */
	oldover = amap->am_anon;
	bcopy(oldover, newover, sizeof(struct vm_anon*) * amap->am_nslot);
	bzero(newover + amap->am_nslot, sizeof(struct vm_anon*) * slotadded);
	amap->am_anon = newover;

	/* do am_bckptr */
	oldbck = amap->am_bckptr;
	bcopy(oldbck, newbck, sizeof(int) * amap->am_nslot);
	bzero(newbck + amap->am_nslot, sizeof(int) * slotadded); /* XXX: needed? */
	amap->am_bckptr = newbck;

#ifdef VM_AMAP_PPREF
	/* do ppref */
	oldppref = amap->am_ppref;
	if (newppref) {
		bcopy(oldppref, newppref, sizeof(int) * amap->am_nslot);
		bzero(newppref + amap->am_nslot, sizeof(int) * slotadded);
		amap->am_ppref = newppref;
		if ((slotoff + slotmapped) < amap->am_nslot)
			vm_amap_pp_adjref(amap, slotoff + slotmapped,
					(amap->am_nslot - (slotoff + slotmapped)) <<
					PAGE_SHIFT, 1);
		pp_setreflen(newppref, amap->am_nslot, 1, slotadded);
	}
#endif

	/* update master values */
	amap->am_nslot = slotneed;
	amap->am_maxslot = slotneed;

	/* unlock */
	amap_unlock(amap);

	/* and free */
	FREE(oldsl, M_VMAMAP);
	FREE(oldbck, M_VMAMAP);
	FREE(oldover, M_VMAMAP);
#ifdef VM_AMAP_PPREF
	if (oldppref && oldppref != PPREF_NONE)
		FREE(oldppref, M_VMAMAP);
#endif
}

/*
 * amap_share_protect: change protection of anons in a shared amap
 *
 * for shared amaps, given the current data structure layout, it is
 * not possible for us to directly locate all maps referencing the
 * shared anon (to change the protection).  in order to protect data
 * in shared maps we use pmap_page_protect().  [this is useful for IPC
 * mechanisms like map entry passing that may want to write-protect
 * all mappings of a shared amap.]  we traverse am_anon or am_slots
 * depending on the current state of the amap.
 *
 * => entry's map and amap must be locked by the caller
 */
void
vm_amap_share_protect(entry, prot)
	vm_map_entry_t entry;
	vm_prot_t prot;
{
	vm_amap_t amap;
	int slots, lcv, slot, stop;

	amap = entry->aref.ar_amap;
	AMAP_B2SLOT(slots, (entry->end - entry->start));
	stop = entry->aref.ar_pageoff + slots;

	if (slots < amap->am_nused) {
		/* cheaper to traverse am_anon */
		for (lcv = entry->aref.ar_pageoff ; lcv < stop ; lcv++) {
			if (amap->am_anon[lcv] == NULL)
				continue;
			if (amap->am_anon[lcv]->u.an_page != NULL) {
				pmap_page_protect(amap->am_anon[lcv]->u.an_page->offset, prot);
			}
		}
		return;
	}

	/* cheaper to traverse am_slots */
	for (lcv = 0 ; lcv < amap->am_nused ; lcv++) {
		slot = amap->am_slots[lcv];
		if (slot < entry->aref.ar_pageoff || slot >= stop)
			continue;
		if (amap->am_anon[slot]->u.an_page != NULL)
			pmap_page_protect(amap->am_anon[slot]->u.an_page->offset, prot);
	}
	return;
}

/*
 * amap_wipeout: wipeout all anon's in an amap; then free the amap!
 *
 * => called from amap_unref when the final reference to an amap is
 *	discarded (i.e. when reference count == 1)
 * => the amap should be locked (by the caller)
 */

void
vm_amap_wipeout(amap)
	vm_amap_t amap;
{
	int lcv, slot;
	vm_anon_t anon;

	vm_amap_list_remove(amap);
	amap_unlock(amap);

	for (lcv = 0 ; lcv < amap->am_nused ; lcv++) {
		int refs;

		slot = amap->am_slots[lcv];
		anon = amap->am_anon[slot];

		if (anon == NULL || anon->an_ref == 0)
			panic("amap_wipeout: corrupt amap");

		simple_lock(&anon->an_lock); /* lock anon */

		refs = --anon->an_ref;
		simple_unlock(&anon->an_lock);
		if (refs == 0) {
			/*
			 * we had the last reference to a vm_anon. free it.
			 */
			vm_anon_free(anon);
		}
	}

	/*
	 * now we free the map
	 */

	amap->am_ref = 0;	/* ... was one */
	amap->am_nused = 0;
	vm_amap_free(amap);	/* will unlock and free amap */
}

/*
 * amap_copy: ensure that a map entry's "needs_copy" flag is false
 *	by copying the amap if necessary.
 *
 * => an entry with a null amap pointer will get a new (blank) one.
 * => the map that the map entry belongs to must be locked by caller.
 * => the amap currently attached to "entry" (if any) must be unlocked.
 * => if canchunk is true, then we may clip the entry into a chunk
 * => "startva" and "endva" are used only if canchunk is true.  they are
 *     used to limit chunking (e.g. if you have a large space that you
 *     know you are going to need to allocate amaps for, there is no point
 *     in allowing that to be chunked)
 */

void
vm_amap_copy(map, entry, waitf, canchunk, startva, endva)
	vm_map_t map;
	vm_map_entry_t entry;
	int waitf;
	bool_t canchunk;
	vm_offset_t startva, endva;
{
	vm_amap_t amap, srcamap;
	int slots, lcv;
	vm_offset_t chunksize;

	/*
	 * is there a map to copy?   if not, create one from scratch.
	 */

	if (entry->aref.ar_amap == NULL) {

		/*
		 * check to see if we have a large amap that we can
		 * chunk.  we align startva/endva to chunk-sized
		 * boundaries and then clip to them.
		 */

		if (canchunk && atop(entry->end - entry->start) >= VM_AMAP_LARGE) {
			/* convert slots to bytes */
			chunksize = VM_AMAP_CHUNK << PAGE_SHIFT;
			startva = (startva / chunksize) * chunksize;
			endva = roundup(endva, chunksize);
			vm_map_clip_start(map, entry, startva);
			/* watch out for endva wrap-around! */
			if (endva >= startva) {
				vm_map_clip_end(map, entry, endva);
			}
		}

		entry->aref.ar_pageoff = 0;
		entry->aref.ar_amap = vm_amap_alloc(entry->end - entry->start, 0, waitf);
		if (entry->aref.ar_amap != NULL ) {
			entry->needs_copy = FALSE;
		}
		return;
	}

	/*
	 * first check and see if we are the only map entry
	 * referencing the amap we currently have.  if so, then we can
	 * just take it over rather than copying it.  note that we are
	 * reading am_ref with the amap unlocked... the value can only
	 * be one if we have the only reference to the amap (via our
	 * locked map).  if we are greater than one we fall through to
	 * the next case (where we double check the value).
	 */

	if (entry->aref.ar_amap->am_ref == 1) {
		entry->needs_copy = FALSE;
		return;
	}

	/*
	 * looks like we need to copy the map.
	 */

	AMAP_B2SLOT(slots, entry->end - entry->start);
	amap = vm_amap_alloc1(slots, 0, waitf);
	if (amap == NULL) {
		return;
	}
	srcamap = entry->aref.ar_amap;
	amap_lock(srcamap);

	/*
	 * need to double check reference count now that we've got the
	 * src amap locked down.  the reference count could have
	 * changed while we were in malloc.  if the reference count
	 * dropped down to one we take over the old map rather than
	 * copying the amap.
	 */

	if (srcamap->am_ref == 1) {		/* take it over? */
		entry->needs_copy = FALSE;
		amap->am_ref--;				/* drop final reference to map */
		vm_amap_free(amap);			/* dispose of new (unused) amap */
		amap_unlock(srcamap);
		return;
	}

	/*
	 * we must copy it now.
	 */

	for (lcv = 0 ; lcv < slots; lcv++) {
		amap->am_anon[lcv] =
		    srcamap->am_anon[entry->aref.ar_pageoff + lcv];
		if (amap->am_anon[lcv] == NULL)
			continue;
		simple_lock(&amap->am_anon[lcv]->an_lock);
		amap->am_anon[lcv]->an_ref++;
		simple_unlock(&amap->am_anon[lcv]->an_lock);
		amap->am_bckptr[lcv] = amap->am_nused;
		amap->am_slots[amap->am_nused] = lcv;
		amap->am_nused++;
	}

	/*
	 * drop our reference to the old amap (srcamap) and unlock.
	 * we know that the reference count on srcamap is greater than
	 * one (we checked above), so there is no way we could drop
	 * the count to zero.  [and no need to worry about freeing it]
	 */

	srcamap->am_ref--;
	if (srcamap->am_ref == 1 && (srcamap->am_flags & AMAP_SHARED) != 0)
		srcamap->am_flags &= ~AMAP_SHARED;   /* clear shared flag */
#ifdef VM_AMAP_PPREF
	if (srcamap->am_ppref && srcamap->am_ppref != PPREF_NONE) {
		vm_amap_pp_adjref(srcamap, entry->aref.ar_pageoff,
		    entry->end - entry->start, -1);
	}
#endif

	amap_unlock(srcamap);

	vm_amap_list_insert(amap);

	/*
	 * install new amap.
	 */

	entry->aref.ar_pageoff = 0;
	entry->aref.ar_amap = amap;
	entry->needs_copy = FALSE;

	/*
	 * done!
	 */
}

/*
 * amap_cow_now: resolve all copy-on-write faults in an amap now for fork(2)
 *
 *	called during fork(2) when the parent process has a wired map
 *	entry.   in that case we want to avoid write-protecting pages
 *	in the parent's map (e.g. like what you'd do for a COW page)
 *	so we resolve the COW here.
 *
 * => assume parent's entry was wired, thus all pages are resident.
 * => assume pages that are loaned out (loan_count) are already mapped
 *	read-only in all maps, and thus no need for us to worry about them
 * => assume both parent and child vm_map's are locked
 * => caller passes child's map/entry in to us
 * => if we run out of memory we will unlock the amap and sleep _with_ the
 *	parent and child vm_map's locked(!).    we have to do this since
 *	we are in the middle of a fork(2) and we can't let the parent
 *	map change until we are done copying all the map entrys.
 * => XXXCDC: out of memory should cause fork to fail, but there is
 *	currently no easy way to do this (needs fix)
 * => page queues must be unlocked (we may lock them)
 */

void
vm_amap_cow_now(map, entry)
	vm_map_t  map;
	vm_map_entry_t entry;
{
	vm_amap_t amap;
	int lcv, slot;
	vm_anon_t anon, nanon;
	vm_page_t pg, npg;

	amap = entry->aref.ar_amap;
	/*
	 * note that if we unlock the amap then we must ReStart the "lcv" for
	 * loop because some other process could reorder the anon's in the
	 * am_anon[] array on us while the lock is dropped.
	 */
ReStart:
	amap_lock(amap);

	for (lcv = 0 ; lcv < amap->am_nused ; lcv++) {

		/*
		 * get the page
		 */

		slot = amap->am_slots[lcv];
		anon = amap->am_anon[slot];
		simple_lock(&anon->an_lock);

		pg = anon->u.an_page;

		/*
		 * page must be resident since parent is wired
		 */

		if (pg == NULL) {
		    panic("amap_cow_now: non-resident wired page in anon %p",
			anon);
		}

		/*
		 * if the anon ref count is one and the page is not loaned,
		 * then we are safe (the child has exclusive access to the
		 * page).  if the page is loaned, then it must already be
		 * mapped read-only.
		 *
		 * we only need to get involved when these are not true.
		 * [note: if loan_count == 0, then the anon must own the page]
		 */

		if (anon->an_ref > 1) {

			/*
			 * if the page is busy then we have to unlock, wait for
			 * it and then restart.
			 */
			if (pg->flags & PG_BUSY) {
				pg->flags |= PG_WANTED;
				amap_unlock(amap);
				goto ReStart;
			}

			/*
			 * ok, time to do a copy-on-write to a new anon
			 */
			nanon = vm_anon_alloc();
			if (nanon) {
				npg = vm_page_anon_alloc(NULL, 0, nanon);
			} else {
				npg = NULL;	/* XXX: quiet gcc warning */
			}
			if (nanon == NULL || npg == NULL) {
				/* out of memory */
				/*
				 * XXXCDC: we should cause fork to fail, but
				 * we can't ...
				 */
				if (nanon) {
					vm_anon_free(nanon);
				}
				simple_unlock(&anon->an_lock);
				amap_unlock(amap);
				vm_wait();
				goto ReStart;
			}

			/*
			 * got it... now we can copy the data and replace anon
			 * with our new one...
			 */
			vm_page_copy(pg, npg);			/* old -> new */
			anon->an_ref--;					/* can't drop to zero */
			amap->am_anon[slot] = nanon;	/* replace */

			/*
			 * drop PG_BUSY on new page ... since we have had it's
			 * owner locked the whole time it can't be
			 * PG_RELEASED | PG_WANTED.
			 */
			npg->flags &= ~(PG_BUSY|PG_FAKE);
			vm_page_lock_queues();
			vm_page_activate(npg);
			vm_page_unlock_queues();
		}

		simple_unlock(&anon->an_lock);
		/*
		 * done with this anon, next ...!
		 */

	}	/* end of 'for' loop */

	return;
}

/*
 * amap_splitref: split a single reference into two seperate references
 *
 * => called from uvm_map's clip routines
 * => origref's map should be locked
 * => origref->ar_amap should be unlocked (we will lock)
 */
void
vm_amap_splitref(origref, splitref, offset)
	vm_aref_t origref, splitref;
	vm_offset_t offset;
{
	int leftslots;

	AMAP_B2SLOT(leftslots, offset);
	if (leftslots == 0) {
		panic("amap_splitref: split at zero offset");
	}

	/*
	 * lock the amap
	 */
	amap_lock(origref->ar_amap);

	/*
	 * now: amap is locked and we have a valid am_mapped array.
	 */

	if (origref->ar_amap->am_nslot - origref->ar_pageoff - leftslots <= 0) {
		panic("amap_splitref: map size check failed");
	}

#ifdef VM_AMAP_PPREF
        /*
	 * establish ppref before we add a duplicate reference to the amap
	 */
	if (origref->ar_amap->am_ppref == NULL) {
		vm_amap_pp_establish(origref->ar_amap);
	}
#endif

	splitref->ar_amap = origref->ar_amap;
	splitref->ar_amap->am_ref++;		/* not a share reference */
	splitref->ar_pageoff = origref->ar_pageoff + leftslots;

	amap_unlock(origref->ar_amap);
}

#ifdef VM_AMAP_PPREF

/*
 * amap_pp_establish: add a ppref array to an amap, if possible
 *
 * => amap locked by caller
 */
void
vm_amap_pp_establish(amap)
	vm_amap_t amap;
{

	MALLOC(amap->am_ppref, int *, sizeof(int) * amap->am_maxslot, M_VMAMAP, M_NOWAIT);

	/*
	 * if we fail then we just won't use ppref for this amap
	 */
	if (amap->am_ppref == NULL) {
		amap->am_ppref = PPREF_NONE;	/* not using it */
		return;
	}

	/*
	 * init ppref
	 */
	bzero(amap->am_ppref, sizeof(int) * amap->am_maxslot);
	pp_setreflen(amap->am_ppref, 0, amap->am_ref, amap->am_nslot);
	return;
}

/*
 * amap_pp_adjref: adjust reference count to a part of an amap using the
 * per-page reference count array.
 *
 * => map and amap locked by caller
 * => caller must check that ppref != PPREF_NONE before calling
 */
void
vm_amap_pp_adjref(amap, curslot, bytelen, adjval)
	vm_amap_t amap;
	int curslot;
	vm_size_t bytelen;
	int adjval;
{
	int slots, stopslot, *ppref, lcv;
	int ref, len;

	/*
	 * get init values
	 */

	AMAP_B2SLOT(slots, bytelen);
	stopslot = curslot + slots;
	ppref = amap->am_ppref;

	/*
	 * first advance to the correct place in the ppref array, fragment
	 * if needed.
	 */

	for (lcv = 0 ; lcv < curslot ; lcv += len) {
		pp_getreflen(ppref, lcv, &ref, &len);
		if (lcv + len > curslot) {     /* goes past start? */
			pp_setreflen(ppref, lcv, ref, curslot - lcv);
			pp_setreflen(ppref, curslot, ref, len - (curslot -lcv));
			len = curslot - lcv;   /* new length of entry @ lcv */
		}
	}

	/*
	 * now adjust reference counts in range (make sure we dont overshoot)
	 */

	if (lcv != curslot) {
		panic("vm_amap_pp_adjref: overshot target");
	}

	for (/* lcv already set */; lcv < stopslot ; lcv += len) {
		pp_getreflen(ppref, lcv, &ref, &len);
		if (lcv + len > stopslot) {     /* goes past end? */
			pp_setreflen(ppref, lcv, ref, stopslot - lcv);
			pp_setreflen(ppref, stopslot, ref,
			    len - (stopslot - lcv));
			len = stopslot - lcv;
		}
		ref = ref + adjval;    /* ADJUST! */
		if (ref < 0) {
			panic("vm_amap_pp_adjref: negative reference count");
		}
		pp_setreflen(ppref, lcv, ref, len);
		if (ref == 0) {
			vm_amap_wiperange(amap, lcv, len);
		}
	}

}

/*
 * amap_wiperange: wipe out a range of an amap
 * [different from amap_wipeout because the amap is kept intact]
 *
 * => both map and amap must be locked by caller.
 */
void
vm_amap_wiperange(amap, slotoff, slots)
	vm_amap_t amap;
	int slotoff, slots;
{
	int byanon, lcv, stop, curslot, ptr;
	vm_anon_t anon;

	/*
	 * we can either traverse the amap by am_anon or by am_slots depending
	 * on which is cheaper.    decide now.
	 */

	if (slots < amap->am_nused) {
		byanon = TRUE;
		lcv = slotoff;
		stop = slotoff + slots;
	} else {
		byanon = FALSE;
		lcv = 0;
		stop = amap->am_nused;
	}

	/*
	 * ok, now do it!
	 */

	for (; lcv < stop; lcv++) {
		int refs;

		/*
		 * verify the anon is ok.
		 */
		if (byanon) {
			if (amap->am_anon[lcv] == NULL)
				continue;
			curslot = lcv;
		} else {
			curslot = amap->am_slots[lcv];
			if (curslot < slotoff || curslot >= stop) {
				continue;
			}
		}
		anon = amap->am_anon[curslot];

		/*
		 * remove it from the amap
		 */
		amap->am_anon[curslot] = NULL;
		ptr = amap->am_bckptr[curslot];
		if (ptr != (amap->am_nused - 1)) {
			amap->am_slots[ptr] = amap->am_slots[amap->am_nused - 1];
			amap->am_bckptr[amap->am_slots[ptr]] = ptr;    /* back ptr. */
		}
		amap->am_nused--;

		/*
		 * drop anon reference count
		 */
		simple_lock(&anon->an_lock);
		refs = --anon->an_ref;
		simple_unlock(&anon->an_lock);
		if (refs == 0) {
			/*
			 * we just eliminated the last reference to an anon.
			 * free it.
			 */
			vm_anon_free(anon);
		}
	}
}

#endif

/*
 * amap_lookup: look up a page in an amap
 *
 * => amap should be locked by caller.
 */
vm_anon_t
vm_amap_lookup(aref, offset)
	vm_aref_t aref;
	vm_offset_t offset;
{
	int slot;
	vm_amap_t amap;

	amap = aref->ar_amap;
	AMAP_B2SLOT(slot, offset);
	slot += aref->ar_pageoff;

	if (slot >= amap->am_nslot) {
		panic("amap_lookup: offset out of range");
	}

	return (amap->am_anon[slot]);
}

/*
 * amap_lookups: look up a range of pages in an amap
 *
 * => amap should be locked by caller.
 * => XXXCDC: this interface is biased toward array-based amaps.  fix.
 */
void
vm_amap_lookups(aref, offset, anons, npages)
	vm_aref_t aref;
	vm_offset_t offset;
	vm_anon_t *anons;
	int npages;
{
	vm_amap_t amap;
	int slot;

	amap = aref->ar_amap;
	AMAP_B2SLOT(slot, offset);
	slot += aref->ar_pageoff;

	if ((slot + (npages - 1)) >= amap->am_nslot) {
		panic("amap_lookups: offset out of range");
	}

	bcopy(&amap->am_anon[slot], anons, npages * sizeof(struct vm_anon *));

	return;
}

/*
 * amap_add: add (or replace) a page to an amap
 *
 * => caller must lock amap.
 * => if (replace) caller must lock anon because we might have to call
 *	pmap_page_protect on the anon's page.
 * => returns an "offset" which is meaningful to amap_unadd().
 */
int
vm_amap_add(aref, offset, anon, replace)
	vm_aref_t aref;
	vm_offset_t offset;
	vm_anon_t anon;
	int replace;
{
	vm_amap_t amap;
	int slot;

	amap = aref->ar_amap;
	AMAP_B2SLOT(slot, offset);
	slot += aref->ar_pageoff;

	if (slot >= amap->am_nslot) {
		panic("amap_add: offset out of range");
	}

	if (replace) {

		if (amap->am_anon[slot] == NULL) {
			panic("amap_add: replacing null anon");
		}
		if (amap->am_anon[slot]->u.an_page != NULL && (amap->am_flags & AMAP_SHARED) != 0) {
			pmap_page_protect(VM_PAGE_TO_PHYS(amap->am_anon[slot]->u.an_page), VM_PROT_NONE);
			/*
			 * XXX: suppose page is supposed to be wired somewhere?
			 */
		}
	} else {   /* !replace */
		if (amap->am_anon[slot] != NULL) {
			panic("amap_add: slot in use");
		}

		amap->am_bckptr[slot] = amap->am_nused;
		amap->am_slots[amap->am_nused] = slot;
		amap->am_nused++;
	}
	amap->am_anon[slot] = anon;

	return (slot);
}

/*
 * amap_unadd: remove a page from an amap, given we know the slot #.
 *
 * => caller must lock amap
 */
void
vm_amap_unadd(amap, slot)
	vm_amap_t 	amap;
	vm_offset_t 	slot;
{
	int ptr;

	if (slot >= amap->am_nslot) {
		panic("amap_add: offset out of range");
	}

	if (amap->am_anon[slot] == NULL) {
		panic("amap_unadd: nothing there");
	}

	amap->am_anon[slot] = NULL;
	ptr = amap->am_bckptr[slot];

	if (ptr != (amap->am_nused - 1)) {	/* swap to keep slots contig? */
		amap->am_slots[ptr] = amap->am_slots[amap->am_nused - 1];
		amap->am_bckptr[amap->am_slots[ptr]] = ptr;	/* back link */
	}
	amap->am_nused--;
}

/*
 * amap_ref: gain a reference to an amap
 *
 * => amap must not be locked (we will lock)
 * => called at fork time to gain the child's reference
 */
void
vm_amap_ref(entry, flags)
	vm_map_entry_t entry;
	int flags;
{
	vm_amap_t amap;

	amap = entry->aref.ar_amap;
	amap_lock(amap);
	amap->am_ref++;
	if (flags & AMAP_SHARED) {
		amap->am_flags |= AMAP_SHARED;
	}
#ifdef VM_AMAP_PPREF
	if (amap->am_ppref == NULL && (flags & AMAP_REFALL) == 0 && (entry->start - entry->end) >> PAGE_SHIFT != amap->am_nslot) {
		vm_amap_pp_establish(amap);
	}
	if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
		if (flags & AMAP_REFALL) {
			vm_amap_pp_adjref(amap, 0, amap->am_nslot << PAGE_SHIFT, 1);
		} else {
			vm_amap_pp_adjref(amap, entry->aref.ar_pageoff, entry->end - entry->start, 1);
		}
	}
#endif
	amap_unlock(amap);
}

/*
 * amap_unref: remove a reference to an amap
 *
 * => caller must remove all pmap-level references to this amap before
 *	dropping the reference
 * => called from uvm_unmap_detach [only]  ... note that entry is no
 *	longer part of a map and thus has no need for locking
 * => amap must be unlocked (we will lock it).
 */
void
vm_amap_unref(entry, all)
	vm_map_entry_t entry;
	int all;
{
	vm_amap_t amap;

	amap = entry->aref.ar_amap;

	/*
	 * lock it
	 */
	amap_lock(amap);

	/*
	 * if we are the last reference, free the amap and return.
	 */

	if (amap->am_ref == 1) {
		vm_amap_wipeout(amap);	/* drops final ref and frees */
		return;					/* no need to unlock */
	}

	/*
	 * otherwise just drop the reference count(s)
	 */

	amap->am_ref--;
	if (amap->am_ref == 1 && (amap->am_flags & AMAP_SHARED) != 0) {
		amap->am_flags &= ~AMAP_SHARED;	/* clear shared flag */
	}
#ifdef VM_AMAP_PPREF
	if (amap->am_ppref == NULL && all == 0 && (entry->start - entry->end) >> PAGE_SHIFT != amap->am_nslot) {
		vm_amap_pp_establish(amap);
	}
	if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
		if (all) {
			vm_amap_pp_adjref(amap, 0, amap->am_nslot << PAGE_SHIFT, -1);
		} else {
			vm_amap_pp_adjref(amap, entry->aref.ar_pageoff, entry->end - entry->start, -1);
		}
	}
#endif
	amap_unlock(amap);
}

void
vm_amap_cleaner_segment(amap, anon, segment)
	vm_amap_t 	 	amap;
	vm_anon_t 		anon;
	vm_segment_t 	segment;
{
	vm_page_t page;

	if (!TAILQ_EMPTY(&segment->memq)) {
		TAILQ_FOREACH(page, &segment->memq, pageq) {
			if (page->segment == segment) {
				if (anon->u.an_page != page) {
					simple_unlock(&anon->an_lock);
				}
				if (amap_refs(amap) > 1) {
					vm_page_lock_queues();
					if (page->wire_count != 0) {
						vm_page_unlock_queues();
						simple_unlock(&anon->an_lock);
					}
					if (page->anon != anon) {
						simple_unlock(&anon->an_lock);
					}
					vm_page_lock_queues();
					vm_page_deactivate(page);
					vm_page_unlock_queues();
				}
				if (page->wire_count != 0) {
					simple_unlock(&anon->an_lock);
				}
			}
		}
	} else {
		if (amap_refs(amap) > 1) {
			vm_segment_lock_lists();
			if (segment->wire_tracker != 0) {
				vm_segment_unlock_lists();
				simple_unlock(&anon->an_lock);
			}
			if (segment->anon != anon) {
				simple_unlock(&anon->an_lock);
			}
			vm_segment_lock_lists();
			vm_segment_deactivate(segment);
			vm_segment_unlock_lists();
		}
		if (segment->wire_tracker != 0) {
			simple_unlock(&anon->an_lock);
		}
	}
}

void
vm_amap_cleaner_page(amap, anon, segment, page)
	vm_amap_t 	 	amap;
	vm_anon_t 		anon;
	vm_segment_t 	segment;
	vm_page_t 		page;
{
	vm_page_t chkpage;

	if (page->segment != segment) {
		chkpage = vm_page_lookup(segment, page->offset);
		if (chkpage != NULL) {
			if (chkpage->segment == segment) {
				if (chkpage != page) {
					page = chkpage;
				}
			}
		}
	}
	if (amap_refs(amap) > 1) {
		vm_page_lock_queues();
		if (page->wire_count != 0) {
			vm_page_unlock_queues();
			simple_unlock(&anon->an_lock);
		}
		if (page->anon != anon) {
			simple_unlock(&anon->an_lock);
		}
		vm_page_lock_queues();
		vm_page_deactivate(page);
		vm_page_unlock_queues();
	}
	if (page->wire_count != 0) {
		simple_unlock(&anon->an_lock);
	}
}

/*
 *	vm_amap_cleaner:		[ internal use only ]
 */
void
vm_amap_cleaner(amap, anon)
	vm_amap_t 	 	amap;
	vm_anon_t 		anon;
{
	vm_segment_t segment;
	vm_page_t page;

	simple_lock(&anon->an_lock);
	segment = anon->u.an_segment;
	page = anon->u.an_page;
	if (segment != NULL) {
		if (page != NULL) {
			vm_amap_cleaner_page(amap, anon, segment, page);
		} else {
			vm_amap_cleaner_segment(amap, anon, segment);
		}
	}
}

/*
 *	vm_amap_clean:
 *
 *	Clean anonymous memory. Primarily used to help vm_map_clean
 *	when dealing with amaps and anons.
 */
void
vm_amap_clean(current, size, offset, amap)
	vm_map_entry_t 	current;
	vm_size_t 		size;
	vm_offset_t 	offset;
	vm_amap_t 	 	amap;
{
	vm_anon_t anon;
	int refs;

	amap_lock(amap);
	for (; size != 0; size -= PAGE_SIZE, offset += PAGE_SIZE) {
		anon = vm_amap_lookup(&current->aref, offset);
		vm_amap_cleaner(amap, anon);
		vm_amap_unadd(current->aref.ar_amap, offset);
		refs = --anon->an_ref;
		simple_unlock(&anon->an_lock);
		if (refs == 0) {
			vm_anon_free(anon);
		}
		continue;
	}
	amap_unlock(amap);
}
