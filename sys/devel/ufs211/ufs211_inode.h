/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)inode.h	1.4 (2.11BSD GTE) 1995/12/24
 */

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon1 and icommon2 is read in from permanent inode on volume.
 */

#ifndef _UFS211_INODE_H_
#define	_UFS211_INODE_H_

/*
 * 28 of the di_addr address bytes are used; 7 addresses of 4
 * bytes each: 4 direct (4Kb directly accessible) and 3 indirect.
 */
#define	NDADDR	4			        /* direct addresses in inode */
#define	NIADDR	3			        /* indirect addresses in inode */
#define	NADDR	(NDADDR + NIADDR)	/* total addresses in inode */

struct ufs211_icommon2 {
	time_t	ic_atime;		/* time last accessed */
	time_t	ic_mtime;		/* time last modified */
	time_t	ic_ctime;		/* time created */
};

struct ufs211_inode {
	struct	ufs211_inode 	*i_chain[2];	/* must be first */
	u_short					i_flag;
	u_short					i_count;		/* reference count */
	ufs211_dev_t			i_dev;			/* device where inode resides */
	ufs211_ino_t			i_number;		/* i number, 1-to-1 with device address */
	u_short					i_id;			/* unique identifier */
	struct	fs 				*i_fs;			/* file sys associated with this inode */
	union {
		struct {
			u_char			I_shlockc;		/* count of shared locks */
			u_char			I_exlockc;		/* count of exclusive locks */
		} i_l;
		struct	proc 		*I_rsel;		/* pipe read select */
	} i_un0;
	union {
		struct	proc 		*I_wsel;		/* pipe write select */
	} i_un1;
	union {
		ufs211_daddr_t		I_addr[NADDR];		/* normal file/directory */
		struct {
			ufs211_daddr_t	I_db[NDADDR];	/* normal file/directory */
			ufs211_daddr_t	I_ib[NIADDR];
		} i_f;
		struct {
			/*
			 * the dummy field is here so that the de/compression
			 * part of the iget/iput routines works for special
			 * files.
			 */
			u_short			I_dummy;
			ufs211_dev_t	I_rdev;		/* dev type */
		} i_d;
	} i_un2;
	union {
		ufs211_daddr_t		if_lastr;	/* last read (read-ahead) */
		struct	socket 		*is_socket;
		struct	{
			struct ufs211_inode  *if_freef;	/* free list forward */
			struct ufs211_inode  **if_freeb;	/* free list back */
		} i_fr;
	} i_un3;
	struct ufs211_icommon1 {
		u_short			ic_mode;	/* mode and type of file */
		u_short			ic_nlink;	/* number of links to file */
		uid_t			ic_uid;		/* owner's user id */
		gid_t			ic_gid;		/* owner's group id */
		ufs211_off_t	ic_size;	/* number of bytes in file */
	} i_ic1;
/*
 * Can't afford another 4 bytes and mapping the flags out would be prohibitively
 * expensive.  So, a 'u_short' is used for the flags - see the comments in
 * stat.h for more information.
*/
	u_short			        i_flags;		/* user changeable flags */
	struct ufs211_icommon2  i_ic2;
};

/*
 * Inode structure as it appears on
 * a disk block.
 */
struct ufs211_dinode {
	struct	ufs211_icommon1     di_icom1;
	ufs211_daddr_t		        di_addr[7];			/* 7 block addresses 4 bytes each */
	u_short				        di_reserved[5];		/* pad of 10 to make total size 64 */
	u_short				        di_flags;
	struct	ufs211_icommon2 	di_icom2;
};

#define	i_mode		i_ic1.ic_mode
#define	i_nlink		i_ic1.ic_nlink
#define	i_uid		i_ic1.ic_uid
#define	i_gid		i_ic1.ic_gid
#define	i_size		i_ic1.ic_size
#define	i_shlockc	i_un0.i_l.I_shlockc
#define	i_exlockc	i_un0.i_l.I_exlockc
#define	i_rsel		i_un0.I_rsel
#define	i_text		i_un1.I_text
#define	i_wsel		i_un1.I_wsel
#define	i_db		i_un2.i_f.I_db
#define	i_ib		i_un2.i_f.I_ib
#define	i_atime		i_ic2.ic_atime
#define	i_mtime		i_ic2.ic_mtime
#define	i_ctime		i_ic2.ic_ctime
#define	i_rdev		i_un2.i_d.I_rdev
#define	i_addr		i_un2.I_addr
#define	i_dummy		i_un2.i_d.I_dummy
#define	i_lastr		i_un3.if_lastr
#define	i_socket	i_un3.is_socket
#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]
#define	i_freef		i_un3.i_fr.if_freef
#define	i_freeb		i_un3.i_fr.if_freeb

#define di_ic1		di_icom1
#define di_ic2		di_icom2
#define	di_mode		di_ic1.ic_mode
#define	di_nlink	di_ic1.ic_nlink
#define	di_uid		di_ic1.ic_uid
#define	di_gid		di_ic1.ic_gid
#define	di_size		di_ic1.ic_size
#define	di_atime	di_ic2.ic_atime
#define	di_mtime	di_ic2.ic_mtime
#define	di_ctime	di_ic2.ic_ctime

#endif /* _UFS211_INODE_H_ */
