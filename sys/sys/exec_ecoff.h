/*	$NetBSD: exec_ecoff.h,v 1.11 1999/04/27 05:36:43 cgd Exp $	*/

/*
 * Copyright (c) 1994 Adam Glass
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
 *      This product includes software developed by Adam Glass.
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

#ifndef _SYS_EXEC_ECOFF_H_
#define _SYS_EXEC_ECOFF_H_

#include <machine/ecoff_machdep.h>

struct ecoff_filehdr {
	u_short 			f_magic;		/* magic number */
	u_short 			f_nscns;		/* # of sections */
	u_int   			f_timdat;		/* time and date stamp */
	u_long  			f_symptr;		/* file offset of symbol table */
	u_int   			f_nsyms;		/* # of symbol table entries */
	u_short 			f_opthdr;		/* sizeof the optional header */
	u_short 			f_flags;		/* flags??? */
};

struct ecoff_aouthdr {
	u_short 			magic;
	u_short 			vstamp;
	ECOFF_PAD
	u_long  			tsize;
	u_long  			dsize;
	u_long  			bsize;
	u_long  			entry;
	u_long  			text_start;
	u_long  			data_start;
	u_long  			bss_start;
	ECOFF_MACHDEP;
};

struct ecoff_scnhdr {	/* needed for size info */
	char				s_name[8];		/* name */
	u_long  			s_paddr;		/* physical addr? for ROMing?*/
	u_long  			s_vaddr;		/* virtual addr? */
	u_long  			s_size;			/* size */
	u_long  			s_scnptr;		/* file offset of raw data */
	u_long  			s_relptr;		/* file offset of reloc data */
	u_long  			s_lnnoptr;		/* file offset of line data */
	u_short 			s_nreloc;		/* # of relocation entries */
	u_short 			s_nlnno;		/* # of line entries */
	u_int   			s_flags;		/* flags */
};

struct ecoff_exechdr {
	struct ecoff_filehdr f;
	struct ecoff_aouthdr a;
};

/* ECOFF symbol header */
struct ecoff_symhdr {
	int16_t				magic;			/* validity of the symbol table, this field must contain the constant magicSym, defined as 0x1992 */
	int16_t				vstamp;			/* symbol table version stamp: maj_sym_stamp: 3 or min_sym_stamp: 13 */
	int32_t				ilineMax;		/* number of line number entries */
	int32_t				cbLine;			/* byte size of (packed) line number entries */
	int32_t				cbLineOfset;	/* byte offset to start of (packed) line numbers */
	int32_t				idnMax;			/* obsolete */
	int32_t				cbDnOffset;		/* obsolete */
	int32_t				ipdMax;			/* number of procedure descriptors */
	int32_t				cbPdOffset;		/* byte offset to start of procedure descriptors */
	int32_t				isymMax;		/* number of local symbols */
	int32_t				cbSymOffset;	/* byte offset to start of local symbols */
	int32_t				ioptMax;		/* byte size of optimization symbol table */
	int32_t				cbOptOffset;	/* byte offset to start of optimization entries */
	int32_t				iauxMax;		/* number of auxiliary symbols */
	int32_t				cbAuxOffset;	/* byte offset to start of auxiliary symbols */
	int32_t				issMax;			/* byte size of local string table */
	int32_t				cbSsOffset;		/* byte offset to start of local strings */
	int32_t				issExtMax;		/* byte size of external string table */
	int32_t				cbSsExtOffset;	/* byte offset to start of external strings */
	int32_t				ifdMax;			/* number of file descriptors */
	int32_t				cbFdOffset;		/* byte offset to start of file descriptors */
	int32_t				crfd;			/* number of relative file descriptors */
	int32_t				cbRfdOffset;	/* byte offset to start of relative file descriptors */
	int32_t				iextMax;		/* number of external symbols */
	int32_t				cbExtOffset;	/* byte offset to start of external symbols */
	int32_t				esymMax;
};

/* ECOFF local symbol entry */
struct ecoff_lsym {
	long 				ls_value;		/* a field that can contain an address, size, offset, or index */
	int 				ls_iss;			/* byte offset from the issBase field of a file descriptor table entry to the name of the symbol. -1 if no name for symbol */
	uint				ls_type;		/* symbol type */
	uint				ls_class;		/* storage class */
	uint				ls_index;		/* an index into either the local symbol table or auxiliary symbol table, depending on the symbol type and class */
};

/* ECOFF external symbol entry */
struct ecoff_esym {
	struct	ecoff_lsym 		es_lsym;		/* local symbol entry pointer */
	int32_t				es_value;		/* symbol address for most defined symbols */
	int32_t				es_iss;			/* byte offset in external string table to symbol name. -1 if no name for symbol */
	uint				es_type:6;		/* symbol type */
	uint				es_class:5;		/* storage class */
	unsigned			es_symauxindex:20;	/* an index into the auxiliary symbol table for a type description or an index into the local symbol table to pointing to a related symbol */
	uint				es_weakext;		/* flag set to identify the symbol as a weak external. */
	u_int16_t			es_ifd;			/* index of the file descriptor where the symbol is defined. -1 for undefined symbols */
	u_int16_t			es_flags;
};

/* ECOFF symbol type constants */
enum symtype {
	stNil,
	stGlobal,
	stStatic,
	stParam,
	stLocal,
	stLabel,
	stProc,
	stBlock,
	stEnd,
	stMember,
	stTypedef,
	stFile,
	stStaticProc,
	stConstant,
	stBase,
	stVirtBase,
	stTag,
	stInter,
	stModule,
	stNamespace,
	stModview,
	stUsing,
	stAlias
};

/* ECOFF storage class constants */
enum strclass {
	scNil,
	scText,
	scData,
	scBss,
	scRegister,
	scAbs,
	scUndefined,
	scUnallocated,
	scTlsUndefined,
	scInfo,
	scSData,
	scSBss,
	scRData,
	scVar,
	scCommon,
	scSCommon,
	scVarRegister,
	scVariant,
	scFileDesc,
	scSUndefined,
	scInit,
	scReportDesc,
	scXData,
	scPData,
	scFini,
	scRConst,
	scTlsCommon,
	scTlsData,
	scTlsBss,
	scMax
};

#define ECOFF_HDR_SIZE (sizeof(struct ecoff_exechdr))

#define ECOFF_OMAGIC 	0407
#define ECOFF_NMAGIC 	0410
#define ECOFF_ZMAGIC 	0413

#define ECOFF_MAGIC_SYM 0x1992

#define ECOFF_ROUND(value, by) 													\
        (((value) + (by) - 1) & ~((by) - 1))

#define ECOFF_BLOCK_ALIGN(ep, value) 											\
	((ep)->a.magic == ECOFF_ZMAGIC ? ECOFF_ROUND((value), ECOFF_LDPGSZ) : 		\
	(value))

#define ECOFF_TXTOFF(ep) 														\
        ((ep)->a.magic == ECOFF_ZMAGIC ? 0 : 									\
        ECOFF_ROUND(ECOFF_HDR_SIZE + (ep)->f.f_nscns * 							\
		     sizeof(struct ecoff_scnhdr), ECOFF_SEGMENT_ALIGNMENT(ep)))

#define ECOFF_DATOFF(ep) 														\
        ((ECOFF_BLOCK_ALIGN((ep), ECOFF_TXTOFF(ep) + (ep)->a.tsize)))

#define ECOFF_SEGMENT_ALIGN(ep, value) 											\
        (ECOFF_ROUND((value), ((ep)->a.magic == ECOFF_ZMAGIC ? ECOFF_LDPGSZ : 	\
         ECOFF_SEGMENT_ALIGNMENT(ep))))

#ifdef _KERNEL

int	 exec_ecoff_linker(struct proc *, struct exec_linker *);
int	 exec_ecoff_prep_zmagic(struct proc *, struct exec_linker *, struct ecoff_exechdr *, struct vnode *);
int	 exec_ecoff_prep_nmagic(struct proc *, struct exec_linker *, struct ecoff_exechdr *, struct vnode *);
int	 exec_ecoff_prep_omagic(struct proc *, struct exec_linker *, struct ecoff_exechdr *, struct vnode *);
#endif /* _KERNEL */
#endif /* _SYS_EXEC_ECOFF_H_ */
