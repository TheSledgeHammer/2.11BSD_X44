/*
 * Copyright (c) 2008 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "map.h"
#include "gpt.h"

//#define DEFAULT_BOOTDIR     "/usr/mdec/boot0"
#define DEFAULT_BOOTPMBR    "/usr/mdec/pmbr"
#define DEFAULT_BOOTGPT     "/usr/mdec/gptboot"

//static const char *bootpath = DEFAULT_BOOTDIR;
static const char *pmbrpath = DEFAULT_BOOTPMBR;
static const char *gptpath = DEFAULT_BOOTGPT;
static u_long boot_size;

static void
usage_boot(void)
{
	fprintf(stderr, "usage: %s [-b pmbr] [-g gptboot] [-s count] device\n", getprogname());
	exit(1);
}

static map_t *
bootmap(int fd, uuid_t *uuid, unsigned int entry)
{
	off_t  block;
	off_t  size;
	map_t *gpt, *tpg;
	map_t *tbl, *lbt;
	map_t *map;

	/*
	 * Paramters for boot partition
	 */
	block = 0;
	size = (off_t)1024 * 1024 * 1024 / 512;		/* 1GB */

	/* Find a GPT partition with the requested UUID type. */
	gpt = map_find(MAP_TYPE_PRI_GPT_HDR);
	if (gpt == NULL) {
		warnx("%s: error: no primary GPT header", device_name);
		return (NULL);
	}

	tpg = map_find(MAP_TYPE_SEC_GPT_HDR);
	if (tpg == NULL) {
		warnx("%s: error: no secondary GPT header", device_name);
		return (NULL);
	}

	tbl = map_find(MAP_TYPE_PRI_GPT_TBL);
	lbt = map_find(MAP_TYPE_SEC_GPT_TBL);
	if (tbl == NULL || lbt == NULL) {
		warnx("%s: error: no primary or secondary gpt table", device_name);
		return (NULL);
	}

	map = map_alloc(block, size);
	if (map == NULL) {
		warnx("%s: error: no space available on device", device_name);
		return (NULL);
	}

	if (!gpt_write_crc(fd, map, gpt, tbl, uuid, entry)) {
		return (NULL);
	}

	if (!gpt_write_crc(fd, map, lbt, tpg, uuid, entry)) {
		return (NULL);
	}
	return (map);
}

static int
gpt_find(map_t *gpt, map_t **mapp, uuid_t *uuid)
{
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	map_t *map;
	unsigned int i;

	for (i = 0; i < le32toh(hdr->hdr_entries); i++) {
		ent = gpt_ent(hdr, gpt, i);
		if (uuid_equal(&ent->ent_type, uuid, NULL)) {
			break;
		}
	}

	if (i == le32toh(hdr->hdr_entries)) {
		*mapp = NULL;
		return (0);
	}

	for (map = map_find(MAP_TYPE_GPT_PART); map != NULL; map = map->map_next) {
		if (map->map_type != MAP_TYPE_GPT_PART) {
			if (map->map_start == (off_t)le64toh(ent->ent_lba_start)) {
				*mapp = map;
				return (0);
			}
		}
	}
	warnx("internal map list is corrupted");
	return (1);
}

static void
bootgpt(int fd, map_t *map, uuid_t *uuid, unsigned int entry)
{
	struct stat sb;
	off_t bootsize, ofs;
    map_t *gptboot;
    char *buf;
    ssize_t nbytes;
    int bfd;

	if (boot_size == 0) {
		/* Default to 64k. */
		bootsize = 65536 / secsz;
	} else {
		if (boot_size * secsz < 16384) {
			warnx("invalid boot partition size %lu", boot_size);
			return;
		}
		bootsize = boot_size;
	}

	bfd = open(gptpath, O_RDONLY);
	if (bfd < 0 || fstat(bfd, &sb) < 0) {
		warn("unable to open GPT boot loader");
		return;
	}

	if (!gpt_find(map, &gptboot, uuid)) {
		warn("error: GPT not found");
		return;
	}
	if (gptboot != NULL) {
		if (gptboot->map_size * secsz < sb.st_size) {
			warnx("%s: error: boot partition is too small", device_name);
			return;
		}
	} else if (bootsize * secsz < sb.st_size) {
		warnx("%s: error: proposed size for boot partition is too small",
				device_name);
		return;
	} else {
		gptboot = bootmap(fd, uuid, entry);
		if (gptboot == NULL) {
			warn("error: GPT not found");
			return;
		}
	}

	bootsize = (sb.st_size + secsz - 1) / secsz * secsz;
	buf = calloc(1, bootsize);
	nbytes = read(bfd, buf, sb.st_size);
	if (nbytes < 0) {
		warn("unable to read GPT boot loader");
		return;
	}
	if (nbytes != sb.st_size) {
		warnx("short read of GPT boot loader");
		return;
	}
	close(bfd);
	ofs = gptboot->map_start * secsz;
	if (lseek(fd, ofs, SEEK_SET) != ofs) {
		warnx("%s: error: unable to seek to boot partition", device_name);
		return;
	}
	nbytes = write(fd, buf, bootsize);
	if (nbytes < 0) {
		warn("unable to write GPT boot loader");
		return;
	}
	if (nbytes != bootsize) {
		warnx("short write of GPT boot loader");
		return;
	}
	gpt_write(fd, gptboot);
	free(buf);
}

static void
bootpmbr(int fd, map_t *map, unsigned int entry)
{
	struct stat sb;
	off_t  block;
	off_t  size;
	map_t *pmbrboot;
	struct mbr *mbr;
	ssize_t nbytes;
	int bfd;

	block = map->map_start;
	size = map->map_size;

	pmbrboot = map_find(MAP_TYPE_PMBR);
	if (pmbrboot == NULL) {
		warn("error: PMBR not found");
		return;
	}

	bfd = open(pmbrpath, O_RDONLY);
	if (bfd < 0 || fstat(bfd, &sb) < 0) {
		warn("unable to open PMBR boot loader");
		return;
	}

	if (sb.st_size != secsz) {
		warnx("invalid PMBR boot loader");
		return;
	}

	mbr = pmbrboot->map_data;
	if (mbr == NULL) {
		warn("No PMBR data!");
		return;
	}

	nbytes = read(bfd, mbr->mbr_code, sizeof(mbr->mbr_code));
	if (nbytes < 0) {
		warn("unable to read PMBR boot loader");
		return;
	}

	if (nbytes != sizeof(mbr->mbr_code)) {
		warnx("short read of PMBR boot loader");
		return;
	}
	close(bfd);
	gpt_write(fd, pmbrboot);

	/*
	 * Generate partition #1
	 */
	gpt_create_pmbr_part(&mbr->mbr_part[1], block, size);
}

static void
bootset(int fd)
{
	uuid_t uuid;
	unsigned int entry;
	map_t *map;

	entry = 0;
	map = bootmap(fd, &uuid, entry);
	if (map == NULL) {
		return;
	}
	/* PMBR */
	bootpmbr(fd, map, entry);
	/* GPT */
	bootgpt(fd, map, &uuid, entry);

	gpt_write(fd, map);
}

int
cmd_boot(int argc, char *argv[])
{
	int ch, fd;
	char *p;

	while ((ch = getopt(argc, argv, "b:g:s:")) != -1) {
		switch (ch) {
		case 'b':
			pmbrpath = optarg;
			break;
		case 'g':
			gptpath = optarg;
			break;
		case 's':
			if (boot_size > 0) {
				usage_boot();
			}
			boot_size = strtol(optarg, &p, 10);
			if (*p != '\0' || boot_size < 1) {
				usage_boot();
			}
			break;
		default:
			usage_boot();
		}
	}

	if (argc == optind) {
		usage_boot();
	}

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}
		bootset(fd);

		gpt_close(fd);
	}
	return (0);
}
