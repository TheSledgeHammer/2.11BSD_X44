/*-
 * Copyright (c) 1992, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)exec.h	8.1 (Berkeley) 6/11/93
 *	$Id$
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)exec.h	1.2 (2.11BSD GTE) 10/31/93: originally from exec.h
 */

#ifndef _SYS_EXEC_AOUT_H_
#define _SYS_EXEC_AOUT_H_

#define N_GETMAGIC(ex) 														\
	( (ex).a_midmag & 0xffff )
#define N_GETMID(ex) 														\
	( (N_GETMAGIC_NET(ex) == ZMAGIC) ? N_GETMID_NET(ex) : 					\
	((ex).a_midmag >> 16) & 0x03ff )
#define N_GETFLAG(ex) 														\
	( (N_GETMAGIC_NET(ex) == ZMAGIC) ? N_GETFLAG_NET(ex) : 					\
	((ex).a_midmag >> 26) & 0x3f )
#define N_SETMAGIC(ex,mag,mid,flag) 										\
	( (ex).a_midmag = (((flag) & 0x3f) <<26) | (((mid) & 0x03ff) << 16) | 	\
	((mag) & 0xffff) )

#define N_GETMAGIC_NET(ex) 													\
	(ntohl((ex).a_midmag) & 0xffff)
#define N_GETMID_NET(ex) 													\
	((ntohl((ex).a_midmag) >> 16) & 0x03ff)
#define N_GETFLAG_NET(ex) 													\
	((ntohl((ex).a_midmag) >> 26) & 0x3f)
#define N_SETMAGIC_NET(ex,mag,mid,flag) 									\
	( (ex).a_midmag = htonl( (((flag)&0x3f)<<26) | (((mid)&0x03ff)<<16) | 	\
	(((mag)&0xffff)) ) )
#define N_ALIGN(ex,x) 														\
	(N_GETMAGIC(ex) == ZMAGIC || N_GETMAGIC(ex) == QMAGIC || 				\
	 N_GETMAGIC_NET(ex) == ZMAGIC || N_GETMAGIC_NET(ex) == QMAGIC ? 		\
	 ((x) + __LDPGSZ - 1) & ~(__LDPGSZ - 1) : (x))


//#ifdef OVERLAY
#ifdef notyet /* Not ready to be used system-wide */

#define	N_BADMAG(ex) 														\
	(N_GETMAGIC(ex) != OMAGIC && N_GETMAGIC(ex) != NMAGIC && 				\
	 N_GETMAGIC(ex) != ZMAGIC && N_GETMAGIC(ex) != QMAGIC && 				\
	 N_GETMAGIC(ex) != A_MAGIC3 && N_GETMAGIC(ex) != A_MAGIC4 &&			\
	 N_GETMAGIC(ex) != A_MAGIC5 && N_GETMAGIC(ex) != A_MAGIC6 &&			\
	 N_GETMAGIC_NET(ex) != OMAGIC && N_GETMAGIC_NET(ex) != NMAGIC && 		\
	 N_GETMAGIC_NET(ex) != ZMAGIC && N_GETMAGIC_NET(ex) != QMAGIC &&		\
	 N_GETMAGIC_NET(ex) != A_MAGIC3 && N_GETMAGIC_NET(ex) != A_MAGIC4 &&	\
	 N_GETMAGIC_NET(ex) != A_MAGIC5 && N_GETMAGIC_NET(ex) != A_MAGIC6)

/* Address of the bottom of the text segment. */
#define N_TXTADDR(ex) 														\
	((N_GETMAGIC(ex) == OMAGIC || N_GETMAGIC(ex) == NMAGIC || 				\
	N_GETMAGIC(ex) == ZMAGIC || N_GETMAGIC(ex) == A_MAGIC5 || 				\
	N_GETMAGIC(ex) == A_MAGIC6) ? 0 : __LDPGSZ)

/* Address of the bottom of the data segment. */
#define N_DATADDR(ex) 														\
	N_ALIGN(ex, N_TXTADDR(ex) + (ex).a_text)

/* Text segment offset. */
#define	N_TXTOFF(ex) 														\
	(N_GETMAGIC(ex) == ZMAGIC ? __LDPGSZ : (N_GETMAGIC(ex) == QMAGIC || 	\
	N_GETMAGIC_NET(ex) == ZMAGIC) ? 0 : sizeof(struct exec)) ||				\
	(N_GETMAGIC(ex) == A_MAGIC5 || N_GETMAGIC(ex) == A_MAGIC6 ?				\
	sizeof(struct ovlhdr) + sizeof(struct exec) : sizeof(struct exec))

#endif /* notyet */

//#else

/* Valid magic number check. */
#define	N_BADMAG(ex) 														\
	(N_GETMAGIC(ex) != OMAGIC && N_GETMAGIC(ex) != NMAGIC && 				\
	 N_GETMAGIC(ex) != ZMAGIC && N_GETMAGIC(ex) != QMAGIC && 				\
	 N_GETMAGIC_NET(ex) != OMAGIC && N_GETMAGIC_NET(ex) != NMAGIC && 		\
	 N_GETMAGIC_NET(ex) != ZMAGIC && N_GETMAGIC_NET(ex) != QMAGIC)

/* Address of the bottom of the text segment. */
#define N_TXTADDR(ex) 														\
	((N_GETMAGIC(ex) == OMAGIC || N_GETMAGIC(ex) == NMAGIC || 				\
	N_GETMAGIC(ex) == ZMAGIC) ? 0 : __LDPGSZ)

/* Address of the bottom of the data segment. */
#define N_DATADDR(ex) 														\
	N_ALIGN(ex, N_TXTADDR(ex) + (ex).a_text)

/* Text segment offset. */
#define	N_TXTOFF(ex) 														\
	(N_GETMAGIC(ex) == ZMAGIC ? __LDPGSZ : (N_GETMAGIC(ex) == QMAGIC || 	\
	N_GETMAGIC_NET(ex) == ZMAGIC) ? 0 : sizeof(struct exec))

//#endif /* OVERLAY */

/* Data segment offset. */
#define	N_DATOFF(ex) 														\
	N_ALIGN(ex, N_TXTOFF(ex) + (ex).a_text)

/* Relocation table offset. */
#define N_RELOFF(ex) 														\
	N_ALIGN(ex, N_DATOFF(ex) + (ex).a_data)

/* Symbol table offset. */
#define N_SYMOFF(ex) 														\
	(N_RELOFF(ex) + (ex).a_trsize + (ex).a_drsize)

/* String table offset. */
#define	N_STROFF(ex) 														\
	(N_SYMOFF(ex) + (ex).a_syms)

/*
 * Header prepended to each a.out file.
 * only manipulate the a_midmag field via the
 * N_SETMAGIC/N_GET{MAGIC,MID,FLAG} macros in a.out.h
 */
struct exec {
	unsigned long	a_midmag;   /* htonl(flags<<26 | mid<<16 | magic) */
	unsigned long	a_text;		/* size of text segment */
	unsigned long	a_data;		/* size of initialized data */
	unsigned long	a_bss;		/* size of uninitialized data */
	unsigned long	a_syms;		/* size of symbol table */
	unsigned long	a_entry; 	/* entry point */
	unsigned long	a_trsize;	/* text relocation size */
	unsigned long	a_drsize;	/* data relocation size */
	unsigned long	a_unused;	/* not used */
	unsigned long	a_flag; 	/* relocation info stripped */
};

#define	NOVL		15				/* number of overlays */
struct	ovlhdr {
	int				max_ovl;		/* maximum overlay size */
	unsigned int	ov_siz[NOVL];	/* size of i'th overlay */
};

/*
 * eXtended header definition for use with the new macros in a.out.h
*/
struct xexec {
	struct	exec	e;
	struct	ovlhdr	o;
};

#define a_magic 	a_midmag /* XXX Hack to work with current kern_execve.c */

/* a_magic */
#define	A_MAGIC1	0407	/* normal: old impure format */
#define	A_MAGIC2	0410	/* read-only text */
#define	A_MAGIC3	0411	/* separated I&D */
#define	A_MAGIC4	0405	/* overlay */
#define	A_MAGIC5	0430	/* auto-overlay (nonseparate) */
#define	A_MAGIC6	0431	/* auto-overlay (separate) */

#define	OMAGIC		A_MAGIC1 /* Refer to above A_MAGIC1 */
#define	NMAGIC		A_MAGIC2 /* Refer to above A_MAGIC2 */
#define	ZMAGIC		0413	 /* demand load format */
#define QMAGIC      0314     /* "compact" demand load format */

/* a_mid */
#define	MID_ZERO	0		/* unknown - implementation dependent */
#define MID_I386	134		/* i386 BSD binary */

/* a_flags */
#define EX_PIC		0x10	/* contains position independant code */
#define EX_DYNAMIC	0x20	/* contains run-time link-edit info */
#define EX_DPMASK	0x30	/* mask for the above */

#define AOUT_HDR_SIZE	(sizeof(struct exec))

#include <machine/aout_machdep.h>

#ifdef _KERNEL
/* the "a.out" format's entry in the exec switch */
int	exec_aout_linker(struct proc *, struct exec_linker *);
int	exec_aout_prep_zmagic(struct proc *, struct exec_linker *);
int	exec_aout_prep_nmagic(struct proc *, struct exec_linker *);
int	exec_aout_prep_omagic(struct proc *, struct exec_linker *);

/*
 * MD portion
 */
int cpu_exec_aout_linker(struct proc *, struct exec_linker *);
#endif /* KERNEL */
#endif /*_SYS_EXEC_AOUT_H_ */
