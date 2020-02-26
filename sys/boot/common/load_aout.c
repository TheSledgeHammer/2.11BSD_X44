/*
 * load_aout.c
 *
 *  Created on: 24 Feb 2020
 *      Author: marti
 */
#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/common/load_elf.c,v 1.30.2.1 2004/09/03 19:25:40 iedowse Exp $"); */

#include <sys/param.h>
#include <sys/exec.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include "bootstrap.h"

int
aout_loadfile(char *filename, u_int64_t dest, struct preloaded_file **result)
{
    struct preloaded_file	*fp;
    u_long                  *marks;
    int						err;
    u_int					pad;
    ssize_t					bytes_read;
    int                     fd;
}
