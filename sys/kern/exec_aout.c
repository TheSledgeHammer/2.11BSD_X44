/*	$NetBSD: exec_aout.c,v 1.17.2.1 2002/09/04 03:43:55 itojun Exp $	*/

/*
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



#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_aout.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/sysent.h>

#include <vm/include/vm.h>

int
exec_aout_linker(elp)
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec *) elp->el_image_hdr;
	int error;

	if (elp->el_hdrvalid < sizeof(struct exec)) {
		return ENOEXEC;
	}

	/*
	 * Set file/virtual offset based on a.out variant.
	 *	We do two cases: host byte order and network byte order
	 *	(for NetBSD compatibility)
	 */
	switch ((int)(a_out->a_magic & 0xffff)) {
	case ZMAGIC:
		error = exec_aout_prep_zmagic(elp);
		break;
	case NMAGIC:
		error = exec_aout_prep_nmagic(elp);
		break;
	case OMAGIC:
		error = exec_aout_prep_omagic(elp);
		break;
	default:
		/* NetBSD compatibility */
		switch ((int)(ntohl(a_out->a_magic) & 0xffff)) {
		case ZMAGIC:
			error = exec_aout_prep_zmagic(elp);
			break;
		case NMAGIC:
			error = exec_aout_prep_nmagic(elp);
			break;
		case OMAGIC:
			error = exec_aout_prep_omagic(elp);
			break;
		default:
			error = cpu_exec_aout_linker(elp); /* For CPU Architecture */
		}
	}
	return exec_mmap_to_vmspace(elp, a_out->a_entry);
}

/*
 * exec_aout_prep_zmagic(): Prepare a 'native' ZMAGIC binary's exec linker
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up the
 * text, data, bss, and stack segments.
 */
int
exec_aout_prep_zmagic(elp)
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec *) elp->el_image_hdr;
	struct vmspace *vmspace = elp->el_proc->p_vmspace;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;

	/* set up for text */
	exec_mmap_setup(&vmspace->vm_map, elp->el_taddr, round_page(a_out->a_text),
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);

	/* set up for data segment */
	exec_mmap_setup(&vmspace->vm_map, elp->el_daddr, round_page(a_out->a_data),
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, a_out->a_text);

	/* set up for bss segment */
	if (a_out->a_bss > 0) {
		exec_mmap_setup(&vmspace->vm_map, elp->el_daddr + a_out->a_data, a_out->a_bss,
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);
	}

	return (*elp->el_esch->ex_setup_stack)(elp);
}


/* exec_aout_prep_nmagic(): Prepare a 'native' NMAGIC binary's exec linker */
int
exec_aout_prep_nmagic(elp)
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec *) elp->el_image_hdr;
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	long bsize, baddr;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = roundup(elp->el_taddr + a_out->a_text, __LDPGSZ);
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;

	/* set up for text */
	exec_mmap_setup(&vmspace->vm_map, elp->el_taddr, a_out->a_text,
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, sizeof(struct exec));

	/* set up for data segment */
	exec_mmap_setup(&vmspace->vm_map, elp->el_daddr, a_out->a_data,
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, a_out->a_text + sizeof(struct exec));

	/* set up for bss segment */
	baddr = roundup(elp->el_daddr + a_out->a_data, NBPG);
	bsize = elp->el_daddr + elp->el_dsize - baddr;
	if (bsize > 0) {
		exec_mmap_setup(&vmspace->vm_map, baddr, bsize
				VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
				MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);
	}

	return (*elp->el_esch->ex_setup_stack)(elp);
}

/* exec_aout_prep_omagic(): Prepare a 'native' OMAGIC binary's exec linker */
int
exec_aout_prep_omagic(elp)
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec *) elp->el_image_hdr;
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	long dsize, bsize, baddr;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;

	/* set up for text and data segment */
	exec_mmap_setup(&vmspace->vm_map, elp->el_taddr, a_out->a_text + a_out->a_data,
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, sizeof(struct exec));

	/* set up for bss segment */
	baddr = roundup(elp->el_daddr + a_out->a_data, NBPG);
	bsize = elp->el_daddr + elp->el_dsize - baddr;
	if (bsize > 0) {
		exec_mmap_setup(&vmspace->vm_map, baddr, bsize, VM_PROT_READ | VM_PROT_EXECUTE,
				VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE, MAP_PRIVATE | MAP_FIXED,
				(caddr_t)elp->el_vnodep, 0);
	}

	/*
	 * Make sure (# of pages) mapped above equals (vm_tsize + vm_dsize);
	 * obreak(2) relies on this fact. Both `vm_tsize' and `vm_dsize' are
	 * computed (in execve(2)) by rounding *up* `ep_tsize' and `ep_dsize'
	 * respectively to page boundaries.
	 * Compensate `ep_dsize' for the amount of data covered by the last
	 * text page.
	 */
	dsize = elp->el_dsize + a_out->a_text - roundup(a_out->a_text, NBPG);
	elp->el_dsize = (dsize > 0) ? dsize : 0;

	return (*elp->el_esch->ex_setup_stack)(elp);
}

static const struct execsw aout_execsw = { exec_aout_linker, "a.out" };
TEXT_SET(execsw_set, aout_execsw);
