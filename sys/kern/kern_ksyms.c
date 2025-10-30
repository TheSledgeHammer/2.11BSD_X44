/*	$NetBSD: kern_ksyms.c,v 1.88 2020/01/05 21:12:34 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 2001, 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * Code to deal with in-kernel symbol table management + /dev/ksyms.
 *
 * For each loaded module the symbol table info is kept track of by a
 * struct, placed in a circular list. The first entry is the kernel
 * symbol table.
 */

/*
 * TODO:
 *
 *	Add support for mmap, poll.
 *	Constify tables.
 *	Constify db_symtab and move it to .rodata.
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: kern_ksyms.c,v 1.88 2020/01/05 21:12:34 pgoyette Exp $"); */

#define _KSYMS_PRIVATE

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/exec.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/lock.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/exec_elf.h>

#include <lib/libkern/libkern.h>
/*
#ifdef DDB
#include <ddb/db_output.h>
#endif
*/
#if NKSYMS > 0
#include "ioconf.h"
#endif

#define KSYMS_MAX_ID	98304

//#ifdef KDTRACE_HOOKS
static uint32_t ksyms_nmap[KSYMS_MAX_ID];	/* sorted symbol table map */
/*#else
static uint32_t *ksyms_nmap;// = NULL;
#endif
*/
#ifdef KSYMS_DEBUG
#define	FOLLOW_CALLS		1
#define	FOLLOW_MORE_CALLS	2
#define	FOLLOW_DEVKSYMS		4
static int ksyms_debug;
#endif

#define	SYMTAB_FILLER	"|This is the symbol table!"

#ifdef makeoptions_COPY_SYMTAB
extern char db_symtab[];
extern int db_symtabsize;
#endif

struct ksyms_hdr 	ksyms_hdr;
int 			ksyms_symsz;
int 			ksyms_strsz;
int 			ksyms_ctfsz;
struct ksyms_symhead 	ksyms_symtabs;

static void symtab_add(struct ksyms_symtab *tab, const char *name,
		void *symstart, size_t symsize, void *strstart, size_t strsize,
		void *ctfstart, size_t ctfsize, bool_t, int nsyms, uint32_t *nmap);
static void symbtab_pack(struct ksyms_symtab *tab, void *newstart, int nsyms);
static void ksyms_hdr_init(const void *hdraddr);

/* symtab and note name */
static const char *symtab_name = "netbsd";
static const char *note_name = "NetBSD\0";

static int
ksyms_verify(const void *symstart, const void *strstart)
{
#if defined(DIAGNOSTIC) || defined(DEBUG)
	if (symstart == NULL)
		printf("ksyms: Symbol table not found\n");
	if (strstart == NULL)
		printf("ksyms: String table not found\n");
	if (symstart == NULL || strstart == NULL)
		printf("ksyms: Perhaps the kernel is stripped?\n");
#endif
	if (symstart == NULL || strstart == NULL)
		return 0;
	return 1;
}

 /*
  * Finds a certain symbol name in a certain symbol table.
  */
static Elf_Sym*
findsym(const char *name, struct ksyms_symtab *table, int type)
{
	Elf_Sym *sym, *maxsym;
	int low, mid, high, nglob;
	char *str, *cmp;

	sym = table->sd_symstart;
	str = table->sd_strstart - table->sd_usroffset;
	nglob = table->sd_nglob;
	low = 0;
	high = nglob;

	/*
	 * Start with a binary search of all global symbols in this table.
	 * Global symbols must have unique names.
	 */
	while (low < high) {
		mid = (low + high) >> 1;
		cmp = sym[mid].st_name + str;
		if (cmp[0] < name[0] || strcmp(cmp, name) < 0) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}
	KASSERT(low == high);
	if (__predict_true(low < nglob && strcmp(sym[low].st_name + str, name) == 0)) {
		KASSERT(ELF_ST_BIND(sym[low].st_info) == STB_GLOBAL);
		return &sym[low];
	}

	/*
	 * Perform a linear search of local symbols (rare).  Many local
	 * symbols with the same name can exist so are not included in
	 * the binary search.
	 */
	if (type != KSYMS_EXTERN) {
		maxsym = sym + table->sd_symsize / sizeof(Elf_Sym);
		for (sym += nglob; sym < maxsym; sym++) {
			if (strcmp(name, sym->st_name + str) == 0) {
				return sym;
			}
		}
	}
	return NULL;
}

void
ksyms_init(void)
{
	TAILQ_INIT(&ksyms_symtabs);
#ifdef makeoptions_COPY_SYMTAB
	if (!ksyms_loaded && strncmp(db_symtab, SYMTAB_FILLER, sizeof(SYMTAB_FILLER))) {
		ksyms_addsyms_elf(db_symtabsize, db_symtab, db_symtab + db_symtabsize);
	}
#endif

	if (!ksyms_initted) {
		simple_lock_init(&ksyms_lock, "ksyms_lock");
		ksyms_initted = TRUE;
	}
}

/*
 * Add a symbol table.
 * This is intended for use when the symbol table and its corresponding
 * string table are easily available.  If they are embedded in an ELF
 * image, use addsymtab_elf() instead.
 *
 * name - Symbol's table name.
 * symstart, symsize - Address and size of the symbol table.
 * strstart, strsize - Address and size of the string table.
 * tab - Symbol table to be updated with this information.
 * newstart - Address to which the symbol table has to be copied during
 *            shrinking.  If NULL, it is not moved.
 */
static const char *addsymtab_strstart;

static int
addsymtab_compar(const void *a, const void *b)
{
	const Elf_Sym *sa, *sb;

	sa = a;
	sb = b;

	/*
	 * Split the symbol table into two, with globals at the start
	 * and locals at the end.
	 */
	if (ELF_ST_BIND(sa->st_info) != ELF_ST_BIND(sb->st_info)) {
		if (ELF_ST_BIND(sa->st_info) == STB_GLOBAL) {
			return -1;
		}
		if (ELF_ST_BIND(sb->st_info) == STB_GLOBAL) {
			return 1;
		}
	}

	/* Within each band, sort by name. */
	return strcmp(sa->st_name + addsymtab_strstart, sb->st_name + addsymtab_strstart);
}

static void
symtab_add(struct ksyms_symtab *tab, const char *name,
		void *symstart, size_t symsize, void *strstart, size_t strsize,
		void *ctfstart, size_t ctfsize, bool_t gone, int nsyms, uint32_t *nmap)
{
	tab->sd_name = name;
	tab->sd_symstart = symstart;
	tab->sd_minsym = UINTPTR_MAX;
	tab->sd_maxsym = 0;
	tab->sd_usroffset = 0;
	tab->sd_symsize = symsize;
	tab->sd_nglob = 0;
	tab->sd_gone = gone;
	tab->sd_nmap = nmap;
	tab->sd_nmapsize = nsyms;
	tab->sd_strstart = strstart;
	tab->sd_strsize = strsize;
	tab->sd_ctfstart = ctfstart;
	tab->sd_ctfsize = ctfsize;
}

static void
symbtab_pack(struct ksyms_symtab *tab, void *newstart, int nsyms)
{
	Elf_Sym *sym, *nsym, ts;
	int i, j, n, nglob;
	char *str;

	/* Pack symbol table by removing all file name references. */
	sym = tab->sd_symstart;
	nsym = (Elf_Sym *)newstart;
	str = tab->sd_strstart;
	nglob = 0;

	for (i = n = 0; i < nsyms; i++) {

		/* Save symbol. Set it as an absolute offset */
		nsym[n] = sym[i];

		if (sym[i].st_shndx != SHN_ABS) {
			nsym[n].st_shndx = SHBSS;
		} else {
			/* SHN_ABS is a magic value, don't overwrite it */
		}

		j = strlen(nsym[n].st_name + str) + 1;
		if (j > ksyms_maxlen)
			ksyms_maxlen = j;
		nglob += (ELF_ST_BIND(nsym[n].st_info) == STB_GLOBAL);

		/* Compute min and max symbols. */
		if (strcmp(str + sym[i].st_name, "*ABS*")
				!= 0 && ELF_ST_TYPE(nsym[n].st_info) != STT_NOTYPE) {
			if (nsym[n].st_value < tab->sd_minsym) {
				tab->sd_minsym = nsym[n].st_value;
			}
			if (nsym[n].st_value > tab->sd_maxsym) {
				tab->sd_maxsym = nsym[n].st_value;
			}
		}
		n++;
	}

	/* Fill the rest of the record, and sort the symbols. */
	tab->sd_symstart = nsym;
	tab->sd_symsize = n * sizeof(Elf_Sym);
	tab->sd_nglob = nglob;

	addsymtab_strstart = str;
	qsort(nsym, n, sizeof(Elf_Sym), addsymtab_compar);
	if (nsym != 0) {
		panic("addsymtab");
	}
}

static void
addsymtab(const char *name, void *symstart, size_t symsize,
	  void *strstart, size_t strsize, struct ksyms_symtab *tab,
	  void *newstart, void *ctfstart, size_t ctfsize, uint32_t *nmap)
{
	int nsyms = (symsize / sizeof(Elf_Sym));

	/* Sanity check for pre-allocated map table used during startup. */
	if ((nmap == ksyms_nmap) && (nsyms >= KSYMS_MAX_ID)) {
		printf("kern_ksyms: ERROR %d > %d, increase KSYMS_MAX_ID\n",
		    nsyms, KSYMS_MAX_ID);

		/* truncate for now */
		nsyms = KSYMS_MAX_ID - 1;
	}

	symtab_add(tab, name, symstart, symsize, strstart, strsize, ctfstart, ctfsize, FALSE, nsyms, nmap);

#ifdef KSYMS_DEBUG
	printf("newstart %p sym %p ksyms_symsz %zu str %p strsz %zu send %p\n",
	    newstart, symstart, symsize, strstart, strsize,
	    tab->sd_strstart + tab->sd_strsize);
#endif

	if (nmap) {
		memset(nmap, 0, nsyms * sizeof(uint32_t));
	}

	/* Pack symbol table by removing all file name references. */
	symbtab_pack(tab, newstart, nsyms);

	/* ksymsread() is unlocked, so membar. */
	TAILQ_INSERT_TAIL(&ksyms_symtabs, tab, sd_queue);
	ksyms_sizes_calc();
	ksyms_loaded = TRUE;
}

void
ksyms_delsymtab(void)
{
	struct ksyms_symtab *st, *next;
	bool_t resize;

	/* Discard references to symbol tables. */
	simple_lock(&ksyms_lock);
	ksyms_isopen = FALSE;
	resize = FALSE;
	for (st = TAILQ_FIRST(&ksyms_symtabs); st != NULL; st = next) {
		next = TAILQ_NEXT(st, sd_queue);
		if (st->sd_gone) {
			TAILQ_REMOVE(&ksyms_symtabs, st, sd_queue);
			free(st->sd_nmap, M_DEVBUF);
			//free(st->sd_nmapsize, M_DEVBUF);
			free(st, M_DEVBUF);
			resize = TRUE;
		}
	}
	if (resize) {
		ksyms_sizes_calc();
	}
	simple_unlock(&ksyms_lock);
}

/*
 * Setup the kernel symbol table stuff.
 */
void
ksyms_addsyms_elf(int symsize, void *start, void *end)
{
	int i, j;
	Elf_Shdr *shdr;
	char *symstart = NULL, *strstart = NULL;
	size_t strsize = 0;
	Elf_Ehdr *ehdr;
	char *ctfstart = NULL;
	size_t ctfsize = 0;

	if (symsize <= 0) {
		printf("[ Kernel symbol table missing! ]\n");
		return;
	}

	/* Sanity check */
	if (ALIGNED_POINTER(start, sizeof(long)) == 0) {
		printf("[ Kernel symbol table has bad start address %p ]\n", start);
		return;
	}

	ehdr = (Elf_Ehdr*) start;

	/* check if this is a valid ELF header */
	/* No reason to verify arch type, the kernel is actually running! */
	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) || ehdr->e_ident[EI_CLASS] != ELFCLASS || ehdr->e_version > 1) {
		printf("[ Kernel symbol table invalid! ]\n");
		return; /* nothing to do */
	}

	/* Loaded header will be scratched in addsymtab */
	ksyms_hdr_init(start);

	/* Find the symbol table and the corresponding string table. */
	shdr = (Elf_Shdr*) ((uint8_t*) start + ehdr->e_shoff);
	for (i = 1; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_SYMTAB)
			continue;
		if (shdr[i].sh_offset == 0)
			continue;
		symstart = (uint8_t*) start + shdr[i].sh_offset;
		symsize = shdr[i].sh_size;
		j = shdr[i].sh_link;
		if (shdr[j].sh_offset == 0)
			continue; /* Can this happen? */
		strstart = (uint8_t*) start + shdr[j].sh_offset;
		strsize = shdr[j].sh_size;
		break;
	}

	if (!ksyms_verify(symstart, strstart))
		return;

	addsymtab(symtab_name, symstart, symsize, strstart, strsize, &kernel_symtab, symstart, ctfstart, ctfsize, ksyms_nmap);

#ifdef DEBUG
	printf("Loaded initial symtab at %p, strtab at %p, # entries %ld\n",
	    kernel_symtab.sd_symstart, kernel_symtab.sd_strstart,
	    (long)kernel_symtab.sd_symsize/sizeof(Elf_Sym));
#endif
}

/*
 * Setup the kernel symbol table stuff.
 * Use this when the address of the symbol and string tables are known;
 * otherwise use ksyms_init with an ELF image.
 * We need to pass a minimal ELF header which will later be completed by
 * ksyms_hdr_init and handed off to userland through /dev/ksyms.  We use
 * a void *rather than a pointer to avoid exposing the Elf_Ehdr type.
 */
void
ksyms_addsyms_explicit(void *ehdr, void *symstart, size_t symsize, void *strstart, size_t strsize)
{
	if (!ksyms_verify(symstart, strstart))
		return;

	ksyms_hdr_init(ehdr);
	addsymtab(symtab_name, symstart, symsize, strstart, strsize, &kernel_symtab, symstart, NULL, 0, ksyms_nmap);
}

/*
 * In case we exposing the symbol table to the userland using the pseudo-
 * device /dev/ksyms, it is easier to provide all the tables as one.
 * However, it means we have to change all the st_name fields for the
 * symbols so they match the ELF image that the userland will read
 * through the device.
 *
 * The actual (correct) value of st_name is preserved through a global
 * offset stored in the symbol table structure.
 *
 * Call with ksyms_lock held.
 */
static void
ksyms_sizes_calc(void)
{
	struct ksyms_symtab *st;
	int i, delta;

	ksyms_symsz = ksyms_strsz = 0;
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (__predict_false(st->sd_gone))
			continue;
		delta = ksyms_strsz - st->sd_usroffset;
		if (delta != 0) {
			for (i = 0; i < st->sd_symsize/sizeof(Elf_Sym); i++)
				st->sd_symstart[i].st_name += delta;
			st->sd_usroffset = ksyms_strsz;
		}
		ksyms_symsz += st->sd_symsize;
		ksyms_strsz += st->sd_strsize;
	}
}

static void
ksyms_fill_note(void)
{
	int32_t *note = ksyms_hdr.kh_note;
	note[0] = ELF_NOTE_211BSD_NAMESZ;
	note[1] = ELF_NOTE_211BSD_DESCSZ;
	note[2] = ELF_NOTE_TYPE_211BSD_TAG;
	memcpy(&note[3], note_name, 8);
	note[5] = __211BSD_Version__;
}

static void
ksyms_hdr_init(const void *hdraddr)
{
	/* Copy the loaded elf exec header */
	memcpy(&ksyms_hdr.kh_ehdr, hdraddr, sizeof(Elf_Ehdr));

	/* Set correct program/section header sizes, offsets and numbers */
	ksyms_hdr.kh_ehdr.e_phoff = offsetof(struct ksyms_hdr, kh_phdr[0]);
	ksyms_hdr.kh_ehdr.e_phentsize = sizeof(Elf_Phdr);
	ksyms_hdr.kh_ehdr.e_phnum = NPRGHDR;
	ksyms_hdr.kh_ehdr.e_shoff = offsetof(struct ksyms_hdr, kh_shdr[0]);
	ksyms_hdr.kh_ehdr.e_shentsize = sizeof(Elf_Shdr);
	ksyms_hdr.kh_ehdr.e_shnum = NSECHDR;
	ksyms_hdr.kh_ehdr.e_shstrndx = SHSTRTAB;

	/* Text/data - fake */
	ksyms_hdr.kh_phdr[0].p_type = PT_LOAD;
	ksyms_hdr.kh_phdr[0].p_memsz = (unsigned long)-1L;
	ksyms_hdr.kh_phdr[0].p_flags = PF_R | PF_X | PF_W;

#define SHTCOPY(name)  strlcpy(&ksyms_hdr.kh_strtab[offs], (name), \
    sizeof(ksyms_hdr.kh_strtab) - offs), offs += sizeof(name)

	uint32_t offs = 1;
	/* First section header ".note.netbsd.ident" */
	ksyms_hdr.kh_shdr[SHNOTE].sh_name = offs;
	ksyms_hdr.kh_shdr[SHNOTE].sh_type = SHT_NOTE;
	ksyms_hdr.kh_shdr[SHNOTE].sh_offset = offsetof(struct ksyms_hdr, kh_note[0]);
	ksyms_hdr.kh_shdr[SHNOTE].sh_size = sizeof(ksyms_hdr.kh_note);
	ksyms_hdr.kh_shdr[SHNOTE].sh_addralign = sizeof(int);
	SHTCOPY(".note.netbsd.ident");
	ksyms_fill_note();

	/* Second section header; ".symtab" */
	ksyms_hdr.kh_shdr[SYMTAB].sh_name = offs;
	ksyms_hdr.kh_shdr[SYMTAB].sh_type = SHT_SYMTAB;
	ksyms_hdr.kh_shdr[SYMTAB].sh_offset = sizeof(struct ksyms_hdr);
/*	ksyms_hdr.kh_shdr[SYMTAB].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[SYMTAB].sh_link = STRTAB; /* Corresponding strtab */
	ksyms_hdr.kh_shdr[SYMTAB].sh_addralign = sizeof(long);
	ksyms_hdr.kh_shdr[SYMTAB].sh_entsize = sizeof(Elf_Sym);
	SHTCOPY(".symtab");

	/* Third section header; ".strtab" */
	ksyms_hdr.kh_shdr[STRTAB].sh_name = offs;
	ksyms_hdr.kh_shdr[STRTAB].sh_type = SHT_STRTAB;
/*	ksyms_hdr.kh_shdr[STRTAB].sh_offset = filled in at open */
/*	ksyms_hdr.kh_shdr[STRTAB].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[STRTAB].sh_addralign = sizeof(char);
	SHTCOPY(".strtab");

	/* Fourth section, ".shstrtab" */
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_name = offs;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_type = SHT_STRTAB;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_offset = offsetof(struct ksyms_hdr, kh_strtab);
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_size = SHSTRSIZ;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_addralign = sizeof(char);
	SHTCOPY(".shstrtab");

	/* Fifth section, ".bss". All symbols reside here. */
	ksyms_hdr.kh_shdr[SHBSS].sh_name = offs;
	ksyms_hdr.kh_shdr[SHBSS].sh_type = SHT_NOBITS;
	ksyms_hdr.kh_shdr[SHBSS].sh_offset = 0;
	ksyms_hdr.kh_shdr[SHBSS].sh_size = (unsigned long)-1L;
	ksyms_hdr.kh_shdr[SHBSS].sh_addralign = PAGE_SIZE;
	ksyms_hdr.kh_shdr[SHBSS].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	SHTCOPY(".bss");

	/* Sixth section header; ".SUNW_ctf" */
	ksyms_hdr.kh_shdr[SHCTF].sh_name = offs;
	ksyms_hdr.kh_shdr[SHCTF].sh_type = SHT_PROGBITS;
/*	ksyms_hdr.kh_shdr[SHCTF].sh_offset = filled in at open */
/*	ksyms_hdr.kh_shdr[SHCTF].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[SHCTF].sh_link = SYMTAB; /* Corresponding symtab */
	ksyms_hdr.kh_shdr[SHCTF].sh_addralign = sizeof(char);
	SHTCOPY(".SUNW_ctf");
}
