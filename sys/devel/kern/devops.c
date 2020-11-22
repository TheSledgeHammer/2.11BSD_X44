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

#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/user.h>

#include <devel/sys/devsw.h>

extern const struct bdevsw *bdevsw0[];
extern const struct cdevsw *cdevsw0[];
extern const struct linesw *linesw0[];
extern const int sys_bdevsws, sys_cdevsws, sys_linesws;
extern int max_bdevsws, max_cdevsws, max_linesws;

struct devswops 			*dvops;
struct lock_object  		*devsw_lock;

void
dvop_malloc(dvops)
	struct devswops *dvops;
{
	MALLOC(dvops, struct devswops *, sizeof(struct devswops *), M_DEVSW, M_WAITOK);
}

void
dvop_free(dvops)
	struct devswops *dvops;
{
	FREE(dvops, M_DEVSW, M_WAITOK);
}

int
dvop_attach(devsw, devname, device, major)
	struct devswtable 	*devsw;
    const char      	*devname;
    struct device   	*device;
    dev_t 				major;
{
    struct dvop_attach_args args;
    int error;

    args.d_devswtable.dv_ops = &dvops;
    args.d_devswtable = devsw;
    args.d_name = devname;
    args.d_device = device;
    args.d_major = major;

    if(dvops->dvop_attach == NULL) {
    	return (EINVAL);
    }

    error = dvops->dvop_attach(devsw, devname, device, major);

    return (error);
}

int
dvop_detach(devsw, devname, device, major)
	struct devswtable 	*devsw;
	const char      	*devname;
    struct device   	*device;
    dev_t 				major;
{
    struct dvop_detach_args args;
    int error;

    args.d_devswtable.dv_ops = &dvops;
    args.d_devswtable = devsw;
    args.d_name = devname;
    args.d_device = device;
    args.d_major = major;

    if(dvops->dvop_detach == NULL) {
    	return (EINVAL);
    }

    error = dvops->dvop_detach(devsw, devname, device, major);

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

/* bdevsw */
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

	 simple_lock(&devsw_lock);

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
		simple_unlock(&devsw_lock);
		return (ENOMEM);
	}

	if (maj >= max_bdevsws) {
		KASSERT(bdevsw == bdevsw0);
		newptr = malloc(MAXDEVSW * BDEVSW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			simple_unlock(&devsw_lock);
			return (ENOMEM);
		}
		memcpy(newptr, bdevsw, max_bdevsws * BDEVSW_SIZE);
		bdevsw = newptr;
		max_bdevsws = MAXDEVSW;
	}

	if (bdevsw[maj] != NULL) {
		simple_unlock(&devsw_lock);
		return (EEXIST);
	}

	bdevsw = DTOB(args->d_devswtable);
	devsw_add(devsw, bdevsw, maj);

	simple_unlock(&devsw_lock);

	return (0);
}

int
bdevsw_detach(args)
	struct dvop_detach_args *args;
{
	struct bdevsw *bdevsw = DTOB(args->d_devswtable);
	dev_t maj = args->d_major;
	int i;

	simple_lock(&devsw_lock);

	if(bdevsw != NULL) {
		for (i = 0 ; i < max_bdevsws ; i++) {
			if(bdevsw[i] != bdevsw[maj]) {
				continue;
			}
			bdevsw[i] = NULL;
			break;
		}
	}
	devsw_remove(bdevsw, maj);
	simple_unlock(&devsw_lock);

	return (0);
}

/* cdevsw */
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

	simple_lock(&devsw_lock);

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
		simple_unlock(&devsw_lock);
		return (ENOMEM);
	}

	if (maj >= max_cdevsws) {
		KASSERT(cdevsw == cdevsw0);
		newptr = malloc(MAXDEVSW * CDEVSW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			simple_unlock(&devsw_lock);
			return (ENOMEM);
		}
		memcpy(newptr, cdevsw, max_cdevsws * CDEVSW_SIZE);
		cdevsw = newptr;
		max_cdevsws = MAXDEVSW;
	}

	if (cdevsw[maj] != NULL) {
		simple_unlock(&devsw_lock);
		return (EEXIST);
	}

	cdevsw = DTOC(args->d_devswtable);
	devsw_add(devsw, cdevsw, maj);
	simple_unlock(&devsw_lock);

	return (0);
}

int
cdevsw_detach(args)
	struct dvop_detach_args *args;
{
	struct cdevsw *cdevsw = DTOC(args->d_devswtable);
	dev_t maj = args->d_major;
	int i;

	simple_lock(&devsw_lock);

	if(cdevsw != NULL) {
		for (i = 0 ; i < max_cdevsws ; i++) {
			if(cdevsw[i] != cdevsw[maj]) {
				continue;
			}
			cdevsw[i] = NULL;
			break;
		}
	}
	devsw_remove(cdevsw, maj);
	simple_unlock(&devsw_lock);

	return (0);
}

/* linesw */
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

	simple_lock(&devsw_lock);

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
		simple_unlock(&devsw_lock);
		return (ENOMEM);
	}

	if (maj >= max_linesws) {
		KASSERT(linesw == linesw0);
		newptr = malloc(MAXDEVSW * LINESW_SIZE, M_DEVSW, M_NOWAIT);
		if (newptr == NULL) {
			simple_unlock(&devsw_lock);
			return (ENOMEM);
		}
		memcpy(newptr, linesw, max_linesws * LINESW_SIZE);
		linesw = newptr;
		max_linesws = MAXDEVSW;
	}

	if (linesw[maj] != NULL) {
		simple_unlock(&devsw_lock);
		return (EEXIST);
	}

	linesw = DTOL(args->d_devswtable);
	devsw_add(devsw, linesw, maj);
	simple_unlock(&devsw_lock);

	return (0);
}

int
linesw_detach(args)
	struct dvop_detach_args *args;
{
	struct linesw *linesw = DTOL(args->d_devswtable);
	dev_t maj = args->d_major;
	int i;

	simple_lock(&devsw_lock);

	if(linesw != NULL) {
		for (i = 0 ; i < max_linesws ; i++) {
			if(linesw[i] != linesw[maj]) {
				continue;
			}
			linesw[i] = NULL;
			break;
		}
	}
	devsw_remove(linesw, maj);
	simple_unlock(&devsw_lock);

	return (0);
}
