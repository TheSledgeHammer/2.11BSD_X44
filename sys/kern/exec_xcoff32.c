/*	$NetBSD: exec_ecoff.c,v 1.11 2000/04/11 04:37:50 chs Exp $	*/

/*
 * Copyright (c) 2020 Martin Kelly
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

#include <sys/cdefs.h>

#ifndef XCOFFSIZE
#define	XCOFFSIZE		32
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_xcoff.h>
#include <sys/resourcevar.h>

int
exec_xcoff_linker(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	xcoff_exechdr *xcoff = (xcoff_exechdr *) elp->el_image_hdr;
	int error;

	xcoff->f.f_magic = XCOFF_F_MAGIC;

	if (elp->el_hdrvalid < XCOFF_HDR_SIZE) {
		return ENOEXEC;
	}

	if (XCOFF_BADMAG(xcoff)) {
		return ENOEXEC;
	}

	switch (xcoff->a.magic) {
	case XCOFF_OMAGIC:
		error = exec_xcoff_prep_omagic(p, elp, xcoff, elp->el_vnodep);
		break;
	case XCOFF_NMAGIC:
		error = exec_xcoff_prep_nmagic(p, elp, xcoff, elp->el_vnodep);
		break;
	case XCOFF_ZMAGIC:
		error = exec_xcoff_prep_zmagic(p, elp, xcoff, elp->el_vnodep);
		break;
	default:
		return ENOEXEC;
	}

	/* set up the stack */
	if (!error) {
		error = (*elp->el_esch->ex_setup_stack)(p, elp);
	}

	if (error)
		kill_vmcmds(&elp->el_vmcmds);

	return error;
}

int
exec_xcoff_prep_zmagic(p, elp, xcoff, vp)
	struct proc *p;
	struct exec_linker *elp;
	xcoff_exechdr *xcoff;
	struct vnode *vp;
{
	xcoff_aouthdr *xcoff_aout = &xcoff->a;
	struct vmspace *vmspace = p->p_vmspace;

	elp->el_taddr = XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->text_start);
	elp->el_tsize = xcoff_aout->tsize;
	elp->el_daddr = XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->data_start);
	elp->el_dsize = xcoff_aout->dsize + xcoff_aout->bsize;
	elp->el_entry = xcoff_aout->entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn,
			xcoff_aout->tsize - xcoff_aout->dsize, elp->el_taddr,
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), vp,
			XCOFF_TXTOFF(xcoff));

	/* set up for bss segment */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, xcoff_aout->bsize,
			XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->bss_start),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), vp, 0);

	return (0);
}

int
exec_xcoff_prep_nmagic(p, elp, xcoff, vp)
	struct proc *p;
	struct exec_linker *elp;
	xcoff_exechdr *xcoff;
	struct vnode *vp;
{
	xcoff_aouthdr *xcoff_aout = &xcoff->a;

	elp->el_taddr = XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->text_start);
	elp->el_tsize = xcoff_aout->tsize;
	elp->el_daddr = XCOFF_ROUND(xcoff_aout->data_start, XCOFF_LDPGSZ);
	elp->el_dsize = xcoff_aout->dsize + xcoff_aout->bsize;
	elp->el_entry = xcoff_aout->entry;

	/* set up for text */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, elp->el_tsize, elp->el_taddr,
			(VM_PROT_READ | VM_PROT_EXECUTE), (VM_PROT_READ | VM_PROT_EXECUTE),
			vp, XCOFF_TXTOFF(xcoff));

	/* set up for data segment */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, elp->el_dsize, elp->el_daddr,
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), vp,
			XCOFF_DATOFF(xcoff));

	/* set up for bss segment */
	if (xcoff_aout->bsize > 0) {
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, xcoff_aout->bsize,
				XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->bss_start),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), vp, 0);
	}

	return (0);
}

int
exec_xcoff_prep_omagic(p, elp, xcoff, vp)
	struct proc *p;
	struct exec_linker *elp;
	xcoff_exechdr *xcoff;
	struct vnode *vp;
{
	xcoff_aouthdr *xcoff_aout = &xcoff->a;

	elp->el_taddr = XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->text_start);
	elp->el_tsize = xcoff_aout->tsize;
	elp->el_daddr = XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->data_start);
	elp->el_dsize = xcoff_aout->dsize + xcoff_aout->bsize;
	elp->el_entry = xcoff_aout->entry;

	/* set up for text and data */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, xcoff_aout->tsize + xcoff_aout->dsize, elp->el_taddr,
			(VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE), (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE), vp, XCOFF_TXTOFF(xcoff));

	/* set up for bss segment */
	if (xcoff_aout->bsize > 0) {
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, xcoff_aout->bsize, XCOFF_SEGMENT_ALIGN(xcoff, xcoff_aout->bss_start),
				(VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE), (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), NULL, 0);
	}

	return (0);
}
