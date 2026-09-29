/*-
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
 *
 * Copyright (c) 1998 Robert Nordier
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
 *
 * $FreeBSD: src/sys/boot/common/ufsread.c,v 1.12 2003/08/25 23:30:41 obrien Exp $
 * $DragonFly: src/sys/boot/common/ufsread.c,v 1.5 2008/09/13 11:46:28 corecode Exp $
 */

/*
 * fsread.c
 * Changes ufsread.c to be a generic filesystem read function.
 */

#include <sys/cdefs.h>

#ifdef BOOT2
#include "boot2.h"
#else
#include <sys/param.h>
#endif
#include <sys/disklabel.h>
#include <sys/dirent.h>

#include <lib/libsa/stand.h>

int fsread(const char *, void *, size_t, off_t);
ino_t lookup(const char *);

static uint8_t ls;
static off_t fs_off;

static unsigned int
fsfind(const char *name, ino_t *ino)
{
    static char buf[DEV_BSIZE];
    static struct dirent *d;
    char *s;
    int n;

    fs_off = 0;
    n = fsread(name, buf, DEV_BSIZE, fs_off);
    if (n > 0) {
        for (s = buf; s < buf + DEV_BSIZE;) {
            memcpy(d, s, sizeof(struct dirent));
            if (ls) {
                printf("%s ", d->d_name);
            } else if (!strcmp(name, d->d_name)) {
                *ino = d->d_fileno;
                return (d->d_type);
            }
            s += d->d_reclen;
        }
    }
    if (n != -1 && ls) {
        printf("\n");
    }
    return (0);
}

ino_t
lookup(const char *path)
{
    static char name[MAXNAMLEN + 1];
    const char *s;
    ino_t ino;
    ssize_t n;
    int dt;

    ino = 2; /* UFS_ROOTINO */
    dt = DT_DIR;
    for (;;) {
        if (*path == '/') {
            path++;
        }
        if (!*path) {
            break;
        }
        for (s = path; *s && *s != '/'; s++) {
            ;
        }
        n = (s - path);
        if (n > MAXNAMLEN) {
            return (0);
        }
        ls = *path == '?' && n == 1 && !*s;
        memcpy(name, path, n);
        name[n] = 0;
        if (dt != DT_DIR) {
            printf("%s: not a directory.\n", name);
            return (0);
        }
        dt = fsfind(name, &ino);
        if (dt <= 0) {
            break;
        }
        path = s;
    }
    return (dt == DT_REG ? ino : 0);
}

static int
fsread_path(const char *path, int mode, void *buf, size_t nbyte, off_t offset)
{
	char *s, buffer[nbyte];
    int fd;
    off_t pos;
    size_t size, n, nb, ret;

	fd = open(path, mode);
    if (fd < 0) {
        printf("Error opening file");
        return (-1);
    }
    pos = lseek(fd, offset, SEEK_SET);
    if (pos < 0) {
        printf("Error in lseek");
        close(fd);
        return (-1);
    }
    s = buf;
    size = sizeof(buffer);
    n = size - offset;
    if (nbyte > n) {
        nbyte = n;
    }
    nb = nbyte;
    //while (nb) {
    	ret = read(fd, buffer, nb);
        if (ret < 0) {
            printf("Error reading file");
            close(fd);
            return (-1);
        }
        buffer[nb] = '\0';
        //n -= offset;
        if (n > nb) {
            n = nb;
        }
        memcpy(s, buffer, n);
        //s += n;
		//offset += (off_t)n;
        //nb -= n;
    //}
    close(fd);
    return (0);
}

int
fsread(const char *path, void *buf, size_t nbyte, off_t offset)
{
	return (fsread_path(path, O_RDONLY, buf, nbyte, offset));
}
