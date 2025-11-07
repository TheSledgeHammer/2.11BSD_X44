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

#ifndef _PROTOCOLS_DUMPRESTORE_H_
#define _PROTOCOLS_DUMPRESTORE_H_

//#include <stdint.h>
//#include <ufs/ufs/dinode.h>

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

/*
 * special record types
 */
#define TS_TAPE			1   /* dump tape header */
#define TS_INODE		2   /* beginning of file record */
#define TS_BITS			3   /* map of inodes on tape */
#define TS_ADDR			4   /* continuation of file record */
#define TS_END			5   /* end of volume marker */
#define TS_CLRI			6   /* map of inodes deleted since last dump */

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

extern union u_spcl {
	char dummy[TP_BSIZE];
	struct	s_spcl {
		int32_t	c_type;		    	/* record type (see below) */
		int32_t	c_old_date;	    	/* date of this dump */
		int32_t	c_old_ddate;	    /* date of previous dump */
		int32_t	c_volume;	    	/* dump volume number */
		int32_t	c_old_tapea;	    /* logical block of this record */
		uint32_t c_inumber;	    	/* number of inode */
		int32_t	c_magic;	    	/* magic number (see above) */
		int32_t	c_checksum;	    	/* record checksum */

		union {
			struct ufs1_dinode uc_dinode;
			struct {
				uint16_t uc_mode;
				int16_t uc_spare1[3];
				uint64_t uc_size;
				int32_t uc_old_atime;
				int32_t uc_atimensec;
				int32_t uc_old_mtime;
				int32_t uc_mtimensec;
				int32_t uc_spare2[2];
				int32_t uc_rdev;
				int32_t uc_birthtimensec;
				int64_t uc_birthtime;
				int64_t uc_atime;
				int64_t uc_mtime;
				int32_t uc_extsize;
				int32_t uc_spare4[6];
				uint32_t uc_file_flags;
				int32_t uc_spare5[2];
				uint32_t uc_uid;
				uint32_t uc_gid;
				int32_t uc_spare6[2];
			} s_ino;
		} c_ino;

#define c_dinode		c_ino.uc_dinode
#define c_mode			c_ino.s_ino.uc_mode
#define c_spare1		c_ino.s_ino.uc_spare1
#define c_size			c_ino.s_ino.uc_size
#define c_extsize		c_ino.s_ino.uc_extsize
#define c_old_atime		c_ino.s_ino.uc_old_atime
#define c_atime			c_ino.s_ino.uc_atime
#define c_atimensec		c_ino.s_ino.uc_atimensec
#define c_mtime			c_ino.s_ino.uc_mtime
#define c_mtimensec		c_ino.s_ino.uc_mtimensec
#define c_birthtime		c_ino.s_ino.uc_birthtime
#define c_birthtimensec	c_ino.s_ino.uc_birthtimensec
#define c_old_mtime		c_ino.s_ino.uc_old_mtime
#define c_rdev			c_ino.s_ino.uc_rdev
#define c_file_flags	c_ino.s_ino.uc_file_flags
#define c_uid			c_ino.s_ino.uc_uid
#define c_gid			c_ino.s_ino.uc_gid

		int32_t	c_count;	    	/* number of valid c_addr entries */
		char	c_addr[TP_NINDIR];  /* 1 => data; 0 => hole in inode */
		char	c_label[LBLSIZE];   /* dump label */
		int32_t	c_level;	    	/* level of this dump */
		char	c_filesys[NAMELEN]; /* name of dumpped file system */
		char	c_dev[NAMELEN];	    /* name of dumpped device */
		char	c_host[NAMELEN];    /* name of dumpped host */
		int32_t	c_flags;	    	/* additional information */
		int32_t	c_old_firstrec;	    /* first record on volume */
		int64_t c_date;		    	/* date of this dump */
		int64_t c_ddate;	    	/* date of previous dump */
		int64_t c_tapea;	    	/* logical block of this record */
		int64_t c_firstrec;	    	/* first record on volume */
		int32_t	c_spare[24];	    /* reserved for future uses */
	} s_spcl;
} u_spcl;
#define spcl 	u_spcl.s_spcl

/*
 * flag values
 */
#define DR_NEWHEADER	0x0001			/* new format tape header */
#define DR_NEWINODEFMT	0x0002			/* new format inodes on tape */

#define	DUMPOUTFMT	"%-18s %c %s"		/* for printf */
										/* name, incno, ctime(date) */
#define	DUMPINFMT	"%18s %c %[^\n]\n"	/* inverse for scanf */

/* 2.11BSD: DUMPOUTFMT */
#define	OLD_DUMPOUTFMT	"%-16s %c %s"		/* for printf */
						                    /* name, incno, ctime(date) */
#define	OLD_DUMPINFMT	"%16s %c %[^\n]\n"	/* inverse for scanf */

#endif /* !_PROTOCOLS_DUMPRESTORE_H_ */
