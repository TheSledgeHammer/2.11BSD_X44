/*
 * mpx_vnops1.c
 *
 *  Created on: 16 May 2020
 *      Author: marti
 */


int
mpx_lookup(ap)
	struct vop_lookup_args *ap;
{

}

int
mpx_create(ap)
	struct vop_create_args *ap;
{

}

int
mpx_mknod(ap)
	struct vop_mknod_args *ap;
{

}

int
mpx_open(ap)
	struct vop_open_args *ap;
{

}

int
mpx_close(ap)
	struct vop_close_args *ap;
{

}

int
mpx_access(ap)
	struct vop_access_args *ap;
{

}

int
mpx_getattr(ap)
	struct vop_getattr_args *ap;
{

}

int
mpx_setattr(ap)
	struct vop_setattr_args *ap;
{

}

int
mpx_select(ap)
	struct vop_select_args *ap;
{

}

int
mpx_read(ap)
	struct vop_read_args *ap;
{

}

int
mpx_write(ap)
	struct vop_write_args *ap;
{

}

struct vnodeops mpx_vnodeops[] = {
		.vop_lookup = mpx_lookup,		/* lookup */
		.vop_create = mpx_create,		/* create */
		.vop_mknod = mpx_mknod,			/* mknod */
		.vop_open = mpx_open,			/* open */
		.vop_close = mpx_close,			/* close */
		.vop_access = mpx_access,		/* access */
		.vop_getattr = mpx_getattr,		/* getattr */
		.vop_setattr = mpx_setattr,		/* setattr */
		.vop_read = mpx_read,			/* read */
		.vop_write = mpx_write,			/* write */
		.vop_lease = mpx_lease_check,	/* lease */
		.vop_ioctl = mpx_ioctl,			/* ioctl */
		.vop_select = mpx_select,		/* select */
		.vop_revoke = mpx_revoke,		/* revoke */
		.vop_mmap = mpx_mmap,			/* mmap */
		.vop_fsync = mpx_fsync,			/* fsync */
		.vop_seek = mpx_seek,			/* seek */
		.vop_remove = mpx_remove,		/* remove */
		.vop_link = mpx_link,			/* link */
		.vop_rename = mpx_rename,		/* rename */
		.vop_mkdir = mpx_mkdir,			/* mkdir */
		.vop_rmdir = mpx_rmdir,			/* rmdir */
		.vop_symlink = mpx_symlink,		/* symlink */
		.vop_readdir = mpx_readdir,		/* readdir */
		.vop_readlink = mpx_readlink,	/* readlink */
		.vop_abortop = mpx_abortop,		/* abortop */
		.vop_inactive = mpx_inactive,	/* inactive */
		.vop_reclaim = mpx_reclaim,		/* reclaim */
		.vop_lock = mpx_lock,			/* lock */
		.vop_unlock = mpx_unlock,		/* unlock */
		.vop_bmap = mpx_bmap,			/* bmap */
		.vop_strategy = mpx_strategy,	/* strategy */
		.vop_print = mpx_print,			/* print */
		.vop_islocked = mpx_islocked,	/* islocked */
		.vop_pathconf = mpx_pathconf,	/* pathconf */
		.vop_advlock = mpx_advlock,		/* advlock */
		.vop_blkatoff = mpx_blkatoff,	/* blkatoff */
		.vop_valloc = mpx_valloc,		/* valloc */
		.vop_vfree = mpx_vfree,			/* vfree */
		.vop_truncate = mpx_truncate,	/* truncate */
		.vop_update = mpx_update,		/* update */
		.vop_bwrite = mpx_bwrite,		/* bwrite */
		(struct vnodeops *)NULL = (int(*)())NULL
};
