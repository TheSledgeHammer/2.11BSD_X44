/*-
 * Copyright (C) 1994 by Rodney W. Grimes, Milwaukie, Oregon  97222
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Rodney W. Grimes.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RODNEY W. GRIMES ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL RODNEY W. GRIMES BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef	_MACHINE_BOOTINFO_H_
#define	_MACHINE_BOOTINFO_H_

#ifndef _LOCORE

/* Only change the version number if you break compatibility. */
#define	BOOTINFO_VERSION		1

#define	N_BIOS_GEOM				8

#define	BOOTINFO_MAGIC			0xdeadbeeffeedface

struct bootinfo_bios {
#define	bi_endcommon			bi_n_bios_used
	u_int32_t					bi_n_bios_used;
	u_int32_t					bi_size;
	u_int8_t					bi_memsizes_valid;
	u_int8_t					bi_bios_dev;		/* bootdev BIOS unit number */
	u_int8_t					bi_pad[2];
	u_int32_t					bi_basemem;
	u_int32_t					bi_extmem;
};

struct bootinfo_efi {
	u_int32_t					bi_systab;			/* pa of EFI system table */
	u_int32_t					bi_memmap;			/* pa of EFI memory map */
	u_int32_t					bi_memmap_size;		/* size of EFI memory map */
	u_int32_t					bi_memdesc_size;	/* sizeof EFI memory desc */
	u_int32_t					bi_memdesc_version;	/* EFI memory desc version */
};

struct bootinfo_enivronment {
	u_int32_t					bi_kernend;			/* end of kernel space */
	u_int32_t					bi_modulep;			/* pre-loaded modules */
	u_int32_t					bi_symtab;			/* start of kernel sym table */
	u_int32_t					bi_esymtab;			/* end of kernel sym table */
	u_int32_t					bi_envp;			/* environment */
};

struct bootinfo_bootdisk {
	int 						bi_labelsector;
	struct {
		u_int16_t 	type;
		u_int16_t 	checksum;
		char 		packname[16];
	} bi_label;
	int 						bi_biosdev;
	int 						bi_partition;
	u_int32_t					bi_nfs_diskless;	/* struct nfs_diskless * */
};

struct bootinfo_netif {
	char 						bi_ifname[16];
	int 						bi_bus;
#define BI_BUS_ISA 0
#define BI_BUS_PCI 1
	union {
		unsigned int iobase; 						/* ISA */
		unsigned int tag; 							/* PCI, BIOS format */
	} bi_addr;
};

struct bootinfo_console {
	char 						bi_devname[16];
	int 						bi_addr;
	int 						bi_speed;
};

struct bootinfo_biogeom {
	u_int32_t					bi_bios_geom[N_BIOS_GEOM];
	int 						bi_spc;
	int							bi_spt;
	struct dos_partition 		bi_dosparts[NDOSPART];
};

#ifdef _KERNEL
void *lookup_bootinfo (int);
#endif
#endif 	/* _LOCORE */

#ifdef _KERNEL
#define BOOTINFO_MAXSIZE 1000
#endif
#endif	/* !_MACHINE_BOOTINFO_H_ */
