/*	$NetBSD: rtld.h,v 1.9.2.2 2000/10/10 21:48:02 he Exp $	 */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef RTLD_H
#define RTLD_H

#include <stddef.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/exec_elf.h>
#include "rtldenv.h"
#include "link.h"

#ifndef	RTLD_DEFAULT_LIBRARY_PATH
#define	RTLD_DEFAULT_LIBRARY_PATH	"/usr/lib"
#endif
#define _PATH_LD_HINTS				"/etc/ld.so.conf"

extern size_t _rtld_pagesz;

#define round_down(x)	((x) & ~(_rtld_pagesz - 1))
#define round_up(x)	round_down((x) + _rtld_pagesz - 1)

#define NEW(type)	((type *) xmalloc(sizeof(type)))
#define CNEW(type)	((type *) xcalloc(sizeof(type)))

/*
 * C++ has mandated the use of the following keywords for its new boolean
 * type.  We might as well follow their lead.
 */
typedef unsigned char bool;
#define false	0
#define true	1

struct Struct_Obj_Entry;

typedef struct Struct_Needed_Entry {
	struct Struct_Needed_Entry 	*next;
	struct Struct_Obj_Entry 	*obj;
	unsigned long   			name;	/* Offset of name in string table */
} Needed_Entry;

typedef struct _rtld_search_path_t {
	struct _rtld_search_path_t 	*sp_next;
	const char     				*sp_path;
	size_t          			sp_pathlen;
} Search_Path;

/*
 * Shared object descriptor.
 *
 * Items marked with "(%)" are dynamically allocated, and must be freed
 * when the structure is destroyed.
 */

#define RTLD_MAGIC		0xd550b87a
#define RTLD_VERSION	1

typedef struct Struct_Obj_Entry {
	Elf32_Word      magic;		/* Magic number (sanity check) */
	Elf32_Word      version;	/* Version number of struct format */

	struct Struct_Obj_Entry *next;
	char           *path;		/* Pathname of underlying file (%) */
	int             refcount;
	int             dl_refcount;	/* Number of times loaded by dlopen */

	/* These items are computed by map_object() or by digest_phdr(). */
	caddr_t         mapbase;	/* Base address of mapped region */
	size_t          mapsize;	/* Size of mapped region in bytes */
	size_t          textsize;	/* Size of text segment in bytes */
	Elf_Addr        vaddrbase;	/* Base address in shared object file */
	caddr_t         relocbase;	/* Reloc const = mapbase - *vaddrbase */
	Elf_Dyn        *dynamic;	/* Dynamic section */
	caddr_t         entry;		/* Entry point */
	const Elf_Phdr *phdr;		/* Program header if mapped, ow NULL */
	size_t          phsize;		/* Size of program header in bytes */

	/* Items from the dynamic section. */
	Elf_Addr       *pltgot;		/* PLTGOT table */
	const Elf_Rel  *rel;		/* Relocation entries */
	const Elf_Rel  *rellim;		/* Limit of Relocation entries */
	const Elf_RelA *rela;		/* Relocation entries */
	const Elf_RelA *relalim;	/* Limit of Relocation entries */
	const Elf_Rel  *pltrel;		/* PLT relocation entries */
	const Elf_Rel  *pltrellim;	/* Limit of PLT relocation entries */
	const Elf_RelA *pltrela;	/* PLT relocation entries */
	const Elf_RelA *pltrelalim;	/* Limit of PLT relocation entries */
	const Elf_Sym  *symtab;		/* Symbol table */
	const char     *strtab;		/* String table */
	unsigned long   strsize;	/* Size in bytes of string table */

	const Elf_Word *buckets;	/* Hash table buckets array */
	unsigned long   nbuckets;	/* Number of buckets */
	const Elf_Word *chains;		/* Hash table chain array */
	unsigned long   nchains;	/* Number of chains */

	Search_Path    *rpaths;		/* Search path specified in object */
	Needed_Entry   *needed;		/* Shared objects needed by this (%) */

	void            (*init) (void);/* Initialization function to call */
	void            (*fini) (void);/* Termination function to call */

	/* Entry points for dlopen() and friends. */
	void           *(*dlopen) (const char *, int);
	void           *(*dlsym) (void *, const char *);
	char           *(*dlerror) (void);
	int             (*dlclose) (void *);

	int             mainprog:1;	/* True if this is the main program */
	int             rtld:1;		/* True if this is the dynamic linker */
	int             textrel:1;	/* True if there are relocations to text seg */
	int             symbolic:1;	/* True if generated with "-Bsymbolic" */
	int             printed:1;	/* True if ldd has printed it */

	struct link_map linkmap;	/* for GDB */
} Obj_Entry;

extern struct r_debug 	_rtld_debug;
extern Obj_Entry 		*_rtld_objlist;
extern Obj_Entry 		**_rtld_objtail;
extern Obj_Entry 		_rtld_objself;
extern Search_Path 		*_rtld_paths;
extern bool 			_rtld_trust;
extern const char 		*_rtld_error_message;

/* rtld_start.S */
void _rtld_bind_start (void);

/* rtld.c */
void 			_rtld_error (const char *, ...) __attribute__((__format__(__printf__,1,2)));
void 			_rtld_die (void) __dead;
char 			*_rtld_dlerror (void);
void 			*_rtld_dlopen (const char *, int);
void 			*_rtld_dlsym (void *, const char *);
int 			_rtld_dlclose (void *);
void 			_rtld_debug_state (void);
void 			_rtld_linkmap_add (Obj_Entry *);
void 			_rtld_linkmap_delete (Obj_Entry *);

/* headers.c */
void 			_rtld_digest_dynamic (Obj_Entry *);
Obj_Entry 		*_rtld_digest_phdr (const Elf_Phdr *, int, caddr_t);

/* load.c */
Obj_Entry 		*_rtld_load_object (char *, bool);
int 			_rtld_load_needed_objects (Obj_Entry *);
int 			_rtld_preload (const char *, bool);

/* path.c */
void 			_rtld_add_paths (Search_Path **, const char *, bool);

/* reloc.c */
int 			_rtld_do_copy_relocations (const Obj_Entry *, bool);
caddr_t 		_rtld_bind (const Obj_Entry *, Elf_Word);
int 			_rtld_relocate_objects (Obj_Entry *, bool, bool);
int 			_rtld_relocate_nonplt_object (const Obj_Entry *, const Elf_RelA *, bool);
int 			_rtld_relocate_plt_object (const Obj_Entry *, const Elf_RelA *, caddr_t *, bool, bool);

/* search.c */
char 			*_rtld_find_library (const char *, const Obj_Entry *);

/* symbol.c */
unsigned long 	_rtld_elf_hash (const char *);
const Elf_Sym 	*_rtld_symlook_obj (const char *, unsigned long, const Obj_Entry *, bool);
const Elf_Sym 	*_rtld_find_symdef (const Obj_Entry *, Elf_Word, const char *, const Obj_Entry *, const Obj_Entry **, bool);

/* map_object.c */
Obj_Entry 		*_rtld_map_object (const char *, int);

#endif
