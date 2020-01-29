/*	$NetBSD: exec_elf_common.c,v 1.8 1998/10/03 20:39:33 christos Exp $	*/

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/fcntl.h>
#include <sys/resourcevar.h>

#include <sys/mman.h>
#include <vm/include/vm_param.h>
#include <vm/include/vm_map.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <vm/include/vm.h>

/*
 * exec_elf_setup_stack(): Set up the stack segment for an a.out
 * executable.
 *
 * Note that the ep_ssize parameter must be set to be the current stack
 * limit; this is adjusted in the body of execve() to yield the
 * appropriate stack segment usage once the argument length is
 * calculated.
 *
 * This function returns an int for uniformity with other (future) formats'
 * stack setup functions.  They might have errors to return.
 */

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
