/*-
 * Copyright (c) 1997, 1998, 1999
 *	Nan Yang Computer Services Limited.  All rights reserved.
 *
 *  Parts copyright (c) 1997, 1998 Cybernet Corporation, NetMAX project.
 *
 *  Written by Greg Lehey
 *
 *  This software is distributed under the so-called ``Berkeley
 *  License'':
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Nan Yang Computer
 *	Services Limited.
 * 4. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even if
 * advised of the possibility of such damage.
 *
 * $Id: vinumvar.h,v 1.1.1.1.2.1 2004/09/05 20:36:04 tron Exp $
 * $FreeBSD$
 */


#ifndef _DEV_ADVVM_VAR_H_
#define _DEV_ADVVM_VAR_H_

#include <sys/time.h>
#include <sys/types.h>

/* Directory for device nodes. */
#define ADVVM_DIR   "/dev/advvm"

#define ADVVM_ROOT
enum constants {
	ADVVM_VERSION = 1,
	ADVVM_HEADER = 512,
	ADVVM_MAGIC = 0x11081953,				/* magic number */

	ADVVM_BDEV_MAJOR = 162,					/* major number for block device */
	ADVVM_CDEV_MAJOR = 162,					/* major number for character device */


	ADVVM_VOLUME_TYPE = 0,
	ADVVM_DOMAIN_TYPE = 1,
	ADVVM_FILESET_TYPE = 2,



    /*
     * Define a minor device number.
     * This is not used directly; instead, it's
     * called by the other macros.
     */
#define ADVVM_MINOR(o, t)  (o | (t << ADVVM_TYPE_SHIFT))

	ADVVM_TYPE_SHIFT = 18,
	ADVVM_MAXVOL = 0x3fffd,				    /* highest numbered volume */

	ADVVM_MAXFILESET = 0x3ffff,
	ADVVM_MAXDOMAIN = 0x7ffff,

	MAXDOMAIN = 8,
	MAXFILESET = 256,
	MAXDRIVENAME = 32,		    			/* maximum length of a device name */
	MAXVOLUMENAME = 64,          			/* maximum length of a volume name */
	MAXDOMAINNAME = 64,          			/* maximum length of a domain name */
	MAXFILESETNAME = 64,          			/* maximum length of a fileset name */

    /* Create device minor numbers */
    /* Character device */
#define ADVVM_CDEV(o, t)		makedev (ADVVM_CDEV_MAJOR, ADVVM_MINOR (o, t))

#define ADVVM_CDEV_VOL(v)		makedev (ADVVM_CDEV_MAJOR, ADVVM_MINOR (v, ADVVM_VOLUME_TYPE))

#define ADVVM_CDEV_DOM(d)		makedev (ADVVM_CDEV_MAJOR, ADVVM_MINOR (d, ADVVM_DOMAIN_TYPE))

#define ADVVM_CDEV_FST(f)		makedev (ADVVM_CDEV_MAJOR, ADVVM_MINOR (f, ADVVM_FILESET_TYPE))

    /* Block device */
#define ADVVM_BDEV(o, t)		makedev (ADVVM_BDEV_MAJOR, ADVVM_MINOR (o, t))

#define ADVVM_BDEV_VOL(v)		makedev (ADVVM_BDEV_MAJOR, ADVVM_MINOR (v, ADVVM_VOLUME_TYPE))

#define ADVVM_BDEV_DOM(d)		makedev (ADVVM_BDEV_MAJOR, ADVVM_MINOR (d, ADVVM_DOMAIN_TYPE))

#define ADVVM_BDEV_FST(f)		makedev (ADVVM_BDEV_MAJOR, ADVVM_MINOR (f, ADVVM_FILESET_TYPE))

    /* extract device type */
#define DEVTYPE(x) 				((minor (x) >> ADVVM_TYPE_SHIFT) & 3)


	ADVVMHOSTNAMELEN = 32,		/* host name field in label */
};

struct advvm_label {
    char              		alb_sysname[ADVVMHOSTNAMELEN];
    char              		alb_name[MAXDRIVENAME];
    struct timeval    		alb_creation_date;
    struct timeval    		alb_last_update;
    off_t             		alb_drive_size;
};

struct advvm_block {
    uint64_t            	ablk_start;
    uint64_t            	ablk_end;
    uint32_t            	ablk_size;
    caddr_t					ablk_addr;
    int						ablk_flags;
};

struct advvm_header {
	long					ahd_magic;
	struct advvm_label		ahd_label;
	struct advvm_block		ahd_block;
};

/* see: advvm.c */
extern struct advvm_label 	*advlab;
extern struct advvm_block 	*advblk;

#endif /* _DEV_ADVVM_VAR_H_ */
