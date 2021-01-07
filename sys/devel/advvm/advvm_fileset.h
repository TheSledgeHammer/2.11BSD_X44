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

/* filesets is a pool of advvm_domains */

#ifndef _DEV_ADVVM_FILESET_H_
#define _DEV_ADVVM_FILESET_H_

#include <sys/queue.h>
#include <sys/types.h>

#include <advvm_volume.h>

struct tag_directory {
    const char                      *tag_name;
    uint32_t                        tag_id;
};
typedef struct tag_directory        *tag_dir_t;

/* should be filedirectory structure within bsd "system" */
struct file_directory {
    tag_dir_t                       fdr_tag;
    const char                      *fdr_name;
    struct device                   *fdr_dev;
};
typedef struct file_directory       *file_dir_t;

struct fileset {
    TAILQ_ENTRY(volume)             fst_entries;                    /* list of fileset entries per domain */
    char                            fst_name[MAXFILESETNAME];       /* fileset name */
    uint32_t                        fst_id;                         /* fileset id */


    /* fileset tag information */
    tag_dir_t                       fst_tags;

    /* domain-related fields */
    struct domain                   *fst_domain;                    /* domain this fileset belongs too */
#define fst_domain_name             fst_domain->dom_name            /* domain name */
#define fst_domain_id               fst_domain->dom_id              /* domain id */
};
typedef struct fileset              *fileset_t;

#endif /* _DEV_ADVVM_FILESET_H_ */
