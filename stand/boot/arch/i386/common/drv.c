/*-
 * Copyright (c) 1998 Robert Nordier
 * Copyright (c) 2010 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 */

#include <sys/cdefs.h>
/*__FBSDID("$FreeBSD$");*/

#include <sys/param.h>

#include <lib/libsa/stand.h>
#include <stand/boot/common/rbx.h>

#include <btxv86.h>

#include "drv.h"
#include "edd.h"

static struct edd_params params;
static struct edd_packet packet;

static int v86_run(uint32_t, uint32_t, uint32_t, uint32_t, void *, void *);
static uint64_t edd_params_do(struct edd_params *, unsigned int);
static int edd_packet_do(struct edd_packet *, unsigned int, void *, daddr_t, unsigned int);

static int
v86_run(uint32_t ctl, uint32_t addr, uint32_t eax, uint32_t edx, void *seg, void *off)
{
	v86.ctl = ctl;
	v86.addr = addr;
	v86.eax = eax;
	v86.edx = edx;
	v86.ds = VTOPSEG(seg);
	v86.esi = VTOPOFF(off);
	v86int();
	if (V86_CY(v86.efl)) {
		return (-1);
	}
	return (0);
}

static uint64_t
edd_params_do(struct edd_params *eparams, unsigned int drive)
{
	int error;

	eparams->len = sizeof(struct edd_params);
	error = v86_run(V86_FLAGS, 0x13, 0x4800, drive, eparams, eparams);
	if (error != 0) {
		printf("error %u\n", v86.eax >> 8 & 0xff);
		return (0);
	}
	return (eparams->sectors);
}

static int
edd_packet_do(struct edd_packet *epacket, unsigned int drive, void *buf, daddr_t lba, unsigned int nblk)
{
	int error;

	epacket->len = sizeof(struct edd_packet);
	epacket->count = nblk;
	epacket->off = VTOPOFF(buf);
	epacket->seg = VTOPSEG(buf);
	epacket->lba = lba;
	error = v86_run(V86_FLAGS, 0x13, 0x4200, drive, epacket, epacket);
	if (error != 0) {
		printf("error %u lba %llu\n", v86.eax >> 8 & 0xff, lba);
		return (error);
	}
	return (0);
}

uint64_t
drvsize(struct dsk *dskp)
{
	return (edd_params_do(&params, dskp->drive));
}

int
drvread(struct dsk *dskp, void *buf, daddr_t lba, unsigned int nblk)
{
	static unsigned c = 0x2d5c7c2f;

	if (!OPT_CHECK(RBX_QUIET))
		printf("%c\b", c = c << 8 | c >> 24);
	return (edd_packet_do(&packet, dskp->drive, buf, lba, nblk));
}

#if defined(GPT) || defined(ZFS)
int
drvwrite(struct dsk *dskp, void *buf, daddr_t lba, unsigned int nblk)
{
	return (edd_packet_do(&packet, dskp->drive, buf, lba, nblk));
}

#endif	/* GPT || ZFS */

#ifdef notyet

uint64_t
drvsize(struct dsk *dskp)
{

	params.len = sizeof(struct edd_params);
	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	v86.eax = 0x4800;
	v86.edx = dskp->drive;
	v86.ds = VTOPSEG(&params);
	v86.esi = VTOPOFF(&params);
	v86int();
	if (V86_CY(v86.efl)) {
		printf("error %u\n", v86.eax >> 8 & 0xff);
		return (0);
	}
	return (params.sectors);
}



int
drvread(struct dsk *dskp, void *buf, daddr_t lba, unsigned nblk)
{
	static unsigned c = 0x2d5c7c2f;

	if (!OPT_CHECK(RBX_QUIET))
		printf("%c\b", c = c << 8 | c >> 24);
	packet.len = sizeof(struct edd_packet);
	packet.count = nblk;
	packet.off = VTOPOFF(buf);
	packet.seg = VTOPSEG(buf);
	packet.lba = lba;
	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	v86.eax = 0x4200;
	v86.edx = dskp->drive;
	v86.ds = VTOPSEG(&packet);
	v86.esi = VTOPOFF(&packet);
	v86int();
	if (V86_CY(v86.efl)) {
		printf("%s: error %u lba %llu\n", BOOTPROG, v86.eax >> 8 & 0xff, lba);
		return (-1);
	}
	return (0);
}

#if defined(GPT) || defined(ZFS)
int
drvwrite(struct dsk *dskp, void *buf, daddr_t lba, unsigned nblk)
{

	packet.len = sizeof(struct edd_packet);
	packet.count = nblk;
	packet.off = VTOPOFF(buf);
	packet.seg = VTOPSEG(buf);
	packet.lba = lba;
	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	v86.eax = 0x4300;
	v86.edx = dskp->drive;
	v86.ds = VTOPSEG(&packet);
	v86.esi = VTOPOFF(&packet);
	v86int();
	if (V86_CY(v86.efl)) {
		printf("error %u lba %llu\n", v86.eax >> 8 & 0xff, lba);
		return (-1);
	}
	return (0);
}
#endif	/* GPT || ZFS */
#endif
