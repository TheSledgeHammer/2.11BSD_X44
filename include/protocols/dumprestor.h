/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dumprestore.h	8.2 (Berkeley) 1/21/94
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dumprestor.h	5.1 (Berkeley) 12/13/86
 */

#ifndef _DUMPRESTORE_H_
#define _DUMPRESTORE_H_

#include <ufs/ufs/dinode.h>

/*
 * TP_BSIZE is the size of file blocks on the dump tapes.
 * Note that TP_BSIZE must be a multiple of DEV_BSIZE.
 *
 * NTREC is the number of TP_BSIZE blocks that are written
 * in each tape record. HIGHDENSITYTREC is the number of
 * TP_BSIZE blocks that are written in each tape record on
 * 6250 BPI or higher density tapes.
 *
 * TP_NINDIR is the number of indirect pointers in a TS_INODE
 * or TS_ADDR record. Note that it must be a power of two.
 */
#define TP_BSIZE		1024
#define NTREC			10
#define HIGHDENSITYTREC	32
#define TP_NINDIR		(TP_BSIZE/2)
#define LBLSIZE			16
#define NAMELEN			64
#define MLEN			16
#define MSIZ			4096

#define TS_TAPE			1
#define TS_INODE		2
#define TS_BITS			3
#define TS_ADDR			4
#define TS_END			5
#define TS_CLRI			6

#define OFS_MAGIC		(int)60011
#define NFS_MAGIC		(int)60012
#ifndef FS_UFS2_MAGIC
#define FS_UFS2_MAGIC   (int)0x19540119
#endif
#define MAGIC			OFS_MAGIC
#define CHECKSUM		(int)84446

#ifdef notyet
struct	spcl {
	int					c_type;
	time_t				c_date;
	time_t				c_ddate;
	int					c_volume;
	daddr_t				c_tapea;
	ino_t				c_inumber;
	int					c_magic;
	int					c_checksum;
	struct ufs1_dinode	c_dinode;
	int					c_count;
	char				c_addr[DEV_BSIZE];
} spcl;

struct	idates {
	char	id_name[16];
	char	id_incno;
	time_t	id_ddate;
};
#endif

union u_spcl {
	char dummy[TP_BSIZE];
	struct	s_spcl {
		long	c_type;		    		/* record type (see below) */
		time_t	c_date;		    		/* date of this dump */
		time_t	c_ddate;	    		/* date of previous dump */
		long	c_volume;	    		/* dump volume number */
		daddr_t	c_tapea;	    		/* logical block of this record */
		ino_t	c_inumber;	    		/* number of inode */
		long	c_magic;	    		/* magic number (see above) */
		long	c_checksum;	    		/* record checksum */
		struct ufs1_dinode	c_dinode;   /* ownership and mode of inode */
		long	c_count;	    		/* number of valid c_addr entries */
		char	c_addr[TP_NINDIR];  	/* 1 => data; 0 => hole in inode */
		char	c_label[LBLSIZE];   	/* dump label */
		long	c_level;	    		/* level of this dump */
		char	c_filesys[NAMELEN]; 	/* name of dumpped file system */
		char	c_dev[NAMELEN];	    	/* name of dumpped device */
		char	c_host[NAMELEN];    	/* name of dumpped host */
		long	c_flags;	    		/* additional information */
		long	c_firstrec;	    		/* first record on volume */
		long	c_spare[32];	    	/* reserved for future uses */
	} s_spcl;
} u_spcl;
#define spcl u_spcl.s_spcl

/*
 * flag values
 */
#define DR_NEWHEADER	0x0001			/* new format tape header */
#define DR_NEWINODEFMT	0x0002			/* new format inodes on tape */

#define	DUMPOUTFMT	"%-16s %c %s"		/* for printf */
										/* name, incno, ctime(date) */
#define	DUMPINFMT	"%16s %c %[^\n]\n"	/* inverse for scanf */

#endif /* !_DUMPRESTORE_H_ */
