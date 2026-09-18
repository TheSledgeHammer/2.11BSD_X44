/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

#include <sys/param.h>
#include <sys/boot.h>
#include <sys/exec.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <bootstrap.h>
#include <libi386.h>
#include <btxv86.h>

#include <machine/bootinfo.h>

struct bootinfo boot;

#ifdef multiboot
#if defined(BOOT_ELF32) || defined(BOOT_ELF64)
static int preload_ksyms(struct bootinfo *, struct preloaded_file *);
#endif
#endif

int
boot_loadfile(char *filename, char *kerntype, uint64_t dest, struct preloaded_file **fp)
{
	size_t size;
	int error;

	error = exec_loadfile(filename, kerntype, dest, LOAD_KERNEL, fp);
	if (error != 0) {
		printf("boot_loadfile failed: %d unable to load kernel\n", error);
		goto out;
	}
	/*
	 * f_addr is already aligned to PAGE_SIZE, make sure
	 * f_size it's also aligned so when the modules are loaded
	 * they are aligned to PAGE_SIZE.
	 */
	size = roundup((*fp)->f_size, PAGE_SIZE);
	(*fp)->f_size = size;

out:
 	return (error);
}

int
boot_exec(struct preloaded_file *fp, char *kerntype)
{
	vm_offset_t entry;
	int error;

	error = bi_load(&boot, fp, kerntype, fp->f_args);
	if (error != 0) {
		printf("bi_load failed: %d\n", error);
		goto out;
	}

	entry = fp->f_marks[MARK_ENTRY] & 0xffffff;

#ifdef DEBUG
    printf("Start @ 0x%lx ...\n", entry);
#endif

    dev_cleanup();
	__exec((void *)entry, &boot.bi_howtop, &boot.bi_bootdevp, 0, 0, 0,
			&boot.bi_bip, &boot.bi_kernend);

out:
	panic("exec returned");
	return (error);
}

/*
 * Multiboot
 */
#ifdef multiboot
#if defined(BOOT_ELF32) || defined(BOOT_ELF64)
static int
preload_ksyms(struct bootinfo *bi, struct preloaded_file *fp)
{
	fp->f_flags = BOOTINFO_MEMORY;

	fp->f_mem_upper = bi->bi_extmem;
	fp->f_mem_lower = bi->bi_basemem;

	if (fp->f_marks[MARK_SYM] != 0) {
		Elf32_Ehdr ehdr;
		void *shbuf, *basekern;
		size_t shlen;
		u_long shaddr;

		bcopy((void *)fp->f_marks[MARK_SYM], &ehdr, sizeof(ehdr));

		if (memcmp(&ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
			goto skip_ksyms;
		}

		shaddr = fp->f_marks[MARK_SYM] + ehdr.e_shoff;

		shlen = ehdr.e_shnum * ehdr.e_shentsize;
		shbuf = alloc(shlen);

		basekern = (void *)(KERNBASE + fp->f_marks[MARK_SYM]);
		bcopy((void *)shaddr, shbuf, shlen);
		ksyms_addr_set(&ehdr, shbuf, basekern);
		bcopy(shbuf, (void *)shaddr, shlen);

		free(shbuf, shlen);

		fp->f_elfshdr_num = ehdr.e_shnum;
		fp->f_elfshdr_size = ehdr.e_shentsize;
		fp->f_elfshdr_addr = shaddr;
		fp->f_elfshdr_shndx = ehdr.e_shstrndx;

		fp->f_flags |= BOOTINFO_ELF_SYMS;
	}

skip_ksyms:
#ifdef DEBUG
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n",
			fp->f_marks[MARK_ENTRY],
			fp->f_marks[MARK_NSYM],
			fp->f_marks[MARK_SYM],
			fp->f_marks[MARK_END]);
#endif

	return (0);
}
#endif /* !BOOT_ELF32 || !BOOT_ELF64 */
#endif
