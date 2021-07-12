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

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

#include <stand.h>
#include <string.h>
#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/exec_linker.h>
#include <machine/elf_machdep.h>
#include <machine/bootinfo.h>

#include <efi.h>
#include <efilib.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include "bootstrap.h"

static EFI_GUID hcdp = HCDP_TABLE_GUID;

/*
 * Return a 'boothowto' value corresponding to the kernel arguments in
 * (kargs) and any relevant environment variables.
 */
static struct 
{
    const char	*ev;
    int			mask;
} howto_names[] = {
    {"boot_askname",	RB_ASKNAME},
    {"boot_cdrom",		RB_CDROM},
    {"boot_ddb",		RB_KDB},
    {"boot_dfltroot",	RB_DFLTROOT},
    {"boot_gdb",		RB_GDB},
    {"boot_multicons",	RB_MULTIPLE},
    {"boot_mute",		RB_MUTE},
    {"boot_pause",		RB_PAUSE},
    {"boot_serial",		RB_SERIAL},
    {"boot_single",		RB_SINGLE},
    {"boot_verbose",	RB_VERBOSE},
    {NULL,	0}
};

extern char *efi_fmtdev(void *vdev);

int
bi_getboothowto(char *kargs)
{
    char	*cp;
    int		howto;
    int		active;
    int		i;
    
    /* Parse kargs */
	howto = 0;
	if (kargs != NULL) {
		cp = kargs;
		active = 0;
		while (*cp != 0) {
			if (!active && (*cp == '-')) {
				active = 1;
			} else if (active)
				switch (*cp) {
				case 'a':
					howto |= RB_ASKNAME;
					break;
				case 'C':
					howto |= RB_CDROM;
					break;
				case 'd':
					howto |= RB_KDB;
					break;
				case 'D':
					howto |= RB_MULTIPLE;
					break;
				case 'm':
					howto |= RB_MUTE;
					break;
				case 'g':
					howto |= RB_GDB;
					break;
				case 'h':
					howto |= RB_SERIAL;
					break;
				case 'p':
					howto |= RB_PAUSE;
					break;
				case 'r':
					howto |= RB_DFLTROOT;
					break;
				case 's':
					howto |= RB_SINGLE;
					break;
				case 'v':
					howto |= RB_VERBOSE;
					break;
				default:
					active = 0;
					break;
				}
			cp++;
		}
	}
	/* get equivalents from the environment */
	for (i = 0; howto_names[i].ev != NULL; i++)
		if (getenv(howto_names[i].ev) != NULL)
			howto |= howto_names[i].mask;
	if (!strcmp(getenv("console"), "comconsole"))
		howto |= RB_SERIAL;
	if (!strcmp(getenv("console"), "nullconsole"))
		howto |= RB_MUTE;
	return (howto);
}

/*
 * Copy the environment into the load area starting at (addr).
 * Each variable is formatted as <name>=<value>, with a single nul
 * separating each variable, and a double nul terminating the environment.
 */
vm_offset_t
bi_copyenv(vm_offset_t addr)
{
    struct env_var	*ep;
    
    /* traverse the environment */
	for (ep = environ; ep != NULL; ep = ep->ev_next) {
		efi_copyin(ep->ev_name, addr, strlen(ep->ev_name));
		addr += strlen(ep->ev_name);
		efi_copyin("=", addr, 1);
		addr++;
		if (ep->ev_value != NULL) {
			efi_copyin(ep->ev_value, addr, strlen(ep->ev_value));
			addr += strlen(ep->ev_value);
		}
		efi_copyin("", addr, 1);
		addr++;
	}
	efi_copyin("", addr, 1);
	addr++;
	return (addr);
}

/*
 * Load the information expected by an alpha kernel.
 *
 * - The kernel environment is copied into kernel space.
 * - Module metadata are formatted and placed in kernel space.
 */
int
bi_load(struct bootinfo *bi, struct preloaded_file *fp, UINTN *mapkey, UINTN pages)
{
    char					*rootdevname;
    struct efi_devdesc		*rootdev;
    struct preloaded_file	*xp;
    vm_offset_t				addr, bootinfo_addr;
    vm_offset_t				ssym, esym, nsym;
    EFI_STATUS				status;
    UINTN					bisz, key;

    /*
     * Version 1 bootinfo.
     */
    bi->bi_magic = BOOTINFO_MAGIC;
    bi->bi_version = 1;

    /*
     * Calculate boothowto.
     */
    bi->bi_boothowto = bi_getboothowto(fp->f_args);

    /*
     * Stash EFI System Table.
     */
    bi->bi_efi.bi_systab = (u_int64_t) ST;

    /* 
     * Allow the environment variable 'rootdev' to override the supplied
     * device. This should perhaps go to MI code and/or have $rootdev
     * tested/set by MI code before launching the kernel.
     */
	rootdevname = getenv("rootdev");
	efi_getdev((void**) (&rootdev), rootdevname, NULL);
	if (rootdev == NULL) { /* bad $rootdev/$currdev */
		printf("can't determine root device\n");
		return (EINVAL);
	}

    /* Try reading the /etc/fstab file to select the root device */
    getrootmount(efi_fmtdev((void *)rootdev));
    free(rootdev);

	nsym = ssym = esym = 0;
	nsym = fp->f_marks[MARK_NSYM];
	ssym = fp->f_marks[MARK_SYM];
	esym = fp->f_marks[MARK_END];

	if (nsym == 0 || ssym == 0 || esym == 0) {
		nsym = ssym = esym = 0; 				/* sanity */
	}

	bi->bi_envp->bi_nsymtab = nsym;
    bi->bi_envp->bi_symtab = ssym;
    bi->bi_envp->bi_esymtab = esym;

    bi->bi_efi.bi_hcdp = (uint64_t)efi_get_table(&hcdp); 	/* DIG64 HCDP table addr. */
    //fpswa_init(&bi->bi_efi.bi_fpswa);						/* find FPSWA interface */

    /* find the last module in the chain */
    addr = 0;
    for (xp = file_findfile(NULL, NULL); xp != NULL; xp = xp->f_next) {
	if (addr < (xp->f_addr + xp->f_size))
	    addr = xp->f_addr + xp->f_size;
    }

    /* pad to a page boundary */
    addr = roundup(addr, PAGE_SIZE);

    /* copy our environment */
	bi->bi_envp->bi_environment = addr;
    addr = bi_copyenv(addr);

    /* pad to a page boundary */
    addr = roundup(addr, PAGE_SIZE);

    /* copy module list and metadata */
    //bi->bi_modulep = addr;
    //addr = bi_copymodules(addr);

    /* all done copying stuff in, save end of loaded object space */
    bi->bi_envp.bi_kernend = addr;

	/*
	 * Read the memory map and stash it after bootinfo. Align the memory map
	 * on a 16-byte boundary (the bootinfo block is page aligned).
	 */
	bisz = (sizeof(struct bootinfo) + 0x0f) & ~0x0f;
	bi->bi_efi.bi_memmap = ((u_int64_t) bi) + bisz;
	bi->bi_efi.bi_memmap_size = EFI_PAGE_SIZE * pages - bisz;
	status = BS->GetMemoryMap(&bi->bi_efi.bi_memmap_size,
			(EFI_MEMORY_DESCRIPTOR*) bi->bi_efi.bi_memmap, &key,
			&bi->bi_efi.bi_memdesc_size, &bi->bi_efi.bi_memdesc_version);
	if (EFI_ERROR(status)) {
		printf("bi_load: Can't read memory map\n");
		return (EINVAL);
	}
	*mapkey = key;

    return (0);
}
