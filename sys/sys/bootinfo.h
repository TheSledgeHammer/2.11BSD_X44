/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _BOOTINFO_H_
#define _BOOTINFO_H_

#include <machine/bootinfo.h>

/* Only change the version number if you break compatibility. */

/*
 * A zero bootinfo field often means that there is no info available.
 * Flags are used to indicate the validity of fields where zero is a
 * normal value.
 */
/* Bootinfo: Machine-Independent */
union bootinfo {
	u_int32_t					bi_version;
	u_int32_t					bi_kernelname;		/* represents a char * */
	u_int32_t					bi_magic;			/* BOOTINFO_MAGIC */
	u_int32_t					bi_boothowto;		/* value for boothowto */
	char 						bi_bootpath[80];
	int				 			bi_len;
	int 						bi_type;

/* Bootinfo Sections: Machine-Dependent (machine/bootinfo.h) */
	struct bootinfo_bios 		bi_bios;			/* Bios */
	struct bootinfo_efi			bi_efi;				/* EFI */
	struct bootinfo_enivronment bi_envp;			/* Environment */
	struct bootinfo_bootdisk	bi_disk;			/* Disk */
	struct bootinfo_netif		bi_net;				/* Network */
	struct bootinfo_console		bi_cons;			/* Console */
	struct bootinfo_biogeom		bi_geom;			/* Geometry */
};

extern union bootinfo 			bootinfo;

/* Methods to Call Various bootinfo */
struct bootinfo_bios *get_bootinfo_bios(union bootinfo *);
struct bootinfo_efi *get_bootinfo_efi(union bootinfo *);
struct bootinfo_enivronment *get_bootinfo_environment(union bootinfo *);
struct bootinfo_bootdisk *get_bootinfo_bootdisk(union bootinfo *);
struct bootinfo_netif 	*get_bootinfo_netif(union bootinfo *);
struct bootinfo_console *get_bootinfo_console(union bootinfo *);
struct bootinfo_biogeom *get_bootinfo_biogeom(union bootinfo *);

#endif /* _BOOTINFO_H_ */
