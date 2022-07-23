/*	$NetBSD: subr_kobj.c,v 1.66 2018/06/23 14:22:30 jakllsch Exp $	*/

/*
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

/*
 * Kernel loader for ELF objects.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec_elf.h>
#include <sys/ksyms.h>
#include <vm/include/vm_extern.h>

#include <dev/misc/kobj/kobj.h>
#include <dev/misc/kobj/kobj_impl.h>

#define kobj_error(_kobj, ...) kobj_out(__func__, __LINE__, _kobj, __VA_ARGS__)

static int	kobj_relocate(kobj_t, bool);
static void	kobj_out(const char *, int, kobj_t, const char *, ...);

/*
 * kobj_sym_lookup:
 *
 *	Symbol lookup function to be used when the symbol index
 *	is known (ie during relocation).
 */
int
kobj_sym_lookup(ko, symidx, val)
	kobj_t ko;
	uintptr_t symidx;
	Elf_Addr *val;
{
	const Elf_Sym *sym;
	const char *symbol;

	sym = ko->ko_symtab + symidx;

	if (symidx == SHN_ABS) {
		*val = (uintptr_t)sym->st_value;
		return 0;
	} else if (symidx >= ko->ko_symcnt) {
		/*
		 * Don't even try to lookup the symbol if the index is
		 * bogus.
		 */
		kobj_error(ko, "symbol index out of range");
		return EINVAL;
	}

	/* Quick answer if there is a definition included. */
	if (sym->st_shndx != SHN_UNDEF) {
		*val = (uintptr_t)sym->st_value;
		return 0;
	}

	/* If we get here, then it is undefined and needs a lookup. */
	switch (ELF_ST_BIND(sym->st_info)) {
	case STB_LOCAL:
		/* Local, but undefined? huh? */
		kobj_error(ko, "local symbol undefined");
		return EINVAL;

	case STB_GLOBAL:
		/* Relative to Data or Function name */
		symbol = ko->ko_strtab + sym->st_name;

		/* Force a lookup failure if the symbol name is bogus. */
		if (*symbol == 0) {
			kobj_error(ko, "bad symbol name");
			return EINVAL;
		}
		if (sym->st_value == 0) {
			kobj_error(ko, "bad value");
			return EINVAL;
		}

		*val = (uintptr_t)sym->st_value;
		return 0;

	case STB_WEAK:
		kobj_error(ko, "weak symbols not supported");
		return EINVAL;

	default:
		return EINVAL;
	}
}

/*
 * kobj_findbase:
 *
 *	Return base address of the given section.
 */
static uintptr_t
kobj_findbase(ko, sec)
	kobj_t ko;
	int sec;
{
	int i;

	for (i = 0; i < ko->ko_nprogtab; i++) {
		if (sec == ko->ko_progtab[i].sec) {
			return (uintptr_t)ko->ko_progtab[i].addr;
		}
	}
	return 0;
}

void
kobj_self_reloc(ko)
	kobj_t ko;
{
	int error;

	error = kobj_relocate(ko, FALSE);
	if (error != 0) {
		error = kobj_relocate(ko, TRUE);
		if (error != 0) {
			kobj_error(ko, "self relocation error");
		} else {
			goto pass;
		}
	} else {
		goto pass;
	}

pass:
	printf("self relocation successful: %s", ko->ko_name);
	printf("\n");
}

/*
 * kobj_relocate:
 *
 *	Resolve relocations for the loaded object.
 */
static int
kobj_relocate(ko, local)
	kobj_t ko;
	bool local;
{
	const Elf_Rel *rellim;
	const Elf_Rel *rel;
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	const Elf_Sym *sym;
	uintptr_t base;
	int i, error;
	uintptr_t symidx;

	/*
	 * Perform relocations without addend if there are any.
	 */
	for (i = 0; i < ko->ko_nrel; i++) {
		rel = ko->ko_reltab[i].rel;
		if (rel == NULL) {
			continue;
		}
		rellim = rel + ko->ko_reltab[i].nrel;
		base = kobj_findbase(ko, ko->ko_reltab[i].sec);
		if (base == 0) {
			panic("%s:%d: %s: lost base for e_reltab[%d] sec %d", __func__, __LINE__, ko->ko_name, i, ko->ko_reltab[i].sec);
		}
		for (; rel < rellim; rel++) {
			symidx = ELF_R_SYM(rel->r_info);
			if (symidx >= ko->ko_symcnt) {
				continue;
			}
			sym = ko->ko_symtab + symidx;
			if (local != (ELF_ST_BIND(sym->st_info) == STB_LOCAL)) {
				continue;
			}
			error = kobj_reloc(ko, base, rel, false, local);
			if (error != 0) {
				return (ENOENT);
			}
		}
	}

	/*
	 * Perform relocations with addend if there are any.
	 */
	for (i = 0; i < ko->ko_nrela; i++) {
		rela = ko->ko_relatab[i].rela;
		if (rela == NULL) {
			continue;
		}
		relalim = rela + ko->ko_relatab[i].nrela;
		base = kobj_findbase(ko, ko->ko_relatab[i].sec);
		if (base == 0) {
			panic("%s:%d: %s: lost base for e_relatab[%d] sec %d", __func__, __LINE__, ko->ko_name, i, ko->ko_relatab[i].sec);
		}
		for (; rela < relalim; rela++) {
			symidx = ELF_R_SYM(rela->r_info);
			if (symidx >= ko->ko_symcnt) {
				continue;
			}
			sym = ko->ko_symtab + symidx;
			if (local != (ELF_ST_BIND(sym->st_info) == STB_LOCAL)) {
				continue;
			}
			error = kobj_reloc(ko, base, rela, TRUE, local);
			if (error != 0) {
				return (ENOENT);
			}
		}
	}

	return (0);
}

/*
 * kobj_out:
 *
 *	Utility function: log an error.
 */
static void
kobj_out(const char *fname, int lnum, kobj_t ko, const char *fmt, ...)
{
	va_list ap;

	printf("%s, %d: [%s]: linker error: ", fname, lnum, ko->ko_name);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}
