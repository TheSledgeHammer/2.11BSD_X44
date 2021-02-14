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
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/boot.h>

#include <lib/libsa/loadfile.h>
#include <lib/libsa/stand.h>
#include "bootstrap.h"
#include "libi386.h"
#include "btxv86.h"

#include <machine/bootinfo.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/specialreg.h>

struct bootinfo boot;

void
bi_init(void)
{
	bi_alloc(&boot);
}

void
bi_alloc(bi)
	struct bootinfo *bi;
{
	memset(bi, 0, sizeof(struct bootinfo *));
	bi->bi_magic = BOOTINFO_MAGIC;
}

/*
 * Check to see if this CPU supports long mode.
 */
static int
bi_checkcpu(void)
{
	char 			*cpu_vendor;
	int 			vendor[3];
	int 			eflags;
	unsigned int 	regs[4];

	/* Check for presence of "cpuid". */
	eflags = read_eflags();
	write_eflags(eflags ^ PSL_ID);
	if (!((eflags ^ read_eflags()) & PSL_ID))
		return (0);

	/* Fetch the vendor string. */
	do_cpuid(0, regs);
	vendor[0] = regs[1];
	vendor[1] = regs[3];
	vendor[2] = regs[2];
	cpu_vendor = (char*) vendor;

	/* Check for vendors that support AMD features. */
	if (strncmp(cpu_vendor, INTEL_VENDOR_ID, 12) != 0
			&& strncmp(cpu_vendor, AMD_VENDOR_ID, 12) != 0
			&& strncmp(cpu_vendor, HYGON_VENDOR_ID, 12) != 0
			&& strncmp(cpu_vendor, CENTAUR_VENDOR_ID, 12) != 0)
		return (0);

	/* Has to support AMD features. */
	do_cpuid(0x80000000, regs);
	if (!(regs[0] >= 0x80000001))
		return (0);

	/* Check for long mode. */
	do_cpuid(0x80000001, regs);

	return (regs[3] & AMDID_LM);
}

int
bi_load_stage0(struct bootinfo *bi, struct preloaded_file *fp, char *args)
{
	char 					*rootdevname;
	struct i386_devdesc 	*rootdev;
	struct preloaded_file 	*xp;
	caddr_t 				addr, bootinfo_addr;
	char 					*kernelname;
	caddr_t 				ssym, esym, nsym;
	int 					bootdevnr;

	/* Check long mode support */
	if (!bi_checkcpu()) {
		printf("CPU doesn't support long mode\n");
		return (EINVAL);
	}

	/*
	 * Version 1 bootinfo.
	 */
	bi->bi_version = 1;

	/*
	 * Calculate boothowto.
	 */
	bi->bi_boothowto = bi_getboothowto(fp->f_args);

	/*
	 * Allow the environment variable 'rootdev' to override the supplied device
	 * This should perhaps go to MI code and/or have $rootdev tested/set by
	 * MI code before launching the kernel.
	 */
	rootdevname = getenv("rootdev");
	i386_getdev((void**) (&rootdev), rootdevname, NULL);
	if (rootdev == NULL) { /* bad $rootdev/$currdev */
		printf("can't determine root device\n");
		return (EINVAL);
	}

	/* Try reading the /etc/fstab file to select the root device */
	getrootmount(i386_fmtdev((void*) rootdev));
	/* Do legacy rootdev guessing */

	/* XXX - use a default bootdev of 0.  Is this ok??? */
	bootdevnr = 0;

	switch (rootdev->dd.d_dev->dv_type) {
	case DEVT_CD:
	case DEVT_DISK:
		/* pass in the BIOS device number of the current disk */
		bi->bi_bios.bi_bios_dev = bd_unit2bios(rootdev);
		bootdevnr = bd_getdev(rootdev);
		break;
	case DEVT_NET: /* to be implemented */
	default:
		printf("WARNING - don't know how to boot from device type %d\n",
				rootdev->dd.d_dev->dv_type);
	}
	if (bootdevnr == -1) {
		printf("root device %s invalid\n", i386_fmtdev(rootdev));
		return (EINVAL);
	}
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

    /* all done copying stuff in, save end of loaded object space */
    bi->bi_envp->bi_kernend = addr;

    return (0);
}

int
bi_load_stage1(struct bootinfo bi, struct preloaded_file *fp, char *args, vm_offset_t addr, int add_smap)
{
	int error = bi_load_stage0(bi, fp, args);
	if(error != 0) {
		return (error);
	} else {
		bi_load_legacy(bi, fp, args);
		//bi_load_smap(fp, add_smap);
	}
	return (0);
}

void
bi_load_legacy(struct bootinfo bi, struct preloaded_file *fp, char *args)
{
	int						bootdevnr, i, howto;
    char					*kernelname;
    const char				*kernelpath;

	/* legacy bootinfo structure */
	kernelname = getenv("kernelname");
	i386_getdev(NULL, kernelname, &kernelpath);
	bi.bi_version = BOOTINFO_VERSION;
	bi.bi_kernelname = 0; 						/* XXX char * -> kernel name */
	bi.bi_disk.bi_nfs_diskless = 0; 			/* struct nfs_diskless * */
	bi.bi_bios.bi_n_bios_used = 0; 				/* XXX would have to hook biosdisk driver for these */
	for (i = 0; i < N_BIOS_GEOM; i++)
		bi->bi_geom.bi_bios_geom[i] = bd_getbigeom(i);
	bi.bi_bios.bi_size = sizeof(bi);
	bi.bi_bios.bi_memsizes_valid = 1;
	bi.bi_bios.bi_basemem = bios_basemem / 1024;
	bi.bi_bios.bi_extmem = bios_extmem / 1024;
	bi.bi_kernelname = VTOP(kernelpath);

	/* legacy boot arguments */
	bi.bi_leg.bi_howtop = howto | RB_BOOTINFO;
	bi.bi_leg.bi_bootdevp = bootdevnr;
	bi.bi_leg.bi_bip = VTOP(&bi);
}

/*
 * Not Supported
void
bi_load_smap(struct preloaded_file *fp, int add_smap)
{
	if (add_smap != 0)
		bios_addsmapdata(fp);
}
*/
