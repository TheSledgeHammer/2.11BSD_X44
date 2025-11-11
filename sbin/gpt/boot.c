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

#include <sys/diskmbr.h>

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

#define DEFAULT_BOOTDIR0 "/usr/mdec/boot0"	/* current default boot path */
#define DEFAULT_BOOTDIR1 "/boot/boot0"

static void
usage_boot(void)
{
	fprintf(stderr, "usage: %s device\n", getprogname());
	exit(1);
}

static map_t *
bootgpt(int fd, unsigned int entry)
{
	uuid_t uuid;
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

	if (!gpt_write_crc(fd, map, gpt, tbl, &uuid, entry)) {
		return (NULL);
	}
	if (!gpt_write_crc(fd, map, lbt, tpg, &uuid, entry)) {
		return (NULL);
	}
	return (map);
}

static void
bootset(int fd, const char *bootpath)
{
	struct stat st;
	off_t  block;
	off_t  size;
	unsigned int entry;
	map_t *map, *pmbr;
	struct mbr *mbr;
	ssize_t nbytes;
	int bfd;

	entry = 0;
	map = bootgpt(fd, entry);
	if (map == NULL) {
		return;
	}
	block = map->map_start;
	size  = map->map_size;

	pmbr = map_find(MAP_TYPE_PMBR);
	if (pmbr == NULL) {
		errx(1, "error: PMBR not found");
	}

	bfd = open(bootpath, O_RDONLY);
	if (bfd < 0 || fstat(bfd, &st) < 0) {
		errx(1, "unable to open PMBR boot loader");
	}

	if (st.st_size != secsz) {
		errx(1, "invalid PMBR boot loader");
	}

	mbr = pmbr->map_data;
	if (mbr == NULL) {
		errx(1, "No PMBR data!");
	}

	nbytes = read(bfd, mbr->mbr_code, sizeof(mbr->mbr_code));
	if (nbytes < 0) {
		errx(1, "unable to read PMBR boot loader");
	}

	if (nbytes != sizeof(mbr->mbr_code)) {
		errx(1, "short read of PMBR boot loader");
	}
	close(bfd);
	gpt_write(fd, pmbr);

	/*
	 * Generate partition #1
	 */
	gpt_create_pmbr_part(mbr->mbr_part[1], block, size);

	gpt_write(fd, map);
}

int
cmd_boot(int argc, char *argv[])
{
	int ch, fd;

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch(ch) {
		default:
			usage_boot();
		}
	}

	if (argc == optind)
		usage_boot();

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}
		bootset(fd, DEFAULT_BOOTDIR0);
		gpt_close(fd);
	}
	return (0);
}
