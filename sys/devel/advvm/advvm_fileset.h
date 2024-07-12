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
 *	@(#)advvm_fileset.h 1.0 	7/01/21
 */

/* filesets is a pool of advvm_domains. */

#ifndef _DEV_ADVVM_FILESET_H_
#define _DEV_ADVVM_FILESET_H_

#include <sys/queue.h>
#include <sys/types.h>

#include <advvm_volume.h>

struct advvm_fileset {
    TAILQ_ENTRY(advvm_fileset)      fst_entries;                    /* list of fileset entries per domain */
    char                            fst_name[ADVVM_NAME_LEN];       /* fileset name */
    uint32_t                        fst_id[ADVVM_UUID_LEN];         /* fileset uid */
    uint32_t						fst_hash;						/* tag/file directory hash */

    /* fileset mcell information */
    advvm_cell_t 					*fst_mcell;						/* mcell */
    /* fileset tag information */
    advvm_tag_dir_t                 fst_tag;						/* tag directory */
    /* fileset directory information */
    advvm_file_dir_t		   		fst_fdir;						/* file directory */
    /* fileset allocation */
    advvm_storage_t 		    	*fst_storage;					/* deprecation pending!! */

    u_long							fst_start;						/* region substart within domain */
    u_long							fst_end;						/* region subend within domain */
    u_long							fst_size;						/* region size within domain */
    u_long 							fst_boundary;					/* region boundary within domain */
    u_long							fst_alignment;					/* region alignment within domain */

    /* domain-related fields */
    advvm_domain_t             	    *fst_domain;                    /* domain this fileset belongs too */
#define fst_domain_name             fst_domain->dom_name            /* domain name */
#define fst_domain_id               fst_domain->dom_id              /* domain id */
};
typedef struct advvm_fileset        advvm_fileset_t;

void				advvm_fileset_init(advvm_fileset_t *, char *, uint32_t);
void				advvm_fileset_set_domain(advvm_domain_t *, advvm_fileset_t *);
void				advvm_fileset_create(advvm_domain_t *, advvm_fileset_t *);
advvm_volume_t 		*advvm_filset_find(advvm_domain_t *, char *, uint32_t);
void				advvm_filset_insert(advvm_domain_t *, advvm_fileset_t *, char *, uint32_t);
void				advvm_filset_remove(advvm_domain_t *, char *, uint32_t);
void				advvm_fileset_destroy(advvm_fileset_t *);
#endif /* _DEV_ADVVM_FILESET_H_ */
