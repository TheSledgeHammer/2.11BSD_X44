/*
 * ovl_object.c
 *
 *  Created on: 25 Aug 2020
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <devel/vm/include/vm_page.h>
#include <devel/vm/ovl/ovl.h>
#include <devel/vm/ovl/ovl_object.h>

struct ovl_object	kernel_ovl_object_store;
struct ovl_object	vm_ovl_object_store;

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
ovl_object_init(size)
	vm_size_t	size;
{
	register int	i;

	TAILQ_INIT(&vm_object_cached_list);
	TAILQ_INIT(&vm_object_list);
	ovl_object_count = 0;
	simple_lock_init(&vm_cache_lock);
	simple_lock_init(&vm_object_list_lock);

	for (i = 0; i < VM_OBJECT_HASH_COUNT; i++)
		TAILQ_INIT(&vm_object_hashtable[i]);

	kernel_object = &kernel_object_store;
	_vm_object_allocate(size, kernel_object);

	kmem_object = &kmem_object_store;
	_vm_object_allocate(VM_KMEM_SIZE + VM_MBUF_SIZE, kmem_object);
}
