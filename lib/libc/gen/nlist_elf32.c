/* $NetBSD: nlist_elf32.c,v 1.24 2003/07/26 19:24:43 salo Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
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
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>

/* If not included by nlist_elf64.c, ELFSIZE won't be defined. */
#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#include "namespace.h"

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <nlist.h>			/* for 'struct nlist' declaration */

#include "nlist_private.h"
#if defined(NLIST_ELF32) || defined(NLIST_ELF64)
#include <sys/exec_elf.h>
#endif

#include <sys/ksyms.h>      /* after sys/exec_elf.h */

#if (defined(NLIST_ELF32) && (ELFSIZE == 32)) || \
    (defined(NLIST_ELF64) && (ELFSIZE == 64))

/* No need to check for off < 0 because it is unsigned */
#define	check(off, size)	(off + size > mappedsize)
#define	BAD			        goto out
#define	BADUNMAP		    goto unmap

int
ELFNAMEEND(__fdnlist)(fd, list)
	int fd;
	struct nlist *list;
{
	struct stat st;
    Elf_Ehdr ehdr;
#if defined(_LP64) || ELFSIZE == 32 || defined(ELF64_MACHDEP_ID)
#if (ELFSIZE == 32)
	Elf32_Half nshdr;
#elif (ELFSIZE == 64)
	Elf64_Half nshdr;
#endif
	Elf_Ehdr *ehdrp;
	Elf_Shdr *shdrp, *symshdrp, *symstrshdrp;
	Elf_Sym *symp;
	Elf_Off shdr_off;
	Elf_Word shdr_size;
	struct nlist *p;
	char *mappedfile, *strtab;
	size_t mappedsize, nsyms;
    int nent;
#endif
	size_t i;
	int rv;

	_DIAGASSERT(fd != -1);
	_DIAGASSERT(list != NULL);

	rv = -1;

	/*
	 * If we can't fstat() the file, something bad is going on.
	 */
	if (fstat(fd, &st) < 0)
		BAD;

	/*
	 * Map the file in its entirety.
	 */
	if (st.st_size > SIZE_T_MAX) {
		errno = EFBIG;
		BAD;
	}

	/*
	 * Read the elf header of the file.
	 */
	if ((ssize_t)(i = pread(fd, &ehdr, sizeof(Elf_Ehdr), (off_t)0)) == -1)
		BAD;

	/*
	 * Check that the elf header is correct.
	 */
	if (i != sizeof(Elf_Ehdr))
		BAD;
	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 ||
	    ehdr.e_ident[EI_CLASS] != ELFCLASS)
		BAD;

	switch (ehdr.e_machine) {
	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		BAD;
	}

#if defined(_LP64) || ELFSIZE == 32 || defined(ELF64_MACHDEP_ID)
    symshdrp = symstrshdrp = NULL;

	if (S_ISCHR(st.st_mode)) {
		const char *nlistname;
		Elf_Sym sym;

		/*
		 * Character device; assume /dev/ksyms.
		 */
		nent = 0;
		for (p = list; !ISLAST(p); ++p) {

			p->n_other = 0;
			p->n_desc = 0;
			nlistname = N_NAME(p);
			if (*nlistname == '_') {
				nlistname++;
            }
			p->n_value = sym.st_value;
			switch (ELFDEFNNAME(ST_TYPE)(sym.st_info)) {
			case STT_NOTYPE:
				p->n_type = N_UNDF;
				break;
			case STT_OBJECT:
				p->n_type = N_DATA;
				break;
			case STT_FUNC:
				p->n_type = N_TEXT;
				break;
			case STT_FILE:
				p->n_type = N_FN;
				break;
			default:
				p->n_type = 0;
				/* catch other enumerations for gcc */
				break;
			}
			if (ELFDEFNNAME(ST_BIND)(sym.st_info) != STB_LOCAL)
				p->n_type |= N_EXT;
			nent++;
			p->n_value = 0;
			p->n_type = 0;
		}
		return nent;
	}

	mappedsize = (size_t)st.st_size;
	mappedfile = mmap(NULL, mappedsize, PROT_READ, MAP_PRIVATE|MAP_FILE,
	    fd, (off_t)0);
	if (mappedfile == (char *)-1)
		BAD;

	/*
	 * Make sure we can access the executable's header
	 * directly, and make sure the recognize the executable
	 * as an ELF binary.
	 */
	if (check(0, sizeof *ehdrp))
		BADUNMAP;
	ehdrp = (Elf_Ehdr *)(void *)&mappedfile[0];

	/*
	 * Find the symbol list and string table.
	 */
	nshdr = ehdrp->e_shnum;
	shdr_off = ehdrp->e_shoff;
	shdr_size = ehdrp->e_shentsize * nshdr;

	if (check(shdr_off, shdr_size) ||
	    (sizeof *shdrp != ehdrp->e_shentsize))
		BADUNMAP;
	shdrp = (Elf_Shdr *)(void *)&mappedfile[shdr_off];

	for (i = 0; i < nshdr; i++) {
		if (shdrp[i].sh_type == SHT_SYMTAB) {
			symshdrp = &shdrp[i];
			symstrshdrp = &shdrp[shdrp[i].sh_link];
		}
	}

	/* Make sure we're not stripped. */
	if (symshdrp == NULL || symshdrp->sh_offset == 0)
		BADUNMAP;

	/* Make sure the symbols and strings are safely mapped. */
	if (check(symshdrp->sh_offset, symshdrp->sh_size))
		BADUNMAP;
	if (check(symstrshdrp->sh_offset, symstrshdrp->sh_size))
		BADUNMAP;

	symp = (void *)&mappedfile[(size_t)symshdrp->sh_offset];
	nsyms = (size_t)symshdrp->sh_size / sizeof(*symp);
	strtab = &mappedfile[(size_t)symstrshdrp->sh_offset];

	/*
	 * Clean out any left-over information for all valid entries.
	 * Type and value are defined to be 0 if not found; historical
	 * versions cleared other and desc as well.
	 *
	 * XXX Clearing anything other than n_type and n_value violates
	 * the semantics given in the man page.
	 */
	nent = 0;
	for (p = list; !ISLAST(p); ++p) {
		p->n_type = 0;
		p->n_other = 0;
		p->n_desc = 0;
		p->n_value = 0;
		++nent;
	}

	for (i = 0; i < nsyms; i++) {
		for (p = list; !ISLAST(p); ++p) {
			const char *nlistname;
			char *symtabname;

			/* This may be incorrect */
			nlistname = N_NAME(p);
			if (*nlistname == '_')
				nlistname++;

			symtabname = &strtab[symp[i].st_name];

			if (!strcmp(symtabname, nlistname)) {
				/*
				 * Translate (roughly) from ELF to nlist
				 */
				p->n_value = symp[i].st_value;
				switch (ELFDEFNNAME(ST_TYPE)(symp[i].st_info)) {
				case STT_NOTYPE:
					p->n_type = N_UNDF;
					break;
				case STT_OBJECT:
					p->n_type = N_DATA;
					break;
				case STT_FUNC:
					p->n_type = N_TEXT;
					break;
				case STT_FILE:
					p->n_type = N_FN;
					break;
				default:
					/* catch other enumerations for gcc */
					break;
				}
				if (ELFDEFNNAME(ST_BIND)(symp[i].st_info) !=
				    STB_LOCAL)
					p->n_type |= N_EXT;
				p->n_desc = 0;			/* XXX */
				p->n_other = 0;			/* XXX */

				if (--nent <= 0)
					goto done;
				break;	/* into next run of outer loop */
			}
		}
	}

done:
	rv = nent;
unmap:
	munmap(mappedfile, mappedsize);
#endif /* _LP64 || ELFSIZE == 32 || ELF64_MACHDEP_ID */
out:
	return (rv);
}

#endif
