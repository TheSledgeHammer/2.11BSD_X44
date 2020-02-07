
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
#include <sys/exec_aout.h>
#include <test/exec_pecoff.h>
#include <machine/coff_machdep.h>

#define PECOFF_SIGNATURE "PE\0\0"
static const char signature[] = PECOFF_SIGNATURE;

/*
 * Check PE signature.
 */
int
pecoff_signature(l, vp, dp)
	struct lwp *l;
	struct vnode *vp;
	struct pecoff_dos_filehdr *dp;
{
	int error;
	char tbuf[sizeof(signature) - 1];

	if (DOS_BADMAG(dp)) {
		return ENOEXEC;
	}
	error = exec_read_from(l, vp, dp->d_peofs, tbuf, sizeof(tbuf));
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
pecoff_load_file(l, epp, path, vcset, entry, argp)
	struct lwp *l;
	struct exec_package *epp;
	const char *path;
	struct exec_vmcmd_set *vcset;
	u_long *entry;
	struct pecoff_args *argp;
{
	int error, peofs, scnsiz, i;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	struct pecoff_dos_filehdr dh;
	struct coff_filehdr *fp = 0;
	struct coff_aouthdr *ap;
	struct pecoff_opthdr *wp;
	struct coff_scnhdr *sh = 0;
	const char *bp;
}

/*
 * mmap one section.
 */
void
pecoff_load_section(vcset, vp, sh, addr, size, prot)
	struct exec_vmcmd_set *vcset;
	struct vnode *vp;
	struct coff_scnhdr *sh;
	long *addr;
	u_long *size;
	int *prot;
{
	u_long diff, offset;

	*addr = COFF_ALIGN(sh->s_vaddr);
	diff = (sh->s_vaddr - *addr);
	offset = sh->s_scnptr - diff;
	*size = COFF_ROUND(sh->s_size + diff, COFF_LDPGSZ);

	*prot |= (sh->s_flags & COFF_STYP_EXEC) ? VM_PROT_EXECUTE : 0;
	*prot |= (sh->s_flags & COFF_STYP_READ) ? VM_PROT_READ : 0;
	*prot |= (sh->s_flags & COFF_STYP_WRITE) ? VM_PROT_WRITE : 0;

	if (diff == 0 && offset == COFF_ALIGN(offset))
		NEW_VMCMD(vcset, vmcmd_map_pagedvn, *size, *addr, vp,
		 			  offset, *prot);
	else
		NEW_VMCMD(vcset, vmcmd_map_readvn, sh->s_size, sh->s_vaddr, vp,
					  sh->s_scnptr, *prot);

	if (*size < sh->s_paddr) {
		u_long baddr, bsize;
		baddr = *addr + COFF_ROUND(*size, COFF_LDPGSZ);
		bsize = sh->s_paddr - COFF_ROUND(*size, COFF_LDPGSZ);
		DPRINTF(("additional zero space (addr %lx size %ld)\n",
					 baddr, bsize));
		NEW_VMCMD(vcset, vmcmd_map_zero, bsize, baddr, NULLVP, 0, *prot);
		*size = COFF_ROUND(sh->s_paddr, COFF_LDPGSZ);
	}
}

int
exec_pecoff_linker(elp)
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
	if ((error = pecoff_signature(elp->el_vnodep, pecoff_dos)) != 0)
		return error;

	if ((error = vn_marktext(elp->el_vnodep)) != 0)
		return error;

	peofs = pecoff_dos->d_peofs + sizeof(signature) - 1;
	coff_fp = malloc(PECOFF_HDR_SIZE, M_TEMP, M_WAITOK);
	error = exec_read_from(elp->el_vnodep, peofs, coff_fp, PECOFF_HDR_SIZE);
	if (error) {
		free(coff_fp, M_TEMP);
		return error;
	}
	error = exec_pecoff_coff_linker(elp, coff_fp, peofs);

	if (error != 0)
		kill_vmcmds(&elp->el_vmcmds);

	free(coff_fp, M_TEMP);
	return error;
}

int
exec_pecoff_coff_linker(elp, coff_fp, peofs)
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
		error = exec_pecoff_prep_omagic(elp, coff_aout, coff_fp, peofs);
		break;
	case COFF_NMAGIC:
		error = exec_pecoff_prep_nmagic(elp, coff_aout, coff_fp, peofs);
		break;
	case COFF_ZMAGIC:
		error = exec_pecoff_prep_zmagic(elp, coff_aout, coff_fp, peofs);
		break;
	default:
		return ENOEXEC;
	}

	return error;
}

int
exec_pecoff_prep_zmagic(elp, coff_aout, coff_fp, peofs)
	struct exec_linker *elp;
	struct coff_aouthdr *coff_aout;
    struct coff_filehdr *coff_fp;
	int peofs;
{
   	int error, i;
	struct pecoff_opthdr *pecoff_opt;
	long daddr, baddr, bsize;
	u_long tsize, dsize;
	struct coff_scnhdr *sh;
	struct pecoff_args *pecoff_arg;
	int scnsiz = sizeof(struct coff_scnhdr) * coff_fp->f_nscns;
	struct vmspace *vmspace = elp->el_proc->p_vmspace;

    pecoff_opt = (void *)((char *)pecoff_arg + sizeof(struct coff_aouthdr));

	elp->el_tsize = coff_aout->a_tsize;
	elp->el_daddr = VM_MAXUSER_ADDRESS;
	elp->el_dsize = 0;
	/* read section header */
	sh = malloc(scnsiz, M_TEMP, M_WAITOK);
	error = exec_read_from(elp->el_vnodep, peofs + PECOFF_HDR_SIZE, sh,
	    scnsiz);
	if (error) {
		free(sh, M_TEMP);
		return error;
	}
	/*
	 * map section
	 */
	for (i = 0; i < coff_fp->f_nscns; i++) {
		int prot = /*0*/VM_PROT_WRITE;
		long s_flags = sh[i].s_flags;

		if ((s_flags & COFF_STYP_DISCARD) != 0)
			continue;
		sh[i].s_vaddr += pecoff_opt->w_base; /* RVA --> VA */

		if ((s_flags & COFF_STYP_TEXT) != 0) {
			/* set up command for text segment */
/*			DPRINTF(("COFF text addr %lx size %ld offset %ld\n",
				 sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));
*/			pecoff_load_section(&elp->el_vmcmds, elp->el_vnodep,
					   &sh[i], (long *)&elp->el_taddr,
					   &tsize, &prot);
		} else if ((s_flags & COFF_STYP_BSS) != 0) {
			/* set up command for bss segment */
			baddr = sh[i].s_vaddr;
			bsize = sh[i].s_paddr;
			if (bsize)
				NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, bsize, baddr, NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
			elp->el_daddr = min(elp->el_daddr, baddr);
			bsize = baddr + bsize - elp->el_daddr;
			elp->el_dsize = max(elp->el_dsize, bsize);
		} else if ((s_flags & (COFF_STYP_DATA|COFF_STYP_READ)) != 0) {
			/* set up command for data segment */
/*			DPRINTF(("COFF data addr %lx size %ld offset %ld\n",
			sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));*/
			pecoff_load_section(&elp->el_vmcmds, elp->el_vnodep,
					   &sh[i], &daddr, &dsize, &prot);
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
	error = pecoff_load_file(elp, "/usr/libexec/ld.so.dll",
				&elp->el_vmcmds, &elp->el_entry, pecoff_arg);
	if (error) {
		free(sh, M_TEMP);
		return error;
	}

#if 0
	DPRINTF(("text addr: %lx size: %ld data addr: %lx size: %ld entry: %lx\n",
		 elp->el_taddr, elp->el_tsize,
		 elp->el_daddr, elp->el_dsize,
		 elp->el_entry));
#endif

	free(sh, M_TEMP);
	return (*elp->el_esch->ex_setup_stack)(elp);
}


int
exec_pecoff_prep_nmagic(elp, coff_aout, coff_fp, peofs)
	struct exec_linker *elp;
	struct coff_aouthdr *coff_aout;
    struct coff_filehdr *coff_fp;
	int peofs;
{
	return (ENOEXEC);
}

int
exec_pecoff_prep_omagic(elp, coff_aout, coff_fp, peofs)
	struct exec_linker *elp;
	struct coff_aouthdr *coff_aout;
    struct coff_filehdr *coff_fp;
	int peofs;
{
	return (ENOEXEC);
}
