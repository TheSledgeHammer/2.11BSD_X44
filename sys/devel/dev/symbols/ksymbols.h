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

#include <sys/queue.h>

#include <sys/exec_aout.h>
#include <sys/exec_coff.h>
#include <sys/exec_ecoff.h>
#include <sys/exec_elf.h>
#include <sys/exec_xcoff.h>

/* Kernel Symbol Format Name */
#define KSYM_NAME_AOUT    "aout"
#define KSYM_NAME_COFF    "coff"
#define KSYM_NAME_ECOFF   "ecoff"
#define KSYM_NAME_ELF32   "elf32"
#define KSYM_NAME_ELF64   "elf64"
#define KSYM_NAME_MACHO   "macho"
#define KSYM_NAME_PE      "pe"
#define KSYM_NAME_XCOFF32 "xcoff32"
#define KSYM_NAME_XCOFF64 "xcoff64"

/* Kernel Symbol Format Type */
enum ksymbol_types {
	KSYM_TYPE_AOUT,
	KSYM_TYPE_COFF,
	KSYM_TYPE_ECOFF,
	KSYM_TYPE_ELF32,
	KSYM_TYPE_ELF64,
	KSYM_TYPE_MACHO,
	KSYM_TYPE_PE,
	KSYM_TYPE_XCOFF32,
	KSYM_TYPE_XCOFF64,
	KSYM_TYPE_MAX
};

/*
 * Kernel Symbol Type Map
 */
struct ksymbol_map {
	const char *km_name;
	int km_type;
} ksymbol_map[] = {
		{		/* AOUT */
				.km_name = KSYM_NAME_AOUT,
				.km_type = KSYM_TYPE_AOUT,
		},
		{		/* COFF */
				.km_name = KSYM_NAME_COFF,
				.km_type = KSYM_TYPE_COFF,
		},
		{		/* ECOFF */
				.km_name = KSYM_NAME_ECOFF,
				.km_type = KSYM_TYPE_ECOFF,
		},
		{		/* ELF32 */
				.km_name = KSYM_NAME_ELF32,
				.km_type = KSYM_TYPE_ELF32,
		},
		{		/* ELF64 */
				.km_name = KSYM_NAME_ELF64,
				.km_type = KSYM_TYPE_ELF64,
		},
		{		/* MACHO */
				.km_name = KSYM_NAME_MACHO,
				.km_type = KSYM_TYPE_MACHO,
		},
		{		/* PE */
				.km_name = KSYM_NAME_PE,
				.km_type = KSYM_TYPE_PE,
		},
		{		/* XCOFF32 */
				.km_name = KSYM_NAME_XCOFF32,
				.km_type = KSYM_TYPE_XCOFF32,
		},
		{		/* XCOFF64 */
				.km_name = KSYM_NAME_XCOFF64,
				.km_type = KSYM_TYPE_XCOFF64,
		},
};

typedef struct ksymbol_map 		ksymbol_map_t;
typedef struct ksymbol 			ksymbol_t;
typedef struct ksymbol_common 	ksymbol_common_t;
typedef struct ksymbol_file 	ksymbol_file_t;
typedef struct ksymbol_header 	ksymbol_header_t;

/*
 * Kernel Symbol
 */
struct ksymbol {
	struct exec k_aout;
	struct coff_symtab k_coff;
	struct ecoff_symhdr k_ecoff;
	Elf_Sym	k_elf;
	xcoff_syms k_xcoff;
};

/* Common Symbol Format */
struct ksymbol_common {
	const char  *si_name;   /* Format Name */
	int        	si_type;    /* Format Type (i.e. aout, ecoff, elf, etc) */
    uint64_t    si_value;   /* Format value (i.e address) */
    uint64_t    si_size;    /* Format size */
    LIST_ENTRY(ksymbol_common) si_link;
};

/*
 * Table: (Current Active Symbol Format Used)
 * - A modified version of the current struct ksyms_symtab in sys/ksyms.h
 * - accesses sub-table for needed symbol information and ops.
 *
 * Sub-Table: (Per Symbol Format (i.e. aout, elf, etc))
 * - ops: add, delete, compare, sort, pack
 * - uses a common symbol format to access basic info
 */

/*
 * Static allocated ELF header.
 * Basic info is filled in at attach, sizes at open.
 */
#define	SHNOTE		1
#define	SYMTAB		2
#define	STRTAB		3
#define	SHSTRTAB	4
#define	SHBSS		5
#define	SHCTF		6
#define	NSECHDR		7

#define	NPRGHDR		1
#define	SHSTRSIZ	64

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
		char si_strtab[SHSTRSIZ];
		int32_t si_note[6];
	} u_elf;
	union xcoff_hdr {
		xcoff_filehdr si_fhdr;
		xcoff_aouthdr si_ahdr;
		xcoff_scnhdr si_shdr;
	} u_xcoff;
#define si_aout 		u_aout.si_aout
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
#define si_elf_note 	u_elf.si_note
#define si_xcoff_fhdr 	u_xcoff.si_fhdr
#define si_xcoff_ahdr 	u_xcoff.si_ahdr
#define si_xcoff_shdr 	u_xcoff.si_shdr
};

#endif /* SYS_KSYMBOLS_H_ */
