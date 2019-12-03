/*
 * imgact_coff.c
 *
 *  Created on: 26 Nov 2019
 *      Author: marti
 */

#include "sys/imgact_coff.h"

#include <vm/vm.h>
#include "sys/imgact.h"

static int
exec_coff_imgact(iparams)
	struct image_params *iparams;
{
	struct coff_aouthdr *coff = (struct coff_aouthdr *) iparams->image_header;
	struct vmspace *vmspace = iparams->proc->p_vmspace;
	unsigned long vmaddr, virtual_offset, file_offset;
	unsigned long bss_size;
	int error;
	extern struct sysentvec coff_sysvec;

	return (0);
}

