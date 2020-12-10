/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */
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

struct devswtable 		*sys_devsw;
struct devswops 		*sys_dvops;
struct lock_object  	*devswtable_lock;

struct devswtable_head 	devsw_hashtable[MAXDEVSW]; /* hash table of devices in the device switch table (includes bdevsw, cdevsw & linesw) */

void
devswtable_init()
{
	for(int i = 0; i < MAXDEVSW; i++) {
		TAILQ_INIT(&devsw_hashtable[i]);
	}

	KASSERT(sys_bdevsws < MAXDEVSW - 1);
	KASSERT(sys_cdevsws < MAXDEVSW - 1);
	KASSERT(sys_linesws < MAXDEVSW - 1);

	devswtable_allocate(&sys_devsw, &sys_dvops);

	simple_lock_init(&devswtable_lock, "devswtable_lock");
}

/* allocate devswtable structures */
static void
devswtable_allocate(devsw, dvops)
	register struct devswtable *devsw;
	register struct devswops *dvops;
{
	devsw = (struct devswtable *)malloc(sizeof(*devsw), M_DEVSW, M_WAITOK);
	dvops = (struct devswops *)malloc(sizeof(*dvops), M_DEVSW, M_WAITOK);
	devsw->dv_ops = dvops;
}

int
devswtable_hash(data, dev)
	void 	*data;
	dev_t 	dev;
{
	Fnv32_t hash1 = fnv_32_buf(&dev, sizeof(&dev), FNV1_32_INIT) % MAXDEVSW;
	Fnv32_t hash2 = fnv_32_buf(&data, sizeof(&data), FNV1_32_INIT) % MAXDEVSW;
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
	for(entry = TAILQ_FIRST(bucket); entry != NULL; entry = TAILQ_NEXT(entry, dve_link)) {
		devsw = entry->dve_devswtable;
		if(devsw->dv_data == data && devsw->dv_major == major) {
			simple_unlock(&devswtable_lock);
			return (devsw);
		}
	}
	simple_unlock(&devswtable_lock);
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

	simple_lock(&devswtable_lock);
	TAILQ_INSERT_HEAD(bucket, entry, dve_link);
	simple_unlock(&devswtable_lock);
}

void
devswtable_remove(data, major)
	void 	*data;
	dev_t 	major;
{
	struct devswtable_head 		*bucket;
	register devswtable_entry_t entry;
	struct devswtable 			*devsw;

	bucket = &devsw_hashtable[devswtable_hash(data, major)];
	for(entry = TAILQ_FIRST(bucket); entry != NULL; entry = TAILQ_NEXT(entry, dve_link)) {
		devsw = entry->dve_devswtable;
		if(devsw->dv_data == data && devsw->dv_major == major) {
			TAILQ_REMOVE(bucket, entry, dve_link);
		}
	}
}

/*
int
devswtable_attach(major, devsw, type)
	dev_t 				major;
	struct devswtable  *devsw;
	int 				type;
{
	int error = 0;

    switch(type) {
    case BDEVTYPE:
    	struct bdevsw *bd = DTOB(devsw);
    	error = DVOP_ATTACH(devsw, bd, major);
        if(error) {
        	devswtable_add(devsw, bd, major);
            return (error);
        }
        break;

    case CDEVTYPE:
        struct cdevsw *cd = DTOC(devsw);
    	error = DVOP_ATTACH(devsw, cd, major);
        if(error) {
        	devswtable_add(devsw, cd, major);
            return (error);
        }
        break;

    case LINETYPE:
        struct linesw *ld = DTOL(devsw);
    	error = DVOP_ATTACH(devsw, ld, major);
        if(error) {
        	devswtable_add(devsw, ld, major);
            return (error);
        }
        break;
    }

    return (error);
}

int
devswtable_detach(major, devsw, type)
	dev_t 				major;
	struct devswtable  *devsw;
	 int 				type;
{
	int error = 0;

    switch(type) {
    case BDEVTYPE:
    	struct bdevsw *bd = DTOB(devsw);
    	error = DVOP_DETACH(devsw, bd, major);
        if(error) {
        	devswtable_remove(bd, major);
            return (error);
        }
        break;

    case CDEVTYPE:
        struct cdevsw *cd = DTOC(devsw);
    	error = DVOP_DETACH(devsw, cd, major);
        if(error) {
        	devswtable_remove(cd, major);
            return (error);
        }
        break;

    case LINETYPE:
        struct linesw *ld = DTOL(devsw);
    	error = DVOP_DETACH(devsw, ld, major);
        if(error) {
        	devswtable_remove(ld, major);
            return (error);
        }
        break;
    }

    return (error);
}
*/
int
devswtable_lookup_bdevsw(devsw, dev)
	struct devswtable *devsw;
	dev_t 				dev;
{
	struct bdevsw *bdevsw;
	bdevsw = devop_lookup_bdevsw(devsw, dev);

	if(bdevsw == NULL) {
		return (ENXIO);
	}

	return (0);
}

int
devswtable_lookup_cdevsw(devsw, dev)
	struct devswtable 	*devsw;
	dev_t 				dev;
{
	struct cdevsw *cdevsw;
	cdevsw = devop_lookup_cdevsw(devsw, dev);

	if(cdevsw == NULL) {
		return (ENXIO);
	}

	return (0);
}

int
devswtable_lookup_linesw(devsw, dev)
	struct devswtable *devsw;
	dev_t 			dev;
{
	struct linesw *linesw;
	linesw = devop_lookup_linesw(devsw, dev);

	if(linesw == NULL) {
		return (ENXIO);
	}

	return (0);
}

int
devswtable_dev_finder(major, devsw, type)
    dev_t 				major;
	struct devswtable 	*devsw;
    int 				type;
{
    int error = 0;

    switch(type) {
    case BDEVTYPE:
    	struct bdevsw *bd = DTOB(devsw);
        if(bd) {
            error = devswtable_lookup_bdevsw(devsw, major);
            return (error);
        }
        break;

    case CDEVTYPE:
        struct cdevsw *cd = DTOC(devsw);
        if(cd) {
            error = devswtable_lookup_cdevsw(devsw, major);
            return (error);
        }
        break;

    case LINETYPE:
        struct linesw *ld = DTOL(devsw);
        if(ld) {
            error = devswtable_lookup_linesw(devsw, major);
            return (error);
        }
        break;
    }

    return (error);
}

int
devsw_io(major, data, type)
	dev_t 	major;
	void 	*data;
	int		type;
{
	struct devswtable *devsw = devswtable_lookup(data, major);

	if(devsw == NULL) {
		return (ENXIO);
	}

	return (devswtable_dev_finder(major, devsw, type));
}

/* devsw operations */
int
dvop_attach(devsw, device, major)
	struct devswtable 	*devsw;
    struct device   	*device;
    dev_t 				major;
{
    struct dvop_attach_args args;
    int error;

    args.d_devswtable.dv_ops = &dvops;
    args.d_devswtable = devsw;
    args.d_device = device;
    args.d_major = major;

    if(dvops->dvop_attach == NULL) {
    	return (EINVAL);
    }

    error = dvops->dvop_attach(devsw, device, major);

    return (error);
}

int
dvop_detach(devsw, device, major)
	struct devswtable 	*devsw;
    struct device   	*device;
    dev_t 				major;
{
    struct dvop_detach_args args;
    int error;

    args.d_devswtable.dv_ops = &dvops;
    args.d_devswtable = devsw;
    args.d_device = device;
    args.d_major = major;

    if(dvops->dvop_detach == NULL) {
    	return (EINVAL);
    }

    error = dvops->dvop_detach(devsw, device, major);

    return (error);
}

struct bdevsw *
devop_lookup_bdevsw(devsw, dev)
	struct devswtable 	*devsw;
	dev_t 				dev;
{
	struct bdevsw *bdevsw = DTOB(devsw);
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

struct cdevsw *
devop_lookup_cdevsw(devsw, dev)
	struct devswtable 	*devsw;
	dev_t 				dev;
{
	struct cdevsw *cdevsw = DTOC(devsw);
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

struct linesw *
devop_lookup_linesw(devsw, dev)
	struct devswtable 	*devsw;
	dev_t 				dev;
{
	struct linesw *linesw = DTOL(devsw);
	dev_t maj;

	if (dev == NODEV) {
		return (NULL);
	}
	maj = major(dev);
	if (maj < 0 || maj >= max_linesws) {
		return (NULL);
	}

	return (linesw[maj]);
}

/* bdevsw op */
struct devswops *bdevswops = {
	.dvop_attach = bdevsw_attach,
	.dvop_detach = bdevsw_detach,
};

int
bdevsw_attach(args)
	struct dvop_attach_args *args;
{
	struct bdevsw *bdevsw;
	const struct bdevsw **newptr;
	dev_t maj;
	int i;

	 simple_lock(&devswtable_lock);

	if(maj < 0) {
		for (maj = sys_bdevsws; maj < max_bdevsws ; maj++) {
			if (bdevsw[maj] != NULL) {
				continue;
			}
			break;
		}
		maj = args->d_major;
	}

	if(maj >= MAXDEVSW) {
		printf("%s: block majors exhausted", __func__);
		simple_unlock(&devswtable_lock);
		return (ENOMEM);
	}

	if (maj >= max_bdevsws) {
		KASSERT(bdevsw == bdevsw0);
		newptr = malloc(MAXDEVSW * BDEVSW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			simple_unlock(&devswtable_lock);
			return (ENOMEM);
		}
		memcpy(newptr, bdevsw, max_bdevsws * BDEVSW_SIZE);
		bdevsw = newptr;
		max_bdevsws = MAXDEVSW;
	}

	if (bdevsw[maj] != NULL) {
		simple_unlock(&devswtable_lock);
		return (EEXIST);
	}

	simple_unlock(&devswtable_lock);
	return (0);
}

int
bdevsw_detach(args)
	struct dvop_detach_args *args;
{
	struct bdevsw *bdevsw = DTOB(args->d_devswtable);
	dev_t maj = args->d_major;
	int i;

	simple_lock(&devswtable_lock);

	if(bdevsw != NULL) {
		for (i = 0 ; i < max_bdevsws ; i++) {
			if(bdevsw[i] != bdevsw[maj]) {
				continue;
			}
			bdevsw[i] = NULL;
			break;
		}
	}

	simple_unlock(&devswtable_lock);
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

/* cdevsw ops */
struct devswops *cdevswops = {
	.dvop_attach = cdevsw_attach,
	.dvop_detach = cdevsw_detach,
};

int
cdevsw_attach(args)
	struct dvop_attach_args *args;
{
	struct cdevsw *cdevsw;
	const struct cdevsw **newptr;
	dev_t maj;
	int i;

	simple_lock(&devswtable_lock);

	if(maj < 0) {
		for (maj = sys_cdevsws; maj < max_cdevsws ; maj++) {
			if (cdevsw[maj] != NULL) {
				continue;
			}
			break;
		}
		maj = args->d_major;
	}

	if(maj >= MAXDEVSW) {
		printf("%s: character majors exhausted", __func__);
		simple_unlock(&devswtable_lock);
		return (ENOMEM);
	}

	if (maj >= max_cdevsws) {
		KASSERT(cdevsw == cdevsw0);
		newptr = malloc(MAXDEVSW * CDEVSW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			simple_unlock(&devswtable_lock);
			return (ENOMEM);
		}
		memcpy(newptr, cdevsw, max_cdevsws * CDEVSW_SIZE);
		cdevsw = newptr;
		max_cdevsws = MAXDEVSW;
	}

	if (cdevsw[maj] != NULL) {
		simple_unlock(&devswtable_lock);
		return (EEXIST);
	}

	simple_unlock(&devswtable_lock);
	return (0);
}

int
cdevsw_detach(args)
	struct dvop_detach_args *args;
{
	struct cdevsw *cdevsw = DTOC(args->d_devswtable);
	dev_t maj = args->d_major;
	int i;

	simple_lock(&devswtable_lock);

	if(cdevsw != NULL) {
		for (i = 0 ; i < max_cdevsws ; i++) {
			if(cdevsw[i] != cdevsw[maj]) {
				continue;
			}
			cdevsw[i] = NULL;
			break;
		}
	}

	simple_unlock(&devswtable_lock);
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

/* linesw ops */
struct devswops *lineswops = {
	.dvop_attach = linesw_attach,
	.dvop_detach = linesw_detach,
};

int
linesw_attach(args)
	struct dvop_attach_args *args;
{
	struct linesw *linesw;
	const struct cdevsw **newptr;
	dev_t maj;
	int i;

	simple_lock(&devswtable_lock);

	if(maj < 0) {
		for (maj = sys_linesws; maj < max_linesws ; maj++) {
			if (linesw[maj] != NULL) {
				continue;
			}
			break;
		}
		maj = args->d_major;
	}

	if(maj >= MAXDEVSW) {
		printf("%s: line majors exhausted", __func__);
		simple_unlock(&devswtable_lock);
		return (ENOMEM);
	}

	if (maj >= max_linesws) {
		KASSERT(linesw == &linesw0);
		newptr = malloc(MAXDEVSW * LINESW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			simple_unlock(&devswtable_lock);
			return (ENOMEM);
		}
		memcpy(newptr, linesw, max_linesws * LINESW_SIZE);
		linesw = newptr;
		max_linesws = MAXDEVSW;
	}

	if (linesw[maj] != NULL) {
		simple_unlock(&devswtable_lock);
		return (EEXIST);
	}

	simple_unlock(&devswtable_lock);
	return (0);
}

int
linesw_detach(args)
	struct dvop_detach_args *args;
{
	struct linesw *linesw = DTOL(args->d_devswtable);
	dev_t maj = args->d_major;
	int i;

	simple_lock(&devswtable_lock);

	if(linesw != NULL) {
		for (i = 0 ; i < max_linesws ; i++) {
			if(linesw[i] != linesw[maj]) {
				continue;
			}
			linesw[i] = NULL;
			break;
		}
	}

	simple_unlock(&devswtable_lock);
	return (0);
}

/*
 * Look up a line device by reference to its operations set.
 *
 * => Caller must ensure that the device is not detached, and therefore
 *    that the returned major is still valid when dereferenced.
 */
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

/* bdevsw */
int
bdev_open(dev_t dev, int flag, int devtype, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
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

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_close)(dev, fflag, devtype, p);

	return (rv);
}

int
bdev_strategy(dev_t dev, int fflag, int devtype, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_strategy)(dev, fflag, devtype, p);

	return (rv);
}

int
bdev_ioctl(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_ioctl)(dev, cmd, data, fflag, p);

	return (rv);
}


int
bdev_dump(dev_t dev)
{
	const struct bdevsw *d;
	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_dump)(dev);

	return (rv);
}

/*
int
bdev_root()
{
	const struct bdevsw *d;
	dev_t dev;
	int rv;

	int rv, error;

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_root)();

	return (rv);
}
*/

daddr_t
bdev_size(dev_t dev)
{
	const struct bdevsw *d;
	daddr_t rv;
	int error;

	error = devsw_io(dev, d, BDEVTYPE);
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

	error = devsw_io(dev, d, BDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*d->d_discard)(dev, pos, len);

	return (rv);
}

int
cdev_open(dev_t dev, int oflags, int devtype, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;


	error = devsw_io(dev, c, CDEVTYPE);
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

	error = devsw_io(dev, c, CDEVTYPE);
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

	error = devsw_io(dev, c, CDEVTYPE);
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

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_write)(dev, uio, ioflag);

	return (rv);
}

int
cdev_ioctl(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
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

	error = devsw_io(tp->t_dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_stop)(tp, rw);

	return (rv);
}
/*
int
cdev_reset(int uban)
{

}
*/

struct tty
cdev_tty(dev_t dev)
{
	const struct cdevsw *c;
	struct tty *rv;
	int error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_tty)(dev);

	return (rv);
}

int
cdev_select(dev_t dev, int which, struct proc *p)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(dev, c, CDEVTYPE);
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

	error = devsw_io(dev, c, CDEVTYPE);
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

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_mmap)(dev, off, flag);

	return (rv);
}

int
cdev_strategy(struct buf *bp)
{
	const struct cdevsw *c;
	int rv, error;

	error = devsw_io(bp->b_dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_strategy)(bp);

	return (rv);
}

daddr_t
cdev_discard(dev_t dev, off_t pos, off_t len)
{
	const struct cdevsw *c;
	daddr_t rv;
	int error;

	error = devsw_io(dev, c, CDEVTYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*c->d_discard)(dev, pos, len);

	return (rv);
}

int
line_open(dev_t dev, struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
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

	error = devsw_io(tp->t_dev, c, LINETYPE);
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

	error = devsw_io(tp->t_dev, c, LINETYPE);
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

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_write)(tp, uio, flag);

	return (rv);
}

int
line_ioctl(struct tty *tp, int cmd, caddr_t data, int flag, struct proc *p)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
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

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_rint)(c, tp);

	return (rv);
}

/*
int
line_rend()
{

}

int
line_meta()
{

}
*/

int
line_start(struct tty *tp)
{
	const struct linesw *l;
	int rv, error;

	error = devsw_io(tp->t_dev, c, LINETYPE);
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

	error = devsw_io(tp->t_dev, c, LINETYPE);
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

	error = devsw_io(tp->t_dev, c, LINETYPE);
	if(error != 0) {
		return (error);
	}

	rv = (*l->l_poll)(tp, flag, p);

	return (rv);
}
