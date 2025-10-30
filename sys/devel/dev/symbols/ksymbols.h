/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
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
 *
 */

#ifndef SYS_KSYMBOLS_H_
#define SYS_KSYMBOLS_H_

#include <sys/exec_aout.h>
#include <sys/exec_coff.h>
#include <sys/exec_ecoff.h>
#include <sys/exec_elf.h>
#include <sys/exec_xcoff.h>

enum ksymbol_types {
	KSYM_AOUT,
	KSYM_COFF,
	KSYM_ECOFF,
	KSYM_ELF32,
	KSYM_ELF64,
	KSYM_MACHO,
	KSYM_PE,
	KSYM_XCOFF32,
	KSYM_XCOFF64,
};

typedef struct ksymbol_header 	ksymbol_header_t;
typedef struct ksymbol 			ksymbol_t;

/*
 * Kernel Symbol Header
 */
struct ksymbol_header {
	union aout_hdr {
		struct exec si_aout;
	} u_aout;
	union coff_hdr {
		struct coff_filehdr si_fhdr;
		struct coff_aouthdr si_ahdr;
		struct coff_scnhdr si_shdr;
	} u_coff;
	union ecoff_hdr {
		struct ecoff_filehdr si_fhdr;
		struct ecoff_aouthdr si_ahdr;
		struct ecoff_scnhdr si_shdr;
	} u_ecoff;
	union elf_hdr {
		Elf_Ehdr si_ehdr;
		Elf_Phdr si_phdr[NPRGHDR];
		Elf_Shdr si_shdr[NSECHDR];
		char 	si_strtab[SHSTRSIZ];
		size_t	si_shdrsz;				/* ??? */
		int32_t si_note[6];
	} u_elf;
	union xcoff_hdr {
		xcoff_filehdr si_fhdr;
		xcoff_aouthdr si_ahdr;
		xcoff_scnhdr si_shdr;
	} u_xcoff;
#define si_aout 	u_aout.si_aout
#define si_coff_fhdr 	u_coff.si_fhdr
#define si_coff_ahdr 	u_coff.si_ahdr
#define si_coff_shdr 	u_coff.si_shdr
#define si_ecoff_fhdr 	u_ecoff.si_fhdr
#define si_ecoff_ahdr 	u_ecoff.si_ahdr
#define si_ecoff_shdr 	u_ecoff.si_shdr
#define si_elf_ehdr 	u_elf.si_ehdr
#define si_elf_phdr 	u_elf.si_phdr
#define si_elf_shdr 	u_elf.si_shdr
#define si_elf_strtab 	u_elf.si_strtab
#define si_elf_shdrsz 	u_elf.si_shdrsz
#define si_elf_note 	u_elf.si_note
#define si_xcoff_fhdr 	u_xcoff.si_fhdr
#define si_xcoff_ahdr 	u_xcoff.si_ahdr
#define si_xcoff_shdr 	u_xcoff.si_shdr
};

/*
 * Kernel Symbol
 */
struct ksymbol {
	struct exec 		k_aout;
	struct coff_symtab 	k_coff;
	struct ecoff_symhdr	k_ecoff;
	Elf_Sym				k_elf;
	xcoff_syms			k_xcoff;
};
#endif /* SYS_KSYMBOLS_H_ */
