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
#include <sys/boot.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>

#include <bootstrap.h>

int
md_getboothowto(char *kargs)
{
	char *curpos, *next, *string;
	int howto, vidconsole;

	/* Parse kargs */
	howto = boot_parse_cmdline(kargs);
	howto |= boot_env_to_howto();
    string = next = strdup(getenv("console"));
    vidconsole = 0;
    while (next != NULL) {
    	curpos = strsep(&next, " ,");
    	if (!strcmp(curpos, "vidconsole")) {
    		vidconsole = 1;
    	} else if (!strcmp(curpos, "comconsole")) {
    		howto |= RB_SERIAL;
    	} else if (!strcmp(curpos, "nullconsole")) {
    		howto |= RB_MUTE;
    	}
    }

    if (vidconsole && (howto & RB_SERIAL)) {
    	howto |= RB_MULTIPLE;
    }

    /*
     * XXX: Note that until the kernel is ready to respect multiple consoles
     * for the boot messages, the first named console is the primary console
     */
    if (!strcmp(string, "vidconsole")) {
    	howto &= ~RB_SERIAL;
    }

    free(string);
	return (howto);
}

void
md_setboothowto(int howto)
{
	boot_howto_to_env(howto);
}

int
md_load(int howto, vm_offset_t kernend, vm_offset_t envp, const char *kerntype, char *args)
{
	struct preloaded_file *fp, *xp;
	struct devdesc *rootdev;
	vm_offset_t addr;
	char *rootdevname;
	int error;

	howto = md_getboothowto(args);

	/*
	 * Allow the environment variable 'rootdev' to override the supplied device
	 * This should perhaps go to MI code and/or have $rootdev tested/set by
	 * MI code before launching the kernel.
	 */
	rootdevname = getenv("rootdev");
	if (rootdevname == NULL || *rootdevname == '\0') {
		rootdevname = getenv("currdev");
	}

	error = disk_getdev(&rootdev, rootdevname, NULL);
	if (error != 0 || rootdev == NULL) {
		return (error);
	}

	/* Try reading the /etc/fstab file to select the root device */
	if (strcmp(rootdevname, disk_fmtdev(rootdev)) == 0) {
		getrootmount(disk_fmtdev(rootdev));
	} else {
		getrootmount(rootdevname);
	}
	free(rootdev);

	/* Find the last module in the chain */
	addr = 0;
	for (xp = file_findfile(NULL, NULL); xp != NULL; xp = xp->f_next) {
		if (addr < (xp->f_addr + xp->f_size)) {
			addr = xp->f_addr + xp->f_size;
		}
	}
	/* Pad to a page boundary */
	addr = md_align(addr);

	/* Copy our environment */
	envp = addr;
	addr = md_copyenv(addr);

	/* Pad to a page boundary */
	addr = md_align(addr);

	fp = file_findfile(NULL, kerntype);
	if (fp == NULL) {
		return (EINVAL);
	}

	/* all done copying stuff in, save end of loaded object space */
	kernend = addr;
	return (0);
}

/*
 * Copy the environment into the load area starting at (addr).
 * Each variable is formatted as <name>=<value>, with a single nul
 * separating each variable, and a double nul terminating the environment.
 */
vm_offset_t
md_copyenv(vm_offset_t addr)
{
    struct env_var	*ep;

    /* traverse the environment */
    for (ep = environ; ep != NULL; ep = ep->ev_next) {
    	archsw.arch_copyin(ep->ev_name, addr, strlen(ep->ev_name));
    	addr += strlen(ep->ev_name);
    	archsw.arch_copyin("=", addr, 1);
    	addr++;
    	if (ep->ev_value != NULL) {
    		archsw.arch_copyin(ep->ev_value, addr, strlen(ep->ev_value));
    		addr += strlen(ep->ev_value);
    	}
    	archsw.arch_copyin("", addr, 1);
    	addr++;
    }
    archsw.arch_copyin("", addr, 1);
    addr++;
	return (addr);
}

/*
 * Take the ending address and round it up to the currently required
 * alignment. This typically is the page size, but is the larger of the compiled
 * kernel page size, the loader page size, and the typical page size on the
 * platform.
 *
 * XXX For the moment, it's just PAGE_SIZE to make the refactoring go faster,
 * but needs to hook-in the replacement of arch_loadaddr.
 *
 * Also, we may need other logical things when dealing with different types of
 * page sizes and/or masking or sizes. This works well for addr and sizes, but
 * not for masks.
 *
 * Also, this is different than the MOD_ALIGN macro above, which is used for
 * aligning elements in the metadata lists, not for whare modules can begin.
 */
vm_offset_t
md_align(vm_offset_t addr)
{
	return (roundup(addr, PAGE_SIZE));
}
