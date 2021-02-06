/*
 * aobject.c
 *
 *  Created on: 6 Feb 2021
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_pager.h>

#include <devel/vm/uvm/uvm.h>
#include <devel/vm/uvm/vm_aobject.h>

#include <devel/sys/malloctypes.h>

static LIST_HEAD(aobjlist, vm_aobject) 	uao_list;
static simple_lock_data_t 				uao_list_lock;


vm_aobject_init()
{
	struct uao_swhash_elt 	*elt;
	struct vm_aobject 		*aobj;

	LIST_INIT(&uao_list);
	simple_lock_init(&uao_list_lock, "uao_list_lock");

	MALLOC(elt, struct uao_swhash_elt *, sizeof(struct uao_swhash_elt *), M_VMAOBJ, M_WAITOK);
	MALLOC(aobj, struct vm_aobject *, sizeof(struct vm_aobject *), M_VMAOBJ, M_WAITOK);

}

vm_aobject_t
vm_aobject_allocate(size, flags)
	vm_size_t	size;
{
	register vm_aobject_t	result;

	result = (vm_aobject_t)malloc((u_long)sizeof *result, M_VMAOBJ, flags);

	_vm_aobject_allocate(size, result, flags);

	return(result);
}

static void
_vm_aobject_allocate(size, aobj, flags)
	vm_size_t			size;
	struct vm_aobject 	*aobj;
	int 				flags;
{
	struct uao_swhash_elt *elt;
	static int kobj_alloced = 0;					/* not allocated yet */
	int pages = round_page(size) >> PAGE_SHIFT;

	/*
	 * malloc a new aobj unless we are asked for the kernel object
	 */
	if (flags & UAO_FLAG_KERNOBJ) {		/* want kernel object? */
		if (kobj_alloced) {
			panic("uao_create: kernel object already allocated");
		}

		aobj = (struct vm_aobject) &kernel_object_store;
		aobj->u_pages = pages;
		aobj->u_flags = UAO_FLAG_NOSWAP;	/* no swap to start */
		/* we are special, we never die */
		aobj->u_obj.ref_count = VM_OBJ_KERN;
		kobj_alloced = UAO_FLAG_KERNOBJ;
	} else if (flags & UAO_FLAG_KERNSWAP) {
		aobj = (struct vm_aobject) &kernel_object_store;
		if (kobj_alloced != UAO_FLAG_KERNOBJ) {
			panic("uao_create: asked to enable swap on kernel object");
		}
		kobj_alloced = UAO_FLAG_KERNSWAP;
	} else {

	}

	aobj->u_pages = pages;
	aobj->u_flags = 0;			/* normal object */

	/*
 	 * allocate hash/array if necessary
 	 *
 	 * note: in the KERNSWAP case no need to worry about locking since
 	 * we are still booting we should be the only thread around.
 	 */
	if (flags == 0 || (flags & UAO_FLAG_KERNSWAP) != 0) {
		int mflags = (flags & UAO_FLAG_KERNSWAP) != 0 ? M_NOWAIT : M_WAITOK;
		if (UAO_USES_SWHASH(aobj)) {
			aobj->u_swhash = hashinit(UAO_SWHASH_BUCKETS(aobj), M_VMAOBJ, mflags, &aobj->u_swhashmask);
			if (aobj->u_swhash == NULL) {
				panic("uao_create: hashinit swhash failed");
			}
		} else {
			MALLOC(aobj->u_swslots, int *, pages * sizeof(int), M_VMAOBJ, mflags);
			if (aobj->u_swslots == NULL) {
				panic("uao_create: malloc swslots failed");
			}
			memset(aobj->u_swslots, 0, pages * sizeof(int));
		}
		if (flags) {
			aobj->u_flags &= ~UAO_FLAG_NOSWAP; /* clear noswap */
		}
	}

	simple_lock(&uao_list_lock);
	LIST_INSERT_HEAD(&uao_list, aobj, u_list);
	simple_unlock(&uao_list_lock);
}
