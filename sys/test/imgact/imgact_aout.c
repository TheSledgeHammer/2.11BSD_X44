/*
 * imgact_aout.c
 *
 *  Created on: 26 Nov 2019
 *      Author: marti
 */
#include "sys/imgact_aout.h"

#include <vm/vm.h>
#include "sys/imgact.h"

static int	exec_aout_imgact __P((struct image_params *iparams));

static int
exec_aout_imgact(iparams)
	struct image_params *iparams;
{
	struct exec *a_out = (struct exec *) iparams->image_header;
	struct vmspace *vmspace = iparams->proc->p_vmspace;
	unsigned long vmaddr, virtual_offset, file_offset;
	unsigned long bss_size;
	int error;
	extern struct sysentvec aout_sysvec;

	bss_size = roundup(a_out->a_bss, NBPG);

	/* Fill in process VM information */
	vmspace->vm_tsize = a_out->a_text >> PAGE_SHIFT;
	vmspace->vm_dsize = (a_out->a_data + bss_size) >> PAGE_SHIFT;
	vmspace->vm_taddr = (caddr_t) virtual_offset;
	vmspace->vm_daddr = (caddr_t) virtual_offset + a_out->a_text;


	/* Fill in image_params */
	iparams->interpreted = 0;
	iparams->entry_addr = a_out->a_entry;

	iparams->proc->p_sysent = &aout_sysvec;
	return (0);
}



static const struct execsw aout_execsw = { exec_aout_imgact, "a.out" };
TEXT_SET(execsw_set, aout_execsw);
