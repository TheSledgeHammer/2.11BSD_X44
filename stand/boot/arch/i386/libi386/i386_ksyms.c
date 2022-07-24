/*	$NetBSD: multiboot.c,v 1.26 2019/10/18 01:38:28 manu Exp $	*/

/*-
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ksyms.h>
#include <sys/cdefs_elf.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <lib/libsa/loadfile.h>

#include <machine/bootinfo.h>

int
i386_ksyms_addsyms_elf(struct bootinfo *bi)
{
	caddr_t symstart = (caddr_t) bi->bi_symstart;
	caddr_t strstart = (caddr_t) bi->bi_strstart;


	if (bi->bi_flags == BOOTINFO_ELF_SYMS) {
		Elf_Ehdr ehdr;

		KASSERT(esym != 0);

		memset(&ehdr, 0, sizeof(ehdr));
		memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
#ifdef ELFSIZE32
		ehdr.e_ident[EI_CLASS] = ELFCLASS32;
#endif
#ifdef ELFSIZE64
		ehdr.e_ident[EI_CLASS] = ELFCLASS64;
#endif
		ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
		ehdr.e_ident[EI_VERSION] = EV_CURRENT;
		ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
		ehdr.e_ident[EI_ABIVERSION] = 0;
		ehdr.e_type = ET_EXEC;
#ifdef __amd64__
		ehdr.e_machine = EM_X86_64;
#elif __i386__
		ehdr.e_machine = EM_386;
#else
#error "Unknwo ELF machine type"
#endif
		ehdr.e_version = 1;
		ehdr.e_ehsize = sizeof(ehdr);
		ehdr.e_entry = (Elf_Addr) & start;

		ksyms_addsyms_explicit((void*) &ehdr, (void*) symstart, bi->bi_symsize, (void*) strstart, bi->bi_strsize);
	}

	return (bi->bi_flags & BOOTINFO_ELF_SYMS);
}
