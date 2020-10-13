/*
 * ovl_pager.c
 *
 *  Created on: 12 Oct 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <vm/include/vm_page.h>
#include <vm/include/vm_pageout.h>
#include <vm/include/vm_pager.h>
#include <vm/include/vm.h>

#include <vm/ovl/ovl_pager.h>

unsigned long ovl_pager_map_size = (PAGER_MAP_SIZE / 100) * 10;
#define OVL_PAGER_MAP_SIZE	ovl_pager_map_size

ovl_map_t 	ovl_pager_map;
vm_offset_t ovl_pager_sva, ovl_pager_eva;

struct pagerops ovlpagerops = {
	ovl_pager_init,
	ovl_pager_alloc,
	ovl_pager_dealloc
};

static void
ovl_pager_init()
{
	ovl_pager_map = ovl_suballoc(ovl_map, &ovl_pager_sva, &ovl_pager_eva, OVL_PAGER_MAP_SIZE);
}

static vm_pager_t
ovl_pager_alloc(handle, size, prot, foff)
	caddr_t handle;
	register vm_size_t size;
	vm_prot_t prot;
	vm_offset_t foff;
{
	register vm_pager_t  pager;
	register ovl_pager_t ovp;

	if (handle) {
		if (pager == NULL) {

		}
	}
}

static void
ovl_pager_dealloc(pager)
	vm_pager_t pager;
{

}
