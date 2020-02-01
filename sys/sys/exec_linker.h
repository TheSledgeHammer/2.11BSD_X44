/*
 * exec_linker.h
 *
 *  Created on: 28 Nov 2019
 *      Author: marti
 */

#ifndef _SYS_EXEC_LINKER_H_
#define _SYS_EXEC_LINKER_H_

#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <vm/include/vm.h>

struct exec_linker {
	struct 	proc 		*el_proc;				/* our process struct */
	struct 	execa 		*el_uap; 				/* syscall arguments */

	struct	vnode 		*el_vnodep;				/* executable's vnode */
	struct	vattr 		*el_attr;				/* executable's attributes */
	struct 	exec_mmap 	*el_ex_map;				/* executable's mmap to vmspace's vm_mmap */

	void				*el_image_hdr;			/* file's exec header */
	u_int				el_hdrlen;				/* length of ep_hdr */
	u_int				el_hdrvalid;			/* bytes of ep_hdr that are valid */

	segsz_t				el_tsize;				/* size of process's text */
	segsz_t				el_dsize;				/* size of process's data(+bss) */
	segsz_t				el_ssize;				/* size of process's stack */
	caddr_t				el_taddr;				/* process's text address */
	caddr_t				el_daddr;				/* process's data(+bss) address */
	caddr_t				el_maxsaddr;			/* proc's max stack addr ("top") */
	caddr_t				el_minsaddr;			/* proc's min stack addr ("bottom") */
	caddr_t 			el_entry;				/* process's entry point */
	caddr_t				el_entryoffset;			/* offset to entry point */
	caddr_t				el_vm_minaddr;			/* bottom of process address space */
	caddr_t				el_vm_maxaddr;			/* top of process address space */
	u_int				el_flags;				/* flags */

	char 				*el_stringbase;			/* base address of tmp string storage */
	char 				*el_stringp;			/* current 'end' pointer of tmp strings */
	int 				el_stringspace;			/* space left in tmp string storage area */
	int 				el_argc, el_envc;		/* count of argument and environment strings */

	char 				el_vmspace_destroyed;	/* flag - we've blown away original vm space */
	char 				el_interpreted;			/* flag - this executable is interpreted */
	char 				el_interpreter_name[64];/* name of the interpreter */
};

struct exec_mmap {
    vm_offset_t         *em_addr;
    vm_size_t           em_size;
	vm_prot_t 			em_prot;
	vm_prot_t 			em_maxprot;
    int                 em_flags;
    caddr_t             em_handle;
    unsigned long 		em_offset;
};

#include <sys/exec_aout.h>

//#ifdef KERNEL
int		exec_setup_stack(struct exec_linker *);
void 	exec_mmap_setup(struct exec_linker *elp, vm_offset_t *addr, vm_size_t size, vm_prot_t prot, vm_prot_t maxprot, int flags, caddr_t handle,  unsigned long offset);
int 	exec_mmap_to_vmspace(struct exec_linker *);
int		exec_extract_strings (struct exec_linker *);
int		exec_new_vmspace(struct exec_linker *);
#endif

#endif /* _SYS_EXEC_LINKER_H_ */
