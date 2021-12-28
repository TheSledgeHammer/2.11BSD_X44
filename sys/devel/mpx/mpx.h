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

/*
 * Goal:
 * Similar to the new Pipes in FreeBSD.
 * But using the mpx idea.
 *
 * Difference between original mpx & new mpx:
 * - Uses vnodes over inodes
 * - No tty implementation
 * - No linesw and cdevsw as there is no terminal output or mounting requirements
 * - mpx file creation: can use vnodes directly
 * - above reduces complexity & possibly the need of a syscall
 * - ioctls can be reduced and made compatible with sockets & vnodes
 */

#ifndef SYS_DEVEL_SYS_MPX_H_
#define SYS_DEVEL_SYS_MPX_H_

#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/user.h>

/* belongs in vnode.h */
//struct mpx_group 					*vu_mpxgroup;	/* mpx group */
//#define v_mpxgroup					v_un.vu_mpxgroup

/* belongs in file.h */
/*
struct mpx							*f_Mpx;
#define f_mpx						f_un.f_Mpx
*/

struct mpx_chan {
	char 							mpc_index;
	int 							mpc_flags;
	struct mpx_group				*mpc_group;

	struct	clist					mpc_ctlx;
	union {
		struct	clist				datq;
	} cx;
	union {
		struct	clist				datq;
		struct	mpx_chan			*c_chan;
	} cy;
	struct	clist					mpc_ctly;


	struct	tty						*mpc_ttyp;
	struct	tty						*mpc_ottyp;
	char							mpc_line;
	char							mpc_oline;
};

struct mpx_port {
	char 							mpt_index;
	int 							mpt_flags;
	struct mpx_group				*mpt_group;

	struct	clist					mpt_ctlx;
};

struct mpx {
	struct mpxpair					*mpx_pair;
	struct mpx_chan 				*mpx_chan;

	int 							mpx_state;
	ino_t							mpx_ino;
	struct file						*mpx_pgid;

	//struct prgp						*mpx_pgrp; 	/* proc group */
};

/*
 * Tuneable Multiplexor variables
 */
#define	NINDEX		15 	/* comes from ufs inode */

#define	NGROUPS		10	/* number of mpx files permitted at one time */
#define	NCHANS		20	/* number of channel structures */
#define	NPORTS		30	/* number of channels to i/o ports */
#define	CNTLSIZ		10
#define	NLEVELS		4
#define	NMSIZE		50	/* max size of mxlstn file name */

struct mpx_group {
	struct mpx_group				*mpg_group;
	struct vnode					*mpg_vnode;
	struct file						*mpg_file;
	struct mpx_chan					*mpg_chans[NCHANS];

	int 							mpg_state;
	char 							mpg_index;
	char							mpg_rot;
	short							mpg_rotmask;
	short							mpg_datq;
};

struct mpxpair {
	struct mpx						mpp_rmpx;
	struct mpx						mpp_wmpx;

	struct lock_object				mpp_lock;
};

struct mpx_args {
	char							*m_name;
	int								m_cmd;
	int								m_arg[3];
};

#define FMPX						FREAD | FWRITE
#define FMPY 						FREAD | FWRITE

#define MPX_LOCK(mpx)				simple_lock((mpx)->mpp_lock)
#define MPX_UNLOCK(mpx)				simple_unlock((mpx)->mpp_lock)

struct mpx_object {
	struct pgrp 					*mo_pgrp; 	/* proc group */
	pid_t							mo_pid;		/* proc id */
};
typedef struct mpx_object 			*mpx_object_t;

/*
 * flags
 */
#define	INUSE	01
#define	SIOCTL	02
#define	XGRP	04
#define	YGRP	010
#define	WCLOSE	020
#define	ISGRP	0100
#define	BLOCK	0200
#define	EOTMARK	0400
#define	SIGBLK	01000
#define	BLKMSG	01000
#define	ENAMSG	02000
#define	WFLUSH	04000
#define	NMBUF	010000
#define	PORT	020000
#define	ALT		040000
#define	FBLOCK	0100000

/*
 * mpxchan command codes
 */
#define	MPX		5
#define	MPXN	6
#define	CHAN	1
#define JOIN	2
#define EXTR	3
#define	ATTACH	4
#define	CONNECT	7
#define	DETACH	8
#define	DISCON	9
#define DEBUG	10
#define	NPGRP	11
#define	CSIG	12
#define PACK	13

#define	NDEBUGS	30

/*
 * control channel message codes
 */
#define	M_WATCH 1
#define	M_CLOSE 2
#define	M_EOT	3
#define	M_OPEN	4
#define	M_BLK	5
#define	M_UBLK	6
#define	DO_BLK	7
#define	DO_UBLK	8
#define	M_IOCTL	12
#define	M_IOANS	13
#define	M_SIG	14

/*
 * debug codes other than mpxchan cmds
 */
#define	MCCLOSE 29
#define	MCOPEN	28
#define	ALL		27
#define	SCON	26
#define	MSREAD	25
#define	SDATA	24
#define	MCREAD	23
#define	MCWRITE	22

#endif /* SYS_DEVEL_SYS_MPX_H_ */
