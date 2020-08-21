/*
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

/* NOTE: The Follow Contents of exec_xcoff.h is derived from IBM Knowledge Center:
 * https://www.ibm.com/support/knowledgecenter/ssw_aix_72/filesreference/XCOFF.html
 */

#ifndef _SYS_EXEC_XCOFF_H_
#define _SYS_EXEC_XCOFF_H_

#include <machine/types.h>
#include <machine/xcoff_machdep.h>

#define XCOFF_LDPGSZ 4096

/* xcoff file header */
typedef struct {
	uint16_t 	magic;			/* magic number */
	uint16_t 	nscns;			/* # of sections */
	uint32_t   	timdat;			/* time and date stamp */
	uint32_t 	symptr;			/* file offset of symbol table */
	uint32_t   	nsyms;			/* # of symbol table entries */
	uint16_t 	opthdr;			/* sizeof the optional header */
	uint16_t 	flags;			/* flags??? */
} xcoff32_filehdr;

typedef struct {
	uint16_t 	magic;			/* magic number */
	uint16_t 	nscns;			/* # of sections */
	uint32_t   	timdat;			/* time and date stamp */
	uint64_t	symptr;			/* file offset of symbol table */
	uint32_t   	nsyms;			/* # of symbol table entries */
	uint16_t 	opthdr;			/* sizeof the optional header */
	uint16_t 	flags;			/* flags??? */
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
			/* Reserved: 0x0080 */
#define XCOFF_F_VARPG		0x0100
			/* Reserved: 0x0200 */
			/* Reserved: 0x0400 */
			/* Reserved: 0x0800 */
#define XCOFF_F_DYNLOAD 	0x1000
#define XCOFF_F_SHROBJ		0x2000
#define XCOFF_F_LOADONLY 	0x4000
			/* Reserved: 0x8000 */

typedef struct {
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

typedef struct {
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

typedef struct {
	char		name[8];		/* name */
	uint32_t  	paddr;			/* physical addr? for ROMing?*/
	uint32_t  	vaddr;			/* virtual addr? */
	uint32_t  	size;			/* size */
	uint32_t  	scnptr;			/* file offset of raw data */
	uint32_t  	relptr;			/* file offset of reloc data */
	uint32_t  	lnnoptr;		/* file offset of line data */
	uint16_t 	nreloc;			/* # of relocation entries */
	uint16_t 	nlnno;			/* # of line entries */
	uint16_t   	flags;			/* flags */
} xcoff32_scnhdr;

typedef struct {
	char		name[8];		/* name */
	uint64_t  	paddr;			/* physical addr? for ROMing?*/
	uint64_t  	vaddr;			/* virtual addr? */
	uint64_t  	size;			/* size */
	uint64_t  	scnptr;			/* file offset of raw data */
	uint64_t  	relptr;			/* file offset of reloc data */
	uint64_t  	lnnoptr;		/* file offset of line data */
	uint32_t 	nreloc;			/* # of relocation entries */
	uint32_t 	nlnno;			/* # of line entries */
	uint32_t   	flags;			/* flags */
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

typedef struct {
	struct xcoff32_filehdr f;
	struct xcoff32_aouthdr a;
	struct xcoff32_scnhdr  s;
} xcoff32_exechdr;

typedef struct {
	struct xcoff64_filehdr f;
	struct xcoff64_aouthdr a;
	struct xcoff64_scnhdr  s;
} xcoff64_exechdr;

#define XCOFF32_HDR_SIZE (sizeof(struct xcoff32_exechdr))
#define XCOFF64_HDR_SIZE (sizeof(struct xcoff64_exechdr))

#if !defined(XCOFFSIZE)
# if !defined(_KERNEL)
#  define XCOFFSIZE ARCH_XCOFFSIZE
# else
#  define XCOFFSIZE KERN_XCOFFSIZE
#endif
#endif

#if defined(XCOFFSIZE) && (XCOFFSIZE == 32)
#define xcoff_filehdr		xcoff32_filehdr
#define xcoff_aouthdr		xcoff32_aouthdr
#define xcoff_scnhdr		xcoff32_scnhdr

#define xcoff_exechdr		xcoff32_exechdr
#define XCOFF_HDR_SIZE		XCOFF32_HDR_SIZE

#elif defined(XCOFFSIZE) && (XCOFFSIZE == 64)
#define xcoff_filehdr		xcoff64_filehdr
#define xcoff_aouthdr		xcoff64_aouthdr
#define xcoff_scnhdr		xcoff64_scnhdr

#define xcoff_exechdr		xcoff64_exechdr
#define XCOFF_HDR_SIZE		XCOFF64_HDR_SIZE
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
int	 exec_xcoff_linker (struct exec_linker *));
void cpu_exec_xcoff_setregs (struct proc *, struct exec_linker *, u_long);

int	 exec_xcoff_prep_zmagic (struct exec_linker *, struct xcoff_exechdr *, struct vnode *);
int	 exec_xcoff_prep_nmagic (struct exec_linker *, struct xcoff_exechdr *, struct vnode *);
int	 exec_xcoff_prep_omagic (struct exec_linker *, struct xcoff_exechdr *, struct vnode *);
#endif /* _KERNEL */

#endif /* _SYS_EXEC_XCOFF_H_ */
