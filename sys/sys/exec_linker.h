/*	$NetBSD: exec.h,v 1.68.4.4 2001/02/03 20:00:02 he Exp $	*/

/*-
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1993 Theo de Raadt
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)exec.h	8.4 (Berkeley) 2/19/95
 */
#ifndef _SYS_EXEC_LINKER_H_
#define _SYS_EXEC_LINKER_H_

#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <vm/include/vm.h>

struct exec_linker {
	const char 					*el_name;				/* file's name */
	struct 	proc 		        *el_proc;			    /* our process struct */
	struct 	execa 		        *el_uap; 			    /* syscall arguments */
	const struct execsw 		*el_esch;				/* execsw entry */

	struct	vnode 		        *el_vnodep;			    /* executable's vnode */
	struct	vattr 		        *el_attr;			    /* executable's attributes */
	struct 	exec_vmcmd_set    	*el_vmcmds;			    /* executable's vmcmd_set */

	struct 	nameidata			el_ndp;					/* executable's nameidata */

	struct  emul 				*el_emul;				/* os emulation */
	void						*el_emul_arg;			/* emulation argument */

	void				        *el_image_hdr;			/* file's exec header */
	u_int				        el_hdrlen;				/* length of ep_hdr */
	u_int				        el_hdrvalid;			/* bytes of ep_hdr that are valid */

	segsz_t				        el_tsize;				/* size of process's text */
	segsz_t				        el_dsize;				/* size of process's data(+bss) */
	segsz_t				        el_ssize;				/* size of process's stack */
	caddr_t				        el_taddr;				/* process's text address */
	caddr_t				        el_daddr;				/* process's data(+bss) address */
	caddr_t				        el_maxsaddr;			/* proc's max stack addr ("top") */
	caddr_t				        el_minsaddr;			/* proc's min stack addr ("bottom") */
	caddr_t 			        el_entry;				/* process's entry point */
	caddr_t				        el_entryoffset;			/* offset to entry point */
	caddr_t			        	el_vm_minaddr;			/* bottom of process address space */
	caddr_t				        el_vm_maxaddr;			/* top of process address space */
	u_int				        el_flags;				/* flags */
	int							el_fd;					/* a file descriptor we're holding */
	char						**el_fa;				/* a fake args vector for scripts */

	char 				        *el_stringbase;			/* base address of tmp string storage */
	char 				        *el_stringp;			/* current 'end' pointer of tmp strings */
	int 				        el_stringspace;			/* space left in tmp string storage area */
	int 				        el_argc, el_envc;		/* count of argument and environment strings */
};

/* exec vmspace-creation command set; see below */
struct exec_vmcmd_set {
    u_int				    evs_cnt;
	u_int			        evs_used;
	struct exec_vmcmd     	*evs_cmds;
};

#define	EXEC_DEFAULT_VMCMD_SETSIZE	9	/* # of cmds in set to start */

#define	EXEC_INDIR		0x0001		/* script handling already done */
#define	EXEC_HASFD		0x0002		/* holding a shell script */
#define	EXEC_HASARGL	0x0004		/* has fake args vector */
#define	EXEC_SKIPARG	0x0008		/* don't copy user-supplied argv[0] */
#define	EXEC_DESTR		0x0010		/* destructive ops performed */
#define	EXEC_32			0x0020		/* 32-bit binary emulation */
#define	EXEC_HASES		0x0040		/* don't update exec switch pointer */

struct exec_vmcmd {
	int					(*ev_proc)(struct proc *, struct exec_vmcmd *);
    vm_offset_t         *ev_addr;
    vm_size_t           ev_size;
	vm_prot_t 			ev_prot;
	vm_prot_t 			ev_maxprot;
    int                 ev_flags;
    caddr_t             ev_handle;
    unsigned long 		ev_offset;

#define	VMCMD_RELATIVE	0x0001	/* ev_addr is relative to base entry */
#define	VMCMD_BASE		0x0002	/* marks a base entry */
#define	VMCMD_FIXED		0x0004	/* entry must be mapped at ev_addr */
#define	VMCMD_STACK		0x0008	/* entry is for a stack */
};

/* list of supported emulations */
static
LIST_HEAD(emlist_head, emul_entry) el_head;
struct emul_entry {
	LIST_ENTRY(emul_entry)	el_list;
	const struct emul		*el_emul;
	int						ro_entry;
};

/* list of dynamically loaded execsw entries */
static
LIST_HEAD(execlist_head, exec_entry) ex_head;
struct exec_entry {
	LIST_ENTRY(exec_entry)	ex_list;
	const struct execsw		*ex;
};

/* structure used for building execw[] */
struct execsw_entry {
	struct execsw_entry	*next;
	const struct execsw	*ex;
};

extern struct lock 	exec_lock;
u_int				exec_maxhdrsz;

#ifdef KERNEL

#ifdef DEBUG
void 	kill_vmcmd		  	 __P((struct exec_vmcmd **));
void 	vmcmdset_extend 	 __P((struct exec_vmcmd_set *));
void 	kill_vmcmds	 	 	 __P((struct exec_vmcmd_set *));

int 	vmcmd_map_pagedvn 	 __P((struct exec_linker *));
int 	vmcmd_map_readvn 	 __P((struct exec_linker *));
int 	vmcmd_readvn 		 __P((struct exec_linker *));
int		vmcmd_map_zero 		 __P((struct exec_linker *));
int 	vmcmd_create_vmspace __P((struct exec_linker *));

int 	exec_extract_strings  __P((struct exec_linker *, char *, char * const *));
int 	*exec_copyout_strings __P((struct exec_linker *, struct ps_strings *));
void 	*copyargs			 __P((struct exec_linker *, struct ps_strings *, void *, void *));

void 	setregs				 __P((struct proc *, struct exec_linker *, u_long));

int		check_exec			 __P((struct exec_linker *));

int 	exec_setup_stack 	 __P((struct exec_linker *));

void 	new_vmcmd		 	 __P((struct exec_vmcmd_set *evsp, struct exec_linker *elp, vm_offset_t *addr, vm_size_t size, vm_prot_t prot, vm_prot_t maxprot, int flags, caddr_t handle,  unsigned long offset));

#define	NEW_VMCMD(evsp,elp,addr,size,prot,maxprot,handle,offset) \
	new_vmcmd(evsp,elp,addr,size,prot,maxprot,0,handle,offset)
#define	NEW_VMCMD2(evsp,elp,addr,size,prot,maxprot,flags,handle,offset) \
	new_vmcmd(evsp,elp,addr,size,prot,maxprot,flags,handle,offset)
#else	/* DEBUG */
#define	NEW_VMCMD(evsp,elp,addr,size,prot,maxprot,handle,offset) \
		NEW_VMCMD2(evsp,elp,addr,size,prot,maxprot,0,handle,offset)
#define	NEW_VMCMD2(evsp,elp,addr,size,prot,maxprot,flags,handle,offset) do { \
	struct exec_vmcmd *vcp; \
	if ((evsp)->evs_used >= (evsp)->evs_cnt) \
		vmcmdset_extend(evsp); \
	vcp = &(evsp)->evs_cmds[(evsp)->evs_used++]; \
	vcp->ev_proc = (elp->el_proc); \
	vcp->ev_addr = (addr); \
	vcp->ev_size = (size); \
	vcp->ev_prot = (prot); \
    vcp->ev_maxprot = (maxprot); \
	vcp->ev_flags = (flags); \
	if((vcp->ev_handle = (handle)) != NULLVP) \
			VREF(handle); \
    vcp->ev_offset = (offset); \
} while (0)

#endif /* EXEC_DEBUG */
extern int	exec_maxhdrsz;
#endif 	/* _KERNEL */
#include <sys/exec_aout.h>
#endif  /* _SYS_EXEC_LINKER_H_ */
