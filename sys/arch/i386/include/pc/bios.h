/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1997 Michael Smith
 * Copyright (c) 1998 Jonathan Lemon
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

#ifndef _MACHINE_PC_BIOS_H_
#define _MACHINE_PC_BIOS_H_

#include <machine/vmparam.h>

/*
 * Signature structure for the BIOS32 Service Directory header
 */
struct bios32_SDheader
{
    u_int8_t	sig[4];
    u_int32_t	entry;
    u_int8_t	revision;
    u_int8_t	len;
    u_int8_t	cksum;
    u_int8_t	pad[5];
};

/*
 * PCI BIOS functions
 */
#define PCIBIOS_BIOS_PRESENT		0xb101
#define PCIBIOS_READ_CONFIG_BYTE	0xb108
#define PCIBIOS_READ_CONFIG_WORD	0xb109
#define PCIBIOS_READ_CONFIG_DWORD	0xb10a
#define PCIBIOS_WRITE_CONFIG_BYTE	0xb10b
#define PCIBIOS_WRITE_CONFIG_WORD	0xb10c
#define PCIBIOS_WRITE_CONFIG_DWORD	0xb10d
#define PCIBIOS_GET_IRQ_ROUTING		0xb10e
#define PCIBIOS_ROUTE_INTERRUPT		0xb10f

/*
 * PCI interrupt routing table.
 *
 * $PIR in the BIOS segment contains a PIR_table
 * int 1a:b106 returns PIR_table in buffer at es:(e)di
 * int 1a:b18e returns PIR_table in buffer at es:(e)di
 * int 1a:b406 returns es:di pointing to the BIOS PIR_table
 */
struct PIR_header
{
    int8_t		ph_signature[4];
    u_int16_t	ph_version;
    u_int16_t	ph_length;
    u_int8_t	ph_router_bus;
    u_int8_t	ph_router_dev_fn;
    u_int16_t	ph_pci_irqs;
    u_int16_t	ph_router_vendor;
    u_int16_t	ph_router_device;
    u_int32_t	ph_miniport;
    u_int8_t	ph_res[11];
    u_int8_t	ph_checksum;
} __packed;

struct PIR_intpin
{
    u_int8_t	link;
    u_int16_t	irqs;
} __packed;

struct PIR_entry
{
    u_int8_t			pe_bus;
    u_int8_t			pe_res1:3;
    u_int8_t			pe_device:5;
    struct PIR_intpin	pe_intpin[4];
    u_int8_t			pe_slot;
    u_int8_t			pe_res3;
} __packed;

struct PIR_table
{
    struct PIR_header	pt_header;
    struct PIR_entry	pt_entry[0];
} __packed;

/*
 * Int 15:E820 'SMAP' structure
 */
#define SMAP_SIG				0x534D4150			/* 'SMAP' */

#define	SMAP_TYPE_MEMORY		1
#define	SMAP_TYPE_RESERVED		2
#define	SMAP_TYPE_ACPI_RECLAIM	3
#define	SMAP_TYPE_ACPI_NVS		4
#define	SMAP_TYPE_ACPI_ERROR	5
#define	SMAP_TYPE_DISABLED		6
#define	SMAP_TYPE_PMEM			7
#define	SMAP_TYPE_PRAM			12

#define	SMAP_XATTR_ENABLED		0x00000001
#define	SMAP_XATTR_NON_VOLATILE	0x00000002
#define	SMAP_XATTR_MASK			(SMAP_XATTR_ENABLED | SMAP_XATTR_NON_VOLATILE)

struct bios_smap {
    u_int64_t	base;
    u_int64_t	length;
    u_int32_t	type;
} __packed;

/* Structure extended to include extended attribute field in ACPI 3.0. */
struct bios_smap_xattr {
    u_int64_t	base;
    u_int64_t	length;
    u_int32_t	type;
    u_int32_t	xattr;
} __packed;

/*
 * System Management BIOS
 */
#define	SMBIOS_START	0xf0000
#define	SMBIOS_STEP		0x10
#define	SMBIOS_OFF		0
#define	SMBIOS_LEN		4
#define	SMBIOS_SIG		"_SM_"

struct smbios_eps {
	uint8_t		anchor_string[4];				/* '_SM_' */
	uint8_t		checksum;
	uint8_t		length;
	uint8_t		major_version;
	uint8_t		minor_version;
	uint16_t	maximum_structure_size;
	uint8_t		entry_point_revision;
	uint8_t		formatted_area[5];
	uint8_t		intermediate_anchor_string[5];	/* '_DMI_' */
	uint8_t		intermediate_checksum;
	uint16_t	structure_table_length;
	uint32_t	structure_table_address;
	uint16_t	number_structures;
	uint8_t		BCD_revision;
};

struct smbios_structure_header {
	uint8_t		type;
	uint8_t		length;
	uint16_t	handle;
};

#ifdef _KERNEL
#define BIOS_PADDRTOVADDR(x)	((x) + PMAP_MAP_LOW)
#define BIOS_VADDRTOPADDR(x)	((x) - PMAP_MAP_LOW)

struct bios_oem_signature {
	char * anchor;		/* search anchor string in BIOS memory */
	size_t offset;		/* offset from anchor (may be negative) */
	size_t totlen;		/* total length of BIOS string to copy */
} __packed;

struct bios_oem_range {
	u_int from;		/* shouldn't be below 0xe0000 */
	u_int to;		/* shouldn't be above 0xfffff */
} __packed;

struct bios_oem {
	struct bios_oem_range range;
	struct bios_oem_signature signature[];
} __packed;

struct segment_info {
	u_int	base;
	u_int	limit;
};

#define BIOSCODE_FLAG	0x01
#define BIOSDATA_FLAG	0x02
#define BIOSUTIL_FLAG	0x04
#define BIOSARGS_FLAG	0x08

struct bios_segments {
	struct	segment_info code32;	/* 32-bit code (mandatory) */
	struct	segment_info code16;	/* 16-bit code */
	struct	segment_info data;		/* 16-bit data */
	struct	segment_info util;		/* 16-bit utility */
	struct	segment_info args;		/* 16-bit args */
};

struct bios_regs {
	u_int	eax;
	u_int	ebx;
	u_int	ecx;
	u_int	edx;
	u_int	esi;
	u_int	edi;
};

struct bios_args {
	u_int					entry;	/* entry point of routine */
	struct	bios_regs 		r;
	struct	bios_segments 	seg;
};

/*
 * BIOS32 Service Directory entry.  Caller supplies name, bios32_SDlookup
 * fills in the rest of the details.
 */
struct bios32_SDentry {
	union {
		u_int8_t 	name[4]; 	/* service identifier */
		u_int32_t 	id; 		/* as a 32-bit value */
	} ident;
	u_int32_t 		base; 		/* base of service */
	u_int32_t 		len; 		/* service length */
	u_int32_t 		entry; 		/* entrypoint offset from base */
	vm_offset_t 	ventry; 	/* entrypoint in kernel virtual segment */
};

/*
 * Exported lookup results
 */
extern struct bios32_SDentry	PCIbios;

void 		bios32_init (void);
int			bios_oem_strings(struct bios_oem *oem, u_char *buffer, size_t maxlen);
uint32_t	bios_sigsearch(uint32_t start, u_char *sig, int siglen, int paralen, int sigofs);
int			bios16(struct bios_args *, char *, ...);
int			bios16_call(struct bios_regs *, char *);
int			bios32(struct bios_regs *, u_int, u_short);
int			bios32_SDlookup(struct bios32_SDentry *ent);
void		set_bios_selectors(struct bios_segments *, int);

#endif

#endif /* _MACHINE_PC_BIOS_H_ */
