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
 *	@(#)advvm_domain.h 1.0 	7/01/21
 */

/* a domain is a pool of advvm_volumes */

#ifndef _DEV_ADVVM_DOMAIN_H_
#define _DEV_ADVVM_DOMAIN_H_

#include <sys/queue.h>
#include <sys/types.h>

#include <advvm_volume.h>

TAILQ_HEAD(advdomain_list, advvm_domain);
struct advvm_domain {
    TAILQ_ENTRY(advvm_domain)           		dom_entries;                /* list of domains */
    char                            			dom_name[MAXDOMAINNAME];    /* domain name */
    uint32_t                        			dom_id;                     /* domain id */
    int											dom_flags;
    int											dom_refcnt;

    TAILQ_HEAD(advvolume_list, advvm_volume) 	dom_volumes;                /* head list of volumes */
    TAILQ_HEAD(advfileset_list, advvm_fileset)  dom_filesets;               /* head list of filesets */

    /* domain allocation */
    advvm_storage_t								*dom_storage;
    u_long										dom_start;
    u_long										dom_end;
};
typedef struct advvm_domain               		advvm_domain_t;

extern struct advdomain_list              		domain_list;
uint32_t        advvm_hash(advvm_domain_t *);
void			advvm_domain_create(advvm_domain_t, char *, uint32_t, u_long, u_long, int);
advvm_domain_t 	*advvm_domain_find(char *, uint32_t);
void			advvm_domain_insert(advvm_domain_t);
void			advvm_domain_remove(char *, uint32_t);
void			advvm_domain_destroy(advvm_domain_t);
#endif /* _DEV_ADVVM_DOMAIN_H_ */
