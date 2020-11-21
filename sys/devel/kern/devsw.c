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

#include <sys/queue.h>
#include <sys/user.h>
#include <devel/sys/devsw.h>

struct devswops 	*dvops;
struct devsw_head 	devsw_list;

void
devsw_add(devsw)
	struct devsw *devsw;
{
	TAILQ_INSERT_HEAD(&devsw_list, devsw, dv_entries);
}

void
devsw_remove(devsw)
	struct devsw *devsw;
{
	TAILQ_REMOVE(&devsw_list, devsw, dv_entries);
}

devsw_lookup()
{

}

void
devsw_init()
{
	TAILQ_INIT(&devsw_list);
	dvop_malloc(&dvops);
}

void
dvop_malloc(dvops)
	struct devswops *dvops;
{
	MALLOC(dvops, struct devswops *, sizeof(struct devswops *), M_DEVSW, M_WAITOK);
}

int
dvop_attach(devname, device, major)
    const char      *devname;
    struct device   *device;
    int 			major;
{
    struct dvop_attach_args args;
    int error;

    args.d_devsw.dv_ops = &dvops;
    args.d_name = devname;
    args.d_device = device;
    args.d_major = major;

    if(dvops->dvop_attach == NULL) {
    	return (EOPNOTSUPP);
    }

    error = dvops->dvop_attach(devname, device, major);

    return (error);
}

int
dvop_detach(devname, device, major)
    const char      *devname;
    struct device   *device;
    int 			major;
{
    struct dvop_detach_args args;
    int error;

    args.d_devsw.dv_ops = &dvops;
    args.d_name = devname;
    args.d_device = device;
    args.d_major = major;

    if(dvops->dvop_detach == NULL) {
    	return (EOPNOTSUPP);
    }

    error = dvops->dvop_detach(devname, device, major);

    return (error);
}

int
dvop_lookup(major)
	int 		major;
{
    struct dvop_lookup_args args;
    int error;

    args.d_devsw.dv_ops = &dvops;

    args.d_major = major;

    if(dvops->dvop_lookup == NULL) {
    	return (EOPNOTSUPP);
    }

    error = dvops->dvop_lookup(major);

	return (error);
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
	struct bdevsw *bdevsw = DTOB(args->d_devsw);
	int maj = args->d_major;

	return (0);
}

int
bdevsw_detach(args)
	struct dvop_detach_args *args;
{
	struct bdevsw *bdevsw = DTOB(args->d_devsw);
	int maj = args->d_major;

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
	struct cdevsw *cdevsw = DTOC(args->d_devsw);
	int maj = args->d_major;

	return (0);
}

int
cdevsw_detach(args)
	struct dvop_detach_args *args;
{
	struct cdevsw *cdevsw = DTOC(args->d_devsw);
	int maj = args->d_major;

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
	struct linesw *linesw = DTOL(args->d_devsw);
	int maj = args->d_major;

	return (0);
}

int
linesw_detach(args)
	struct dvop_detach_args *args;
{
	struct linesw *linesw = DTOL(args->d_devsw);
	int maj = args->d_major;

	return (0);
}
