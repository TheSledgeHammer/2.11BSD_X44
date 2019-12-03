/*
 * imgact_ecoff.c
 *
 *  Created on: 26 Nov 2019
 *      Author: marti
 */


#include "sys/imgact_ecoff.h"

#include <vm/vm.h>
#include "sys/imgact.h"

static int
exec_ecoff_imgact(iparams)
	struct image_params *iparams;
{
	struct ecoff_aouthdr *ecoff = (struct ecoff_aouthdr *) iparams->image_header;
	struct vmspace *vmspace = iparams->proc->p_vmspace;
	unsigned long vmaddr, virtual_offset, file_offset;
	unsigned long bss_size;
	int error;
	extern struct sysentvec ecoff_sysvec;

	return (0);
}
