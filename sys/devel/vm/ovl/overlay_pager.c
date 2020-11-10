/*
 * overlay_pager.c
 *
 *  Created on: 8 Nov 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/mount.h>

#include <devel/vm/ovl/ovl.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_page.h>

struct pagerlst 	overlay_pager_list;

static void			overlay_pager_init(void);
static vm_pager_t	overlay_pager_alloc(caddr_t, vm_size_t, vm_prot_t, vm_offset_t);
static boolean_t	overlay_pager_dealloc(vm_pager_t);

struct pagerops overlaypagerops = {
		overlay_pager_init,
		overlay_pager_alloc,
		overlay_pager_dealloc,
};

static void
overlay_pager_init()
{
	TAILQ_INIT(&overlay_pager_list);
}

static vm_pager_t
overlay_pager_alloc(handle, size, prot, foff)
	caddr_t handle;
	vm_size_t size;
	vm_prot_t prot;
	vm_offset_t foff;
{
	register vm_pager_t  	pager;
	register ovl_pager_t 	ovl;
	ovl_object_t			ovl_object;

	if (handle) {
		pager = vm_pager_lookup(&overlay_pager_list, handle);
		if (pager != NULL) {
			if (ovl_object_lookup(pager) == NULL) {
				panic("overlay_pager_alloc: bad overlay object");
			}
			return (pager);
		}
	}

	ovl = (ovl_pager_t)handle;
	ovl_object = ovl->ovl_object;
	pager = ovl_object->ovo_pager;

	vm_object_t fake_vm_object = ovl_object->ovo_vm_object;

	if (ovl_object == NULL) {
		ovl_object = ovl_object_allocate(0);
		fake_vm_object = vm_object_allocate(0);
	} else {

		if (fake_vm_object) { 											/* check ovl_object contains a vm object */
			fake_vm_object = ovl_object_lookup_vm_object(ovl_object); 	/* search for the vm_object */
		}

		TAILQ_INSERT_TAIL(&overlay_pager_list, pager, pg_list);

		/*
		 * Associate ovl_object with pager & add vm_object to
		 * the ovl_object's list of associated vm_objects.
		 */
		ovl_object_enter(ovl_object, pager);
		ovl_object_enter_vm_object(ovl_object, fake_vm_object);
		ovl_object_setpager(ovl_object, pager, (vm_offset_t)0, FALSE);
	}

	pager->pg_handle = handle;
	pager->pg_ops = &overlaypagerops;
	pager->pg_type = PG_OVERLAY;
	pager->pg_data = ovl;

	return (pager);
}

static void
overlay_pager_dealloc(pager)
	vm_pager_t  	pager;
{
	register ovl_pager_t  ovl;
	ovl_object_t		  ovl_object;

	TAILQ_REMOVE(&overlay_pager_list, pager, pg_list);

	ovl = (ovl_pager_t) pager->pg_data;
	ovl_object = ovl->ovl_object;

	free((caddr_t)ovl, M_VMPGDATA);
	free((caddr_t)pager, M_VMPAGER);
}
