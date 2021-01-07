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

#define HOSTNAMELEN         256
#define MAXDRIVENAME        32		    /* maximum length of a device name */
#define MAXVOLUMENAME       64          /* maximum length of a volume name */
#define MAXDOMAINNAME       64          /* maximum length of a domain name */
#define MAXFILESETNAME      64          /* maximum length of a fileset name */

struct label {
    char                            lb_sysname[HOSTNAMELEN];
    char                            lb_name[MAXVOLUMENAME];
    uint32_t                        lb_id;
    struct timeval                  lb_creation_date;
    struct timeval                  lb_last_update;
    uint32_t                        lb_drive_size;
};

struct volume {
    TAILQ_ENTRY(volume)             vol_entries;                /* list of volume entries per domain */

    /* device or drive fields */
    char                            vol_device[MAXDRIVENAME];   /* drive/device volume is on */
    struct label                    *vol_label;                 /* label information */
    struct volume_block             *vol_block;                 /* block information per volume */
    int                             vol_flags;                  /* flags */

    /* domain-related fields */
    struct domain                   *vol_domain;                /* domain this volume belongs too */
#define vol_domain_name             vol_domain->dom_name        /* domain name */
#define vol_domain_id               vol_domain->dom_id          /* domain id */
    int                             vol_domain_allocated;       /* number of entries in a domain */
    int                             vol_domain_used;            /* and the number used */
};
typedef struct volume               *volume_t;

enum filesystem_format {
    UFS1,       /* UFS version 1 */
    UFS2,       /* UFS version 2 */
    MFS,        /* BSD's MFS */
    LFS,        /* BSD's LFS */
    MSDOSFS     /* FAT */
};

/* volume physical block information */
struct volume_block {
    enum filesystem_format          vblk_filesystem;             /* supported filesystem formats */
    int                             vblk_fsblocksize;            /* size of filesystem blocks */
    uint32_t                        vblk_size;
    uint64_t                        vblk_start;
    uint64_t                        vblk_end;
    uint32_t                        vblk_sectors;                /* number of sectors still available */
    uint64_t                        vblk_secperblock;
};

#endif /* _DEV_ADVVM_VOLUME_H_ */
