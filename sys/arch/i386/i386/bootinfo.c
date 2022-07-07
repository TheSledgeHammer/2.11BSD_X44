/*
 * Copyright (c) 2022 Martin Kelly
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bootinfo.h>
#include <machine/param.h>
#include <machine/vmparam.h>

char bootsize[BOOTINFO_MAXSIZE];
int  *esym;

void
bootinfo_init_ksyms(envp)
	struct bootinfo_enivronment envp;
{
	extern int 		end;
	vm_offset_t 	addend;

	if (envp.bi_environment != 0) {
		ksyms_addsyms_elf(*(int*) &end, ((int*) &end) + 1, esym);
		addend = (caddr_t) envp.bi_environment < KERNBASE ? PMAP_MAP_LOW : 0;
		init_static_kenv((char *)envp.bi_environment + addend, 0);
	} else {
		ksyms_addsyms_elf(*(int*) &end, ((int*) &end) + 1, esym);
		init_static_kenv(NULL, 0);
	}

	envp.bi_symtab += KERNBASE;
	envp.bi_esymtab += KERNBASE;
	ksyms_addsyms_elf(envp.bi_nsymtab, (int*)envp.bi_symtab, (int*)envp.bi_esymtab);
}

void
bootinfo_init_environment(envp, ksyms_elf_arg)
	struct bootinfo_enivronment envp;
	int (*ksyms_elf_arg)(struct bootinfo *);
{
	vm_offset_t addend;

	if (ksyms_elf_arg) {
		bootinfo_init_ksyms(envp);
	} else {
		if (envp.bi_environment != 0) {
			addend = (caddr_t)envp.bi_environment < KERNBASE ? PMAP_MAP_LOW : 0;
			init_static_kenv((char *)envp.bi_environment + addend, 0);
		} else {
			init_static_kenv(NULL, 0);
		}
	}
}

void
bootinfo_startup(boot, ksyms_elf_arg)
	struct bootinfo 	*boot;
	int (*ksyms_elf_arg)(struct bootinfo *);
{
	struct bootinfo *bt;

	bt = boot;
	if(bt == NULL) {
		bt = bootinfo_alloc(bootsize, sizeof(int));
	}

	bootinfo_init_environment(bt->bi_envp, ksyms_elf_arg);
}

struct bootinfo *
bootinfo_alloc(size, len)
	char *size;
	int len;
{
	struct bootinfo *boot;
	boot = (struct bootinfo *)(size + len);

	return (boot);
}

void *
bootinfo_lookup(type)
	int type;
{
	struct bootinfo *help;
	int n = *(int*)bootsize;
	help = bootinfo_alloc(bootsize, sizeof(int));

	while (n--) {
		if (help->bi_type == type) {
			return (help);
		}
		help = bootinfo_alloc((char *)help, help->bi_len);
	}
	return (0);
}
