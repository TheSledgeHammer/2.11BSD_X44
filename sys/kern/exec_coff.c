/*	$NetBSD: coff_exec.c,v 1.34 2019/11/20 19:37:52 pgoyette Exp $	*/

/*
 * Copyright (c) 1994, 1995 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * originally from kern/exec_ecoff.c
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
 *      This product includes software developed by Scott Bartram.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_coff.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>

int
exec_coff_linker(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	int error;
	struct coff_filehdr *fp = elp->el_image_hdr;
	struct coff_aouthdr *ap;

	if (elp->el_hdrvalid < COFF_HDR_SIZE)
		return ENOEXEC;

	if (COFF_BADMAG(fp))
		return ENOEXEC;

	ap = (void *)((char *)elp->el_image_hdr + sizeof(struct coff_filehdr));
	switch (ap->a_magic) {
	case COFF_OMAGIC:
		error = exec_coff_prep_omagic(p, elp, fp, ap);
		break;
	case COFF_NMAGIC:
		error = exec_coff_prep_nmagic(p, elp, fp, ap);
		break;
	case COFF_ZMAGIC:
		error = exec_coff_prep_zmagic(p, elp, fp, ap);
		break;
	default:
		return ENOEXEC;
	}

	if (error)
		kill_vmcmds(&elp->el_vmcmds);

	return error;
}

/*
 * coff_find_section - load specified section header
 */
int
coff_find_section(p, vp, fp, sh, s_type)
	struct proc *p;
	struct vnode *vp;
	struct coff_filehdr *fp;
	struct coff_scnhdr *sh;
	int s_type;
{
	int i, pos, error;
	size_t siz, resid;

	pos = COFF_HDR_SIZE;
	for (i = 0; i < fp->f_nscns; i++, pos += sizeof(struct coff_scnhdr)) {
		siz = sizeof(struct coff_scnhdr);
		error = vn_rdwr(UIO_READ, vp, (void *) sh,
		    siz, pos, UIO_SYSSPACE, IO_NODELOCKED, p->p_cred,
		    &resid, NULL);
		if (error) {
			DPRINTF(("section hdr %d read error %d\n", i, error));
			return error;
		}
		siz -= resid;
		if (siz != sizeof(struct coff_scnhdr)) {
			DPRINTF(("incomplete read: hdr %d ask=%d, rem=%zu got %zu\n",
			    s_type, sizeof(struct coff_scnhdr),
			    resid, siz));
			return ENOEXEC;
		}
		DPRINTF(("found section: %lu\n", sh->s_flags));
		if (sh->s_flags == s_type)
			return 0;
	}
	return ENOEXEC;
}

/*
 * exec_coff_prep_zmagic(): Prepare a COFF ZMAGIC binary's exec package
 *
 * First, set the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */
int
exec_coff_prep_zmagic(p, elp, fp, ap)
	struct proc *p;
	struct exec_linker *elp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
{
	int error;
	u_long offset;
	long dsize;
	long  baddr, bsize;
	struct coff_scnhdr sh;

	DPRINTF(("enter exec_coff_prep_zmagic\n"));

	/* set up command for text segment */
	error = coff_find_section(p, elp->el_vnodep, fp, &sh, COFF_STYP_TEXT);
	if (error) {
		DPRINTF(("can't find text section: %d\n", error));
		return error;
	}
	DPRINTF(("COFF text addr %lu size %ld offset %ld\n", sh.s_vaddr, sh.s_size, sh.s_scnptr));
	elp->el_taddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - elp->el_taddr);
	elp->el_tsize = sh.s_size + (sh.s_vaddr - elp->el_taddr);

	error = vn_marktext(elp->el_vnodep);
	if (error)
		return (error);

	DPRINTF(("VMCMD: addr %lx size %lx offset %lx\n", elp->el_taddr, elp->el_tsize, offset));
	if (!(offset & PAGE_MASK) && !(elp->el_taddr & PAGE_MASK)) {
		elp->el_tsize =	round_page(elp->el_tsize);
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_pagedvn, elp->el_tsize, elp->el_taddr,
				(VM_PROT_READ | VM_PROT_EXECUTE), (VM_PROT_READ | VM_PROT_EXECUTE),
				elp->el_vnodep, offset);
	} else {
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, elp->el_tsize, elp->el_taddr,
					(VM_PROT_READ | VM_PROT_EXECUTE), (VM_PROT_READ | VM_PROT_EXECUTE),
					elp->el_vnodep, offset);
	}

	/* set up command for data segment */
	error = coff_find_section(p, elp->el_vnodep, fp, &sh, COFF_STYP_DATA);
	if (error) {
		DPRINTF(("can't find data section: %d\n", error));
		return error;
	}
	DPRINTF(("COFF data addr %lx size %ld offset %ld\n", sh.s_vaddr, sh.s_size, sh.s_scnptr));
	elp->el_daddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - elp->el_daddr);
	dsize = sh.s_size + (sh.s_vaddr - elp->el_daddr);

	elp->el_dsize = dsize + ap->a_bsize;

	DPRINTF(("VMCMD: addr %lx size %lx offset %lx\n", elp->el_daddr, dsize, offset));
	if (!(offset & PAGE_MASK) && !(elp->el_daddr & PAGE_MASK)) {
		dsize = round_page(dsize);
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_pagedvn, dsize, elp->el_daddr,
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				elp->el_vnodep, offset);
	} else {
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, dsize, elp->el_daddr,
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				elp->el_vnodep, offset);
	}

	/* set up command for bss segment */
	baddr = round_page(elp->el_daddr + dsize);
	bsize = elp->el_daddr + elp->el_dsize - baddr;
	if (bsize > 0) {
		DPRINTF(("VMCMD: addr %x size %x offset %x\n", baddr, bsize, 0));
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, bsize, baddr,
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				NULL, 0);
	}

	/* set up entry point */
	elp->el_entry = ap->a_entry;

	return (*elp->el_esch->ex_setup_stack)(p, elp);
}


/*
 * exec_coff_prep_nmagic(): Prepare a 'native' NMAGIC COFF binary's exec
 *                          package.
 */
int
exec_coff_prep_nmagic(p, elp, fp, ap)
	struct proc *p;
	struct exec_linker *elp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
{
	elp->el_taddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_tstart);
	elp->el_tsize = ap->a_tsize;
	elp->el_daddr = COFF_ROUND(ap->a_dstart, COFF_LDPGSZ);
	elp->el_dsize = ap->a_dsize;
	elp->el_entry = ap->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, elp->el_tsize, elp->el_taddr,
			(VM_PROT_READ | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_EXECUTE),
			elp->el_vnodep, COFF_TXTOFF(fp, ap));

	/* set up command for data segment */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, elp->el_dsize, elp->el_daddr,
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			elp->el_vnodep, COFF_DATOFF(fp, ap));

	/* set up command for bss segment */
	if (ap->a_bsize > 0)
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, ap->a_bsize,
				COFF_SEGMENT_ALIGN(fp, ap, ap->a_dstart + ap->a_dsize),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				NULL, 0);

	return (*elp->el_esch->ex_setup_stack)(p, elp);
}

/*
 * exec_coff_prep_omagic(): Prepare a COFF OMAGIC binary's exec package
 */
int
exec_coff_prep_omagic(p, elp, fp, ap)
	struct proc *p;
	struct exec_linker *elp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
{
	elp->el_taddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_tstart);
	elp->el_tsize = ap->a_tsize;
	elp->el_daddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_dstart);
	elp->el_dsize = ap->a_dsize;
	elp->el_entry = ap->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, ap->a_tsize + ap->a_dsize, elp->el_taddr,
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			elp->el_vnodep, COFF_TXTOFF(fp, ap));

	/* set up command for bss segment */
	if (ap->a_bsize > 0)
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, ap->a_bsize,
				COFF_SEGMENT_ALIGN(fp, ap, ap->a_dstart + ap->a_dsize),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				NULL, 0);

	return (*elp->el_esch->ex_setup_stack)(p, elp);
}
