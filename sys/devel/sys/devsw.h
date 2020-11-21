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

#ifndef _SYS_DEVSW_H_
#define _SYS_DEVSW_H_

#include <sys/conf.h>
#include <sys/queue.h>

/*
 * device switch table
 */
struct devsw_head;
TAILQ_HEAD(devsw_head, devsw);
struct devsw {
    TAILQ_ENTRY(devsw)  dv_entries;
    char	            *dv_name;
    void                *dv_data;
    struct devswops    	*dv_ops;
};

struct devswops {
    int     (*dvop_attach)(const char *, struct device *, int);
    int     (*dvop_detach)(const char *, struct device *, int);
    int		(*dvop_lookup)();
};

struct dvop_attach_args {
    struct devsw    d_devsw;
    const char      *d_name;
    struct device   *d_device;
    int				d_major;
};

struct dvop_detach_args {
    struct devsw    d_devsw;
    const char      *d_name;
    struct device   *d_device;
    int				d_major;
};

struct dvop_lookup_args {
	struct devsw    d_devsw;

	int				d_major;
};

struct bdevsw;
struct cdevsw;
struct linesw;

/* conversion macros */
#define DTOB(dv)  ((struct bdevsw *)(dv)->dv_data)
#define DTOC(dv)  ((struct cdevsw *)(dv)->dv_data)
#define DTOL(dv)  ((struct linesw *)(dv)->dv_data)

struct devsw_head 		devsw_list;		/* list of devices in the device switch table (includes bdevsw, cdevsw & linesw) */
extern struct devswops 	dvops;			/* device switch table operations */

#endif /* _SYS_DEVSW_H_ */
