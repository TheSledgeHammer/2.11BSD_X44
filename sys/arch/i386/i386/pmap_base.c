/*
 * pmap_base.c
 *
 *  Created on: 7 Mar 2020
 *      Author: marti
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vmmeter.h>
#include <sys/sysctl.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/pmap.h>

#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/cputypes.h>
#include <machine/md_var.h>
#include <machine/vmparam.h>
#include <machine/pmap_base.h>


static struct pmap_args *pmap_args_ptr;


void
pmap_bootstrap(vm_offset_t firstaddr, vm_offset_t loadaddr)
{
	pmap_args_ptr->pmap_bootstrap(firstaddr, loadaddr);
}

int
pmap_isvalidphys(int addr)
{
	return (pmap_args_ptr->pmap_isvalidphys(addr));
}

void *
pmap_bootstrap_alloc(size)
{
	return (pmap_args_ptr->pmap_bootstrap_alloc(size));
}

void
pmap_init(vm_offset_t phys_start, vm_offset_t phys_end)
{
	pmap_args_ptr->pmap_init(phys_start, phys_end);
}

vm_offset_t
pmap_map(vm_offset_t virt, vm_offset_t start, vm_offset_t end, int prot)
{
	return (pmap_args_ptr->pmap_map(virt, start, end, prot));
}

pmap_t
pmap_create(vm_size_t size)
{
	return (pmap_args_ptr->pmap_create(size));
}

void
pmap_pinit(struct pmap *pmap)
{
	pmap_args_ptr->pmap_pinit(pmap);
}

void
pmap_destroy(struct pmap pmap)
{
	pmap_args_ptr->pmap_destroy(pmap);
}

void
pmap_release(struct pmap *pmap)
{
	pmap_args_ptr->pmap_release(pmap);
}

void
pmap_reference(pmap_t pmap)
{
	pmap_args_ptr->pmap_reference(pmap);
}

void
pmap_remove(struct pmap *pmap, vm_offset_t sva, vm_offset_t eva)
{
	pmap_args_ptr->pmap_remove(pmap, sva, eva);
}

void
pmap_remove_all(vm_offset_t pa)
{
	pmap_args_ptr->pmap_remove_all(pa);
}

void
pmap_copy_on_write(vm_offset_t pa)
{
	pmap_args_ptr->pmap_copy_on_write(pa);
}

void
pmap_protect(pmap_t pmap, vm_offset_t sva, vm_offset_t eva, vm_prot_t prot)
{
	pmap_args_ptr->pmap_protect(pmap, sva, eva, prot);
}

void
pmap_enter(pmap_t pmap, vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired)
{
	pmap_args_ptr->pmap_enter(pmap, va, pa, prot, wired);
}

void
pmap_page_protect(vm_offset_t phys, vm_prot_t prot)
{
	pmap_args_ptr->pmap_page_protect(phys, prot);
}

void
pmap_change_wiring(pmap_t pmap, vm_offset_t va, boolean_t wired)
{
	pmap_args_ptr->pmap_change_wiring(pmap, va, wired);
}

struct pte *
pmap_pte(pmap_t pmap, vm_offset_t va)
{
	return (pmap_args_ptr->pmap_pte(pmap, va));
}

vm_offset_t
pmap_extract(pmap_t pmap, vm_offset_t va)
{
	return (pmap_args_ptr->pmap_extract(pmap, va));
}

void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vm_offset_t dst_addr, vm_size_t len, vm_offset_t src_addr)
{
	pmap_args_ptr->pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr);
}

void
pmap_update()
{
	pmap_args_ptr->pmap_update();
}

void
pmap_collect(pmap_t pmap)
{
	pmap_args_ptr->pmap_collect(pmap);
}

void
pmap_activate(pmap_t pmap, struct pcb *pcbp)
{
	pmap_args_ptr->pmap_activate(pmap, pcbp);
}

void
pmap_zero_page(vm_offset_t phys)
{
	pmap_args_ptr->pmap_zero_page(phys);
}

void
pmap_copy_page(vm_offset_t src, vm_offset_t dst)
{
	pmap_args_ptr->pmap_copy_page(src, dst);
}

void
pmap_pageable(pmap_t pmap, vm_offset_t sva, vm_offset_t eva, boolean_t pageable)
{
	pmap_args_ptr->pmap_pageable(pmap, sva, eva, pageable);
}

void
pmap_clear_modify(vm_offset_t pa)
{
	pmap_args_ptr->pmap_clear_modify(pa);
}

void
pmap_clear_reference(vm_offset_t pa)
{
	pmap_args_ptr->pmap_clear_reference(pa);
}

boolean_t
pmap_is_referenced(vm_offset_t pa)
{
	return (pmap_args_ptr->pmap_is_referenced(pa));
}

boolean_t
pmap_is_modified(vm_offset_t pa)
{
	return (pmap_args_ptr->pmap_is_modified(pa));
}

vm_offset_t
pmap_phys_address(int ppn)
{
	return (pmap_args_ptr->pmap_phys_address(ppn));
}

void
i386_protection_init()
{
	pmap_args_ptr->i386_protection_init();
}

boolean_t
pmap_testbit(vm_offset_t pa, int bit)
{
	return (pmap_args_ptr->pmap_testbit(pa, bit));
}

void
pmap_changebit(vm_offset_t pa, int bit, boolean_t setem)
{
	pmap_args_ptr->pmap_changebit(pa, bit, setem);
}

void
pmap_pvdump(vm_offset_t pa)
{
	pmap_args_ptr->pmap_pvdump(pa);
}

void
pmap_check_wiring(char *str, vm_offset_t va)
{
	pmap_args_ptr->pmap_change_wiring(str, va);
}

void
pads(pmap_t pm)
{
	pmap_args_ptr->pads(pm);
}
