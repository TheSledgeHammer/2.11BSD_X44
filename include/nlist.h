/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)nlist.h	5.6.1 (2.11BSD GTE) 1/15/94
 */

#ifndef	_NLIST_H_
#define	_NLIST_H_

#include <sys/cdefs.h>
#include <sys/types.h>

/*
 * Symbol table entry format.  The #ifdef's are so that programs including
 * nlist.h can initialize nlist structures statically.
 */

struct	oldnlist {			/* XXX - compatibility/conversion aid */
	char	n_name[8];		/* symbol name */
	int		n_type;				/* type flag */
unsigned int	n_value;	/* value */
};

struct	nlist {
#ifdef	_AOUT_INCLUDE_
	union {
		char *n_name;		/* In memory address of symbol name */
		off_t n_strx;		/* String table offset (file) */
	} n_un;
# define N_NAME(nlp)	((nlp)->n_un.n_name)
#else
	const char	*n_name;	/* symbol name (in memory) */
	char	*n_filler;		/* need to pad out to the union's size */
# define N_NAME(nlp)	((nlp)->n_name)
#endif
	u_char	n_type;			/* Type of symbol - see below */
	char	n_ovly;			/* Overlay number */
#define	n_hash	n_desc		/* used internally by ld(1); XXX */
	short 	n_desc;			/* used by stab entries */
	u_long	n_value;		/* Symbol value */
};

/*
 * Simple values for n_type.
 */
#define	N_UNDF		0x00		/* undefined */
#define	N_ABS		0x02		/* absolute address */
#define	N_TEXT		0x04		/* text segment */
#define	N_DATA		0x06		/* data segment */
#define	N_BSS		0x08		/* bss segment */
#define	N_REG		0x10		/* register symbol */
#define	N_INDR		0x0a		/* alias definition */
#define	N_SIZE		0x0c		/* pseudo type, defines a symbol's size */
#define	N_COMM		0x12		/* common reference */
#define N_SETA		0x14		/* absolute set element symbol */
#define N_SETT		0x16		/* text set element symbol */
#define N_SETD		0x18		/* data set element symbol */
#define N_SETB		0x1a		/* bss set element symbol */
#define N_SETV		0x1c		/* set vector symbol */
#define	N_FN		0x1e		/* file name (N_EXT on) */
#define	N_WARN		0x1e		/* warning message (N_EXT off) */

#define	N_EXT		0x01		/* external (global) bit, OR'ed in */
#define	N_TYPE		0x1e		/* mask for all the type bits */

#define	N_FORMAT	"%06o"		/* namelist value format; XXX */
#define	N_STAB		0x0e0		/* mask for debugger symbols -- stab(5) */

__BEGIN_DECLS
int nlist(const char *, struct nlist *);
int __fdnlist(int, struct nlist *);		/* XXX for libkvm */
__END_DECLS

#endif	/* !_NLIST_H_ */
