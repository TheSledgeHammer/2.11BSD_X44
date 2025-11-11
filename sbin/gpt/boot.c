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

#define DEFAULT_BOOTDIR     "/usr/mdec/boot0"
#define DEFAULT_BOOTPMBR    "/usr/mdec/pmbr"
#define DEFAULT_BOOTGPT     "/usr/mdec/gptboot"

static const char *bootpath = DEFAULT_BOOTDIR;
static const char *pmbrpath = DEFAULT_BOOTPMBR;
static const char *gptpath = DEFAULT_BOOTGPT;


static void
usage_boot(void)
{
	fprintf(stderr, "usage: %s [-b pmbr] [-g gptboot] [-s count] device\n", getprogname());
	exit(1);
}

static map_t *
bootmap(int fd, unsigned int entry)
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

#ifdef notyet

static int
gpt_find(map_t **mapp)
{
    struct gpt_hdr *hdr;
    struct gpt_ent *ent;
    map_t *map;
    unsigned int i;

    for (i = 0; i < le32toh(hdr->hdr_entries); i++) {
        ent = gpt_ent(hdr, tbl, i);
        if (uuid_equal(&ent->ent_type, type, NULL)) {
            break;
        }
    }

    if (i == le32toh(hdr->hdr_entries)) {
        *mapp = NULL;
        return (0);
    }

    for (map = map_find(MAP_TYPE_GPT_PART); map != NULL;
	     map = map->map_next) {
        if (map->map_type != MAP_TYPE_GPT_PART) {
            if (map->map_start == (off_t)le64toh(ent->ent_lba_start)) {
                *mapp = map;
                return (0);
            }
        }
    }
   	errx(1, "internal map list is corrupted");
}

static void
bootgpt(int fd)
{
    unsigned int entry;
    map_t *gptboot;
    int bfd;

    bfd = open(gptpath, O_RDONLY);
    if (bfd < 0 || fstat(bfd, &st) < 0) {
		errx(1, "unable to open GPT boot loader");
	}

    if (!gpt_find(&gptboot)) {
        errx(1, "error: GPT not found");
    }
    if (gptboot != NULL) {
        if (gptboot->map_size * secsz < st.st_size) {
            errx(1, "%s: error: boot partition is too small", 
                device_name);
        }
    } else {
        gptboot = bootmap(fd, entry);
        if (gptboot == NULL) {
             errx(1, "error: GPT not found");
        }
    }

	close(bfd);
	gpt_write(fd, gptboot);

    
}

#endif

static void
bootset(int fd)
{
	struct stat st;
	off_t  block;
	off_t  size;
	unsigned int entry;
	map_t *map, *pmbrboot;
	struct mbr *mbr;
	ssize_t nbytes;
	int bfd;

	entry = 0;
	map = bootmap(fd, entry);
	if (map == NULL) {
		return;
	}
	block = map->map_start;
	size  = map->map_size;

	pmbrboot = map_find(MAP_TYPE_PMBR);
	if (pmbrboot == NULL) {
		errx(1, "error: PMBR not found");
	}

	bfd = open(pmbrpath, O_RDONLY);
	if (bfd < 0 || fstat(bfd, &st) < 0) {
		errx(1, "unable to open PMBR boot loader");
	}

	if (st.st_size != secsz) {
		errx(1, "invalid PMBR boot loader");
	}

	mbr = pmbrboot->map_data;
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
	gpt_write(fd, pmbrboot);

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

	while ((ch = getopt(argc, argv, "b:g:s:")) != -1) {
		switch(ch) {
        case 'b':
            pmbrpath = optarg;
            break;
        case 'g':
//            gptpath = optarg;
//            break;
        case 's':
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
		bootset(fd);
		gpt_close(fd);
	}
	return (0);
}
