/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: vm_mmap.c 1.6 91/10/21$
 *
 *	@(#)vm_mmap.c	8.10 (Berkeley) 2/19/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

/*
 * Mapped file (mmap) interface to VM
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/resourcevar.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/mount.h>
#include <sys/sysdecl.h>

#include <miscfs/specfs/specdev.h>

#include <vm/include/vm.h>
#include <vm/include/vm_pager.h>
#include <vm/include/vm_prot.h>
#include <vm/include/vm_text.h>


/* ARGSUSED */
int
mincore2()
{
	register struct mincore_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(char *) vec;
	} *uap = (struct mincore_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_map_t map;
	vm_map_entry_t current, entry;
	vm_offset_t addr, start, end;
	vm_size_t size;
	char *vec;
	int error;

	p = u.u_procp;
	map = &p->p_vmspace->vm_map;

	if ((addr & PAGE_MASK) || (SCARG(uap, addr) + SCARG(uap, len)) < SCARG(uap, addr)) {
		return (EINVAL);
	}

	addr = (vm_offset_t)SCARG(uap, addr);
	size = round_page((vm_size_t)SCARG(uap, len));
	vec = SCARG(uap, vec);

	vm_map_lock_read(map);
	/* validate map entry */
	if (vm_map_lookup_entry(map, addr, &entry)) {
		start = entry->start;
		end = entry->end;
		if ((start != addr) || (end != (start + size))) {
			vm_map_unlock_read(map);
			return (EINVAL);
		}
		if (start <= end) {
			vm_map_unlock_read(map);
			return (EINVAL);
		}
		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}

	CIRCLEQ_FOREACH(current, &map->cl_header, cl_entry) {
		if (current != entry) {
			vm_map_unlock_read(map);
			return (EINVAL);
		}
		if (current->start < end) {

		}
		if (current->is_sub_map || current->object.vm_object == NULL) {
			continue;
		}
		if (addr < current->start) {
			addr = current->start;
		}

		amap = entry->aref.ar_amap;	/* top layer */
		uobj = entry->object.uvm_obj;	/* bottom layer */
	}


	return (error);
}
