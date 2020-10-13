/*
 * vm_page.c
 *
 *  Created on: 12 Oct 2020
 *      Author: marti
 */



#include <sys/param.h>
#include <sys/systm.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm_map.h>
#include <vm/include/vm_pageout.h>

/*
 *	Associated with page of user-allocatable memory is a
 *	page structure.
 */

struct pglist		*vm_page_buckets;			/* Array of buckets */
int					vm_page_bucket_count = 0;	/* How big is array? */
int					vm_page_hash_mask;			/* Mask for hash function */
simple_lock_data_t	bucket_lock;				/* lock for all buckets XXX */

struct pglist		vm_page_queue_free;
struct pglist		vm_page_queue_active;
struct pglist		vm_page_queue_inactive;
simple_lock_data_t	vm_page_queue_lock;
simple_lock_data_t	vm_page_queue_free_lock;

/* has physical page allocation been initialized? */
boolean_t vm_page_startup_initialized;

vm_page_t	vm_page_array;
long		first_page;
long		last_page;
vm_offset_t	first_phys_addr;
vm_offset_t	last_phys_addr;
vm_size_t	page_mask;
int			page_shift;

void
vm_set_page_size()
 {

	if (cnt.v_page_size == 0)
		cnt.v_page_size = DEFAULT_PAGE_SIZE;
	page_mask = cnt.v_page_size - 1;
	if ((page_mask & cnt.v_page_size) != 0)
		panic("vm_set_page_size: page size not a power of two");
	for (page_shift = 0;; page_shift++)
		if ((1 << page_shift) == cnt.v_page_size)
			break;
}

void
vm_page_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register vm_page_t		m;
	register struct pglist	*bucket;
	vm_size_t				npages;
	int						i;
	vm_offset_t				pa;
	extern	vm_offset_t		kentry_data;
	extern	vm_size_t		kentry_data_size;


	/*
	 *	Initialize the locks
	 */

	simple_lock_init(&vm_page_queue_free_lock);
	simple_lock_init(&vm_page_queue_lock);

	/*
	 *	Initialize the queue headers for the free queue,
	 *	the active queue and the inactive queue.
	 */

	TAILQ_INIT(&vm_page_queue_free);
	TAILQ_INIT(&vm_page_queue_active);
	TAILQ_INIT(&vm_page_queue_inactive);


	if (vm_page_bucket_count == 0) {
		vm_page_bucket_count = 1;
		while (vm_page_bucket_count < atop(*end - *start))
			vm_page_bucket_count <<= 1;
	}

	vm_page_hash_mask = vm_page_bucket_count - 1;

	/*
	 *	Allocate (and initialize) the hash table buckets.
	 */
	vm_page_buckets = (struct pglist *)
	    pmap_bootstrap_alloc(vm_page_bucket_count * sizeof(struct pglist));
	bucket = vm_page_buckets;

	for (i = vm_page_bucket_count; i--;) {
		TAILQ_INIT(bucket);
		bucket++;
	}

	simple_lock_init(&bucket_lock);

	*end = trunc_page(*end);

	kentry_data_size = round_page(MAX_KMAP*sizeof(struct vm_map) + MAX_KMAPENT*sizeof(struct vm_map_entry));
	kentry_data = (vm_offset_t) pmap_bootstrap_alloc(kentry_data_size);

	cnt.v_free_count = npages = (*end - *start + sizeof(struct vm_page))
			/ (PAGE_SIZE + sizeof(struct vm_page));


	first_page = *start;
	first_page += npages*sizeof(struct vm_page);
	first_page = atop(round_page(first_page));
	last_page  = first_page + npages - 1;

	first_phys_addr = ptoa(first_page);
	last_phys_addr  = ptoa(last_page) + PAGE_MASK;

	m = vm_page_array = (vm_page_t)pmap_bootstrap_alloc(npages * sizeof(struct vm_page));


	pa = first_phys_addr;
	while (npages--) {
		m->flags = 0;
		m->object = NULL;
		m->phys_addr = pa;
#ifdef i386
		if (pmap_isvalidphys(m->phys_addr)) {
			TAILQ_INSERT_TAIL(&vm_page_queue_free, m, pageq);
		} else {
			/* perhaps iomem needs it's own type, or dev pager? */
			m->flags |= PG_FICTITIOUS | PG_BUSY;
			cnt.v_free_count--;
		}
#else /* i386 */
		TAILQ_INSERT_TAIL(&vm_page_queue_free, m, pageq);
#endif /* i386 */
		m++;
		pa += PAGE_SIZE;
	}


	simple_lock_init(&vm_pages_needed_lock);

	vm_page_startup_initialized = TRUE;
}

void
vm_page_insert(mem, pagedirectory, offset)
	register vm_page_t			mem;
	register vm_pagedirectory_t	pagedirectory;
	register vm_offset_t		offset;
{
	register struct pglist	*bucket;
	int						spl;

	mem->pdtable = pagedirectory;
	mem->offset = offset;

	bucket = &vm_page_buckets[vm_page_hash(pagedirectory, offset)];
	spl = splimp();
	simple_lock(&bucket_lock);
	TAILQ_INSERT_TAIL(bucket, mem, hashq);
	simple_unlock(&bucket_lock);
	(void) splx(spl);


	TAILQ_INSERT_TAIL(&pagedirectory->pd_pglist, mem, listq);
	mem->flags |= PG_TABLED;

	pagedirectory->pd_resident_page_count++;
}
