/*
 * load_exec.c
 *
 *  Created on: 26 Feb 2020
 *      Author: marti
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/common/load_elf.c,v 1.30.2.1 2004/09/03 19:25:40 iedowse Exp $"); */

#include <sys/param.h>
#include <sys/exec.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include "bootstrap.h"

/*
 * Kernel Types:
 *  #define AOUT_KERNELTYPE 	"aout kernel"
 *  #define ECOFF_KERNELTYPE 	"ecoff kernel"
 *  #define ELF32_KERNELTYPE 	"elf32 kernel"
 *  #define ELF64_KERNELTYPE 	"elf64 kernel"
 *
 *  Refer to the end of bootstrap.h for more information
 */

int
exec_loadfile(char *kerneltype, char *filename, long dest, struct preloaded_file **result)
{
    struct preloaded_file	*fp;
    u_long                  *marks;
    int						err;
    u_int					pad;
    ssize_t					bytes_read;
    int                     fd;

    /*
     * Open the image, read and validate header
     */
    if (filename == NULL)	/* can't handle nameless */
    	return(EFTYPE);

    fp = file_alloc();

    if (fp == NULL) {
#ifdef BOOT_AOUT
	    printf("aout_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_ECOFF
	    printf("ecoff_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_ELF32
	    printf("elf32_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_ELF64
	    printf("elf64_loadfile: cannot allocate module info\n");
#endif
	    err = EPERM;
	    goto out;
    }

    marks = fp->marks;
    fp->f_name = strdup(filename);
    fp->f_type = strdup(kerneltype);

    marks[MARK_START] = dest;

    if ((fd = loadfile(filename, marks, LOAD_KERNEL)) == -1) {
	    err = EPERM;
	    goto oerr;
    }
    close(fd);

    dest = marks[MARK_ENTRY];

    printf("%s entry at 0x%lx\n", filename, (uintmax_t)dest);

    fp->f_size = marks[MARK_END] - marks[MARK_START];
    fp->f_addr = marks[MARK_START];

    if (fp->f_size == 0 || fp->f_addr == 0)
    	goto ioerr;

    /* Load OK, return module pointer */
    *result = fp;
    err = 0;
    goto out;

 ioerr:
    err = EIO;
 oerr:
    file_discard(fp);
 out:
    return(err);
}
