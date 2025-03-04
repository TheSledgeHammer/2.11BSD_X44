/*	$NetBSD: subr_devsw.c,v 1.38 2017/11/07 18:35:57 christos Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by MAEKAWA Masahide <gehenna@NetBSD.org>, and by Andrew Doran.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/devsw.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/fnv_hash.h>
#include <sys/user.h>

extern const struct bdevsw **bdevsw, *bdevsw0[];
extern const struct cdevsw **cdevsw, *cdevsw0[];
extern const struct linesw **linesw, *linesw0[];
extern const int sys_bdevsws, sys_cdevsws, sys_linesws;
extern int max_bdevsws, max_cdevsws, max_linesws;

struct devswtable 					sys_devsw;
struct devswtable_head 				devsw_hashtable[MAXDEVSW];
struct lock_object  				devswtable_lock;

#define devswtable_lock_init(lock)	(simple_lock_init(lock, "devswtable_lock"))
#define devswtable_lock(lock)		(simple_lock(lock))
#define devswtable_unlock(lock)		(simple_unlock(lock))

#define devswtable_io_init(major, sw)	((major) > 0 ? (sw) : ENXIO)

static void devswtable_allocate(struct devswtable *);
void devsw_io_add(struct devswtable *, dev_t, const struct bdevsw *, const struct cdevsw *, const struct linesw *);
void devsw_io_remove(dev_t, const struct bdevsw *, const struct cdevsw *, const struct linesw *);
int  devsw_io_attach(struct devswtable *, dev_t, const struct bdevsw *, const struct cdevsw *, const struct linesw *);
void devsw_io_detach(dev_t, const struct bdevsw *, const struct cdevsw *, const struct linesw *);
int  devsw_io_lookup(dev_t, const void *, int);

void
devswtable_init(void)
{
	int i;

	for(i = 0; i < MAXDEVSW; i++) {
		TAILQ_INIT(&devsw_hashtable[i]);
	}

	KASSERT(sys_bdevsws < MAXDEVSW - 1);
	KASSERT(sys_cdevsws < MAXDEVSW - 1);
	KASSERT(sys_linesws < MAXDEVSW - 1);

	devswtable_allocate(&sys_devsw);

	devswtable_lock_init(&devswtable_lock);
}

/* configure devswtable */
int
devswtable_configure(devsw, major, bdev, cdev, line)
	struct devswtable 	*devsw;
	dev_t				major;
	const struct bdevsw 		*bdev;
	const struct cdevsw 		*cdev;
	const struct linesw 		*line;
{
	int error, rv;

	rv = devsw_io_attach(devsw, major, bdev, cdev, line);
	if (rv != 0) {
		return (rv);
	}
	error = devswtable_io_init(major, rv);
	if(error == ENXIO) {
		devsw_io_detach(major, bdev, cdev, line);
		return (ENXIO);
	}

	return (error);
}

/* allocate devswtable structures */
static void
devswtable_allocate(devsw)
	register struct devswtable 	*devsw;
{
	devsw = (struct devswtable *)malloc(sizeof(*devsw), M_DEVSW, M_WAITOK);
}

int
devswtable_hash(data, major)
	void 	*data;
	dev_t 	major;
{
	Fnv32_t hash1 = fnv_32_buf(&data, sizeof(&data), FNV1_32_INIT) % MAXDEVSW;
	Fnv32_t hash2 = fnv_32_buf(&major, sizeof(&major), FNV1_32_INIT) % MAXDEVSW;
	return (hash1^hash2);
}

struct devswtable *
devswtable_lookup(data, major)
	void 	*data;
	dev_t 	major;
{
	struct devswtable_head 		*bucket;
	register devswtable_entry_t entry;
	struct devswtable 			*devsw;

	simple_lock(&devswtable_lock);
	bucket = &devsw_hashtable[devswtable_hash(data, major)];
	TAILQ_FOREACH(entry, bucket, dve_link) {
		devsw = entry->dve_devswtable;
		if(devsw->dv_data == data && devsw->dv_major == major) {
			devswtable_unlock(&devswtable_lock);
			return (devsw);
		}
	}
	devswtable_unlock(&devswtable_lock);
	return (NULL);
}

void
devswtable_add(devsw, data, major)
	struct devswtable *devsw;
	void 			*data;
	dev_t 			major;
{
	struct devswtable_head 		*bucket;
	register devswtable_entry_t entry;

	devsw->dv_data = data;
	devsw->dv_major = major;

	bucket = &devsw_hashtable[devswtable_hash(data, major)];
	entry = (devswtable_entry_t) malloc((u_long) sizeof(*entry), M_DEVSWHASH, M_WAITOK);
	entry->dve_devswtable = devsw;

	devswtable_lock(&devswtable_lock);
	TAILQ_INSERT_HEAD(bucket, entry, dve_link);
	devswtable_unlock(&devswtable_lock);
}

void
devswtable_remove(data, major)
	void 		*data;
	dev_t 		major;
{
	struct devswtable_head 		*bucket;
	register devswtable_entry_t entry;
	struct devswtable 			*devsw;

	bucket = &devsw_hashtable[devswtable_hash(data, major)];
	devswtable_lock(&devswtable_lock);
	TAILQ_FOREACH(entry, bucket, dve_link) {
		devsw = entry->dve_devswtable;
		if (devsw->dv_data == data && devsw->dv_major == major) {
			TAILQ_REMOVE(bucket, entry, dve_link);
			devswtable_unlock(&devswtable_lock);
		}
	}
}

/* return number of items on the device switch depending on type */
const int
devsw_nelems(type)
    int type;
{
    switch(type) {
    case BDEVTYPE:
        return (sys_bdevsws);
        
    case CDEVTYPE:
        return (sys_cdevsws);
        
    case LINETYPE:
        return (sys_linesws);
    }
    return (0);
}

/* BDEVSW */
int
bdevsw_attach(bdev, major)
	struct bdevsw 	*bdev;
	dev_t			major;
{
	const struct bdevsw **newptr;
	dev_t maj;
	int i;

	 devswtable_lock(&devswtable_lock);

	if(bdev == NULL) {
		return (0);
	}

	if(major < 0) {
		for (maj = sys_bdevsws; maj < max_bdevsws ; maj++) {
			if (bdevsw[maj] != NULL) {
				continue;
			}
			break;
		}
		maj = major;
	}

	if(major >= MAXDEVSW) {
		printf("%s: block majors exhausted", __func__);
		devswtable_unlock(&devswtable_lock);
		return (ENOMEM);
	}

	if (major >= max_bdevsws) {
		KASSERT(bdevsw == bdevsw0);
		newptr = calloc(MAXDEVSW, BDEVSW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			devswtable_unlock(&devswtable_lock);
			return (ENOMEM);
		}
		memcpy(newptr, bdevsw, max_bdevsws * BDEVSW_SIZE);
		bdevsw = newptr;
		max_bdevsws = MAXDEVSW;
	}

	if (bdevsw[major] != NULL) {
		devswtable_unlock(&devswtable_lock);
		return (EEXIST);
	}

	bdevsw[major] = bdev;
	devswtable_unlock(&devswtable_lock);
	return (0);
}

int
bdevsw_detach(bdev)
	struct bdevsw *bdev;
{
	int i;

	devswtable_lock(&devswtable_lock);

	if(bdevsw != NULL) {
		for (i = 0 ; i < max_bdevsws ; i++) {
			if(bdevsw[i] != bdev) {
				continue;
			}
			bdevsw[i] = NULL;
			break;
		}
	}

	devswtable_unlock(&devswtable_lock);
	return (0);
}

/*
 * Look up a block device by reference to its operations set.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the returned major is still valid when dereferenced.
 */
dev_t
bdevsw_lookup_major(bdev)
	const struct bdevsw *bdev;
{
	dev_t maj;
	for(maj = 0; maj < max_bdevsws; maj++) {
		if(bdevsw[maj] == bdev) {
			return (maj);
		}
	}
	return (NODEVMAJOR);
}

const struct bdevsw *
bdevsw_lookup(dev)
	dev_t 	dev;
{
	dev_t maj;

	if (dev == NODEV) {
		return (NULL);
	}
	maj = major(dev);
	if (maj < 0 || maj >= max_bdevsws) {
		return (NULL);
	}

	return (bdevsw[maj]);
}

void
bdevsw_add(devsw, bdev, major)
	struct devswtable 	*devsw;
	struct bdevsw 		*bdev;
	dev_t				major;
{
	devswtable_add(devsw, bdev, major);
}

void
bdevsw_remove(bdev, major)
	struct bdevsw 	*bdev;
	dev_t			major;
{
	devswtable_remove(bdev, major);
}

/* CDEVSW */
int
cdevsw_attach(cdev, major)
	struct cdevsw 	*cdev;
	dev_t 			major;
{
	const struct cdevsw **newptr;
	dev_t maj;
	int i;

	devswtable_lock(&devswtable_lock);

	if(cdev == NULL) {
		return (0);
	}

	if(major < 0) {
		for (maj = sys_cdevsws; maj < max_cdevsws ; maj++) {
			if (cdevsw[maj] != NULL) {
				continue;
			}
			break;
		}
		maj = major;
	}

	if(major >= MAXDEVSW) {
		printf("%s: character majors exhausted", __func__);
		devswtable_unlock(&devswtable_lock);
		return (ENOMEM);
	}

	if (major >= max_cdevsws) {
		KASSERT(cdevsw == cdevsw0);
		newptr = calloc(MAXDEVSW, CDEVSW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			devswtable_unlock(&devswtable_lock);
			return (ENOMEM);
		}
		memcpy(newptr, cdevsw, max_cdevsws * CDEVSW_SIZE);
		cdevsw = newptr;
		max_cdevsws = MAXDEVSW;
	}

	if (cdevsw[major] != NULL) {
		devswtable_unlock(&devswtable_lock);
		return (EEXIST);
	}

	cdevsw[major] = cdev;
	devswtable_unlock(&devswtable_lock);
	return (0);
}

int
cdevsw_detach(cdev)
	struct cdevsw *cdev;
{
	int i;

	devswtable_lock(&devswtable_lock);

	if(cdev != NULL) {
		for (i = 0 ; i < max_cdevsws ; i++) {
			if(cdevsw[i] != cdev) {
				continue;
			}
			cdevsw[i] = NULL;
			break;
		}
	}

	devswtable_unlock(&devswtable_lock);
	return (0);
}

/*
 * Look up a character device by reference to its operations set.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the returned major is still valid when dereferenced.
 */
dev_t
cdevsw_lookup_major(cdev)
	const struct cdevsw *cdev;
{
	dev_t maj;
	for(maj = 0; maj < max_cdevsws; maj++) {
		if(cdevsw[maj] == cdev) {
			return (maj);
		}
	}
	return (NODEVMAJOR);
}

const struct cdevsw *
cdevsw_lookup(dev)
	dev_t 	dev;
{
	dev_t maj;

	if (dev == NODEV) {
		return (NULL);
	}
	maj = major(dev);
	if (maj < 0 || maj >= max_cdevsws) {
		return (NULL);
	}

	return (cdevsw[maj]);
}

void
cdevsw_add(devsw, cdev, major)
	struct devswtable 	*devsw;
	struct cdevsw 		*cdev;
	dev_t				major;
{
	devswtable_add(devsw, cdev, major);
}

void
cdevsw_remove(cdev, major)
	struct cdevsw 	*cdev;
	dev_t			major;
{
	devswtable_remove(cdev, major);
}

/* LINESW */
int
linesw_attach(line, major)
	struct linesw 	*line;
	dev_t 			major;
{
	const struct linesw **newptr;
	dev_t maj;
	int i;

	devswtable_lock(&devswtable_lock);

	if(line == NULL) {
		return (0);
	}

	if(major < 0) {
		for (maj = sys_linesws; maj < max_linesws ; maj++) {
			if (linesw[maj] != NULL) {
				continue;
			}
			break;
		}
		maj = major;
	}

	if(major >= MAXDEVSW) {
		printf("%s: line majors exhausted", __func__);
		devswtable_unlock(&devswtable_lock);
		return (ENOMEM);
	}

	if (major >= max_linesws) {
		KASSERT(linesw == linesw0);
		newptr = calloc(MAXDEVSW, LINESW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			devswtable_unlock(&devswtable_lock);
			return (ENOMEM);
		}
		memcpy(newptr, linesw, max_linesws * LINESW_SIZE);
		linesw = newptr;
		max_linesws = MAXDEVSW;
	}

	if (linesw[major] != NULL) {
		devswtable_unlock(&devswtable_lock);
		return (EEXIST);
	}

	linesw[major] = line;
	devswtable_unlock(&devswtable_lock);
	return (0);
}

int
linesw_detach(line)
	struct linesw *line;
{
	int i;

	devswtable_lock(&devswtable_lock);

	if(linesw != NULL) {
		for (i = 0 ; i < max_linesws ; i++) {
			if(linesw[i] != line) {
				continue;
			}
			linesw[i] = NULL;
			break;
		}
	}

	devswtable_unlock(&devswtable_lock);
	return (0);
}

dev_t
linesw_lookup_major(line)
	const struct linesw *line;
{
	dev_t maj;
	for(maj = 0; maj < max_linesws; maj++) {
		if(linesw[maj] == line) {
			return (maj);
		}
	}
	return (NODEVMAJOR);
}

const struct linesw *
linesw_lookup(dev)
	dev_t 	dev;
{
	dev_t 	maj;

	if (dev == NODEV) {
		return (NULL);
	}
	maj = major(dev);
	if (maj < 0 || maj >= max_linesws) {
		return (NULL);
	}

	return (linesw[maj]);
}

void
linesw_add(devsw, line, major)
	struct devswtable 	*devsw;
	struct linesw 		*line;
	dev_t				major;
{
	devswtable_add(devsw, line, major);
}

void
linesw_remove(line, major)
	struct linesw 	*line;
	dev_t			major;
{
	devswtable_remove(line, major);
}

/* DEVSW IO */
void
devsw_io_add(devsw, major, bdev, cdev, line)
	struct devswtable 	*devsw;
	dev_t				major;
	const struct bdevsw 		*bdev;
	const struct cdevsw 		*cdev;
	const struct linesw 		*line;
{
	if (bdev) {
		bdevsw_add(devsw, bdev, major);
	}
	if (cdev) {
		cdevsw_add(devsw, cdev, major);
	}
	if (line) {
		linesw_add(devsw, line, major);
	}
}

void
devsw_io_remove(major, bdev, cdev, line)
	dev_t			major;
	const struct bdevsw 	*bdev;
	const struct cdevsw 	*cdev;
	const struct linesw 	*line;
{
	if (bdev) {
		bdevsw_remove(bdev, major);
	}
	if (cdev) {
		cdevsw_remove(cdev, major);
	}
	if (line) {
		linesw_remove(line, major);
	}
}

int
devsw_io_attach(devsw, major, bdev, cdev, line)
	struct devswtable 	*devsw;
	dev_t				major;
	const struct bdevsw 		*bdev;
	const struct cdevsw 		*cdev;
	const struct linesw 		*line;
{
	int error;

	if(bdev) {
		devsw_io_add(devsw, major, bdev, NULL, NULL);
		error = bdevsw_attach(bdev, major);
		if(error != 0) {
			return (ENXIO);
		}
	}
	if(cdev) {
		devsw_io_add(devsw, major, NULL, cdev, NULL);
		error = cdevsw_attach(cdev, major);
		if(error != 0) {
			return (ENXIO);
		}
	}
	if(line) {
		devsw_io_add(devsw, major, NULL, NULL, line);
		error = linesw_attach(line, major);
		if(error != 0) {
			return (ENXIO);
		}
	}

	return (error);
}

void
devsw_io_detach(major, bdev, cdev, line)
	dev_t			major;
	const struct bdevsw 	*bdev;
	const struct cdevsw 	*cdev;
	const struct linesw 	*line;
{
	int error;

	if(bdev) {
		devsw_io_remove(major, bdev, NULL, NULL);
		error = bdevsw_detach(bdev, major);
		if(error) {
			printf("devsw_io_detach: block device detached");
		}else {
			printf("devsw_io_detach: block device not found");
		}
	}
	if(cdev) {
		devsw_io_remove(major, NULL, cdev, NULL);
		error = cdevsw_detach(cdev, major);
		if(error) {
			printf("devsw_io_detach: character device detached");
		} else {
			printf("devsw_io_detach: character device not found");
		}
	}
	if(line) {
		devsw_io_remove(major, NULL, NULL, line);
		error = linesw_detach(line, major);
		if(error) {
			printf("devsw_io_detach: line device detached");
		} else {
			printf("devsw_io_detach: line device not found");
		}
	}
}

int
devsw_io_lookup(major, data, type)
	dev_t 	major;
	const void 	*data;
	int		type;
{
	struct devswtable *devsw;
    const struct bdevsw *bd;
    const struct cdevsw *cd;
    const struct linesw *ld;
    
    devsw = devswtable_lookup(data, major);
	if(devsw == NULL) {
		return (NODEV);
	}

    switch(type) {
    case BDEVTYPE:
    	bd = bdevsw_lookup(major);
        if(bd == DTOB(devsw)) {
            return (0);
        }
        break;

    case CDEVTYPE:
        cd = cdevsw_lookup(major);
        if(cd == DTOC(devsw)) {
        	return (0);
        }
        break;

    case LINETYPE:
        ld = linesw_lookup(major);
        if(ld == DTOL(devsw)) {
        	return (0);
        }
        break;
    }

    return (ENXIO);
}

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
int
devsw_io_iskmemdev(dev)
	register dev_t dev;
{
	if (major(dev) == 2 && (minor(dev) == 0 || minor(dev) == 1)) {
		return (1);
	}
	return (0);
}

int
devsw_io_iszerodev(dev)
	dev_t dev;
{
	if (major(dev) == 2 && minor(dev) == 12) {
		return (1);
	}
	return (0);
}

/*
 * Routine to determine if a device is a disk.
 *
 * A minimal stub routine can always return 0.
 */
int
devsw_io_isdisk(dev, type)
	dev_t dev;
	register int type;
{
	switch (major(dev)) {
	default:
		return (0);
	}
}

/*
 * Convert from character dev_t to block dev_t.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the major number is still valid when dereferenced.
 */
dev_t
devsw_io_chrtoblk(dev)
	dev_t dev;
{
	dev_t rv, bmajor, cmajor;
	int i;

	cmajor = major(dev);
	bmajor = NODEVMAJOR;
	rv = NODEV;

	devswtable_lock(&devswtable_lock);
	if (cmajor < 0 || cmajor >= max_cdevsws || cdevsw[cmajor] == NULL) {
		devswtable_unlock(&devswtable_lock);
		return (NODEV);
	}
	if (bmajor >= 0 && bmajor < max_bdevsws && bdevsw[bmajor] != NULL) {
		rv = makedev(bmajor, minor(dev));
	}
	devswtable_unlock(&devswtable_lock);

	return (rv);
}

/*
 * Convert from block dev_t to character dev_t.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the major number is still valid when dereferenced.
 */
dev_t
devsw_io_blktochr(dev)
	dev_t dev;
{
	dev_t rv, bmajor, cmajor;
	int i;

	bmajor = major(dev);
	cmajor = NODEVMAJOR;
	rv = NODEV;

	devswtable_lock(&devswtable_lock);
	if (bmajor < 0 || bmajor >= max_bdevsws || bdevsw[bmajor] == NULL) {
		devswtable_unlock(&devswtable_lock);
		return (NODEV);
	}
	if (cmajor >= 0 && cmajor < max_cdevsws && cdevsw[cmajor] != NULL) {
		rv = makedev(cmajor, minor(dev));
	}
	devswtable_unlock(&devswtable_lock);

	return (rv);
}

/* bdevsw dev ops */
int
bdev_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io_lookup(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_open)(dev, flag, devtype, p);

	return (rv);
}

int
bdev_close(dev_t dev, int fflag, int devtype, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io_lookup(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_close)(dev, fflag, devtype, p);

	return (rv);
}

void
bdev_strategy(struct buf *bp)
{
	const struct bdevsw *d;
	int error;

	error = devsw_io_lookup(bp->b_dev, d, BDEVTYPE);
	if(error != 0) {
		return;
	}

	(*d->d_strategy)(bp);

	return;
}

int
bdev_ioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io_lookup(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_ioctl)(dev, cmd, data, fflag, p);

	return (rv);
}

int
bdev_dump(dev_t dev, daddr_t blkno, caddr_t addr, size_t size)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io_lookup(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_dump)(dev, blkno, addr, size);

	return (rv);
}

int
bdev_root(void)
{
	return (-1);
}

int
bdev_size(dev_t dev)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io_lookup(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_psize)(dev);

	return (rv);
}

daddr_t
bdev_discard(dev_t dev, off_t pos, off_t len)
{
	const struct bdevsw *d;
	daddr_t rv;
	int error;

	error = devsw_io_lookup(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_discard)(dev, pos, len);

	return (rv);
}

/* cdevsw dev ops */
int
cdev_open(dev_t dev, int oflags, int devtype, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;


	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_open)(dev, oflags, devtype, p);

	return (rv);
}

int
cdev_close(dev_t dev, int fflag, int devtype, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_close)(dev, fflag, devtype, p);

	return (rv);
}

int
cdev_read(dev_t dev, struct uio *uio, int ioflag)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_read)(dev, uio, ioflag);

	return (rv);
}

int
cdev_write(dev_t dev, struct uio *uio, int ioflag)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_write)(dev, uio, ioflag);

	return (rv);
}

int
cdev_ioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_ioctl)(dev, cmd, data, fflag, p);

	return (rv);
}

int
cdev_stop(struct tty *tp, int rw)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_stop)(tp, rw);

	return (rv);
}

int
cdev_reset(int uban)
{
	return (-1);
}

struct tty *
cdev_tty(dev_t dev)
{
	const struct cdevsw *c;
	struct tty *rv;
	int error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if((error != 0) && (c = cdevsw_lookup(dev)) == NULL) {
		return (NULL);
	}
	
	rv = (*c->d_tty)(dev);

	return (rv);
}

int
cdev_select(dev_t dev, int which, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_select)(dev, which, p);

	return (rv);
}

int
cdev_poll(dev_t dev, int events, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_poll)(dev, events, p);

	return (rv);
}

caddr_t
cdev_mmap(dev_t dev, off_t off, int flag)
{
	const struct cdevsw *c;
	caddr_t rv;
	int error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return ((caddr_t)error);
	}

	rv = (*c->d_mmap)(dev, off, flag);

	return (rv);
}

void
cdev_strategy(struct buf *bp)
{
	const struct cdevsw *c;
	int error;

	error = devsw_io_lookup(bp->b_dev, c, CDEVTYPE);
	if(error != 0) {
		return;
	}

	(*c->d_strategy)(bp);

	return;
}

int
cdev_kqfilter(dev_t dev, struct knote *kn)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_kqfilter)(dev, kn);

	return (rv);
}

daddr_t
cdev_discard(dev_t dev, off_t pos, off_t len)
{
	const struct cdevsw *c;
	daddr_t rv;
	int error;

	error = devsw_io_lookup(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_discard)(dev, pos, len);

	return (rv);
}

/* linesw dev ops */
int
line_open(dev_t dev, struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_open)(dev, tp);

	return (rv);
}

int
line_close(struct tty *tp, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_close)(tp, flag);

	return (rv);
}

int
line_read(struct tty *tp, struct uio *uio, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_read)(tp, uio, flag);

	return (rv);
}

int
line_write(struct tty *tp, struct uio *uio, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_write)(tp, uio, flag);

	return (rv);
}

int
line_ioctl(struct tty *tp, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_ioctl)(tp, cmd, data, flag, p);

	return (rv);
}

int
line_rint(int c, struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_rint)(c, tp);

	return (rv);
}

int
line_rend(void)
{
	return (-1);
}

int
line_meta(void)
{
	return (-1);
}

int
line_start(struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_start)(tp);

	return (rv);
}

int
line_modem(struct tty *tp, int flag)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_modem)(tp, flag);

	return (rv);
}

int
line_poll(struct tty *tp, int flag, struct proc *p)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io_lookup(tp->t_dev, l, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_poll)(tp, flag, p);

	return (rv);
}
