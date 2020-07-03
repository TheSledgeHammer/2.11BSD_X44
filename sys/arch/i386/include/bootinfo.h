/*	$NetBSD: bootinfo.h,v 1.30 2019/06/21 02:08:55 nonaka Exp $	*/

/*
 * Copyright (c) 1997
 *	Matthias Drochner.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _MACHINE_BOOTINFO_H_
#define _MACHINE_BOOTINFO_H_

#include <lib/libsa/diskmbr.h>

#define	BOOTINFO_VERSION		1

#define	N_BIOS_GEOM				8

#define	BOOTINFO_MAGIC			0xdeadbeeffeedface
#define BOOTINFO_MAXSIZE 		16384

#define BOOTINFO_MEMORY        	0x00000001
#define BOOTINFO_AOUT_SYMS      0x00000010
#define BOOTINFO_ELF_SYMS		0x00000020

struct bootinfo {
	u_int32_t					bi_version;
	u_int32_t					bi_kernelname;		/* represents a char * */
	u_int32_t					bi_magic;			/* BOOTINFO_MAGIC */
	u_int32_t					bi_boothowto;		/* value for boothowto */
	char 						bi_bootpath[80];
	int				 			bi_len;
	int 						bi_type;
	uint32_t					bi_nentries;		/* Number of bootinfo_* entries in bi_data. */
	uint32_t 					bi_entry[1];
	//uint8_t						bi_data[BOOTINFO_MAXSIZE - sizeof(uint32_t)];

	struct bootinfo_bios {		/* BIOS */
#define	bi_endcommon			bi_n_bios_used
		u_int32_t				bi_n_bios_used;
		u_int32_t				bi_size;
		u_int8_t				bi_memsizes_valid;
		u_int8_t				bi_bios_dev;		/* bootdev BIOS unit number */
		u_int8_t				bi_pad[2];
		u_int32_t				bi_basemem;
		u_int32_t				bi_extmem;
	} bi_bios;

	struct bootinfo_efi { 		/* EFI */
		u_int32_t				bi_systab;			/* pa of EFI system table */
		u_int32_t				bi_memmap;			/* pa of EFI memory map */
		u_int32_t				bi_memmap_size;		/* size of EFI memory map */
		u_int32_t				bi_memdesc_size;	/* sizeof EFI memory desc */
		u_int32_t				bi_memdesc_version;	/* EFI memory desc version */
	} bi_efi;

	struct bootinfo_enivronment {/* ENVIRONMENT */
		u_int32_t				bi_kernend;			/* end of kernel space */
		u_int32_t				bi_modulep;			/* pre-loaded modules */
		u_int32_t				bi_nsymtab;
		u_int32_t				bi_symtab;			/* start of kernel sym table */
		u_int32_t				bi_esymtab;			/* end of kernel sym table */
		u_int32_t				bi_environment;		/* environment */
	} bi_envp;

	struct bootinfo_bootdisk {	/* BOOTDISK */
		int 					bi_labelsector;
		struct {
			u_int16_t 			type;
			u_int16_t 			checksum;
			char 				packname[16];
		} bi_label;
		int 					bi_biosdev;
		int 					bi_partition;
		u_int32_t				bi_nfs_diskless;	/* struct nfs_diskless * */
	} bi_disk;

	struct bootinfo_netif {		/* NETWORK */
		char 					bi_ifname[16];
		int 					bi_bus;
#define BI_BUS_ISA 0
#define BI_BUS_PCI 1
		union {
			unsigned int 		iobase; 			/* ISA */
			unsigned int 		tag; 				/* PCI, BIOS format */
		} bi_addr;
	} bi_net;

	struct bootinfo_console {	/* CONSOLE */
		char 					bi_devname[16];
		int 					bi_addr;
		int 					bi_speed;
	} bi_cons;

	struct bootinfo_biosgeom {	/* GEOMETRY */
		u_int32_t				bi_bios_geom[N_BIOS_GEOM];
		int 					bi_spc;
		int						bi_spt;
		struct dos_partition 	bi_dosparts[NDOSPART];
	} bi_geom;

	struct bootinfo_legacy {	/* LEGACY */
		int 					*bi_howtop;
		int 					*bi_bootdevp;
		vm_offset_t 			*bi_bip;
	} bi_leg;

	struct bootinfo_symbols {	/* ELF SYMBOLS */
		uint32_t				bi_flags;
		u_long					bi_marks;
		void *					bi_symstart;
		size_t					bi_symsize;
		void *					bi_strstart;
		size_t					bi_strsize;
	} bi_sym;
};

#ifdef _KERNEL
extern struct bootinfo 	bootinfo;
extern int				end;
extern int 				*esym;
#endif

#endif /* _MACHINE_BOOTINFO_H_ */
