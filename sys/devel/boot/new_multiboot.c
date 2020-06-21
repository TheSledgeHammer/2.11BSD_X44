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

#include "bootstrap.h"
#include "new_multiboot.h"
#include "libi386.h"
#include <btxv86.h>

static const char mbl_name[] = "FreeBSD Loader";

static int
multiboot_exec(struct multiboot_package *mbp)
{
	struct multiboot_info	*mbi;
	int 					i, len;
	char 					*cmdline;

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

	if (mbp->mbp_args) {
		mbi->mi_flags |= MULTIBOOT_INFO_CMDLINE;
		len = strlen(mbp->mbp_file) + 1 + strlen(mbp->mbp_args) + 1;
		cmdline = alloc(len);
		snprintf(cmdline, len, "%s %s", mbp->mbp_file, mbp->mbp_args);
		mbi->mi_cmdline = (char *) vtophys(cmdline);
	}

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
		vpbcopy(shbuf, (void*) shaddr, shlen);

		dealloc(shbuf, shlen);

		mbi->mi_elfshdr_num = ehdr.e_shnum;
		mbi->mi_elfshdr_size = ehdr.e_shentsize;
		mbi->mi_elfshdr_addr = shaddr;
		mbi->mi_elfshdr_shndx = ehdr.e_shstrndx;

		mbi->mi_flags |= MULTIBOOT_INFO_ELF_SYMS;
	}
error:

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
