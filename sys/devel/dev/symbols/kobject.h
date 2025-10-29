/*	$NetBSD: kobj_impl.h,v 1.5 2016/07/20 13:36:19 maxv Exp $	*/

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998-2000 Doug Rabson
 * Copyright (c) 2004 Peter Wemm
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef SYS_KOBJECT_H_
#define SYS_KOBJECT_H_

#include <ksymbols.h>

typedef struct {
	void		*addr;
	Elf_Off		size;
	int			flags;
	int			sec;		/* Original section */
	const char	*name;
} progent_t;

typedef struct {
	Elf_Rel		*rel;
	int 		nrel;
	int 		sec;
	size_t		size;
} relent_t;

typedef struct {
	Elf_Rela	*rela;
	int			nrela;
	int			sec;
	size_t		size;
} relaent_t;


typedef enum kobjtype {
	KT_UNSET,
	KT_VNODE,
	KT_MEMORY
} kobjtype_t;

/* kobj sections */
typedef struct {
	progent_t			*ko_progtab;
	relaent_t			*ko_relatab;
	relent_t			*ko_reltab;
	int					ko_nrel;
	int					ko_nrela;
	int					ko_nprogtab;
} kobject_section_t;

struct ksymbol_segment_t;
struct ksymbol_string_t;

typedef struct {
	char				ko_name[MAXMODNAME];
	kobjtype_t			ko_type;
	void				*ko_source;
	ssize_t				ko_memsize;
	ksymbol_segment_t 	*ko_segs;
	ksymbol_string_t 	*ko_strs;
	ksymbol_header_t 	*ko_hdr;
	kobject_section_t  	*ko_sects;
	ksymbol_t 			*ko_symtab;
	size_t				ko_symcnt;			/* Number of symbols */
	bool				ko_ksyms;
	bool				ko_loaded;
} kobject_t;

/*
 * Kernel Symbol Segment (If applicable)
 */
typedef struct {
	/* segment address */
	caddr_t		ks_text_address;	/* Address of text segment */
	caddr_t		ks_data_address;	/* Address of data segment */
	caddr_t		ks_stack_address;	/* Address of stack segment */
	caddr_t		ks_rodata_address;	/* Address of rodata segment */
	/* segment size */
	size_t		ks_text_size;		/* Size of text segment */
	size_t		ks_data_size;		/* Size of data/bss segment */
	size_t		ks_stack_size;		/* Size of stack(bss) segment */
	size_t		ks_rodata_size;		/* Size of rodata segment */
} ksymbol_segment_t;

/*
 * Kernel Symbol String (If applicable)
 */
typedef struct {
	char		*ks_strtab;			/* String table */
	char 		*ks_strstart;		/* Address of corresponding string table */
	char		*ks_shstrtab;		/* Section name string table */
	size_t		ks_strtabsz;		/* Number of bytes in string table */
	size_t		ks_shstrtabsz;		/* Number of bytes in scn str table */
} ksymbol_string_t;

/*
 * Kernel Symbol CTF (If applicable)
 */
typedef struct {
	void 		*ks_ctfstart;		/* Address of CTF contents */
	int 		ks_ctfsize;			/* Size in bytes of CTF contents */
} ksymbol_ctf_t;

#endif /* SYS_KOBJECT_H_ */
