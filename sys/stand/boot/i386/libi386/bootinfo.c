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


#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/bootinfo.h>

#include <lib/libsa/loadfile.h>
#include <machine/bootinfo.h>

#include <boot/bootstand.h>
#include "bootstrap.h"
#include "libi386.h"
#include "btxv86.h"

int
bi_getboothowto(char *kargs)
{
    char	*curpos, *next, *string;
    int		howto;
    int		vidconsole;

    //howto = boot_parse_cmdline(kargs);
    //howto |= boot_env_to_howto();

    howto = 0;

    /* Enable selected consoles */
    string = next = strdup(getenv("console"));
    vidconsole = 0;
    while (next != NULL) {
    	curpos = strsep(&next, " ,");
    	if (*curpos == '\0')
    		continue;
    	if (!strcmp(curpos, "vidconsole"))
    		vidconsole = 1;
    	else if (!strcmp(curpos, "comconsole"))
    		howto |= RB_SERIAL;
    	else if (!strcmp(curpos, "nullconsole"))
    		howto |= RB_MUTE;
    }

    if (vidconsole && (howto & RB_SERIAL))
    	howto |= RB_MULTIPLE;

    /*
     * XXX: Note that until the kernel is ready to respect multiple consoles
     * for the boot messages, the first named console is the primary console
     */
    if (!strcmp(string, "vidconsole"))
    	howto &= ~RB_SERIAL;

    free(string);

    return(howto);
}

void
bi_setboothowto(int howto)
{
	struct bootinfo_common *common;
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
    	i386_copyin(ep->ev_name, addr, strlen(ep->ev_name));
    	addr += strlen(ep->ev_name);
    	i386_copyin("=", addr, 1);
    	addr++;
    	if (ep->ev_value != NULL) {
    		i386_copyin(ep->ev_value, addr, strlen(ep->ev_value));
    		addr += strlen(ep->ev_value);
    	}
    	i386_copyin("", addr, 1);
    	addr++;
    }
    i386_copyin("", addr, 1);
    addr++;
    return(addr);
}

/* Copied from NetBSD arch/ia64
 * Load the information expected by an ia64 kernel.
 *
 * - The kernel environment is copied into kernel space.
 * - Module metadata are formatted and placed in kernel space.
 */
int
bi_load(union bootinfo *bi, struct preloaded_file *fp, char *args)
{
	char					*rootdevname;
	struct i386_devdesc		*rootdev;
	struct preloaded_file	*xp;
	caddr_t					addr, bootinfo_addr;
    char					*kernelname;
    caddr_t					ssym, esym;
	struct file_metadata	*md;

	/*
	 * boot environment
	 */
	struct bootinfo_environment bienvp = get_bootinfo_environment(bi);

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
     * Allow the environment variable 'rootdev' to override the supplied device
     * This should perhaps go to MI code and/or have $rootdev tested/set by
     * MI code before launching the kernel.
     */
    rootdevname = getenv("rootdev");
    i386_getdev((void **)(&rootdev), rootdevname, NULL);
    if (rootdev == NULL) {		/* bad $rootdev/$currdev */
    	printf("can't determine root device\n");
    	return(EINVAL);
    }


    /* Try reading the /etc/fstab file to select the root device */
    getrootmount(i386_fmtdev((void *)rootdev));
    free(rootdev);

    ssym = esym = 0;

    ssym = fp->marks[MARK_SYM];
    esym = fp->marks[MARK_END];

    if (ssym == 0 || esym == 0)
    	ssym = esym = 0;		/* sanity */

    bienvp->bi_symtab = ssym;
    bienvp->bi_esymtab = esym;

    /* find the last module in the chain */
    addr = 0;
	for (xp = file_findfile(NULL, NULL); xp != NULL; xp = xp->f_next) {
		if (addr < (xp->f_addr + xp->f_size))
			addr = xp->f_addr + xp->f_size;
	}

	/* pad to a page boundary */
	addr = (addr + PAGE_MASK) & ~PAGE_MASK;

	/* copy our environment */
	bienvp = addr;
	addr = bi_copyenv(addr);

	/* pad to a page boundary */
	addr = (addr + PAGE_MASK) & ~PAGE_MASK;

	/* all done copying stuff in, save end of loaded object space */
	bienvp->bi_kernend = addr;
    
    return ((bi));
}
