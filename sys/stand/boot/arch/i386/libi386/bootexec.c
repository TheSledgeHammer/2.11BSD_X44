/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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

#include <sys/param.h>
#include <sys/boot.h>
#include <sys/exec.h>
#include <sys/user.h>

#include <lib/libsa/loadfile.h>
#include <i386/include/bootinfo.h>
#include <lib/libsa/stand.h>

#include "bootstrap.h"
#include "libi386.h"
#include "btxv86.h"

static int boot_loadfile(char *args, uint64_t dest, struct preloaded_file *fp);
static int boot_exec(struct preloaded_file *fp);

struct bootinfo boot;
struct file_format i386_boot = { boot_loadfile, boot_exec };

static int
boot_loadfile(char *args, uint64_t dest, struct preloaded_file *fp)
{
	int error;

#ifdef BOOT_AOUT
	error = aout_loadfile(args, dest, fp);
	if (error != 0) {
		printf("aout_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_ECOFF
	error = ecoff_loadfile(args, dest, fp);
	if (error != 0) {
		printf("ecoff_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_ELF32
	error = elf32_loadfile(args, dest, fp);
	if (error != 0) {
		printf("elf32_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_ELF64
	error = elf64_loadfile(args, dest, fp);
	if (error != 0) {
		printf("elf64_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_XCOFF32
	error = xcoff32_loadfile(args, dest, fp);
	if (error != 0) {
		printf("xcoff32_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_XCOFF64
	error = xcoff64_loadfile(args, dest, fp);
	if (error != 0) {
		printf("xcoff64_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif

	/*
	 * f_addr is already aligned to PAGE_SIZE, make sure
	 * f_size it's also aligned so when the modules are loaded
	 * they are aligned to PAGE_SIZE.
	 */
	(*fp)->f_size = roundup((*fp)->f_size, PAGE_SIZE);

out:
  return (error);
}

static int
boot_exec(struct preloaded_file *fp)
{
  int				 	error;
  vm_offset_t			entry, last_addr;

  #ifdef BOOT_AOUT
	fp = file_findfile(NULL, AOUT_KERNELTYPE);
#endif
#ifdef BOOT_ECOFF
	fp = file_findfile(NULL, ECOFF_KERNELTYPE);
#endif
#ifdef BOOT_ELF32 && ELFSIZE == 32
	fp = file_findfile(NULL, ELF32_KERNELTYPE);
#endif
#ifdef BOOT_ELF64 && ELFSIZE == 64
	fp = file_findfile(NULL, ELF64_KERNELTYPE);
#endif
#ifdef BOOT_XCOFF32 && XCOFFSIZE == 32
	fp = file_findfile(NULL, XCOFF32_KERNELTYPE);
#endif
#ifdef BOOT_XCOFF64 && XCOFFSIZE == 64
	fp = file_findfile(NULL, XCOFF64_KERNELTYPE);
#endif

	if (fp == NULL) {
		error = EINVAL;
		goto error;
	}

	last_addr = roundup(max_addr(), PAGE_SIZE);

	error = bi_load_stage1(&boot, fp, fp->f_args, last_addr, 0);
	if (error != 0) {
		printf("bi_load_stage1 failed: %d\n", error);
		goto error;
	}

	if(preload_ksyms(&boot, fp)) {
		entry = preload_ksyms(&boot, fp);
#ifdef DEBUG
    printf("Start @ 0x%lx ...\n", entry);
#endif
		&boot->bi_sym.bi_flags = fp->f_flags;
		dev_cleanup();
		__exec((void *) entry,  &boot->bi_leg.bi_howtop, &boot->bi_leg.bi_bootdevp, 0, 0, 0, &boot->bi_leg.bi_bip, &boot->bi_envp.bi_kernend);
	} else if (!preload_ksyms(fp)) {
		entry = &boot->bi_entry & 0xffffff;
#ifdef DEBUG
    printf("Start @ 0x%lx ...\n", entry);
#endif
		dev_cleanup();
		__exec((void *) entry, &boot->bi_leg.bi_howtop, &boot->bi_leg.bi_bootdevp, 0, 0, 0, &boot->bi_leg.bi_bip, &boot->bi_envp.bi_kernend);
	} else {
#ifdef DEBUG
    printf("Start @ 0x%lx ...\n", &boot->bi_entry);
#endif
		dev_cleanup();
		__exec((void *) &boot->bi_entry,  &boot->bi_leg.bi_howtop, &boot->bi_leg.bi_bootdevp, 0, 0, 0, &boot->bi_leg.bi_bip, &boot->bi_envp.bi_kernend);
	}

error:
    panic("exec returned");
    return (error);
}

static vm_offset_t
max_addr(void)
{
	struct preloaded_file	*fp;
	vm_offset_t		 addr = 0;

	for (fp = file_findfile(NULL, NULL); fp != NULL; fp = fp->f_next) {
		if (addr < (fp->f_addr + fp->f_size))
			addr = fp->f_addr + fp->f_size;
	}
	return (addr);
}
