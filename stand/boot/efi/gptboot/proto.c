/*-
 * Copyright (c) 2019 Netflix, Inc
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/diskgpt.h>

#include <stand.h>
#include <efi.h>
#include <eficonsctl.h>
#include <efichar.h>

#include "paths.h"
#include "proto.h"
//#include "gpt.h"		/* FreeBSD: libsa/gpt.h & gpt.c */

#include <machine/elf.h>
#include <machine/stdarg.h>

static const uuid_t freebsd_ufs_uuid = GPT_ENT_TYPE_FREEBSD_UFS;
static char secbuf[4096] __aligned(4096);
static struct dsk dsk;
static dev_info_t *devices = NULL;
static dev_info_t *raw_device = NULL;

static EFI_GUID BlockIoProtocolGUID = BLOCK_IO_PROTOCOL;
static EFI_GUID DevicePathGUID = DEVICE_PATH_PROTOCOL;

/*
 * Shim routine for the gpt code to read in the gpt table. The
 * devinfo is always going to be for the raw device.
 */
int
drvread(struct dsk *dskp, void *buf, daddr_t lba, unsigned nblk)
{

}

/*
 * Shim routine for the gpt code to write in the gpt table. The
 * devinfo is always going to be for the raw device.
 */
int
drvwrite(struct dsk *dskp, void *buf, daddr_t lba, unsigned nblk)
{

}

/*
 * Return the number of LBAs the drive has.
 */
uint64_t
drvsize(struct dsk *dskp)
{

}

static int
partition_number(EFI_DEVICE_PATH *devpath)
{

}

/*
 * Find the raw partition for the imgpath and save it
 */
static void
probe_handle(EFI_HANDLE h, EFI_DEVICE_PATH *imgpath)
{

}

static void
probe_handles(EFI_HANDLE *handles, UINTN nhandles, EFI_DEVICE_PATH *imgpath)
{

}

static dev_info_t *
find_partition(int part)
{

}

void
choice_protocol(EFI_HANDLE *handles, UINTN nhandles, EFI_DEVICE_PATH *imgpath)
{

}
