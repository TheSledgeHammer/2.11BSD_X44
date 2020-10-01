/*
 * vm_object.c
 *
 *  Created on: 30 Sep 2020
 *      Author: marti
 */


/*
 *	Virtual memory object module.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>

#include <devel/vm/include/vm_page.h>
#include <devel/vm/segment/vm_object.h>
#include <devel/vm/include/vm.h>

/*
 *	Virtual memory objects maintain the actual data
 *	associated with allocated virtual memory.  A given
 *	page of memory exists within exactly one object.
 *
 *	An object is only deallocated when all "references"
 *	are given up.  Only one "reference" to a given
 *	region of an object should be writeable.
 *
 *	Associated with each object is a list of all resident
 *	memory pages belonging to that object; this list is
 *	maintained by the "vm_page" module, and locked by the object's
 *	lock.
 *
 *	Each object also records a "pager" routine which is
 *	used to retrieve (and store) pages to the proper backing
 *	storage.  In addition, objects may be backed by other
 *	objects from which they were virtual-copied.
 *
 *	The only items within the object structure which are
 *	modified after time of creation are:
 *		reference count		locked by object's lock
 *		pager routine		locked by object's lock
 *
 */

struct vm_object	kernel_object_store;
struct vm_object	kmem_object_store;

#define	VM_OBJECT_HASH_COUNT	157

int		vm_cache_max = 100;	/* can patch if necessary */
struct	vm_object_hash_head vm_object_hashtable[VM_OBJECT_HASH_COUNT];

long	object_collapses = 0;
long	object_bypasses  = 0;

static void _vm_object_allocate (vm_size_t, vm_object_t);

/*
 *	vm_object_init:
 *
 *	Initialize the VM objects module.
 */
void
vm_object_init(size)
	vm_size_t	size;
{
	register int	i;

	RB_INIT(&vm_object_cached_tree);
	RB_INIT(&vm_object_tree);
	vm_object_count = 0;
	simple_lock_init(&vm_cache_lock);
	simple_lock_init(&vm_object_tree_lock);

	for (i = 0; i < VM_OBJECT_HASH_COUNT; i++)
		RB_INIT(&vm_object_hashtable[i]);

	kernel_object = &kernel_object_store;
	_vm_object_allocate(size, kernel_object);

	kmem_object = &kmem_object_store;
	_vm_object_allocate(VM_KMEM_SIZE + VM_MBUF_SIZE, kmem_object);
}

/*
 *	vm_object_allocate:
 *
 *	Returns a new object with the given size.
 */

vm_object_t
vm_object_allocate(size)
	vm_size_t	size;
{
	register vm_object_t	result;

	result = (vm_object_t)malloc((u_long)sizeof(*result), M_VMOBJ, M_WAITOK);

	_vm_object_allocate(size, result);

	return(result);
}

static void
_vm_object_allocate(size, object)
	vm_size_t				size;
	register vm_object_t	object;
{
	CIRCLEQ_INIT(&object->segl);
	vm_object_lock_init(object);
	object->ref_count = 1;
	object->resident_segment_count = 0;
	object->size = size;
	object->flags = OBJ_INTERNAL;	/* vm_allocate_with_segment will reset */
	object->segment_active = 0;
	object->copy = NULL;

	/*
	 *	Object starts out read-write, with no pager.
	 */

	object->pager = NULL;
	object->segment_offset = 0;
	object->shadow = NULL;
	object->shadow_offset = (vm_offset_t) 0;

	simple_lock(&vm_object_tree_lock);
	RB_INSERT(objecttree, &vm_object_tree, object);
	vm_object_count++;
	cnt.v_nzfod += atop(size);
	simple_unlock(&vm_object_tree_lock);
}

/*
 *	vm_object_reference:
 *
 *	Gets another reference to the given object.
 */
void
vm_object_reference(object)
	register vm_object_t	object;
{
	if (object == NULL)
		return;

	vm_object_lock(object);
	object->ref_count++;
	vm_object_unlock(object);
}

/*
 *	vm_object_deallocate:
 *
 *	Release a reference to the specified object,
 *	gained either through a vm_object_allocate
 *	or a vm_object_reference call.  When all references
 *	are gone, storage associated with this object
 *	may be relinquished.
 *
 *	No object may be locked.
 */
void
vm_object_deallocate(object)
	register vm_object_t	object;
{
	vm_object_t	temp;

	while (object != NULL) {

		/*
		 *	The cache holds a reference (uncounted) to
		 *	the object; we must lock it before removing
		 *	the object.
		 */

		vm_object_cache_lock();

		/*
		 *	Lose the reference
		 */
		vm_object_lock(object);
		if (--(object->ref_count) != 0) {

			/*
			 *	If there are still references, then
			 *	we are done.
			 */
			vm_object_unlock(object);
			vm_object_cache_unlock();
			return;
		}

		/*
		 *	See if this object can persist.  If so, enter
		 *	it in the cache, then deactivate all of its
		 *	pages.
		 */

		if (object->flags & OBJ_CANPERSIST) {

			RB_INSERT(objecttree, &vm_object_cached_tree, object);
			vm_object_cached++;
			vm_object_cache_unlock();

			//vm_object_deactivate_pages(object);
			vm_object_unlock(object);

			vm_object_cache_trim();
			return;
		}

		/*
		 *	Make sure no one can look us up now.
		 */
		vm_object_remove(object->pager);
		vm_object_cache_unlock();

		temp = object->shadow;
		vm_object_terminate(object);
		/* unlocks and deallocates object */
		object = temp;
	}
}

/*
 *	vm_object_terminate actually destroys the specified object, freeing
 *	up all previously used resources.
 *
 *	The object must be locked.
 */
void
vm_object_terminate(object)
	register vm_object_t	object;
{
	register vm_segment_t	p;
	vm_object_t				shadow_object;

	/*
	 *	Detach the object from its shadow if we are the shadow's
	 *	copy.
	 */
	if ((shadow_object = object->shadow) != NULL) {
		vm_object_lock(shadow_object);
		if (shadow_object->copy == object)
			shadow_object->copy = NULL;
#if 0
		else if (shadow_object->copy != NULL)
			panic("vm_object_terminate: copy/shadow inconsistency");
#endif
		vm_object_unlock(shadow_object);
	}

	/*
	 * Wait until the pageout daemon is through with the object.
	 */
	while (object->segment_active) {
		vm_object_sleep(object, object, FALSE);
		vm_object_lock(object);
	}

	/*
	 * If not an internal object clean all the pages, removing them
	 * from paging queues as we go.
	 *
	 * XXX need to do something in the event of a cleaning error.
	 */
	//if ((object->flags & OBJ_INTERNAL) == 0)
	//	(void) vm_object_page_clean(object, 0, 0, TRUE, TRUE);

	/*
	 * Now free the pages.
	 * For internal objects, this also removes them from paging queues.
	 */
	while ((p = CIRCLEQ_FIRST(object->segl)) != NULL) {
		//VM_PAGE_CHECK(p);
		//vm_page_lock_queues();
		//vm_page_free(p);
		//cnt.v_pfree++;
		//vm_page_unlock_queues();
	}
	vm_object_unlock(object);

	/*
	 * Let the pager know object is dead.
	 */

	if (object->pager != NULL)
		vm_pager_deallocate(object->pager);

	simple_lock(&vm_object_tree_lock);
	RB_REMOVE(objecttree, &vm_object_tree, object);
	vm_object_count--;
	simple_unlock(&vm_object_tree_lock);

	/*
	 * Free the space for the object.
	 */
	free((caddr_t)object, M_VMOBJ);
}

/*
 *	Trim the object cache to size.
 */
void
vm_object_cache_trim()
{
	register vm_object_t	object;

	vm_object_cache_lock();
	while (vm_object_cached > vm_cache_max) {
		object = RB_FIRST(objecttree, vm_object_cached_tree);
		vm_object_cache_unlock();

		if (object != vm_object_lookup(object->pager))
			panic("vm_object_deactivate: I'm sooo confused.");

		pager_cache(object, FALSE);

		vm_object_cache_lock();
	}
	vm_object_cache_unlock();
}

/*
 *	vm_object_shadow:
 *
 *	Create a new object which is backed by the
 *	specified existing object range.  The source
 *	object reference is deallocated.
 *
 *	The new object and offset into that object
 *	are returned in the source parameters.
 */

void
vm_object_shadow(object, offset, length)
	vm_object_t	*object;	/* IN/OUT */
	vm_offset_t	*offset;	/* IN/OUT */
	vm_size_t	length;
{
	register vm_object_t	source;
	register vm_object_t	result;

	source = *object;

	/*
	 *	Allocate a new object with the given length
	 */

	if ((result = vm_object_allocate(length)) == NULL)
		panic("vm_object_shadow: no object for shadowing");

	/*
	 *	The new object shadows the source object, adding
	 *	a reference to it.  Our caller changes his reference
	 *	to point to the new object, removing a reference to
	 *	the source object.  Net result: no change of reference
	 *	count.
	 */
	result->shadow = source;

	/*
	 *	Store the offset into the source object,
	 *	and fix up the offset into the new object.
	 */

	result->shadow_offset = *offset;

	/*
	 *	Return the new things
	 */

	*offset = 0;
	*object = result;
}

/* vm object rbtree comparator */
int
vm_object_rb_compare(obj1, obj2)
	struct vm_object *obj1, *obj2;
{
	if(obj1->size < obj2->size) {
		return (-1);
	} else if(obj1->size > obj2->size) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(objecttree, vm_object, object_tree, vm_object_rb_compare);
RB_GENERATE(objecttree, vm_object, object_tree, vm_object_rb_compare);
RB_PROTOTYPE(vm_object_hash_head, vm_object_hash_entry, hash_links, vm_object_rb_compare);
RB_GENERATE(vm_object_hash_head, vm_object_hash_entry, hash_links, vm_object_rb_compare);

/*
 *	vm_object_hash hashes the pager/id pair.
 */
unsigned long
vm_object_hash(pager)
	vm_pager_t pager;
{
    Fnv32_t hash1 = fnv_32_buf(&pager, sizeof(&pager), FNV1_32_INIT) % VM_OBJECT_HASH_COUNT;
    Fnv32_t hash2 = (((unsigned long)pager)%VM_OBJECT_HASH_COUNT);
    return (hash1^hash2);
}

/*
 *	vm_object_lookup looks in the object cache for an object with the
 *	specified pager and paging id.
 */

vm_object_t
vm_object_lookup(pager)
	vm_pager_t	pager;
{
	struct vm_object_hash_head		*bucket;
	register vm_object_hash_entry_t	entry;
	vm_object_t						object;

	vm_object_cache_lock();
	bucket = &vm_object_hashtable[vm_object_hash(pager)];
	for (entry = RB_FIRST(vm_object_hash_head, bucket); entry != NULL; entry = RB_NEXT(vm_object_hash_head, bucket, entry)) {
		object = entry->object;
		if (object->pager == pager) {
			vm_object_lock(object);
			if (object->ref_count == 0) {
				RB_REMOVE(objecttree, &vm_object_cached_tree, object);
				vm_object_cached--;
			}
			object->ref_count++;
			vm_object_unlock(object);
			vm_object_cache_unlock();
			return (object);
		}
	}

	vm_object_cache_unlock();
	return (NULL);
}

/*
 *	vm_object_enter enters the specified object/pager/id into
 *	the hash table.
 */

void
vm_object_enter(object, pager)
	vm_object_t	object;
	vm_pager_t	pager;
{
	struct vm_object_hash_head	*bucket;
	register vm_object_hash_entry_t	entry;

	/*
	 *	We don't cache null objects, and we can't cache
	 *	objects with the null pager.
	 */

	if (object == NULL)
		return;
	if (pager == NULL)
		return;

	bucket = &vm_object_hashtable[vm_object_hash(pager)];
	entry = (vm_object_hash_entry_t)malloc((u_long)sizeof *entry, M_VMOBJHASH, M_WAITOK);
	entry->object = object;
	object->flags |= OBJ_CANPERSIST;

	vm_object_cache_lock();
	RB_INSERT(vm_object_hash_head, bucket, entry);
	vm_object_cache_unlock();
}

/*
 *	vm_object_remove:
 *
 *	Remove the pager from the hash table.
 *	Note:  This assumes that the object cache
 *	is locked.  XXX this should be fixed
 *	by reorganizing vm_object_deallocate.
 */

void
vm_object_remove(pager)
	register vm_pager_t	pager;
{
	struct vm_object_hash_head		*bucket;
	register vm_object_hash_entry_t	entry;
	register vm_object_t			object;

	bucket = &vm_object_hashtable[vm_object_hash(pager)];

	for (entry = RB_FIRST(vm_object_hash_head, bucket); entry != NULL; entry = RB_NEXT(vm_object_hash_head, bucket, entry)) {
		object = entry->object;
		if (object->pager == pager) {
			RB_REMOVE(vm_object_hash_head, bucket, entry);
			free((caddr_t)entry, M_VMOBJHASH);
			break;
		}
	}
}

/*
 *	vm_object_cache_clear removes all objects from the cache.
 *
 */
void
vm_object_cache_clear()
{
	register vm_object_t	object;

	/*
	 *	Remove each object in the cache by scanning down the
	 *	list of cached objects.
	 */
	vm_object_cache_lock();
	while ((object = RB_FIRST(objtree, &vm_object_cached_tree)) != NULL) {
		vm_object_cache_unlock();

		/*
		 * Note: it is important that we use vm_object_lookup
		 * to gain a reference, and not vm_object_reference, because
		 * the logic for removing an object from the cache lies in
		 * lookup.
		 */
		if (object != vm_object_lookup(object->pager))
			panic("vm_object_cache_clear: I'm sooo confused.");
		pager_cache(object, FALSE);

		vm_object_cache_lock();
	}
	vm_object_cache_unlock();
}
