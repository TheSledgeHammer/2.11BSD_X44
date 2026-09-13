/*	$NetBSD: exec.c,v 1.76 2020/04/04 19:50:54 christos Exp $	 */

/*
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * 	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
 * 	Perry E. Metzger.  All rights reserved.
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
 * 	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Starts a NetBSD ELF kernel. The low level startup is done in startprog.S.
 * This is a special version of exec.c to support use of XMS.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <lib/libsa/loadfile.h>

#include <bootstrap.h>

#if defined(BOOT_ELF32) || defined(BOOT_ELF64)

void
ksyms_addr_set(void *ehdr, void *shdr, void *symbase)
{
	int class;
	Elf32_Ehdr *ehdr32 = NULL;
	Elf64_Ehdr *ehdr64 = NULL;
	uint64_t shnum;
	int i;

	class = ((Elf_Ehdr *)ehdr)->e_ident[EI_CLASS];

	switch (class) {
	case ELFCLASS32:
		ehdr32 = (Elf32_Ehdr *)ehdr;
		shnum = ehdr32->e_shnum;
		break;
	case ELFCLASS64:
		ehdr64 = (Elf64_Ehdr *)ehdr;
		shnum = ehdr64->e_shnum;
		break;
	default:
		panic("Unexpected ELF class");
		break;
	}

	for (i = 0; i < shnum; i++) {
		Elf64_Shdr *shdrp64 = NULL;
		Elf32_Shdr *shdrp32 = NULL;
		uint64_t shtype, shaddr, shsize, shoffset;

		switch (class) {
		case ELFCLASS64:
			shdrp64 = &((Elf64_Shdr *)shdr)[i];
			shtype = shdrp64->sh_type;
			shaddr = shdrp64->sh_addr;
			shsize = shdrp64->sh_size;
			shoffset = shdrp64->sh_offset;
			break;
		case ELFCLASS32:
			shdrp32 = &((Elf32_Shdr *)shdr)[i];
			shtype = shdrp32->sh_type;
			shaddr = shdrp32->sh_addr;
			shsize = shdrp32->sh_size;
			shoffset = shdrp32->sh_offset;
			break;
		default:
			panic("Unexpected ELF class");
			break;
		}

		if (shtype != SHT_SYMTAB && shtype != SHT_STRTAB)
			continue;

		if (shaddr != 0 || shsize == 0)
			continue;

		shaddr = (uint64_t)(uintptr_t)(symbase + shoffset);

		switch (class) {
		case ELFCLASS64:
			shdrp64->sh_addr = shaddr;
			break;
		case ELFCLASS32:
			shdrp32->sh_addr = shaddr;
			break;
		default:
			panic("Unexpected ELF class");
			break;
		}
	}
	return;
}

#endif
