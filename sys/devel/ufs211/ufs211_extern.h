/*
 * ufs211_extern.h
 *
 *  Created on: 18 Apr 2020
 *      Author: marti
 */

typedef u_short	 ufs211_ino_t;
typedef long	 ufs211_daddr_t;
typedef long	 ufs211_off_t;
typedef short	 ufs211_dev_t;
typedef u_short	 ufs211_mode_t;
typedef	u_int	 ufs211_size_t;

int				updlock;		/* lock for sync */
ufs211_daddr_t	rablock;		/* block to be read ahead */

struct ufs211_args {
	struct mount 	    *ufs211_vfs;
	struct vnode 	    *ufs211_rootvp;	    /* block device mounted vnode */
};

#define	VFSTOUFS211(mp)	((struct ufs211_mount *)((mp)->mnt_data))

#define	DEV_BSIZE		1024
#define	DEV_BSHIFT		10			/* log2(DEV_BSIZE) */
#define	DEV_BMASK		0x3ffL		/* DEV_BSIZE - 1 */
#define	MAXBSIZE		1024

/* i_mode */
#define	UFS211_FMT		0170000		/* type of file */
#define	UFS211_FCHR		0020000		/* character special */
#define	UFS211_FDIR		0040000		/* directory */
#define	UFS211_FBLK		0060000		/* block special */
#define	UFS211_FREG		0100000		/* regular */
#define	UFS211_FLNK		0120000		/* symbolic link */
#define	UFS211_FSOCK	0140000		/* socket */
#define	UFS211_SUID		04000		/* set user id on execution */
#define	UFS211_SGID		02000		/* set group id on execution */
#define	UFS211_SVTX		01000		/* save swapped text even after use */
#define	UFS211_READ		0400		/* read, write, execute permissions */
#define	UFS211_WRITE	0200
#define	UFS211_EXEC		0100

__BEGIN_DECLS
int ufs211_lookup(void *);
int ufs211_create(void *);
int ufs211_open(void *);
int ufs211_close(void *);
int ufs211_access(void *);
int ufs211_getattr(void *);
int ufs211_setattr(void *);
int ufs211_read(void *);
int ufs211_write(void *);
int ufs211_fsync(void *);
int ufs211_remove(void *);
int ufs211_rename(void *);
int ufs211_readdir(void *);
int ufs211_inactive(void *);
int ufs211_reclaim(void *);
int ufs211_bmap(void *);
int ufs211_strategy(void *);
int ufs211_print(void *);
int ufs211_advlock(void *);
int ufs211_pathconf(void *);

int ufs211_link(void *);
int ufs211_symlink(void *);
int ufs211_readlink(void *);

int ufs211_mkdir(void *);
int ufs211_rmdir(void *);

int ufs211_mknod(void *);

#ifdef FIFO
int		ufs211fifo_read (struct vop_read_args *);
int		ufs211fifo_write (struct vop_write_args *);
int		ufs211fifo_close (struct vop_close_args *);
#endif
__END_DECLS

