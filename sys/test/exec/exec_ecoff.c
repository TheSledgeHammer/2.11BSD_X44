/*	$NetBSD: exec_ecoff.c,v 1.11 2000/04/11 04:37:50 chs Exp $	*/

/*
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994, 1996, 1999 Christopher G. Demetriou
 * All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_ecoff.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/sysent.h>

#include <vm/vm.h>

int
exec_ecoff_linker(elp)
	struct exec_linker *elp;
{
	struct ecoff_exechdr *ecoff = (struct ecoff_exechdr *) elp->el_image_hdr;
	int error;

	if (elp->el_hdrvalid < ECOFF_HDR_SIZE) {
		return ENOEXEC;
	}

	if (ECOFF_BADMAG(ecoff))
		return ENOEXEC;

	error = cpu_exec_ecoff_hook(elp);

	switch (ecoff->a.magic) {
	case ECOFF_OMAGIC:
		error = exec_ecoff_prep_omagic(elp, ecoff, elp->el_vnodep);
		break;
	case ECOFF_NMAGIC:
		error = exec_ecoff_prep_nmagic(elp, ecoff, elp->el_vnodep);
		break;
	case ECOFF_ZMAGIC:
		error = exec_ecoff_prep_zmagic(elp, ecoff, elp->el_vnodep);
		break;
	default:
		return ENOEXEC;
	}
	if (!error) {
		error = exec_ecoff_setup_stack(elp);
	}

	return exec_mmap_to_vmspace(elp);
}

int
exec_ecoff_prep_zmagic(elp, ecoff, vp)
	struct exec_linker *elp;
	struct ecoff_exechdr *ecoff;
	struct vnode *vp;
{
		struct ecoff_aouthdr *ecoff_aout = &ecoff->a;
		struct vmspace *vmspace = elp->el_proc->p_vmspace;

		elp->el_taddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->text_start);
		elp->el_tsize = ecoff_aout->tsize;
		elp->el_daddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->data_start);
		elp->el_dsize = ecoff_aout->dsize + ecoff_aout->bsize;
		elp->el_entry = ecoff_aout->entry;

		/* set up for text */
		exec_mmap_setup(&vmspace->vm_map, elp->el_taddr, ecoff_aout->tsize,
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, ECOFF_TXTOFF(ecoff));

		/* set up for data segment */
		exec_mmap_setup(&vmspace->vm_map, elp->el_daddr, ecoff_aout->dsize,
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, ECOFF_DATOFF(ecoff));

		/* set up for bss segment */
		exec_mmap_setup(&vmspace->vm_map, ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->bss_start), ecoff_aout->bsize,
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, 0);

		return (0);
}

int
exec_ecoff_prep_nmagic(elp, ecoff, vp)
	struct exec_linker *elp;
	struct ecoff_exechdr *ecoff;
	struct vnode *vp;
{
		struct ecoff_aouthdr *ecoff_aout = &ecoff->a;
		struct vmspace *vmspace = elp->el_proc->p_vmspace;

		elp->el_taddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->text_start);
		elp->el_tsize = ecoff_aout->tsize;
		elp->el_daddr = ECOFF_ROUND(ecoff_aout->data_start, ECOFF_LDPGSZ);
		elp->el_dsize = ecoff_aout->dsize + ecoff_aout->bsize;
		elp->el_entry = ecoff_aout->entry;

		/* set up for text */
		exec_mmap_setup(&vmspace->vm_map, elp->el_taddr, elp->el_tsize,
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, ECOFF_TXTOFF(ecoff));

		/* set up for data segment */
		exec_mmap_setup(&vmspace->vm_map, elp->el_daddr, elp->el_dsize,
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, ECOFF_DATOFF(ecoff));

		/* set up for bss segment */
		if(ecoff_aout->bsize > 0) {
			exec_mmap_setup(&vmspace->vm_map, ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->bss_start), ecoff_aout->bsize,
					VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
					MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, 0);
		}

		return (0);
}

int
exec_ecoff_prep_omagic(elp, ecoff, vp)
	struct exec_linker *elp;
	struct ecoff_exechdr *ecoff;
	struct vnode *vp;
{
		struct ecoff_aouthdr *ecoff_aout = &ecoff->a;
		struct vmspace *vmspace = elp->el_proc->p_vmspace;

		elp->el_taddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->text_start);
		elp->el_tsize = ecoff_aout->tsize;
		elp->el_daddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->data_start);
		elp->el_dsize = ecoff_aout->dsize + ecoff_aout->bsize;
		elp->el_entry = ecoff_aout->entry;

		/* set up for text and data */
		exec_mmap_setup(&vmspace->vm_map, elp->el_taddr, ecoff_aout->tsize + ecoff_aout->dsize,
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, ECOFF_TXTOFF(ecoff));

		/* set up for bss segment */
		if(ecoff_aout->bsize > 0) {
			exec_mmap_setup(&vmspace->vm_map, ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->bss_start), ecoff_aout->bsize,
					VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
					MAP_PRIVATE | MAP_FIXED, (caddr_t)vp, 0);
		}

		return (0);
}

int
exec_ecoff_setup_stack(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;

	elp->el_maxsaddr = USRSTACK - MAXSSIZ;
	elp->el_minsaddr = USRSTACK;
	elp->el_ssize = u->u_rlimit[RLIMIT_STACK].rlim_cur;

	exec_mmap_setup(&vmspace->vm_map, elp->el_maxsaddr, ((elp->el_minsaddr - elp->el_ssize) - elp->el_maxsaddr),
			VM_PROT_NONE, VM_PROT_NONE, MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);

	exec_mmap_setup(&vmspace->vm_map, elp->el_ssize, (elp->el_minsaddr - elp->el_ssize),
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);

	return (0);
}
