/*	$NetBSD: pecoff_exec.h,v 1.9 2005/12/11 12:20:23 christos Exp $	*/

/*
 * Copyright (c) 2000 Masaru OKI
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 * All rights reserved.
 *
 * adapted from sys/sys/exec_ecoff.h
 * based on Intel iBCS2
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

#ifndef SYS_PECOFF_EXEC_H_
#define SYS_PECOFF_EXEC_H_

#include <machine/coff_machdep.h>

struct pecoff_dos_filehdr {
	u_int16_t	d_magic;	/* +0x00 'MZ' */
	u_int8_t	d_stub[0x3a];
	u_int32_t	d_peofs;	/* +0x3c */
};

#define PECOFF_DOS_MAGIC 0x5a4d
#define PECOFF_DOS_HDR_SIZE (sizeof(struct pecoff_dos_filehdr))

#define DOS_BADMAG(dp) ((dp)->d_magic != PECOFF_DOS_MAGIC)

/* magic */
#define COFF_OMAGIC	0407	/* text not write-protected; data seg
				   	   	   	   is contiguous with text */
#define COFF_NMAGIC	0410	/* text is write-protected; data starts
				   	   	   	   at next seg following text */
#define COFF_ZMAGIC	0413	/* text and data segs are aligned for
				   	   	   	   direct paging */
#define COFF_SMAGIC	0443	/* shared lib */

#define COFF_LDPGSZ 4096 	/* XXX */

struct pecoff_imghdr {
	long i_vaddr;
	long i_size;
};

struct pecoff_opthdr {
	long w_base;
	long w_salign;
	long w_falign;
	long w_osvers;
	long w_imgvers;
	long w_subvers;
	long w_rsvd;
	long w_imgsize;
	long w_hdrsize;
	long w_chksum;
	u_short w_subsys;
	u_short w_dllflags;
	long w_ssize;
	long w_cssize;
	long w_hsize;
	long w_chsize;
	long w_lflag;
	long w_nimghdr;
	struct pecoff_imghdr w_imghdr[16];
};


/* s_flags for PE */
#define COFF_STYP_DISCARD		0x2000000
#define COFF_STYP_EXEC			0x20000000
#define COFF_STYP_READ			0x40000000
#define COFF_STYP_WRITE			0x80000000

#define PECOFF_HDR_SIZE 		(COFF_HDR_SIZE + sizeof(struct pecoff_opthdr))
#define PECOFF_AUXSIZE 			(howmany(sizeof(struct pecoff_args), sizeof(char *)))

#define NETBSDPE_ABI_VERSION	0x00000002

struct pecoff_args1 {		/* obsoleted */
	u_long a_base;
	u_long a_entry;
	u_long a_end;
	u_long a_subsystem;
	struct pecoff_imghdr a_imghdr[16];
	u_long a_ldbase;
	u_long a_ldexport;
};

struct pecoff_args {
	u_long a_abiversion;
	u_long a_zero;		/* always 0 */
	u_long a_entry;
	u_long a_end;
	u_long a_ldbase;
	u_long a_ldexport;
	struct pecoff_opthdr a_opthdr;
};

#ifdef _KERNEL
#ifdef DEBUG_PECOFF
#define DPRINTF(a)      uprintf a
#else
#define DPRINTF(a)
#endif

int pecoff_signature(struct proc *, struct vnode *, struct pecoff_dos_filehdr *);
int pecoff_load_file(struct proc *, struct exec_linker *, const char *, struct exec_vmcmd_set *, u_long *, struct pecoff_args *);
int exec_pecoff_linker(struct proc *, struct exec_linker *);
int exec_pecoff_coff_linker(struct proc *, struct exec_linker *, struct coff_filehdr *, int);
int exec_pecoff_prep_omagic(struct proc *, struct exec_linker *, struct coff_filehdr *, struct coff_aouthdr *, int);
int exec_pecoff_prep_mmagic(struct proc *, struct exec_linker *, struct coff_filehdr *, struct coff_aouthdr *, int);
int exec_pecoff_prep_zmagic(struct proc *, struct exec_linker *, struct coff_filehdr *, struct coff_aouthdr *, int);
int	pecoff_copyargs(struct exec_linker *, struct ps_strings *, void *, void *);
#endif /* KERNEL */
#endif /* SYS_PECOFF_EXEC_H_ */
