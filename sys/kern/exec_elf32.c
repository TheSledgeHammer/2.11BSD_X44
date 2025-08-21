/*	$NetBSD: exec_elf32.c,v 1.120.2.1 2007/07/09 10:30:57 liamjfoy Exp $	*/

/*-
 * Copyright (c) 1994, 2000, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef ELFSIZE
#define	ELFSIZE 	32
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/user.h>

#include <machine/cpu.h>
#include <machine/reg.h>

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

#define MAXPHNUM			50

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
elf_copyargs(elp, arginfo, stackp, argp)
	struct exec_linker *elp;
	struct ps_strings *arginfo;
	void *stackp;
	void *argp;
{
	size_t len;
	AuxInfo ai[ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;
	int error;

	if ((error = copyargs(elp, arginfo, stackp, argp)) != 0) {
		return (error);
	}

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)elp->el_emul_arg)) {
		struct vattr *vap = elp->el_attr;

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		free(ap, M_TEMP);
		elp->el_emul_arg = NULL;
	}
	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);
	if ((error = copyout(ai, stackp, len)) != 0)
		return (error);
	stackp = (caddr_t)stackp + len;
	return (0);
}

/*
 * elf_check_header():
 *
 * Check header for validity; return 0 of ok ENOEXEC if error
 */
int
elf_check_header(eh, type)
	Elf_Ehdr *eh;
	int type;
{
	if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
	    eh->e_ident[EI_CLASS] != ELFCLASS)
		return ENOEXEC;

	switch (eh->e_machine) {

	//ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		return ENOEXEC;
	}

	if (ELF_EHDR_FLAGS_OK(eh) == 0)
		return ENOEXEC;

	if (eh->e_type != type)
		return ENOEXEC;

	if (eh->e_shnum > 32768 || eh->e_phnum > 128)
		return ENOEXEC;

	return 0;
}

/*
 * elf_load_psection():
 *
 * Load a psection at the appropriate address
 */
void
elf_load_psection(vcset, vp, ph, addr, size, prot, flags)
	struct exec_vmcmd_set *vcset;
	struct vnode *vp;
	const Elf_Phdr *ph;
	Elf_Addr *addr;
	u_long *size;
	int *prot;
	int flags;
{
	u_long msize, psize, rm, rf;
	long diff, offset;

	/*
	 * If the user specified an address, then we load there.
	 */
	if (*addr == ELF_NO_ADDR)
		*addr = ph->p_vaddr;

	if (ph->p_align > 1) {
		/*
		 * Make sure we are virtually aligned as we are supposed to be.
		 */
		diff = ph->p_vaddr - ELF_TRUNC(ph->p_vaddr, ph->p_align);
		KASSERT(*addr - diff == ELF_TRUNC(*addr, ph->p_align));
		/*
		 * But make sure to not map any pages before the start of the
		 * psection by limiting the difference to within a page.
		 */
		diff &= PAGE_MASK;
	} else
		diff = 0;

	*prot |= (ph->p_flags & PF_R) ? VM_PROT_READ : 0;
	*prot |= (ph->p_flags & PF_W) ? VM_PROT_WRITE : 0;
	*prot |= (ph->p_flags & PF_X) ? VM_PROT_EXECUTE : 0;

	/*
	 * Adjust everything so it all starts on a page boundary.
	 */
	*addr -= diff;
	offset = ph->p_offset - diff;
	*size = ph->p_filesz + diff;
	msize = ph->p_memsz + diff;

	if (ph->p_align >= PAGE_SIZE) {
		if ((ph->p_flags & PF_W) != 0) {
			/*
			 * Because the pagedvn pager can't handle zero fill
			 * of the last data page if it's not page aligned we
			 * map the last page readvn.
			 */
			psize = trunc_page(*size);
		} else {
			psize = round_page(*size);
		}
	} else {
		psize = *size;
	}

	if (psize > 0) {
		NEW_VMCMD2(vcset, ph->p_align < PAGE_SIZE ? vmcmd_map_readvn : vmcmd_map_pagedvn,
				psize, *addr, *prot, *prot, flags, vp, offset);
		flags &= VMCMD_RELATIVE;
	}
	if (psize < *size) {
		NEW_VMCMD2(vcset, vmcmd_map_readvn, *size - psize, *addr + psize,
				*prot, *prot, flags, vp, offset + psize);
	}

	/*
	 * Check if we need to extend the size of the segment (does
	 * bss extend page the next page boundary)?
	 */
	rm = round_page(*addr + msize);
	rf = round_page(*addr + *size);

	if (rm != rf) {
		NEW_VMCMD2(vcset, vmcmd_map_zero, rm - rf, rf,
				*prot, *prot, flags & VMCMD_RELATIVE, NULL, 0);
		*size = msize;
	}
}

/*
 * elf_load_file():
 *
 * Load a file (interpreter/library) pointed to by path
 * [stolen from coff_load_shlib()]. Made slightly generic
 * so it might be used externally.
 */
int
elf_load_file(p, elp, path, vcset, entryoff, ap, last)
	struct proc *p;
	struct exec_linker *elp;
	char *path;
	struct exec_vmcmd_set *vcset;
	u_long *entryoff;
	struct elf_args *ap;
	Elf_Addr *last;
{
	int error, i;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	Elf_Ehdr eh;
	Elf_Phdr *ph = NULL;
	const Elf_Phdr *ph0;
	const Elf_Phdr *base_ph;
	const Elf_Phdr *last_ph;
	u_long phsize;
	Elf_Addr addr = *last;

	/*
	 * 1. open file
	 * 2. read filehdr
	 * 3. map text, data, and bss out of it using VM_*
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, path, p);
	if ((error = namei(&nd)) != 0)
		return error;
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

	error = vn_marktext(vp);
	if (error)
		goto badunlock;

	VOP_UNLOCK(vp, 0, p);

	if ((error = exec_read_from(p, vp, 0, &eh, sizeof(eh))) != 0)
		goto bad;

	if ((error = elf_check_header(&eh, ET_DYN)) != 0)
		goto bad;

	if (eh.e_phnum > MAXPHNUM)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = exec_read_from(p, vp, eh.e_phoff, ph, phsize)) != 0)
		goto bad;

#ifdef ELF_INTERP_NON_RELOCATABLE
	/*
	 * Evil hack:  Only MIPS should be non-relocatable, and the
	 * psections should have a high address (typically 0x5ffe0000).
	 * If it's now relocatable, it should be linked at 0 and the
	 * psections should have zeros in the upper part of the address.
	 * Otherwise, force the load at the linked address.
	 */
	if (*last == ELF_LINK_ADDR && (ph->p_vaddr & 0xffff0000) == 0)
		*last = ELFDEFNNAME(NO_ADDR);
#endif

	/*
	 * If no position to load the interpreter was set by a probe
	 * function, pick the same address that a non-fixed mmap(0, ..)
	 * would (i.e. something safely out of the way).
	 */
	if (*last == ELF_NO_ADDR) {
		u_long limit = 0;
		/*
		 * Find the start and ending addresses of the psections to
		 * be loaded.  This will give us the size.
		 */
		for (i = 0, ph0 = ph, base_ph = NULL; i < eh.e_phnum;
		     i++, ph0++) {
			if (ph0->p_type == PT_LOAD) {
				u_long psize = ph0->p_vaddr + ph0->p_memsz;
				if (base_ph == NULL)
					base_ph = ph0;
				if (psize > limit)
					limit = psize;
			}
		}

		if (base_ph == NULL) {
			error = ENOEXEC;
			goto bad;
		}

		/*
		 * Now compute the size and load address.
		 */
		addr = elp->el_daddr + (round_page(limit) - trunc_page(base_ph->p_vaddr));
	} else
		addr = *last; /* may be ELF_LINK_ADDR */

	/*
	 * Load all the necessary sections
	 */
	for (i = 0, ph0 = ph, base_ph = NULL, last_ph = NULL;
	     i < eh.e_phnum; i++, ph0++) {
		switch (ph0->p_type) {
		case PT_LOAD: {
			u_long size;
			int prot = 0;
			int flags;

			if (base_ph == NULL) {
				/*
				 * First encountered psection is always the
				 * base psection.  Make sure it's aligned
				 * properly (align down for topdown and align
				 * upwards for not topdown).
				 */
				base_ph = ph0;
				flags = VMCMD_BASE;
				if (addr == ELF_LINK_ADDR)
					addr = ph0->p_vaddr;
				if (addr > ELF_TRUNC(addr, ph0->p_align))
					addr = ELF_TRUNC(addr, ph0->p_align);
				else
					addr = ELF_ROUND(addr, ph0->p_align);
			} else {
				u_long limit = round_page(last_ph->p_vaddr + last_ph->p_memsz);
				u_long base = trunc_page(ph0->p_vaddr);

				/*
				 * If there is a gap in between the psections,
				 * map it as inaccessible so nothing else
				 * mmap'ed will be placed there.
				 */
				if (limit != base) {
					NEW_VMCMD2(vcset, vmcmd_map_zero, base - limit, limit - base_ph->p_vaddr,
							VM_PROT_NONE, VM_PROT_NONE, VMCMD_RELATIVE, NULL, 0);
				}

				addr = ph0->p_vaddr - base_ph->p_vaddr;
				flags = VMCMD_RELATIVE;
			}
			last_ph = ph0;
			elf_load_psection(vcset, vp, &ph[i], &addr,
			    &size, &prot, flags);
			/*
			 * If entry is within this psection then this
			 * must contain the .text section.  *entryoff is
			 * relative to the base psection.
			 */
			if (eh.e_entry >= ph0->p_vaddr &&
			    eh.e_entry < (ph0->p_vaddr + size)) {
				*entryoff = eh.e_entry - base_ph->p_vaddr;
			}
			addr += size;
			break;
		}

		case PT_DYNAMIC:
		case PT_PHDR:
			break;

		case PT_NOTE:
			break;

		default:
			break;
		}
	}

	free(ph, M_TEMP);
	/*
	 * This value is ignored if TOPDOWN.
	 */
	*last = addr;
	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0, p);

bad:
	if (ph != NULL)
		free(ph, M_TEMP);
	vrele(vp);
	return error;
}

int
exec_elf_linker(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	Elf_Ehdr *eh = elp->el_image_hdr;
	Elf_Phdr *ph, *pp;
	Elf_Addr phdr = 0, pos = 0;
	int error, i, n, nload;
	char interp[MAXPATHLEN];
	u_long phsize;

	if (elp->el_hdrvalid < sizeof(Elf_Ehdr)) {
		return ENOEXEC;
	}

	/*
	 * XXX allow for executing shared objects. It seems silly
	 * but other ELF-based systems allow it as well.
	 */
	if (elf_check_header(eh, ET_EXEC) != 0 && elf_check_header(eh, ET_DYN) != 0)
		return ENOEXEC;

	if (eh->e_phnum > MAXPHNUM)
			return ENOEXEC;

	error = vn_marktext(elp->el_vnodep);
	if (error)
		return error;

	/*
	 * Allocate space to hold all the program headers, and read them
	 * from the file
	 */
	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = exec_read_from(p, elp->el_vnodep, eh->e_phoff, ph, phsize)) != 0)
		goto bad;

	elp->el_taddr = elp->el_tsize = ELF_NO_ADDR;
	elp->el_daddr = elp->el_dsize = ELF_NO_ADDR;

	interp[0] = '\0';

	for (i = 0; i < eh->e_phnum; i++) {
		pp = &ph[i];
		if (pp->p_type == PT_INTERP) {
			if (pp->p_filesz >= MAXPATHLEN)
				goto bad;
			if ((error = exec_read_from(p, elp->el_vnodep, pp->p_offset, (caddr_t)interp,
					pp->p_filesz)) != 0)
				goto bad;
			break;
		}
	}


	/*
	 * Load all the necessary sections
	 */
	for (i = nload = 0; i < eh->e_phnum; i++) {
		Elf_Addr  addr = ELF_NO_ADDR;
		u_long size = 0;
		int prot = 0;

		pp = &ph[i];

		switch (ph[i].p_type) {
		case PT_LOAD:
			if (nload++ == 2)
				goto bad;
			elf_load_psection(&elp->el_vmcmds, elp->el_vnodep, &ph[i], &addr, &size, &prot, VMCMD_FIXED);

				/*
				 * Decide whether it's text or data by looking
				 * at the entry point.
				 */
			if (eh->e_entry >= addr && eh->e_entry < (addr + size)) {
				elp->el_taddr = addr;
				elp->el_tsize = size;
				if (elp->el_daddr == ELF_NO_ADDR) {
					elp->el_daddr = addr;
					elp->el_dsize = size;
				}
			} else {
				elp->el_daddr = addr;
				elp->el_dsize = size;
			}
			break;

		case PT_SHLIB:
				/* SCO has these sections. */
		case PT_INTERP:
				/* Already did this one. */
		case PT_DYNAMIC:
			break;
		case PT_NOTE:
			break;
		case PT_PHDR:
				/* Note address of program headers (in text segment) */
			phdr = pp->p_vaddr;
			break;

		default:
				/*
				 * Not fatal; we don't need to understand everything.
				 */
			break;
		}
	}

	/*
	 * Check if we found a dynamically linked binary and arrange to load
	 * its interpreter
	 */
	if (interp) {
		struct elf_args *ap;
		int j = elp->el_vmcmds.evs_used;
		u_long interp_offset;

		MALLOC(ap, struct elf_args *, sizeof(struct elf_args), M_TEMP, M_WAITOK);
		if ((error = elf_load_file(p, elp, interp, &elp->el_vmcmds, &interp_offset, ap, &pos)) != 0) {
			FREE(ap, M_TEMP);
			goto bad;
		}
		ap->arg_interp = elp->el_vmcmds.evs_cmds[j].ev_addr;
		elp->el_entry = ap->arg_interp + interp_offset;
		ap->arg_phaddr = phdr;

		ap->arg_phentsize = eh->e_phentsize;
		ap->arg_phnum = eh->e_phnum;
		ap->arg_entry = eh->e_entry;

		elp->el_emul_arg = ap;
	} else {
		elp->el_entry = eh->e_entry;
	}
	free(ph, M_TEMP);
	return (*elp->el_esch->ex_setup_stack)(elp);

bad:
	free(ph, M_TEMP);
	kill_vmcmds(&elp->el_vmcmds);
	return ENOEXEC;
}

int
twoelevenbsd_elf_signature(p, elp, eh)
	struct proc *p;
	struct exec_linker *elp;
	Elf_Ehdr *eh;
{
	size_t i;
	Elf_Phdr *ph;
	size_t phsize;
	int error;
	int isnetbsd = 0;
	char *ndata;

	if (eh->e_phnum > MAXPHNUM)
		return ENOEXEC;

	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
	error = exec_read_from(p, elp->el_vnodep, eh->e_phoff, ph, phsize);
	if (error)
		goto out;

	for (i = 0; i < eh->e_phnum; i++) {
		Elf_Phdr *ephp = &ph[i];
		Elf_Nhdr *np;

		if (ephp->p_type != PT_NOTE ||
		    ephp->p_filesz > 1024 ||
		    ephp->p_filesz < sizeof(Elf_Nhdr) + ELF_NOTE_211BSD_NAMESZ)
			continue;

		np = (Elf_Nhdr *)malloc(ephp->p_filesz, M_TEMP, M_WAITOK);
		error = exec_read_from(p, elp->el_vnodep, ephp->p_offset, np,
		    ephp->p_filesz);
		if (error)
			goto next;

		ndata = (char *)(np + 1);
		switch (np->n_type) {
		case ELF_NOTE_TYPE_211BSD_TAG:
			if (np->n_namesz != ELF_NOTE_211BSD_NAMESZ ||
					np->n_descsz != ELF_NOTE_211BSD_DESCSZ ||
					memcmp(ndata, ELF_NOTE_211BSD_NAME, ELF_NOTE_211BSD_NAMESZ))
				goto next;
			isnetbsd = 1;
			break;

		case ELF_NOTE_TYPE_PAX_TAG:
			if (np->n_namesz != ELF_NOTE_PAX_NAMESZ ||
			    np->n_descsz != ELF_NOTE_PAX_DESCSZ ||
			    memcmp(ndata, ELF_NOTE_PAX_NAME, ELF_NOTE_PAX_NAMESZ))
				goto next;
			(void)memcpy(&elp->el_pax_flags, ndata + ELF_NOTE_PAX_NAMESZ, sizeof(elp->el_pax_flags));
			break;

		default:
			break;
		}

next:
		free(np, M_TEMP);
		continue;
	}
	error = isnetbsd ? 0 : ENOEXEC;

out:
	free(ph, M_TEMP);
	return error;
}

int
twoelevenbsd_elf_probe(p, elp, eh, itp, pos)
	struct proc *p;
	struct exec_linker *elp;
	void *eh;
	char *itp;
	caddr_t *pos;
{
	int error;
	if ((error = twoelevenbsd_elf_signature(p, elp, eh)) != 0)
		return error;
#ifdef ELF_INTERP_NON_RELOCATABLE
	*pos = ELF_LINK_ADDR;
#endif
	return 0;
}
