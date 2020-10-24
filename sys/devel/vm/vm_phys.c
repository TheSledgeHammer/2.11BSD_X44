/*
 * vm_phys.c
 *
 *  Created on: 22 Oct 2020
 *      Author: marti
 */
#include <sys/malloc.h>
#include <devel/vm/include/vm.h>
#include <vm_phys.h>

struct vm_physseg vm_physmem[VM_PHYSSEG_MAX];
int vm_nphysseg = 0;

static void *
vm_physseg_alloc(size_t sz)
{
	if (vm.page_init_done == FALSE) {
		if (sz % sizeof(struct vm_physseg))
			panic("%s: tried to alloc size other than multiple of struct uvm_physseg at boot\n", __func__);

		size_t n = sz / sizeof(struct vm_physseg);
		nseg += n;

		KASSERT(nseg > 0 && nseg <= VM_PHYSSEG_MAX);

		return &vm_physseg[nseg - n];
	}
	return ();
}


vm_phys_init()
{
	size_t freepages, pagecount, n;
	vm_page_t pagearray, curpg;
	int lcv, i;
	caddr_t paddr, pgno;
	struct vm_physseg *seg;

	if (vm_nphysseg == 0)
		panic("uvm_page_bootstrap: no memory pre-allocated");

	freepages = 0;
	for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg ; lcv++, seg++)
			freepages += (seg->vp_end - seg->vp_start);

	pagecount = (((caddr_t)freepages + 1) << PAGE_SHIFT) /(PAGE_SIZE + sizeof(struct vm_page));
	pagearray = (vm_page_t)uvm_pageboot_alloc(pagecount * sizeof(struct vm_page));
	memset(pagearray, 0, pagecount * sizeof(struct vm_page));

	/* init the vm_page structures and put them in the correct place. */
	for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg ; lcv++, seg++) {
		n = seg->vp_end - seg->vp_start;
		if(n > pagecount) {
			panic("uvm_page_init: lost %ld page(s) in init", (long)(n - pagecount));
		}
		/* set up page array pointers */
		seg->vp_pages = pagearray;
		pagearray += n;
		pagecount -= n;
		seg->vp_last_page = seg->vp_pages + (n - 1);

		/* init and free vm_pages (we've already zeroed them) */
		pgno = seg->vp_start;
		paddr = ptoa(pgno);
		for (i = 0, curpg = seg->vp_pages; i < n; i++, curpg++, pgno++, paddr += PAGE_SIZE) {
			curpg->phys_addr = paddr;
			VM_MDPAGE_INIT(curpg);
			if (pgno >= seg->vp_avail_start && pgno < seg->vp_avail_end) {
				vmexp.npages++;
			}
		}
		/* Add pages to free pool. */
	}


	vmexp.reserve_pagedaemon = 4;
	vmexp.reserve_kernel = 6;
	vmexp.anonminpct = 10;
	vmexp.vnodeminpct = 10;
	vmexp.vtextminpct = 5;
	vmexp.anonmin = vmexp.anonminpct * 256 / 100;
	vmexp.vnodemin = vmexp.vnodeminpct * 256 / 100;
	vmexp.vtextmin = vmexp.vtextminpct * 256 / 100;

	vm.page_init_done = TRUE;
}

void
vm_page_physload(caddr_t start, caddr_t end, caddr_t avail_start, caddr_t avail_end, int flags)
{
	int preload, lcv;
	size_t npages;
	struct vm_page *pgs;
	struct vm_physseg *ps, *seg;

	ps->start = start;
	ps->end = end;
	ps->avail_start = avail_start;
	ps->avail_end = avail_end;
	if (preload) {
		ps->pgs = NULL;
	} else {
		ps->pgs = pgs;
		ps->lastpg = pgs + npages - 1;
	}
	vm_nphysseg++;

	return;
}

//vm_pageboot_alloc(size_t size)

#if !defined(PMAP_STEAL_MEMORY)
/*
 * uvm_page_physget: "steal" one page from the vm_physmem structure.
 *
 * => attempt to allocate it off the end of a segment in which the "avail"
 *    values match the start/end values.   if we can't do that, then we
 *    will advance both values (making them equal, and removing some
 *    vm_page structures from the non-avail area).
 * => return false if out of memory.
 */

boolean_t
uvm_page_physget(caddr_t *paddrp)
{
	int lcv;
	struct vm_physseg *seg;

	/* pass 1: try allocating from a matching end */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST) || \
	(VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)
	for (lcv = vm_nphysseg - 1, seg = vm_physmem + lcv; lcv >= 0;
	    lcv--, seg--)
#else
	for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg ; lcv++, seg++)
#endif
	{
		if (vm.page_init_done == TRUE)
			panic("uvm_page_physget: called _after_ bootstrap");

		/* try from front */
		if (seg->avail_start == seg->start &&
		    seg->avail_start < seg->avail_end) {
			*paddrp = ptoa(seg->avail_start);
			seg->avail_start++;
			seg->start++;
			/* nothing left?   nuke it */
			if (seg->avail_start == seg->end) {
				if (vm_nphysseg == 1)
				    panic("uvm_page_physget: out of memory!");
				vm_nphysseg--;
				for (; lcv < vm_nphysseg; lcv++, seg++)
					/* structure copy */
					seg[0] = seg[1];
			}
			return (TRUE);
		}

		/* try from rear */
		if (seg->avail_end == seg->end &&
		    seg->avail_start < seg->avail_end) {
			*paddrp = ptoa(seg->avail_end - 1);
			seg->avail_end--;
			seg->end--;
			/* nothing left?   nuke it */
			if (seg->avail_end == seg->start) {
				if (vm_nphysseg == 1)
				    panic("uvm_page_physget: out of memory!");
				vm_nphysseg--;
				for (; lcv < vm_nphysseg ; lcv++, seg++)
					/* structure copy */
					seg[0] = seg[1];
			}
			return (TRUE);
		}
	}

	/* pass2: forget about matching ends, just allocate something */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST) || \
	(VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)
	for (lcv = vm_nphysseg - 1, seg = vm_physmem + lcv; lcv >= 0;
	    lcv--, seg--)
#else
	for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg ; lcv++, seg++)
#endif
	{

		/* any room in this bank? */
		if (seg->avail_start >= seg->avail_end)
			continue;  /* nope */

		*paddrp = ptoa(seg->avail_start);
		seg->avail_start++;
		/* truncate! */
		seg->start = seg->avail_start;

		/* nothing left?   nuke it */
		if (seg->avail_start == seg->end) {
			if (vm_nphysseg == 1)
				panic("uvm_page_physget: out of memory!");
			vm_nphysseg--;
			for (; lcv < vm_nphysseg ; lcv++, seg++)
				/* structure copy */
				seg[0] = seg[1];
		}
		return (TRUE);
	}

	return (FALSE);        /* whoops! */
}

#endif /* PMAP_STEAL_MEMORY */

/*
 * uvm_page_physload: load physical memory into VM system
 *
 * => all args are PFs
 * => all pages in start/end get vm_page structures
 * => areas marked by avail_start/avail_end get added to the free page pool
 * => we are limited to VM_PHYSSEG_MAX physical memory segments
 */

void
uvm_page_physload(caddr_t start, caddr_t end, caddr_t avail_start,
    caddr_t avail_end, int flags)
{
	int preload, lcv;
	size_t npages;
	struct vm_page *pgs;
	struct vm_physseg *ps, *seg;

#ifdef DIAGNOSTIC
	if (uvmexp.pagesize == 0)
		panic("uvm_page_physload: page size not set!");

	if (start >= end)
		panic("uvm_page_physload: start >= end");
#endif

	/* do we have room? */
	if (vm_nphysseg == VM_PHYSSEG_MAX) {
		printf("uvm_page_physload: unable to load physical memory "
		    "segment\n");
		printf("\t%d segments allocated, ignoring 0x%llx -> 0x%llx\n",
		    VM_PHYSSEG_MAX, (long long)start, (long long)end);
		printf("\tincrease VM_PHYSSEG_MAX\n");
		return;
	}

	/*
	 * check to see if this is a "preload" (i.e. uvm_mem_init hasn't been
	 * called yet, so malloc is not available).
	 */
	for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg; lcv++, seg++) {
		if (seg->pgs)
			break;
	}
	preload = (lcv == vm_nphysseg);

	/* if VM is already running, attempt to malloc() vm_page structures */
	if (!preload) {
		/*
		 * XXXCDC: need some sort of lockout for this case
		 * right now it is only used by devices so it should be alright.
		 */
 		caddr_t paddr;

 		npages = end - start;  /* # of pages */

		pgs = (struct vm_page *)uvm_km_zalloc(kernel_map,
		    npages * sizeof(*pgs));
		if (pgs == NULL) {
			printf("uvm_page_physload: can not malloc vm_page "
			    "structs for segment\n");
			printf("\tignoring 0x%lx -> 0x%lx\n", start, end);
			return;
		}
		/* init phys_addr and free pages, XXX uvmexp.npages */
		for (lcv = 0, paddr = ptoa(start); lcv < npages;
		    lcv++, paddr += PAGE_SIZE) {
			pgs[lcv].phys_addr = paddr;
			VM_MDPAGE_INIT(&pgs[lcv]);
			if (atop(paddr) >= avail_start && atop(paddr) < avail_end) {
				if (flags & PHYSLOAD_DEVICE) {
					atomic_setbits_int(&pgs[lcv].pg_flags, PG_DEV);
					pgs[lcv].wire_count = 1;
				} else {
#if defined(VM_PHYSSEG_NOADD)
		panic("uvm_page_physload: tried to add RAM after vm_mem_init");
#endif
				}
			}
		}

		/* Add pages to free pool. */
		if ((flags & PHYSLOAD_DEVICE) == 0) {
			uvm_pmr_freepages(&pgs[avail_start - start], avail_end - avail_start);
		}

		/* XXXCDC: need hook to tell pmap to rebuild pv_list, etc... */
	} else {
		/* gcc complains if these don't get init'd */
		pgs = NULL;
		npages = 0;

	}

	/* now insert us in the proper place in vm_physmem[] */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_RANDOM)
	/* random: put it at the end (easy!) */
	ps = &vm_physmem[vm_nphysseg];
#elif (VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)
	{
		int x;
		/* sort by address for binary search */
		for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg; lcv++, seg++)
			if (start < seg->start)
				break;
		ps = seg;
		/* move back other entries, if necessary ... */
		for (x = vm_nphysseg, seg = vm_physmem + x - 1; x > lcv;
		    x--, seg--)
			/* structure copy */
			seg[1] = seg[0];
	}
#elif (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
	{
		int x;
		/* sort by largest segment first */
		for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg; lcv++, seg++)
			if ((end - start) >
			    (seg->end - seg->start))
				break;
		ps = &vm_physmem[lcv];
		/* move back other entries, if necessary ... */
		for (x = vm_nphysseg, seg = vm_physmem + x - 1; x > lcv;
		    x--, seg--)
			/* structure copy */
			seg[1] = seg[0];
	}
#else
	panic("uvm_page_physload: unknown physseg strategy selected!");
#endif

	ps->start = start;
	ps->end = end;
	ps->avail_start = avail_start;
	ps->avail_end = avail_end;
	if (preload) {
		ps->pgs = NULL;
	} else {
		ps->pgs = pgs;
		ps->lastpg = pgs + npages - 1;
	}
	vm_nphysseg++;

	return;
}


#if VM_PHYSSEG_MAX > 1
/*
 * vm_physseg_find: find vm_physseg structure that belongs to a PA
 */
int
vm_physseg_find(caddr_t pframe, int *offp)
{
	struct vm_physseg *seg;

#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)
	/* binary search for it */
	int	start, len, try;

	/*
	 * if try is too large (thus target is less than than try) we reduce
	 * the length to trunc(len/2) [i.e. everything smaller than "try"]
	 *
	 * if the try is too small (thus target is greater than try) then
	 * we set the new start to be (try + 1).   this means we need to
	 * reduce the length to (round(len/2) - 1).
	 *
	 * note "adjust" below which takes advantage of the fact that
	 *  (round(len/2) - 1) == trunc((len - 1) / 2)
	 * for any value of len we may have
	 */

	for (start = 0, len = vm_nphysseg ; len != 0 ; len = len / 2) {
		try = start + (len / 2);	/* try in the middle */
		seg = vm_physmem + try;

		/* start past our try? */
		if (pframe >= seg->start) {
			/* was try correct? */
			if (pframe < seg->end) {
				if (offp)
					*offp = pframe - seg->start;
				return(try);            /* got it */
			}
			start = try + 1;	/* next time, start here */
			len--;			/* "adjust" */
		} else {
			/*
			 * pframe before try, just reduce length of
			 * region, done in "for" loop
			 */
		}
	}
	return(-1);

#else
	/* linear search for it */
	int	lcv;

	for (lcv = 0, seg = vm_physmem; lcv < vm_nphysseg ; lcv++, seg++) {
		if (pframe >= seg->start && pframe < seg->end) {
			if (offp)
				*offp = pframe - seg->start;
			return(lcv);		   /* got it */
		}
	}
	return(-1);

#endif
}


