

#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/fcntl.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysent.h>

#include <sys/mman.h>
#include <vm/include/vm.h>
#include <vm/include/vm_param.h>
#include <vm/include/vm_map.h>

#include <machine/cpu.h>
#include <machine/reg.h>

extern char sigcode[], esigcode[];
#ifdef SYSCALL_DEBUG
extern char *syscallnames[];
#endif

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

static int
elf_check_permissions(struct proc *p, struct vnode *vp)
{
	struct vattr attr;
	int error;

	if (vp->v_writecount) {
		return (ETXTBSY);
	}

	error = VOP_GETATTR(vp, &attr, p->p_ucred, p);
	if(error)
		return (error);

	if ((vp->v_mount->mnt_flag & MNT_NOEXEC) ||
	    ((attr.va_mode & 0111) == 0) ||
	    (attr.va_type != VREG)) {
		return (EACCES);
	}

	if (attr.va_size == 0)
		return (ENOEXEC);

	error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	if (error)
		return (error);

	error = VOP_OPEN(vp, FREAD, p->p_ucred, p);
	if (error)
		return (error);

	return (0);
}

void *
elf_copyargs(elp, arginfo, stack, argp)
	struct exec_linker *elp;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	size_t len;
	Aux32Info ai[ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;

	stack = copyargs(elp, arginfo, stack, argp);
	if (!stack)
		return NULL;

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)elp->ep_emul_arg)) {
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
		a->a_v = NBPG;
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

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(Aux32Info);
	if (copyout(ai, stack, len))
		return NULL;
	stack = (caddr_t)stack + len;

	return stack;
}

elf_check_header(const Elf32_Ehdr *hdr, int type)
{
	if (!(hdr->e_ident[EI_MAG0] == ELFMAG0 &&
	      hdr->e_ident[EI_MAG1] == ELFMAG1 &&
	      hdr->e_ident[EI_MAG2] == ELFMAG2 &&
	      hdr->e_ident[EI_MAG3] == ELFMAG3))
		return ENOEXEC;
	if (hdr->e_machine != EM_386 && hdr->e_machine != EM_486)
		return ENOEXEC;

	if (hdr->e_type != type)
		return ENOEXEC;

	return (0);
}


elf_load_psection(elp, vp, ph, addr, size, prot, flags)
	struct exec_linker *elp;
	struct vnode *vp;
	const Elf32_Phdr *ph;
	Elf32_Addr *addr;
	u_long *size;
	int *prot;
	int flags;
{
	u_long uaddr, msize, psize, rm, rf;
	long diff, offset;
	struct vmspace *vmspace = elp->el_proc->p_vmspace;

	/*
	 * If the user specified an address, then we load there.
	 */
	if (*addr != ELFDEFNNAME(NO_ADDR)) {
		if (ph->p_align > 1) {
			*addr = ELF_TRUNC(*addr, ph->p_align);
			uaddr = ELF_TRUNC(ph->p_vaddr, ph->p_align);
		} else
			uaddr = ph->p_vaddr;
		diff = ph->p_vaddr - uaddr;
	} else {
		*addr = uaddr = ph->p_vaddr;
		if (ph->p_align > 1)
			*addr = ELF_TRUNC(uaddr, ph->p_align);
		diff = uaddr - *addr;
	}

	*prot |= (ph->p_flags & PF_R) ? VM_PROT_READ : 0;
	*prot |= (ph->p_flags & PF_W) ? VM_PROT_WRITE : 0;
	*prot |= (ph->p_flags & PF_X) ? VM_PROT_EXECUTE : 0;

	offset = ph->p_offset - diff;
	*size = ph->p_filesz + diff;
	msize = ph->p_memsz + diff;
	psize = round_page(*size);

	if ((ph->p_flags & PF_W) != 0) {
		/*
		 * Because the pagedvn pager can't handle zero fill of the last
		 * data page if it's not page aligned we map the last page
		 * readvn.
		 */
		psize = trunc_page(*size);
	}
	if (psize > 0) {
		exec_mmap_setup(&vmspace->vm_map, *addr, psize, *prot, *prot, flags, (caddr_t)vp, offset);
	}
	if (psize < *size) {
		exec_mmap_setup(&vmspace->vm_map, *addr + psize, *size - psize, *prot, *prot,
				psize > 0 ? flags : flags, (caddr_t)vp, offset + psize);
	}

	/*
	 * Check if we need to extend the size of the segment
	 */
	rm = round_page(*addr + msize);
	rf = round_page(*addr + *size);

	if (rm != rf) {
		exec_mmap_setup(&vmspace->vm_map, rf, rm - rf, *prot, *prot, flags, (caddr_t)vp, 0);
		*size = msize;
	}
}

int
read_from(p, vp, off, buf, size)
	struct vnode *vp;
	u_long off;
	struct proc *p;
	caddr_t buf;
	int size;
{
	int error;
	size_t resid;

	if ((error = vn_rdwr(UIO_READ, vp, buf, size, off, UIO_SYSSPACE,
		    0, p->p_ucred, &resid, p)) != 0)
		return error;
	if (resid != 0)
		return ENOEXEC;
	return (0);
}

int
elf_load_file(p, elp, path, entry, ap, last)
	struct proc *p;
	struct exec_linker *elp;
	char *path;
	u_long *entry;
	struct elf_args	*ap;
	Elf32_Addr *last;
{
	int error, i;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	Elf32_Ehdr eh;
	Elf32_Phdr *ph = NULL;
	Elf32_Phdr *base_ph = NULL;
	u_long phsize;
	char *bp = NULL;
	Elf32_Addr addr = *last;

	bp = path;
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
		elp->el_attr->va_mode &= ~~(S_ISUID | S_ISGID);

	VOP_UNLOCK(vp, 0);

	if ((error = elf_read_from(p, vp, 0, (caddr_t) &eh, sizeof(eh))) != 0)
		goto bad;


	if ((error = elf_check_header(&eh, ET_DYN)) != 0)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf32_Phdr);
	ph = (Elf32_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = elf_read_from(p, vp, eh.e_phoff, (caddr_t) ph, phsize)) != 0)
		goto bad;

	/*
	 * Load all the necessary sections
	 */
	for (i = 0; i < eh.e_phnum; i++) {
		u_long size = 0;
		int prot = 0;
		int flags;

		switch (ph[i].p_type) {
		case PT_LOAD:
			if (base_ph == NULL) {
				addr = *last;
				flags = VMCMD_BASE;
			} else {
				addr = ph[i].p_vaddr - base_ph->p_vaddr;
				flags = VMCMD_RELATIVE;
			}
			elf_load_psection(elp, vp, &ph[i], &addr, &size, &prot, flags);
			/* If entry is within this section it must be text */
			if (eh.e_entry >= ph[i].p_vaddr && eh.e_entry < (ph[i].p_vaddr + size)) {
				/* XXX */
				*entry = addr + eh.e_entry;

				ap->arg_interp = addr;
			}
			if (base_ph == NULL)
				base_ph = &ph[i];
			addr += size;
			break;

		case PT_DYNAMIC:
		case PT_PHDR:
		case PT_NOTE:
			break;

		default:
			break;
		}
	}

	free((char *)ph, M_TEMP);
	*last = addr;
	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0);

bad:
	if (ph != NULL)
		free((char *)ph, M_TEMP);
#ifdef notyet /* XXX cgd 960926 */
	(maybe) VOP_CLOSE it
#endif
	vrele(vp);
	return error;
}

int
exec_elf_linker(elp)
	struct exec_linker *elp;
{
	Elf32_Ehdr *eh = elp->el_image_hdr;
	Elf32_Phdr *ph, *pp;
	Elf32_Addr phdr = 0, pos = 0;
	int error, i, n, nload;
	char interp[MAXPATHLEN];
	u_long phsize;

	if (elp->el_hdrvalid < sizeof(Elf32_Ehdr)) {
		return ENOEXEC;
	}


	/*
	 * XXX allow for executing shared objects. It seems silly
	 * but other ELF-based systems allow it as well.
	 */
	if(elf_check_header(eh, ET_EXEC) != 0 && elf_check_header(eh, ET_DYN) != 0)
		return ENOEXEC;

	return ENOEXEC;
}
