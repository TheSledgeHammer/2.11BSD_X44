/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 */

/* NOTE: The Following Contents of exec_xcoff_opts.h is derived from IBM Knowledge Center:
 * https://www.ibm.com/support/knowledgecenter/ssw_aix_72/filesreference/XCOFF.html
 */

#ifndef _SYS_EXEC_XCOFF_OPTS_H_
#define _SYS_EXEC_XCOFF_OPTS_H_

/* coff data types */
typedef int8_t			coff_char;
typedef int16_t			coff_short;
typedef int32_t			coff_int;
typedef int64_t			coff_long;
typedef uint8_t			coff_uchar;
typedef uint16_t		coff_ushort;
typedef uint32_t		coff_uint;
typedef uint64_t		coff_ulong;

/* xcoff: loader header */
struct xcoff_ldrhdr {
	int32_t 	l_version;		/* loader section version number */
	int32_t		l_nsyms;		/* # of symbol table entries */
	int32_t		l_nreloc;		/* # of relocation table entries */
	int32_t		l_istlen;		/* length of import file ID string table */
	int32_t		l_nimpid;		/* # of import file IDs */
	int32_t		l_impoff;		/* offset to start of import file IDs */
	int32_t		l_stlen;		/* length of string table */
	int32_t 	l_stoff;		/* offset to start of string table */
	int64_t		l_symoff;		/* offset to start of symbol table */
	int16_t 	l_rldoff;		/* offset to start of relocation entries */
};

/* xcoff: loader symbol table */
struct xcoff_ldrsyms {
	char		l_name[8];		/* symbol name or byte offset into string table */
	int32_t		l_zeroes;		/* zero indicates symbol name is referenced from l_offset */
	int32_t		l_offset;		/* byte offset into string table of symbol name */
	int32_t 	l_value;		/* address field */
	int16_t 	l_scnum;		/* section number containing symbol */
	int8_t		l_smtype;		/* symbol type, export, import flags */
	int8_t 		l_smclas;		/* symbol storage class */
	int32_t 	l_ifile;		/* import file ID; ordinal of import file IDs */
	int32_t 	l_parm;			/* parameter type-check field */
};

/* xcoff: loader relocation table */
struct xcoff_ldrreloc {
	int32_t		l_vaddr;		/* address field */
	int32_t 	l_symndx;		/* loader section symbol table index of referenced item */
	int32_t 	l_rtype;		/* relocation type */
	int16_t 	l_value;		/* address field */
	int16_t 	l_rsecnm;		/* file section number being relocated */
};

/* xcoff: subsequent/entry exceptions */
struct xcoff_excpt {
	typedef struct {
		int32_t	e_symndx;		/* symbol table index for function */
	} e_addr;
	int8_t		e_lang;			/* compiler language ID code */
	int8_t		e_reason;		/* value 0 (exception reason code 0) */
};

struct xcoff_sub_excpt {
	typedef struct {
		int32_t	e_paddr;		/* address of the trap instruction */
	} e_addr;
	int8_t		e_lang;			/* compiler language ID code */
	int8_t		e_reason;		/* trap exception reason code */
};

/* e_lang codes */
#define XCOFF_E_LANG_ID_C			0x00
#define XCOFF_E_LANG_ID_FORTRAN		0x01
#define XCOFF_E_LANG_ID_PASCAL		0x02
#define XCOFF_E_LANG_ID_ADA			0x03
#define XCOFF_E_LANG_ID_PL/I		0x04
#define XCOFF_E_LANG_ID_BASIC		0x05
#define XCOFF_E_LANG_ID_LISP		0x06
#define XCOFF_E_LANG_ID_COBOL		0x07
#define XCOFF_E_LANG_ID_MODULA2		0x08
#define XCOFF_E_LANG_ID_C++			0x09
#define XCOFF_E_LANG_ID_RPG			0x0A
#define XCOFF_E_LANG_ID_PL8/PLIX	0x0B
#define XCOFF_E_LANG_ID_ASSEMBLY	0x0C

/* xcoff: relocation entry */
struct xcoff_reloc_entry {
	int32_t		r_vaddr;		/* virtual address (position) in section to be relocated */
	int32_t		r_symndx;		/* symbol table index of item that is referenced */
	int8_t		r_rsize;		/* relocation size and information */
	int8_t		r_rtype;		/* relocation type */
};

/* r_rtype fields */
#define XCOFF_RTYPE_POS				0x00
#define XCOFF_RTYPE_NEG				0x01
#define XCOFF_RTYPE_REL				0x02
#define XCOFF_RTYPE_TOC				0x03
#define XCOFF_RTYPE_TRL				0x04
#define XCOFF_RTYPE_TRLA			0x13
#define XCOFF_RTYPE_GL				0x05
#define XCOFF_RTYPE_TCL				0x06
#define XCOFF_RTYPE_RL				0x0C
#define XCOFF_RTYPE_RLA				0x0D
#define XCOFF_RTYPE_REF				0x0F
#define XCOFF_RTYPE_BA				0x08
#define XCOFF_RTYPE_RBA				0x18
#define XCOFF_RTYPE_BR				0x0A
#define XCOFF_RTYPE_RBR				0x1A
#define XCOFF_RTYPE_TLS				0x20
#define XCOFF_RTYPE_TLS_IE			0x21
#define XCOFF_RTYPE_TLS_LD			0x22
#define XCOFF_RTYPE_TLS_LE			0x23
#define XCOFF_RTYPE_TLSM			0x24
#define XCOFF_RTYPE_TLSML			0x25
#define XCOFF_RTYPE_TOCU			0x30
#define XCOFF_RTYPE_TOCL			0x31

/* xcoff: symbol table entries */
struct xcoff_syms {
	char		n_name[8];		/* symbol name (occupies the same 8 bytes as n_zeroes and n_offset) */
	int32_t 	n_zeroes;		/* zero, indicating name in string table or .debug section (overlays first 4 bytes of n_name) */
	int32_t 	n_offset;		/* offset of the name in string table or .debug section (In XCOFF32: overlays last 4 bytes of n_name) */
	int32_t 	n_value;		/* symbol value; storage class-dependent */
	int16_t 	n_scnum;		/* section number of symbol */
	int16_t 	n_type;			/* basic and derived type specification */
	int8_t 		n_lang;			/* source language ID (overlays first byte of n_type) */
	int8_t 		n_cpu;			/* CPU Type ID (overlays second byte of n_type) */
	int8_t 		n_sclass;		/* storage class of symbol */
	int8_t 		n_numaux;		/* number of auxiliary entries */
};

/* n_scnum */
#define XCOFF_N_DEBUG 				-2
#define XCOFF_N_ABS 				-1
#define XCOFF_N_UNDEF 				0

/* n_value: */
/* relocatable */
enum xcoff_n_rela {
	C_EXT,
	C_WEAKEST,
	C_HIDEXT,
	C_FCN,
	C_BLOCK,
	C_STAT
};
/* zero */
enum xcoff_n_zero {
	C_GSYM,
	C_BCOMM,
	C_DECL,
	C_ENTRY,
	C_ESTAT,
	C_ECOMM
};
/* csect */
enum xcoff_n_csect {
	C_FUN,
	C_STSYM
};
/* file */
enum xcoff_n_file {
	C_BINCL,
	C_EINCL
};
/* comments */
enum xcoff_n_comm {
	C_INFO
};
/* symbol table index */
enum xcoff_n_symidx {
	C_FILE,
	C_BSTAT
};
/* stack frame */
enum xcoff_n_stack {
	C_LSYM,
	C_PSYM
};
/* register */
enum xcoff_n_reg {
	C_RPSYM,
	C_RSYM
};
/* common block */
enum xcoff_n_cblock {
	C_ECOML
};
/* dwarf section */
enum xcoff_n_dwarf {
	C_DWARF
};

#endif /* _SYS_EXEC_XCOFF_OPTS_H_ */
