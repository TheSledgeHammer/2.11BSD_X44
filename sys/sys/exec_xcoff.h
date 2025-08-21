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

/*
 * NOTE: The Following Contents of exec_xcoff.h is derived from IBM Knowledge Center:
 * https://www.ibm.com/support/knowledgecenter/ssw_aix_72/filesreference/XCOFF.html
 */

#ifndef _SYS_EXEC_XCOFF_H_
#define _SYS_EXEC_XCOFF_H_

#include <machine/types.h>
#include <machine/xcoff_machdep.h>

typedef struct xcoff32_filehdr {
	uint16_t 	f_magic;			/* magic number */
	uint16_t 	f_nscns;			/* # of sections */
	uint32_t   	f_timdat;			/* time and date stamp */
	uint32_t 	f_symptr;			/* file offset of symbol table */
	uint32_t   	f_nsyms;			/* # of symbol table entries */
	uint16_t 	f_opthdr;			/* sizeof the optional header */
	uint16_t 	f_flags;			/* flags??? */
} xcoff32_filehdr;

typedef struct xcoff64_filehdr {
	uint16_t 	f_magic;			/* magic number */
	uint16_t 	f_nscns;			/* # of sections */
	uint32_t   	f_timdat;			/* time and date stamp */
	uint64_t	f_symptr;			/* file offset of symbol table */
	uint32_t   	f_nsyms;			/* # of symbol table entries */
	uint16_t 	f_opthdr;			/* sizeof the optional header */
	uint16_t 	f_flags;			/* flags??? */
} xcoff64_filehdr;

/* f_magic */
#define XCOFF_F_32_MAGIC  	0x01DF
#define XCOFF_F_64_MAGIC 	0x01F7

/* f_flags */
#define XCOFF_F_RELFLG		0x0001
#define XCOFF_F_EXEC		0x0002
#define XCOFF_F_LNNO 		0x0004
#define XCOFF_F_FDPR_PROF	0x0010
#define XCOFF_F_FDPR_OPTI 	0x0020
#define XCOFF_F_DSA 		0x0040
			/* Reserved: 	0x0080 */
#define XCOFF_F_VARPG		0x0100
			/* Reserved: 	0x0200 */
			/* Reserved: 	0x0400 */
			/* Reserved:	0x0800 */
#define XCOFF_F_DYNLOAD 	0x1000
#define XCOFF_F_SHROBJ		0x2000
#define XCOFF_F_LOADONLY 	0x4000
			/* Reserved: 	0x8000 */

typedef struct xcoff32_aouthdr {
	uint16_t	mflag;			/* flags */
	uint16_t 	vstamp;			/* version */
	uint32_t  	tsize;			/* text size in bytes */
	uint32_t  	dsize;			/* initialized data size in bytes */
	uint32_t  	bsize;			/* uninitialized data size in bytes */
	uint32_t  	entry;			/* entry point descriptor (virtual address) */
	uint32_t  	text_start;		/* base address of text (virtual address) */
	uint32_t  	data_start;		/* base address of data (virtual address) */
	uint32_t  	bss_start;		/* base address of bss (virtual address) */
	uint32_t	toc;			/* address of TOC anchor */
	uint16_t 	snentry;		/* section number for entry point */
	uint16_t 	sntext;			/* section number for .text */
	uint16_t 	sndata;			/* section number for .data */
	uint16_t 	sntoc;			/* section number for TOC */
	uint16_t 	snloader;		/* section number for loader data */
	uint16_t 	snbss;			/* section number for .bss */
	uint16_t 	algntext;		/* maximum alignment for .text */
	uint16_t 	algndata;		/* maximum alignment for .data */
	uint16_t 	modtype;		/* module type field */
	uint8_t		cpuflag;		/* bit flags - cpu types of objects */
	uint8_t		cputype;		/* reserved for CPU type */
	uint32_t	maxstack;		/* maximum stack size allowed (bytes) */
	uint32_t	maxdata;		/* maximum data size allowed (bytes) */
	uint32_t	debugger;		/* reserved for debuggers */
	uint8_t		textpsize;		/* requested text page size */
	uint8_t		datapsize;		/* requested data page size */
	uint8_t		stackpsize;		/* requested stack page size */
	uint8_t		flags;			/* flags and thread-local storage */
	uint16_t 	sntdata;		/* section number for .tdata */
	uint16_t 	sntbss;			/* section number for .tbss */
} xcoff32_aouthdr;

typedef struct xcoff64_aouthdr {
	uint16_t	mflag;			/* flags */
	uint16_t 	vstamp;			/* version */
	uint64_t  	tsize;			/* text size in bytes */
	uint64_t  	dsize;			/* initialized data size in bytes */
	uint64_t  	bsize;			/* uninitialized data size in bytes */
	uint64_t  	entry;			/* entry point descriptor (virtual address) */
	uint64_t  	text_start;		/* base address of text (virtual address) */
	uint64_t  	data_start;		/* base address of data (virtual address) */
	uint64_t  	bss_start;		/* base address of bss (virtual address) */
	uint64_t	toc;			/* address of TOC anchor */
	uint16_t 	snentry;		/* section number for entry point */
	uint16_t 	sntext;			/* section number for .text */
	uint16_t 	sndata;			/* section number for .data */
	uint16_t 	sntoc;			/* section number for TOC */
	uint16_t 	snloader;		/* section number for loader data */
	uint16_t 	snbss;			/* section number for .bss */
	uint16_t 	algntext;		/* maximum alignment for .text */
	uint16_t 	algndata;		/* maximum alignment for .data */
	uint16_t 	modtype;		/* module type field */
	uint8_t		cpuflag;		/* bit flags - cpu types of objects */
	uint8_t		cputype;		/* reserved for CPU type */
	uint64_t	maxstack;		/* maximum stack size allowed (bytes) */
	uint64_t	maxdata;		/* maximum data size allowed (bytes) */
	uint32_t	debugger;		/* reserved for debuggers */
	uint8_t		textpsize;		/* requested text page size */
	uint8_t		datapsize;		/* requested data page size */
	uint8_t		stackpsize;		/* requested stack page size */
	uint8_t		flags;			/* flags and thread-local storage */
	uint16_t 	sntdata;		/* section number for .tdata */
	uint16_t 	sntbss;			/* section number for .tbss */
	uint16_t 	x64flags;		/* xcoff64 only flags */
} xcoff64_aouthdr;

/* o_flags */
#define XCOFF_O_AOUT_RAS			0x40
#define XCOFF_O_AOUT_TLS_LE			0x80	/* high-order bit of o_flags */

/* x64flags */
#define XCOFF_O_64_AOUT_FORK_COR 	0x2000
#define XCOFF_O_64_AOUT_FORK_POLICY 0x4000
#define XCOFF_O_64_AOUT_SHR_SYMTAB 	0x8000

typedef struct xcoff32_scnhdr {
	char		s_name[8];		/* name */
	uint32_t  	s_paddr;		/* physical addr? for ROMing?*/
	uint32_t  	s_vaddr;		/* virtual addr? */
	uint32_t  	s_size;			/* size */
	uint32_t  	s_scnptr;		/* file offset of raw data */
	uint32_t  	s_relptr;		/* file offset of reloc data */
	uint32_t  	s_lnnoptr;		/* file offset of line data */
	uint16_t 	s_nreloc;		/* # of relocation entries */
	uint16_t 	s_nlnno;		/* # of line entries */
	uint16_t   	s_flags;		/* flags */
} xcoff32_scnhdr;

typedef struct xcoff64_scnhdr {
	char		s_name[8];		/* name */
	uint64_t  	s_paddr;		/* physical addr? for ROMing?*/
	uint64_t  	s_vaddr;		/* virtual addr? */
	uint64_t  	s_size;			/* size */
	uint64_t  	s_scnptr;		/* file offset of raw data */
	uint64_t  	s_relptr;		/* file offset of reloc data */
	uint64_t  	s_lnnoptr;		/* file offset of line data */
	uint32_t 	s_nreloc;		/* # of relocation entries */
	uint32_t 	s_nlnno;		/* # of line entries */
	uint32_t   	s_flags;		/* flags */
} xcoff64_scnhdr;

/* s_flags */
#define XCOFF_S_PAD 		0x0008
#define XCOFF_S_DWARF 		0x0010
#define XCOFF_S_TEXT 		0x0020
#define XCOFF_S_DATA 		0x0040
#define XCOFF_S_BSS 		0x0080
#define XCOFF_S_EXCEPT 		0x0100
#define XCOFF_S_INFO 		0x0200
#define XCOFF_S_TDATA 		0x0400
#define XCOFF_S_TBSS 		0x0800
#define XCOFF_S_LOADER 		0x1000
#define XCOFF_S_DEBUG 		0x2000
#define XCOFF_S_TYPCHK 		0x4000
#define XCOFF_S_OVRFLO 		0x8000

typedef struct xcoff32_exechdr {
	xcoff32_filehdr f;
	xcoff32_aouthdr a;
} xcoff32_exechdr;

typedef struct xcoff64_exechdr {
	xcoff64_filehdr f;
	xcoff64_aouthdr a;
} xcoff64_exechdr;

/* XCOFF relocation entry */
typedef struct xcoff32_reloc {
	uint32_t	r_vaddr;		/* virtual address (position) in section to be relocated */
	uint32_t	r_symndx;		/* symbol table index of item that is referenced */
	uint8_t		r_rsize;		/* relocation size and information */
	uint8_t		r_rtype;		/* relocation type */
} xcoff32_reloc;

typedef struct xcoff64_reloc {
	uint64_t	r_vaddr;		/* virtual address (position) in section to be relocated */
	uint32_t	r_symndx;		/* symbol table index of item that is referenced */
	uint8_t		r_rsize;		/* relocation size and information */
	uint8_t		r_rtype;		/* relocation type */
} xcoff64_reloc;

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

/* XCOFF symbol entries */
typedef struct xcoff32_syms {
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
} xcoff32_syms;

typedef struct xcoff64_syms {
	char		n_name[8];		/* symbol name (occupies the same 8 bytes as n_zeroes and n_offset) */
	int32_t 	n_zeroes;		/* zero, indicating name in string table or .debug section (overlays first 4 bytes of n_name) */
	int32_t 	n_offset;		/* offset of the name in string table or .debug section (In XCOFF32: overlays last 4 bytes of n_name) */
	int64_t 	n_value;		/* symbol value; storage class-dependent */
	int16_t 	n_scnum;		/* section number of symbol */
	int16_t 	n_type;			/* basic and derived type specification */
	int8_t 		n_lang;			/* source language ID (overlays first byte of n_type) */
	int8_t 		n_cpu;			/* CPU Type ID (overlays second byte of n_type) */
	int8_t 		n_sclass;		/* storage class of symbol */
	int8_t 		n_numaux;		/* number of auxiliary entries */
} xcoff64_syms;

/* XCOFF storage class constants */
enum xcoff_strclass {
	scC_EXT,
	scC_WEAKEST,
	scC_HIDEXT,
	scC_FCN,
	scC_BLOCK,
	scC_STAT,
	scC_GSYM,
	scC_BCOMM,
	scC_DECL,
	scC_ENTRY,
	scC_ESTAT,
	scC_ECOMM,
	scC_FUN,
	scC_STSYM,
	scC_BINCL,
	scC_EINCL,
	scC_INFO,
	scC_FILE,
	scC_BSTAT,
	scC_LSYM,
	scC_PSYM,
	scC_RPSYM,
	scC_RSYM,
	scC_ECOML,
	scC_DWARF
};

/* XCOFF loader header */
typedef struct xcoff32_ldrhdr {
	int32_t 	l_version;		/* loader section version number */
	int32_t		l_nsyms;		/* # of symbol table entries */
	int32_t		l_nreloc;		/* # of relocation table entries */
	uint32_t	l_istlen;		/* length of import file ID string table */
	int32_t		l_nimpid;		/* # of import file IDs */
	uint32_t	l_impoff;		/* offset to start of import file IDs */
	uint32_t	l_stlen;		/* length of string table */
	uint32_t 	l_stoff;		/* offset to start of string table */
	uint32_t	l_symoff;		/* offset to start of symbol table */
	uint16_t 	l_rldoff;		/* offset to start of relocation entries */
} xcoff32_ldrhdr;

typedef struct xcoff64_ldrhdr {
	int32_t 	l_version;		/* loader section version number */
	int32_t		l_nsyms;		/* # of symbol table entries */
	int32_t		l_nreloc;		/* # of relocation table entries */
	uint32_t	l_istlen;		/* length of import file ID string table */
	int32_t		l_nimpid;		/* # of import file IDs */
	uint32_t	l_impoff;		/* offset to start of import file IDs */
	uint32_t	l_stlen;		/* length of string table */
	uint64_t 	l_stoff;		/* offset to start of string table */
	uint64_t	l_symoff;		/* offset to start of symbol table */
	uint16_t 	l_rldoff;		/* offset to start of relocation entries */
} xcoff64_ldrhdr;

/* XCOFF loader symbol */
typedef struct xcoff32_ldrsyms {
	char		l_name[8];		/* symbol name or byte offset into string table */
	int32_t		l_zeroes;		/* zero indicates symbol name is referenced from l_offset */
	int32_t		l_offset;		/* byte offset into string table of symbol name */
	int32_t 	l_value;		/* address field */
	int16_t 	l_scnum;		/* section number containing symbol */
	int8_t		l_smtype;		/* symbol type, export, import flags */
	int8_t 		l_smclas;		/* symbol storage class */
	int32_t 	l_ifile;		/* import file ID; ordinal of import file IDs */
	int32_t 	l_parm;			/* parameter type-check field */
} xcoff32_ldrsyms;

typedef struct xcoff64_ldrsyms {
	char		l_name[8];		/* symbol name or byte offset into string table */
	int32_t		l_zeroes;		/* zero indicates symbol name is referenced from l_offset */
	int32_t		l_offset;		/* byte offset into string table of symbol name */
	int64_t 	l_value;		/* address field */
	int16_t 	l_scnum;		/* section number containing symbol */
	int8_t		l_smtype;		/* symbol type, export, import flags */
	int8_t 		l_smclas;		/* symbol storage class */
	int32_t 	l_ifile;		/* import file ID; ordinal of import file IDs */
	int32_t 	l_parm;			/* parameter type-check field */
} xcoff64_ldrsyms;

#define XCOFF32_HDR_SIZE (sizeof(xcoff32_exechdr))
#define XCOFF64_HDR_SIZE (sizeof(xcoff64_exechdr))

#if !defined(XCOFFSIZE)
# if !defined(_KERNEL)
#  define XCOFFSIZE ARCH_XCOFFSIZE
# else
#  define XCOFFSIZE KERN_XCOFFSIZE
#endif
#endif

#if defined(XCOFFSIZE)
#define	CONCAT(x,y)			__CONCAT(x,y)
#define	XCOFFNAME(x)		CONCAT(xcoff,CONCAT(XCOFFSIZE,CONCAT(_,x)))
#define	XCOFFNAME2(x,y)		CONCAT(x,CONCAT(_xcoff,CONCAT(XCOFFSIZE,CONCAT(_,y))))
#define	XCOFFNAMEEND(x)		CONCAT(x,CONCAT(_xcoff,XCOFFSIZE))
#define	XCOFFDEFNNAME(x)	CONCAT(XCOFF,CONCAT(XCOFFSIZE,CONCAT(_,x)))
#endif

#if defined(XCOFFSIZE) && (XCOFFSIZE == 32)
#define xcoff_filehdr		xcoff32_filehdr
#define xcoff_aouthdr		xcoff32_aouthdr
#define xcoff_scnhdr		xcoff32_scnhdr
#define xcoff_reloc			xcoff32_reloc
#define xcoff_syms			xcoff32_syms
#define xcoff_ldrhdr		xcoff32_ldrhdr
#define xcoff_ldrsyms		xcoff32_ldrsyms

#define xcoff_exechdr		xcoff32_exechdr
#define XCOFF_HDR_SIZE		XCOFF32_HDR_SIZE
#define XCOFF_F_MAGIC		XCOFF_F_32_MAGIC

#elif defined(XCOFFSIZE) && (XCOFFSIZE == 64)
#define xcoff_filehdr		xcoff64_filehdr
#define xcoff_aouthdr		xcoff64_aouthdr
#define xcoff_scnhdr		xcoff64_scnhdr
#define xcoff_reloc			xcoff64_reloc
#define xcoff_syms			xcoff64_syms
#define xcoff_ldrhdr		xcoff64_ldrhdr
#define xcoff_ldrsyms		xcoff64_ldrsyms

#define xcoff_exechdr		xcoff64_exechdr
#define XCOFF_HDR_SIZE		XCOFF64_HDR_SIZE
#define XCOFF_F_MAGIC		XCOFF_F_64_MAGIC
#endif

#define XCOFF_OMAGIC 0407
#define XCOFF_NMAGIC 0410
#define XCOFF_ZMAGIC 0413

#define XCOFF_ROUND(value, by) 													\
        (((value) + (by) - 1) & ~((by) - 1))

#define XCOFF_BLOCK_ALIGN(ep, value) 											\
	((ep)->a.magic == XCOFF_ZMAGIC ? XCOFF_ROUND((value), XCOFF_LDPGSZ) : 		\
	(value))

#define XCOFF_TXTOFF(ep) 														\
        ((ep)->a.magic == XCOFF_ZMAGIC ? 0 : 									\
        XCOFF_ROUND(XCOFF_HDR_SIZE + (ep)->f.f_nscns * 							\
		     sizeof(struct xcoff_scnhdr), XCOFF_SEGMENT_ALIGNMENT(ep)))

#define XCOFF_DATOFF(ep) 														\
        ((XCOFF_BLOCK_ALIGN((ep), XCOFF_TXTOFF(ep) + (ep)->a.tsize)))

#define XCOFF_SEGMENT_ALIGN(ep, value) 											\
        (XCOFF_ROUND((value), ((ep)->a.magic == XCOFF_ZMAGIC ? XCOFF_LDPGSZ : 	\
         XCOFF_SEGMENT_ALIGNMENT(ep))))

#ifdef _KERNEL
struct exec_linker;

int	 exec_xcoff_linker(struct proc *, struct exec_linker *);
int	 exec_xcoff_prep_zmagic(struct proc *, struct exec_linker *, xcoff_exechdr *, struct vnode *);
int	 exec_xcoff_prep_nmagic(struct proc *, struct exec_linker *, xcoff_exechdr *, struct vnode *);
int	 exec_xcoff_prep_omagic(struct proc *, struct exec_linker *, xcoff_exechdr *, struct vnode *);
#endif /* _KERNEL */

#endif /* _SYS_EXEC_XCOFF_H_ */
