/*	$NetBSD: pecoff_exec.c,v 1.34 2006/11/16 01:32:44 christos Exp $	*/

/*
 * Copyright (c) 2000 Masaru OKI
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * from compat/ibcs2/ibcs2_exec.c
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
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/stat.h>

#include <sys/exec_coff.h>
#include <sys/exec_pecoff.h>

#include <machine/coff_machdep.h>

#define PECOFF_SIGNATURE "PE\0\0"
static const char signature[] = PECOFF_SIGNATURE;

int
pecoff_copyargs(elp, arginfo, stackp, argp)
	struct exec_linker *elp;
	struct ps_strings *arginfo;
	void *stackp;
	void *argp;
{
	int len = sizeof(struct pecoff_args);
	struct pecoff_args *pecoff_arg;
	int error;

	if ((error = copyargs(elp, arginfo, stackp, argp)) != 0)
		return error;

	pecoff_arg = (struct pecoff_args *)elp->el_emul_arg;
	if ((error = copyout(pecoff_arg, stackp, len)) != 0)
		return error;

#if 0 /*  kern_exec.c? */
	free(ap, M_TEMP);
	elp->el_emul_arg = 0;
#endif

	stackp += len;
	return error;
}

/*
 * Check PE signature.
 */
int
pecoff_signature(p, vp, pecoff_dos)
	struct proc *p;
	struct vnode *vp;
	struct pecoff_dos_filehdr *pecoff_dos;
{
	int error;
	char tbuf[sizeof(signature) - 1];

	if (DOS_BADMAG(pecoff_dos)) {
		return ENOEXEC;
	}
	error = exec_read_from(p, vp, pecoff_dos->d_peofs, tbuf, sizeof(tbuf));
	if (error) {
		return error;
	}
	if (memcmp(tbuf, signature, sizeof(signature) - 1) == 0) {
		return 0;
	}
	return (EFTYPE);
}

/*
 * load(mmap) file.  for dynamic linker (ld.so.dll)
 */
int
pecoff_load_file(p, elp, path, vcset, entry, pecoff_arg)
	struct proc *p;
	struct exec_linker *elp;
	const char *path;
	struct exec_vmcmd_set *vcset;
	u_long *entry;
	struct pecoff_args *pecoff_arg;
{
	int error, peofs, scnsiz, i;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	struct pecoff_dos_filehdr pecoff_dos;
	struct coff_filehdr *coff_fp = 0;
	struct coff_aouthdr *coff_aout;
	struct pecoff_opthdr *pecoff_opt;
	struct coff_scnhdr *coff_sh = 0;

	/*
	 * 1. open file
	 * 2. read filehdr
	 * 3. map text, data, and bss out of it using VM_*
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, path, p);
	if ((error = namei(&nd)) != 0) {
		return error;
	}
	vp = nd.ni_vp;

	/*
	 * Similarly, if it's not marked as executable, or it's not a regular
	 * file, we don't allow it to be used.
	 */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto badunlock;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p)) != 0)
		goto badunlock;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, &attr, p->p_ucred, p)) != 0)
		goto badunlock;
	/*
	 * Check mount point.  Though we're not trying to exec this binary,
	 * we will be executing code from it, so if the mount point
	 * disallows execution or set-id-ness, we punt or kill the set-id.
	 */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto badunlock;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID)
		elp->el_attr->va_mode &= ~(S_ISUID | S_ISGID);

	if ((error = vn_marktext(vp)))
		goto badunlock;

	VOP_UNLOCK(vp, 0, p);
	/*
	 * Read header.
	 */
	error = exec_read_from(p, vp, 0, &pecoff_dos, sizeof(pecoff_dos));
	if (error != 0)
		goto bad;
	if ((error = pecoff_signature(p, vp, &pecoff_dos)) != 0)
		goto bad;
	coff_fp = malloc(PECOFF_HDR_SIZE, M_TEMP, M_WAITOK);
	peofs = pecoff_dos.d_peofs + sizeof(signature) - 1;
	error = exec_read_from(p, vp, peofs, coff_fp, PECOFF_HDR_SIZE);
	if (error != 0)
		goto bad;
	if (COFF_BADMAG(coff_fp)) {
		error = ENOEXEC;
		goto bad;
	}
	coff_aout = (void *)((char *)coff_fp + sizeof(struct coff_filehdr));
	pecoff_opt = (void *)((char *)coff_aout + sizeof(struct coff_aouthdr));
	/* read section header */
	scnsiz = sizeof(struct coff_scnhdr) * coff_fp->f_nscns;
	coff_sh = malloc(scnsiz, M_TEMP, M_WAITOK);
	if ((error = exec_read_from(p, vp, peofs + PECOFF_HDR_SIZE, coff_sh, scnsiz)) != 0)
		goto bad;

	/*
	 * Read section header, and mmap.
	 */
	for (i = 0; i < coff_fp->f_nscns; i++) {
		int prot = 0;
		long addr;
		u_long size;

		if (coff_sh[i].s_flags & COFF_STYP_DISCARD)
			continue;
		/* XXX ? */
		if ((coff_sh[i].s_flags & COFF_STYP_TEXT) &&
		    (coff_sh[i].s_flags & COFF_STYP_EXEC) == 0)
			continue;
		if ((coff_sh[i].s_flags & (COFF_STYP_TEXT|COFF_STYP_DATA| COFF_STYP_BSS| COFF_STYP_READ)) == 0)
			continue;
		coff_sh[i].s_vaddr += pecoff_opt->w_base; /* RVA --> VA */
		pecoff_load_section(vcset, vp, &coff_sh[i], &addr, &size, &prot);
	}
	*entry = pecoff_opt->w_base + coff_aout->a_entry;
	pecoff_arg->a_ldbase = pecoff_opt->w_base;
	pecoff_arg->a_ldexport = pecoff_opt->w_imghdr[0].i_vaddr + pecoff_opt->w_base;

	free(coff_fp, M_TEMP);
	free(coff_sh, M_TEMP);
	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0, p);

bad:
	if (coff_fp != 0)
		free(coff_fp, M_TEMP);
	if (coff_sh != 0)
		free(coff_sh, M_TEMP);
	vrele(vp);
	return error;
}

/*
 * mmap one section.
 */
void
pecoff_load_section(vcset, vp, coff_sh, addr, size, prot)
	struct exec_vmcmd_set *vcset;
	struct vnode *vp;
	struct coff_scnhdr *coff_sh;
	long *addr;
	u_long *size;
	int *prot;
{
	u_long diff, offset;

	*addr = COFF_ALIGN(coff_sh->s_vaddr);
	diff = (coff_sh->s_vaddr - *addr);
	offset = coff_sh->s_scnptr - diff;
	*size = COFF_ROUND(coff_sh->s_size + diff, COFF_LDPGSZ);

	*prot |= (coff_sh->s_flags & COFF_STYP_EXEC) ? VM_PROT_EXECUTE : 0;
	*prot |= (coff_sh->s_flags & COFF_STYP_READ) ? VM_PROT_READ : 0;
	*prot |= (coff_sh->s_flags & COFF_STYP_WRITE) ? VM_PROT_WRITE : 0;

	if (diff == 0 && offset == COFF_ALIGN(offset))
		NEW_VMCMD(vcset, vmcmd_map_pagedvn, *size, *addr, *prot, *prot, vp, offset);
	else
		NEW_VMCMD(vcset, vmcmd_map_readvn, coff_sh->s_size, coff_sh->s_vaddr, *prot, *prot, vp, coff_sh->s_scnptr);

	if (*size < coff_sh->s_paddr) {
		u_long baddr, bsize;
		baddr = *addr + COFF_ROUND(*size, COFF_LDPGSZ);
		bsize = coff_sh->s_paddr - COFF_ROUND(*size, COFF_LDPGSZ);
		DPRINTF(("additional zero space (addr %lx size %ld)\n",
					 baddr, bsize));
		NEW_VMCMD(vcset, vmcmd_map_zero, bsize, baddr, *prot, *prot, NULL, 0);
		*size = COFF_ROUND(coff_sh->s_paddr, COFF_LDPGSZ);
	}
}

int
exec_pecoff_linker(p, elp)
	struct proc *p;
    struct exec_linker *elp;
{
    int error, peofs;
	struct pecoff_dos_filehdr *pecoff_dos = (struct pecoff_dos_filehdr *) elp->el_image_hdr;
	struct coff_filehdr *coff_fp;

    /*
	 * mmap EXE file (PE format)
	 * 1. read header (DOS,PE)
	 * 2. mmap code section (READ|EXEC)
	 * 3. mmap other section, such as data (READ|WRITE|EXEC)
	 */
    if (elp->el_hdrvalid < PECOFF_DOS_HDR_SIZE) {
		return ENOEXEC;
	}
	if ((error = pecoff_signature(p, elp->el_vnodep, pecoff_dos)) != 0)
		return error;

	if ((error = vn_marktext(elp->el_vnodep)) != 0)
		return error;

	peofs = pecoff_dos->d_peofs + sizeof(signature) - 1;
	coff_fp = malloc(PECOFF_HDR_SIZE, M_TEMP, M_WAITOK);
	error = exec_read_from(p, elp->el_vnodep, peofs, coff_fp, PECOFF_HDR_SIZE);
	if (error) {
		free(coff_fp, M_TEMP);
		return error;
	}
	error = exec_pecoff_coff_linker(p, elp, coff_fp, peofs);

	if (error != 0)
		kill_vmcmds(&elp->el_vmcmds);

	free(coff_fp, M_TEMP);
	return error;
}

int
exec_pecoff_coff_linker(p, elp, coff_fp, peofs)
	struct proc *p;
	struct exec_linker *elp;
    struct coff_filehdr *coff_fp;
    int peofs;
{
	struct coff_aouthdr *coff_aout;
	int error;

    if (COFF_BADMAG(coff_fp)) {
		return ENOEXEC;
	}

    coff_aout = (void *)((char *)coff_fp + sizeof(struct coff_filehdr));

	switch (coff_aout->a_magic) {
	case COFF_OMAGIC:
		error = exec_pecoff_prep_omagic(p, elp, coff_aout, coff_fp, peofs);
		break;
	case COFF_NMAGIC:
		error = exec_pecoff_prep_nmagic(p, elp, coff_aout, coff_fp, peofs);
		break;
	case COFF_ZMAGIC:
		error = exec_pecoff_prep_zmagic(p, elp, coff_aout, coff_fp, peofs);
		break;
	default:
		return ENOEXEC;
	}

	return error;
}

int
exec_pecoff_prep_zmagic(p, elp, coff_aout, coff_fp, peofs)
	struct proc *p;
	struct exec_linker *elp;
	struct coff_aouthdr *coff_aout;
    struct coff_filehdr *coff_fp;
	int peofs;
{
   	int error, i;
	struct pecoff_opthdr *pecoff_opt;
	long daddr, baddr, bsize;
	u_long tsize, dsize;
	struct coff_scnhdr *coff_sh;
	struct pecoff_args *pecoff_arg;
	int scnsiz = sizeof(struct coff_scnhdr) * coff_fp->f_nscns;

    pecoff_opt = (void *)((char *)pecoff_arg + sizeof(struct coff_aouthdr));

	elp->el_tsize = coff_aout->a_tsize;
	elp->el_daddr = VM_MAXUSER_ADDRESS;
	elp->el_dsize = 0;
	/* read section header */
	coff_sh = malloc(scnsiz, M_TEMP, M_WAITOK);
	error = exec_read_from(p, elp->el_vnodep, peofs + PECOFF_HDR_SIZE, coff_sh,
	    scnsiz);
	if (error) {
		free(coff_sh, M_TEMP);
		return error;
	}
	/*
	 * map section
	 */
	for (i = 0; i < coff_fp->f_nscns; i++) {
		int prot = /*0*/VM_PROT_WRITE;
		long s_flags = coff_sh[i].s_flags;

		if ((s_flags & COFF_STYP_DISCARD) != 0)
			continue;
		coff_sh[i].s_vaddr += pecoff_opt->w_base; /* RVA --> VA */

		if ((s_flags & COFF_STYP_TEXT) != 0) {
			/* set up command for text segment */
/*			DPRINTF(("COFF text addr %lx size %ld offset %ld\n",
				 sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));
*/			pecoff_load_section(&elp->el_vmcmds, elp->el_vnodep,
					   &coff_sh[i], (long *)&elp->el_taddr,
					   &tsize, &prot);
		} else if ((s_flags & COFF_STYP_BSS) != 0) {
			/* set up command for bss segment */
			baddr = coff_sh[i].s_vaddr;
			bsize = coff_sh[i].s_paddr;
			if (bsize)
				NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, bsize, baddr,
						(VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE),
						(VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE), NULL, 0);
			elp->el_daddr = min(elp->el_daddr, baddr);
			bsize = baddr + bsize - elp->el_daddr;
			elp->el_dsize = max(elp->el_dsize, bsize);
		} else if ((s_flags & (COFF_STYP_DATA|COFF_STYP_READ)) != 0) {
			/* set up command for data segment */
/*			DPRINTF(("COFF data addr %lx size %ld offset %ld\n",
			sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));*/
			pecoff_load_section(&elp->el_vmcmds, elp->el_vnodep,
					   &coff_sh[i], &daddr, &dsize, &prot);
			elp->el_daddr = min(elp->el_daddr, daddr);
			dsize = daddr + dsize - elp->el_daddr;
			elp->el_dsize = max(elp->el_dsize, dsize);
		}
	}
	/* set up ep_emul_arg */
	pecoff_arg = malloc(sizeof(struct pecoff_args), M_TEMP, M_WAITOK);
	elp->el_emul_arg = pecoff_arg;
	pecoff_arg->a_abiversion = NETBSDPE_ABI_VERSION;
	pecoff_arg->a_zero = 0;
	pecoff_arg->a_entry = pecoff_opt->w_base + coff_aout->a_entry;
	pecoff_arg->a_end = elp->el_daddr + elp->el_dsize;
	pecoff_arg->a_opthdr = *pecoff_opt;

	/*
	 * load dynamic linker
	 */
	error = pecoff_load_file(p, elp, "/usr/libexec/ld.so.dll",
				&elp->el_vmcmds, &elp->el_entry, pecoff_arg);
	if (error) {
		free(coff_sh, M_TEMP);
		return error;
	}

#if 0
	DPRINTF(("text addr: %lx size: %ld data addr: %lx size: %ld entry: %lx\n",
		 elp->el_taddr, elp->el_tsize,
		 elp->el_daddr, elp->el_dsize,
		 elp->el_entry));
#endif

	free(coff_sh, M_TEMP);
	return (*elp->el_esch->ex_setup_stack)(p, elp);
}

int
exec_pecoff_prep_nmagic(p, elp, coff_aout, coff_fp, peofs)
	struct proc *p;
	struct exec_linker *elp;
	struct coff_aouthdr *coff_aout;
    struct coff_filehdr *coff_fp;
	int peofs;
{
	return (ENOEXEC);
}

int
exec_pecoff_prep_omagic(p, elp, coff_aout, coff_fp, peofs)
	struct proc *p;
	struct exec_linker *elp;
	struct coff_aouthdr *coff_aout;
    struct coff_filehdr *coff_fp;
	int peofs;
{
	return (ENOEXEC);
}
