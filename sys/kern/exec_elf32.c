

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/sysent.h>

#include <vm/vm.h>

#ifndef ELFSIZE
#define	ELFSIZE		32
#endif


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

int
exec_elf_linker(elp)
	struct exec_linker *elp;
{
	return ENOEXEC;
}

read_from()
{
	exec_new_vmspace();
}

elf_load_file()
{

}

elf_load_section(elp, elf, vp)
	struct exec_linker *elp;
	struct elf_args *elf;
	struct vnode *vp;
{
		ELFDEFNNAME(NO_ADDR);
	exec_mmap_setup();
}

int
exec_elf_setup_stack(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;

	elp->el_maxsaddr = USRSTACK - MAXSSIZ;
	elp->el_minsaddr = USRSTACK;
	elp->el_ssize = u->u_rlimit[RLIMIT_STACK].rlim_cur;

	exec_mmap_setup(&vmspace->vm_map,  elp->el_maxsaddr, ((elp->el_minsaddr - elp->el_ssize) - elp->el_maxsaddr),
			VM_PROT_NONE, VM_PROT_NONE, MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);

	exec_mmap_setup(&vmspace->vm_map,  elp->el_ssize, (elp->el_minsaddr - elp->el_ssize),
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);

	return (0);
}

const struct execsw elf_execsw = { exec_elf_linker, "ELF" };
TEXT_SET(execsw_set, elf_execsw);
