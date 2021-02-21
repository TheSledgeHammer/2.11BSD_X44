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
 *
 *	@(#)advvm_volume.h 1.0 	7/01/21
 */

/* volumes are a pool of disk partitions or disk slices. */

#ifndef _DEV_ADVVM_VOLUME_H_
#define _DEV_ADVVM_VOLUME_H_

#include <sys/queue.h>
#include <sys/types.h>

#include <advvm_var.h>

struct advvm_volume {
    TAILQ_ENTRY(advvm_volume)       vol_entries;                /* list of volume entries per domain */

    char							vol_name[MAXVOLUMENAME];	/* volume name */
    uint32_t 						vol_id;						/* volume id */
    int                             vol_flags;                  /* volume flags */


    advvm_storage_t					*vol_storage;				/* volume allocation */

    /* device or drive fields */
    struct advvm_label              *vol_label;                 /* label information */
    struct advvm_block             	*vol_block;                 /* block information */

    /* domain-related fields */
    advvm_domain_t             		*vol_domain;                /* domain this volume belongs too */
#define vol_domain_name             vol_domain->dom_name        /* domain name */
#define vol_domain_id               vol_domain->dom_id          /* domain id */
    int                             vol_domain_allocated;       /* number of entries in a domain */
    int                             vol_domain_used;            /* the number used */
};
typedef struct advvm_volume         advvm_volume_t;

#endif /* _DEV_ADVVM_VOLUME_H_ */
