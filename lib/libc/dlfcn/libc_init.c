/* $NetBSD: crt0-common.c,v 1.27 2022/06/21 06:52:17 skrll Exp $ */

/*
 * Copyright (c) 1998 Christos Zoulas
 * Copyright (c) 1995 Christopher G. Demetriou
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#if defined(__x86_64__) || defined(__i386__)
#  define HAS_RELOCATE_SELF
#  if defined(__x86_64__)
#  define RELA
#  define REL_TAG DT_RELA
#  define RELSZ_TAG DT_RELASZ
#  define REL_TYPE Elf_Rela
#  else
#  define REL_TAG DT_REL
#  define RELSZ_TAG DT_RELSZ
#  define REL_TYPE Elf_Rel
#  endif

#include <elf.h>

#include "dlfcn_private.h"

struct ps_strings *__ps_strings;

static bool libc_initialised;

static void relocate_self(int, char **, int);

static void
relocate_self(int argc, char **argv, int envc)
{
	AuxInfo *aux;
	uintptr_t relocbase;
	const Elf_Phdr *phdr;
	Elf_Half phnum;

	aux = (AuxInfo *)(argv + argc + envc + 2);
	relocbase = (uintptr_t)~0U;
	phdr = NULL;
	phnum = (Elf_Half)~0;
	for (; aux->a_type != AT_NULL; ++aux) {
		switch (aux->a_type) {
		case AT_BASE:
			if (aux->a_v)
				return;
			break;
		case AT_PHDR:
			phdr = (void*) aux->a_v;
			break;
		case AT_PHNUM:
			phnum = (Elf_Half) aux->a_v;
			break;
		}
	}

	if (phdr == NULL || phnum == (Elf_Half) ~0)
		return;

	const Elf_Phdr *phlimit = phdr + phnum, *dynphdr = NULL;

	for (; phdr < phlimit; ++phdr) {
		if (phdr->p_type == PT_DYNAMIC)
			dynphdr = phdr;
		if (phdr->p_type == PT_PHDR)
			relocbase = (uintptr_t) phdr - phdr->p_vaddr;
	}
	if (dynphdr == NULL || relocbase == (uintptr_t) ~0U)
		return;

	Elf_Dyn *dynp = (Elf_Dyn*) ((uint8_t*) dynphdr->p_vaddr + relocbase);

	const REL_TYPE *relocs = 0, *relocslim;
	Elf_Addr relocssz = 0;

	for (; dynp->d_tag != DT_NULL; dynp++) {
		switch (dynp->d_tag) {
		case REL_TAG:
			relocs = (const REL_TYPE*) (relocbase + dynp->d_un.d_ptr);
			break;
		case RELSZ_TAG:
			relocssz = dynp->d_un.d_val;
			break;
		}
	}
	relocslim = (const REL_TYPE*) ((const uint8_t*) relocs + relocssz);
	for (; relocs < relocslim; ++relocs) {
		Elf_Addr * where;

		where = (Elf_Addr*) (relocbase + relocs->r_offset);

		switch (ELF_R_TYPE(relocs->r_info)) {
		case R_TYPE(RELATIVE): /* word64 B + A */
#ifdef RELA
			*where = (Elf_Addr)(relocbase + relocs->r_addend);
#else
			*where += (Elf_Addr) relocbase;
#endif
			break;
#ifdef IFUNC_RELOCATION
		case IFUNC_RELOCATION:
			break;
#endif
		default:
			abort();
		}
	}
}

void
libc_init(void *prog, int argc, char **argv, int envc)
{
	if (libc_initialised) {
		return;
	}

	libc_initialised = 1;

	if (prog != NULL) {
		dlauxinfo_init(argc, argv, envc);
	}
}

void
crt0_preinit(void *prog, int argc, char **argv, int envc)
{
#if defined(HAS_RELOCATE_SELF)
	relocate_self(argc, argv, envc);
#endif

	if (prog != NULL) {
		__ps_strings = prog;
	}
}
