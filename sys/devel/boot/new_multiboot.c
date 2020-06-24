/*
 * new_multiboot.c
 *
 *  Created on: 21 Jun 2020
 *      Author: marti
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/user.h>
#include <sys/stdint.h>

#include <string.h>
#include <bootstand.h>

#include <libsa/loadfile.h>
#include <i386/include/loadfile_machdep.h>

#include "common/bootstrap.h"
#include "new_multiboot.h"
#include "libi386.h"
#include <btxv86.h>

#define MULTIBOOT_SUPPORTED_FLAGS \
				(MULTIBOOT_PAGE_ALIGN|MULTIBOOT_MEMORY_INFO)

static int multiboot_loadfile(char *args, uint64_t dest, struct preloaded_file *fp);
static int multiboot_exec(struct preloaded_file *);

struct file_format multiboot = 		{ multiboot_loadfile, multiboot_exec };

extern void multiboot_tramp();

static const char mbl_name[] = "211BSD Loader";

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

static int
multiboot_loadfile(char *args, uint64_t dest, struct preloaded_file *fp)
{
	struct multiboot_header	*mbh;
	uint32_t				*magic;
	int			 			i, error;
	caddr_t				 	header_search;
	ssize_t					search_size;
	int			 			fd;
	char					*cmdline;

	/*
	 * Read MULTIBOOT_SEARCH size in order to search for the
	 * multiboot magic header.
	 */
	if (args == NULL)
		return (EFTYPE);
	if ((fd = open(args, O_RDONLY)) == -1)
		return (errno);
	header_search = malloc(MULTIBOOT_SEARCH);
	if (header_search == NULL) {
		close(fd);
		return (ENOMEM);
	}
	search_size = read(fd, header_search, MULTIBOOT_SEARCH);
	magic = (uint32_t*) header_search;

	mbh = NULL;
	for (i = 0; i < (search_size / sizeof(uint32_t)); i++) {
		if (magic[i] == MULTIBOOT_HEADER_MAGIC) {
			mbh = (struct multiboot_header*) &magic[i];
			break;
		}
	}

	if (mbh == NULL) {
		error = EFTYPE;
		goto out;
	}

	/* Valid multiboot header has been found, validate checksum */
	if (mbh->magic + mbh->flags + mbh->checksum != 0) {
		printf("Multiboot checksum failed, magic: 0x%x flags: 0x%x checksum: 0x%x\n", mbh->magic, mbh->flags, mbh->checksum);
		error = EFTYPE;
		goto out;
	}

	if ((mbh->flags & ~MULTIBOOT_SUPPORTED_FLAGS) != 0) {
		printf("Unsupported multiboot flags found: 0x%x\n", mbh->flags);
		error = EFTYPE;
		goto out;
	}

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

	/*
	 * f_addr is already aligned to PAGE_SIZE, make sure
	 * f_size it's also aligned so when the modules are loaded
	 * they are aligned to PAGE_SIZE.
	 */
	(*fp)->f_size = roundup((*fp)->f_size, PAGE_SIZE);

out:
	free(header_search);
	close(fd);
	return (error);
}


static int
multiboot_exec(struct preloaded_file *fp)
{
	struct multiboot_package 	*mbp;
	struct multiboot_info 		*mbi;
	int 						i, len;
	char 						*cmdline;
	int				 			error;
	vm_offset_t					last_addr;

	/*
	 * Don't pass the memory size found by the bootloader, the memory
	 * available to Dom0 will be lower than that.
	 */
	unsetenv("smbios.memory.enabled");

	/* Allocate the multiboot struct and fill the basic details. */
	mbi = malloc(sizeof(struct multiboot_info));
	if (mbi == NULL) {
		error = ENOMEM;
		goto error;
	}

	bzero(mbi, sizeof(struct multiboot_info));
	mbi->flags = MULTIBOOT_INFO_MEMORY | MULTIBOOT_INFO_BOOT_LOADER_NAME;
	mbi->mem_lower = bios_basemem / 1024;
	mbi->mem_upper = bios_extmem / 1024;
	mbi->boot_loader_name = VTOP(mbl_name);

	if (fp->f_args == NULL) {
		if (cmdline != NULL) {
			fp->f_args = strdup(cmdline);
			if (fp->f_args == NULL) {
				error = ENOMEM;
				goto error;
			}
		}
	}
	if (fp->f_args != NULL) {
		len = strlen(fp->f_name) + 1 + strlen(fp->f_args) + 1;
		cmdline = alloc(len);
		if (cmdline == NULL) {
			error = ENOMEM;
			goto error;
		}
		snprintf(cmdline, len, "%s %s", fp->f_name, fp->f_args);
		mbi->mi_cmdline = VTOP(cmdline);
		mbi->mi_flags |= MULTIBOOT_INFO_CMDLINE;
	}

	mbp = malloc(sizeof(struct multiboot_package));
	if(mbp == NULL) {
		error = ENOMEM;
		goto error;
	}
	mbp->mbp_file = fp->f_name;
	mbp->mbp_args = fp->f_args;
	mbp->mbp_marks = fp->f_marks;

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

	if (fp == NULL) {
		error = EINVAL;
		goto error;
	}

	last_addr = roundup(max_addr(), PAGE_SIZE);

	error = bi_load1(fp, fp->f_args, last_addr, 0);
	if (error != 0) {
		printf("bi_load1 failed: %d\n", error);
		goto error;
	}

	dev_cleanup();
	//__exec((void *)VTOP(multiboot_tramp), (void *)entry, (void *)VTOP(mbi));

	panic("exec returned");

error:
	if (mbi) {
		free(mbi);
	}
	if (cmdline) {
		free(cmdline);
	}
	if (mbp) {
		free(mbp);
	}
	return (error);
}

static int
multiboot_ksyms(struct preloaded_file *fp)
{
	struct multiboot_package 	*mbp;
	struct multiboot_info		*mbi;
	int				 			error;

	if (mbp->mbp_marks[MARK_SYM] != 0) {
		Elf32_Ehdr ehdr;
		void *shbuf;
		size_t shlen;
		u_long shaddr;

		bcopy((void *)mbp->mbp_marks[MARK_SYM], &ehdr, sizeof(ehdr));

		if (memcmp(&ehdr.e_ident, ELFMAG, SELFMAG) != 0)
			goto skip_ksyms;

		shaddr = mbp->mbp_marks[MARK_SYM] + ehdr.e_shoff;

		shlen = ehdr.e_shnum * ehdr.e_shentsize;
		shbuf = alloc(shlen);

		bcopy((void*) shaddr, shbuf, shlen);
		ksyms_addr_set(&ehdr, shbuf, (void*) (KERNBASE + mbp->mbp_marks[MARK_SYM]));
		bcopy(shbuf, (void*) shaddr, shlen);

		dealloc(shbuf, shlen);

		mbi->mi_elfshdr_num = ehdr.e_shnum;
		mbi->mi_elfshdr_size = ehdr.e_shentsize;
		mbi->mi_elfshdr_addr = shaddr;
		mbi->mi_elfshdr_shndx = ehdr.e_shstrndx;

		mbi->mi_flags |= MULTIBOOT_INFO_ELF_SYMS;
	}

skip_ksyms:
#ifdef DEBUG
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n",
	mbp->mbp_marks[MARK_ENTRY],
	mbp->mbp_marks[MARK_NSYM],
	mbp->mbp_marks[MARK_SYM],
	mbp->mbp_marks[MARK_END]);
#endif

	return (0);
}
