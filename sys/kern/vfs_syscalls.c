/*
 * Copyright (c) 1989, 1993
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
 *	@(#)vfs_syscalls.c	8.41 (Berkeley) 6/15/95
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/sysdecl.h>
#include <sys/sysctl.h>
#include <sys/unistd.h>

#include <vm/include/vm.h>

static int change_chroot(struct vnode *, struct proc *);
static int change_dir(struct nameidata *ndp, struct proc *p);
static void checkdirs(struct vnode *olddp);
int getvnode(struct filedesc *, int, struct file **);

/*
 * Virtual File System System Calls
 */

/*
 * Mount a file system.
 */
/* ARGSUSED */
int
mount()
{
    register struct mount_args {
        syscallarg(char *) type;
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(caddr_t) data;
    } *uap = (struct mount_args *)u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct vnode *vp;
	struct mount *mp;
	struct vfsconf *vfsp;
	int error, flag;
	struct vattr va;
	u_long fstypenum;
	struct nameidata nd;
	char fstypename[MFSNAMELEN];

	p = u.u_procp;
	/*
	 * Get vnode to be covered
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if (SCARG(uap, flags) & MNT_UPDATE) {
		if ((vp->v_flag & VROOT) == 0) {
			vput(vp);
			return (EINVAL);
		}
		mp = vp->v_mount;
		flag = mp->mnt_flag;
		/*
		 * We only allow the filesystem to be reloaded if it
		 * is currently mounted read-only.
		 */
		if ((SCARG(uap, flags) & MNT_RELOAD) &&
		    ((mp->mnt_flag & MNT_RDONLY) == 0)) {
			vput(vp);
			return (EOPNOTSUPP);	/* Needs translation */
		}
		mp->mnt_flag |=
		    SCARG(uap, flags) & (MNT_RELOAD | MNT_FORCE | MNT_UPDATE);
		/*
		 * Only root, or the user that did the original mount is
		 * permitted to update it.
		 */
		if (mp->mnt_stat.f_owner != p->p_ucred->cr_uid &&
		    (error = suser1(p->p_ucred, &p->p_acflag))) {
			vput(vp);
			return (error);
		}
		/*
		 * Do not allow NFS export by non-root users. Silently
		 * enforce MNT_NOSUID and MNT_NODEV for non-root users.
		 */
		if (p->p_ucred->cr_uid != 0) {
			if (SCARG(uap, flags) & MNT_EXPORTED) {
				vput(vp);
				return (EPERM);
			}
			SCARG(uap, flags) |= MNT_NOSUID | MNT_NODEV;
		}
		if (vfs_busy(mp, LK_NOWAIT, 0, p)) {
			vput(vp);
			return (EBUSY);
		}
		VOP_UNLOCK(vp, 0, p);
		goto update;
	}
	/*
	 * If the user is not root, ensure that they own the directory
	 * onto which we are attempting to mount.
	 */
	if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)) ||
	    (va.va_uid != p->p_ucred->cr_uid &&
	     (error = suser1(p->p_ucred, &p->p_acflag)))) {
		vput(vp);
		return (error);
	}
	/*
	 * Do not allow NFS export by non-root users. Silently
	 * enforce MNT_NOSUID and MNT_NODEV for non-root users.
	 */
	if (p->p_ucred->cr_uid != 0) {
		if (SCARG(uap, flags) & MNT_EXPORTED) {
			vput(vp);
			return (EPERM);
		}
		SCARG(uap, flags) |= MNT_NOSUID | MNT_NODEV;
	}
	if ((error = vinvalbuf(vp, V_SAVE, p->p_ucred, p, 0, 0)))
		return (error);
	if (vp->v_type != VDIR) {
		vput(vp);
		return (ENOTDIR);
	}
#ifdef COMPAT_43
	/*
	 * Historically filesystem types were identified by number. If we
	 * get an integer for the filesystem type instead of a string, we
	 * check to see if it matches one of the historic filesystem types.
	 */
	fstypenum = (u_long)SCARG(uap, type);
	if (fstypenum < maxvfsconf) {
		LIST_FOREACH(vfsp, &vfsconflist, vfc_next) {
			if (vfsp->vfc_typenum == fstypenum) {
				break;
			}
		}
		if (vfsp == NULL) {
			vput(vp);
			return (ENODEV);
		}
		strncpy(fstypename, vfsp->vfc_name, MFSNAMELEN);
	} else
#endif /* COMPAT_43 */
	if ((error = copyinstr(SCARG(uap, type), fstypename, MFSNAMELEN, NULL))) {
		vput(vp);
		return (error);
	}
	LIST_FOREACH(vfsp, &vfsconflist, vfc_next) {
		if (!strcmp(vfsp->vfc_name, fstypename)) {
			break;
		}
	}
	if (vfsp == NULL) {
		vput(vp);
		return (ENODEV);
	}
	if (vp->v_mountedhere != NULL) {
		vput(vp);
		return (EBUSY);
	}

	/*
	 * Allocate and initialize the filesystem.
	 */
	mp = (struct mount *)malloc((u_long)sizeof(struct mount), M_MOUNT, M_WAITOK);
	bzero((char *)mp, (u_long)sizeof(struct mount));
	lockinit(&mp->mnt_lock, PVFS, "vfslock", 0, 0);
	(void)vfs_busy(mp, LK_NOWAIT, 0, p);
	mp->mnt_op = vfsp->vfc_vfsops;
	mp->mnt_vfc = vfsp;
	vfsp->vfc_refcount++;
	mp->mnt_stat.f_type = vfsp->vfc_typenum;
	mp->mnt_flag |= vfsp->vfc_flags & MNT_VISFLAGMASK;
	strncpy(mp->mnt_stat.f_fstypename, vfsp->vfc_name, MFSNAMELEN);
	vp->v_mountedhere = mp;
	mp->mnt_vnodecovered = vp;
	mp->mnt_stat.f_owner = p->p_ucred->cr_uid;
update:
	/*
	 * Set the mount level flags.
	 */
	if (SCARG(uap, flags) & MNT_RDONLY)
		mp->mnt_flag |= MNT_RDONLY;
	else if (mp->mnt_flag & MNT_RDONLY)
		mp->mnt_flag |= MNT_WANTRDWR;
	mp->mnt_flag &=~ (MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
	    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC);
	mp->mnt_flag |= SCARG(uap, flags) & (MNT_NOSUID | MNT_NOEXEC |
	    MNT_NODEV | MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC);
	/*
	 * Mount the filesystem.
	 */
	error = VFS_MOUNT(mp, SCARG(uap, path), SCARG(uap, data), &nd, p);
	if (mp->mnt_flag & MNT_UPDATE) {
		vrele(vp);
		if (mp->mnt_flag & MNT_WANTRDWR)
			mp->mnt_flag &= ~MNT_RDONLY;
		mp->mnt_flag &=~
		    (MNT_UPDATE | MNT_RELOAD | MNT_FORCE | MNT_WANTRDWR);
		if (error)
			mp->mnt_flag = flag;
		vfs_unbusy(mp, p);
		return (error);
	}
	/*
	 * Put the new filesystem on the mount list after root.
	 */
	cache_purge(vp);
	if (!error) {
		simple_lock(&mountlist_slock);
		CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
		simple_unlock(&mountlist_slock);
		checkdirs(vp);
		VOP_UNLOCK(vp, 0, p);
		vfs_unbusy(mp, p);
		if ((error = VFS_START(mp, 0, p)))
			vrele(vp);
	} else {
		mp->mnt_vnodecovered->v_mountedhere = (struct mount *)0;
		mp->mnt_vfc->vfc_refcount--;
		vfs_unbusy(mp, p);
		free((caddr_t)mp, M_MOUNT);
		vput(vp);
	}
	return (error);
}

/*
 * Scan all active processes to see if any of them have a current
 * or root directory onto which the new filesystem has just been
 * mounted. If so, replace them with the new mount point.
 */
static void
checkdirs(olddp)
	struct vnode *olddp;
{
	struct filedesc *fdp;
	struct vnode *newdp;
	struct proc *p;

	if (olddp->v_usecount == 1)
		return;
	if (VFS_ROOT(olddp->v_mountedhere, &newdp))
		panic("mount: lost mount");
	for(p = LIST_FIRST(&allproc); p != 0; p = LIST_NEXT(p, p_list)) {
		fdp = p->p_fd;
		if (fdp->fd_cdir == olddp) {
			vrele(fdp->fd_cdir);
			VREF(newdp);
			fdp->fd_cdir = newdp;
		}
		if (fdp->fd_rdir == olddp) {
			vrele(fdp->fd_rdir);
			VREF(newdp);
			fdp->fd_rdir = newdp;
		}
	}
	if (rootvnode == olddp) {
		vrele(rootvnode);
		VREF(newdp);
		rootvnode = newdp;
	}
	vput(newdp);
}

/*
 * Unmount a file system.
 *
 * Note: unmount takes a path to the vnode mounted on as argument,
 * not special file (as before).
 */
/* ARGSUSED */
int
unmount()
{
    register struct unmount_args {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} *uap = (struct unmount_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct mount *mp;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	mp = vp->v_mount;

	/*
	 * Only root, or the user that did the original mount is
	 * permitted to unmount this filesystem.
	 */
	if ((mp->mnt_stat.f_owner != p->p_ucred->cr_uid) &&
	    (error = suser1(p->p_ucred, &p->p_acflag))) {
		vput(vp);
		return (error);
	}

	/*
	 * Don't allow unmounting the root file system.
	 */
	if (mp->mnt_flag & MNT_ROOTFS) {
		vput(vp);
		return (EINVAL);
	}

	/*
	 * Must be the root of the filesystem
	 */
	if ((vp->v_flag & VROOT) == 0) {
		vput(vp);
		return (EINVAL);
	}
	vput(vp);
	return (dounmount(mp, SCARG(uap, flags), p));
}

/*
 * Do the actual file system unmount.
 */
int
dounmount(mp, flags, p)
	register struct mount *mp;
	int flags;
	struct proc *p;
{
	struct vnode *coveredvp;
	int error;

	simple_lock(&mountlist_slock);
	mp->mnt_flag |= MNT_UNMOUNT;
	lockmgr(&mp->mnt_lock, LK_DRAIN | LK_INTERLOCK, &mountlist_slock, p->p_pid);
	mp->mnt_flag &=~ MNT_ASYNC;
	vnode_pager_umount(mp);	/* release cached vnodes */
	cache_purgevfs(mp);	/* remove cache entries for this file sys */
	if (((mp->mnt_flag & MNT_RDONLY) ||
	     (error = VFS_SYNC(mp, MNT_WAIT, p->p_ucred, p)) == 0) ||
	    (flags & MNT_FORCE))
		error = VFS_UNMOUNT(mp, flags, p);
	simple_lock(&mountlist_slock);
	if (error) {
		mp->mnt_flag &= ~MNT_UNMOUNT;
		lockmgr(&mp->mnt_lock, LK_RELEASE | LK_INTERLOCK | LK_REENABLE,
		    &mountlist_slock, p->p_pid);
		return (error);
	}
	CIRCLEQ_REMOVE(&mountlist, mp, mnt_list);
	if ((coveredvp = mp->mnt_vnodecovered) != NULLVP) {
		coveredvp->v_mountedhere = (struct mount *)0;
		vrele(coveredvp);
	}
	mp->mnt_vfc->vfc_refcount--;
	if (LIST_FIRST(&mp->mnt_vnodelist) != NULL)
		panic("unmount: dangling vnode");
	lockmgr(&mp->mnt_lock, LK_RELEASE | LK_INTERLOCK, &mountlist_slock, p->p_pid);
	if (mp->mnt_flag & MNT_MWAIT)
		wakeup((caddr_t)mp);
	free((caddr_t)mp, M_MOUNT);
	return (0);
}

/*
 * Sync each mounted filesystem.
 */
#ifdef DEBUG
int syncprt = 0;
struct ctldebug debug0 = { "syncprt", &syncprt };
#endif

/* ARGSUSED */
int
sync()
{
    register struct proc *p;
    void *uap = u.u_ap;
	register_t *retval;
	register struct mount *mp, *nmp;
	int asyncflag;

	p = u.u_procp;
	simple_lock(&mountlist_slock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock, p)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
			asyncflag = mp->mnt_flag & MNT_ASYNC;
			mp->mnt_flag &= ~MNT_ASYNC;
			VFS_SYNC(mp, MNT_NOWAIT, p->p_ucred, p);
			if (asyncflag)
				mp->mnt_flag |= MNT_ASYNC;
		}
		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp, p);
	}
	simple_unlock(&mountlist_slock);
#ifdef DEBUG
	if (syncprt)
		vfs_bufstats();
#endif /* DIAGNOSTIC */
	return (0);
}

/*
 * Change filesystem quotas.
 */
/* ARGSUSED */
int
quotactl()
{
    register struct quotactl_args {
		syscallarg(char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(caddr_t) arg;
	} *uap = (struct quotactl_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct mount *mp;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	mp = nd.ni_vp->v_mount;
	vrele(nd.ni_vp);
	return (VFS_QUOTACTL(mp, SCARG(uap, cmd), SCARG(uap, uid),
	    SCARG(uap, arg), p));
}

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
statfs()
{
    register struct statfs_args {
		syscallarg(char *) path;
		syscallarg(struct statfs *) buf;
	} *uap = (struct statfs_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct mount *mp;
	register struct statfs *sp;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return (copyout((caddr_t)sp, (caddr_t)SCARG(uap, buf), sizeof(*sp)));
}

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
fstatfs()
{
	register struct fstatfs_args {
		syscallarg(int) fd;
		syscallarg(struct statfs *) buf;
	} *uap = (struct fstatfs_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct file *fp;
	struct mount *mp;
	register struct statfs *sp;
	int error;

	p = u.u_procp;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATFS(mp, sp, p)))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return (copyout((caddr_t)sp, (caddr_t)SCARG(uap, buf), sizeof(*sp)));
}

/*
 * Get statistics on all filesystems.
 */
int
getfsstat()
{
	register struct getfsstat_args {
		syscallarg(struct statfs *) buf;
		syscallarg(long) bufsize;
		syscallarg(int) flags;
	} *uap = (struct getfsstat_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct mount *mp, *nmp;
	register struct statfs *sp;
	caddr_t sfsp;
	long count, maxcount, error;

	p = u.u_procp;
	maxcount = SCARG(uap, bufsize) / sizeof(struct statfs);
	sfsp = (caddr_t)SCARG(uap, buf);
	count = 0;
	simple_lock(&mountlist_slock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock, p)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		if (sfsp && count < maxcount) {
			sp = &mp->mnt_stat;
			/*
			 * If MNT_NOWAIT is specified, do not refresh the
			 * fsstat cache. MNT_WAIT overrides MNT_NOWAIT.
			 */
			if (((SCARG(uap, flags) & MNT_NOWAIT) == 0 ||
			    (SCARG(uap, flags) & MNT_WAIT)) &&
			    (error = VFS_STATFS(mp, sp, p))) {
				simple_lock(&mountlist_slock);
				nmp = CIRCLEQ_NEXT(mp, mnt_list);
				vfs_unbusy(mp, p);
				continue;
			}
			sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
			if ((error = copyout((caddr_t)sp, sfsp, sizeof(*sp))))
				return (error);
			sfsp += sizeof(*sp);
		}
		count++;
		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp, p);
	}
	simple_unlock(&mountlist_slock);
	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
	return (0);
}

/*
 * Change current working directory to a given file descriptor.
 */
/* ARGSUSED */
int
fchdir()
{
	struct fchdir_args {
		syscallarg(int) fd;
	} *uap = (struct fchdir_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct filedesc *fdp;
	struct vnode *vp, *tdp;
	struct mount *mp;
	struct file *fp;
	int error;

	p = u.u_procp;
	fdp = p->p_fd;
	if ((error = getvnode(fdp, SCARG(uap, fd), &fp)))
		return (error);
	vp = (struct vnode *)fp->f_data;
	VREF(vp);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	while (!error && (mp = vp->v_mountedhere) != NULL) {
		if (vfs_busy(mp, 0, 0, p))
			continue;
		error = VFS_ROOT(mp, &tdp);
		vfs_unbusy(mp, p);
		if (error)
			break;
		vput(vp);
		vp = tdp;
	}
	if (error) {
		vput(vp);
		return (error);
	}
	VOP_UNLOCK(vp, 0, p);
	vrele(fdp->fd_cdir);
	fdp->fd_cdir = vp;
	return (0);
}

/*
 * Change current working directory (``.'').
 */
/* ARGSUSED */
int
chdir()
{
	struct chdir_args {
		syscallarg(char *) path;
	} *uap = (struct chdir_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct filedesc *fdp;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	fdp = p->p_fd;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = change_dir(&nd, p)))
		return (error);
	vrele(fdp->fd_cdir);
	fdp->fd_cdir = nd.ni_vp;
	return (0);
}

/*
 * Change notion of root (``/'') directory.
 */
/* ARGSUSED */
int
chroot()
{
	struct chroot_args {
		syscallarg(char *) path;
	} *uap = (struct chroot_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct filedesc *fdp;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	fdp = p->p_fd;
	if ((error = suser1(p->p_ucred, &p->p_acflag)))
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = change_dir(&nd, p)))
		return (error);
	if (fdp->fd_rdir != NULL)
		vrele(fdp->fd_rdir);
	fdp->fd_rdir = nd.ni_vp;
	return (0);
}

int
fchroot()
{
	struct fchroot_args {
		syscallarg(int) fd;
	} *uap = (struct fchroot_args *) u.u_ap;
	register struct proc *p;
	register struct filedesc *fdp;
	register struct file *fp;
	struct vnode *vp;
	int error;

	p = u.u_procp;
	fdp = p->p_fd;
	if ((error = suser1(p->p_ucred, &p->p_acflag))) {
		return (error);
	}
	if ((error = getvnode(fdp, SCARG(uap, fd), &fp))) {
		return (error);
	}
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VBAD) {
		return (EBADF);
	}
	if ((error = change_chroot(vp, p))) {
		return (error);
	}
	if (fdp->fd_rdir != NULL) {
		vrele(fdp->fd_rdir);
	}
	fdp->fd_rdir = vp;
	vref(vp);
	return (0);
}

/*
 * Common routine for chroot and chdir.
 */
static int
change_chroot(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	int error;

	if (vp->v_type != VDIR) {
		error = ENOTDIR;
	} else {
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	}
	if (error) {
		vput(vp);
	} else {
		VOP_UNLOCK(vp, 0, p);
	}
	return (error);
}

/*
 * Common routine for chroot and chdir.
 */
static int
change_dir(ndp, p)
	register struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *vp;
	int error;

	if ((error = namei(ndp))) {
		return (error);
	}
	vp = ndp->ni_vp;
	return (change_root(vp, p));
}

/*
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
open()
{
	register struct open_args {
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} *uap = (struct open_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct filedesc *fdp;
	register struct file *fp;
	register struct vnode *vp;
	int flags, cmode;
	struct file *nfp;
	int type, indx, error;
	struct flock lf;
	struct nameidata nd;
	extern struct fileops vnops;
	int p_dupfd = (int)u.u_dupfd;

	p = u.u_procp;
	fdp = p->p_fd;
	nfp = falloc();
	error = ufdalloc(nfp);
	if (error != 0) {
	    return (error);
	}
	fp = nfp;
	flags = FFLAGS(SCARG(uap, flags));
	cmode = ((SCARG(uap, mode) &~ fdp->fd_cmask) & ALLPERMS) &~ S_ISTXT;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	p_dupfd = -indx - 1;			/* XXX check for fdopen */
	if ((error = vn_open(&nd, flags, cmode))) {
		ffree(fp);
		if ((error == ENODEV || error == ENXIO) &&
				p_dupfd >= 0 &&			/* XXX from fdopen */
		    (error =
			dupfdopen(fdp, indx, p_dupfd, flags, error)) == 0) {
			*retval = indx;
			return (0);
		}
		if (error == ERESTART)
			error = EINTR;
		fdp->fd_ofiles[indx] = NULL;
		return (error);
	}
	p_dupfd = 0;
	vp = nd.ni_vp;
	fp->f_flag = flags & FMASK;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = (caddr_t)vp;
	if (flags & (O_EXLOCK | O_SHLOCK)) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		if (flags & O_EXLOCK)
			lf.l_type = F_WRLCK;
		else
			lf.l_type = F_RDLCK;
		type = F_FLOCK;
		if ((flags & FNONBLOCK) == 0)
			type |= F_WAIT;
		VOP_UNLOCK(vp, 0, p);
		if ((error = VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf, type))) {
			(void) vn_close(vp, fp->f_flag, fp->f_cred, p);
			ffree(fp);
			fdp->fd_ofiles[indx] = NULL;
			return (error);
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		fp->f_flag |= FHASLOCK;
	}
	VOP_UNLOCK(vp, 0, p);
	*retval = indx;
	return (0);
}

/*
 * Create a special file.
 */
/* ARGSUSED */
int
mknod()
{
	register struct mknod_args {
		syscallarg(char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} *uap = (struct mknod_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct vattr vattr;
	int error;
	int whiteout;
	struct nameidata nd;

	p = u.u_procp;
	if ((error = suser1(p->p_ucred, &p->p_acflag)))
		return (error);
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if (vp != NULL)
		error = EEXIST;
	else {
		VATTR_NULL(&vattr);
		vattr.va_mode = (SCARG(uap, mode) & ALLPERMS) &~ p->p_fd->fd_cmask;
		vattr.va_rdev = SCARG(uap, dev);
		whiteout = 0;

		switch (SCARG(uap, mode) & S_IFMT) {
		case S_IFMT:	/* used by badsect to flag bad sectors */
			vattr.va_type = VBAD;
			break;
		case S_IFCHR:
			vattr.va_type = VCHR;
			break;
		case S_IFBLK:
			vattr.va_type = VBLK;
			break;
		case S_IFWHT:
			whiteout = 1;
			break;
		default:
			error = EINVAL;
			break;
		}
	}
	if (!error) {
		VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
		if (whiteout) {
			error = VOP_WHITEOUT(nd.ni_dvp, &nd.ni_cnd, CREATE);
			if (error)
				VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
			vput(nd.ni_dvp);
		} else {
			error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp,
						&nd.ni_cnd, &vattr);
		}
	} else {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (vp)
			vrele(vp);
	}
	return (error);
}

/*
 * Create a named pipe.
 */
/* ARGSUSED */
int
mkfifo()
{
	register struct mkfifo_args {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} *uap = (struct mkfifo_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
#ifndef FIFO
	return (EOPNOTSUPP);
#else
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if (error = namei(&nd))
		return (error);
	if (nd.ni_vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		return (EEXIST);
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VFIFO;
	vattr.va_mode = (SCARG(uap, mode) & ALLPERMS) &~ p->p_fd->fd_cmask;
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	return (VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr));
#endif /* FIFO */
}

/*
 * Make a hard file link.
 */
/* ARGSUSED */
int
link()
{
	register struct link_args {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} *uap = (struct link_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct nameidata nd;
	int error;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VDIR ||
	    (error = suser1(p->p_ucred, &p->p_acflag)) == 0) {
		nd.ni_cnd.cn_nameiop = CREATE;
		nd.ni_cnd.cn_flags = LOCKPARENT;
		nd.ni_dirp = SCARG(uap, link);
		if ((error = namei(&nd)) == 0) {
			if (nd.ni_vp != NULL)
				error = EEXIST;
			if (!error) {
				VOP_LEASE(nd.ni_dvp, p, p->p_ucred,
				    LEASE_WRITE);
				VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
				error = VOP_LINK(vp, nd.ni_dvp, &nd.ni_cnd);
			} else {
				VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
				if (nd.ni_dvp == nd.ni_vp)
					vrele(nd.ni_dvp);
				else
					vput(nd.ni_dvp);
				if (nd.ni_vp)
					vrele(nd.ni_vp);
			}
		}
	}
	vrele(vp);
	return (error);
}

/*
 * Make a symbolic link.
 */
/* ARGSUSED */

int
symlink()
{
	register struct symlink_args {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} *uap = (struct symlink_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct vattr vattr;
	char *path;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	MALLOC(path, char *, MAXPATHLEN, M_NAMEI, M_WAITOK);
	if ((error = copyinstr(SCARG(uap, path), path, MAXPATHLEN, NULL)))
		goto out;
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, link), p);
	if ((error = namei(&nd)))
		goto out;
	if (nd.ni_vp) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		error = EEXIST;
		goto out;
	}
	VATTR_NULL(&vattr);
	vattr.va_mode = ACCESSPERMS &~ p->p_fd->fd_cmask;
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_SYMLINK(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr, path);
out:
	FREE(path, M_NAMEI);
	return (error);
}

/*
 * Delete a whiteout from the filesystem.
 */
/* ARGSUSED */
int
undelete()
{
	register struct undelete_args {
		syscallarg(char *) path;
	} *uap = (struct undelete_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, DELETE, LOCKPARENT|DOWHITEOUT, UIO_USERSPACE,
	    SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	if (nd.ni_vp != NULLVP || !(nd.ni_cnd.cn_flags & ISWHITEOUT)) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (nd.ni_vp)
			vrele(nd.ni_vp);
		return (EEXIST);
	}

	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	if ((error = VOP_WHITEOUT(nd.ni_dvp, &nd.ni_cnd, DELETE)))
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	vput(nd.ni_dvp);
	return (error);
}

/*
 * Delete a name from the filesystem.
 */
/* ARGSUSED */
int
unlink()
{
	register struct unlink_args {
		syscallarg(char *) path;
	} *uap = (struct unlink_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, DELETE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);

	if (vp->v_type != VDIR ||
	    (error = suser1(p->p_ucred, &p->p_acflag)) == 0) {
		/*
		 * The root of a mounted filesystem cannot be deleted.
		 */
		if (vp->v_flag & VROOT)
			error = EBUSY;
		else
			(void)vnode_pager_uncache(vp);
	}

	if (!error) {
		VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
		error = VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	} else {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (vp != NULLVP)
			vput(vp);
	}
	return (error);
}

/*
 * Reposition read/write file offset.
 */
int
lseek()
{
	register struct lseek_args {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(int) whence;
	} *uap = (struct lseek_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct ucred *cred;
	register struct filedesc *fdp;
	register struct file *fp;
	struct vattr vattr;
	int error;

	p = u.u_procp;
	cred = p->p_ucred;
	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL)
		return (EBADF);
	if (fp->f_type != DTYPE_VNODE)
		return (ESPIPE);
	switch (SCARG(uap, whence)) {
	case L_INCR:
		fp->f_offset += SCARG(uap, offset);
		break;
	case L_XTND:
		if ((error =
		    VOP_GETATTR((struct vnode *)fp->f_data, &vattr, cred, p)))
			return (error);
		fp->f_offset = SCARG(uap, offset) + vattr.va_size;
		break;
	case L_SET:
		fp->f_offset = SCARG(uap, offset);
		break;
	default:
		return (EINVAL);
	}
	*(off_t *)retval = fp->f_offset;
	return (0);
}

/*
 * Check access permissions.
 */
int
access()
{
	register struct access_args {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} *uap = (struct access_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct ucred *cred;
	register struct vnode *vp;
	int error, flags, t_gid, t_uid;
	struct nameidata nd;

	p = u.u_procp;
	cred = p->p_ucred;
	t_uid = cred->cr_uid;
	t_gid = cred->cr_groups[0];
	cred->cr_uid = p->p_cred->p_ruid;
	cred->cr_groups[0] = p->p_cred->p_rgid;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)))
		goto out1;
	vp = nd.ni_vp;

	/* Flags == 0 means only check for existence. */
	if (SCARG(uap, flags)) {
		flags = 0;
		if (SCARG(uap, flags) & R_OK)
			flags |= VREAD;
		if (SCARG(uap, flags) & W_OK)
			flags |= VWRITE;
		if (SCARG(uap, flags) & X_OK)
			flags |= VEXEC;
		if ((flags & VWRITE) == 0 || (error = vn_writechk(vp)) == 0)
			error = VOP_ACCESS(vp, flags, cred, p);
	}
	vput(vp);
out1:
	cred->cr_uid = t_uid;
	cred->cr_groups[0] = t_gid;
	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
stat()
{
	register struct stat_args {
		syscallarg(char *) path;
		syscallarg(struct stat *) ub;
	} *uap = (struct stat_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct stat sb;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	error = copyout((caddr_t)&sb, (caddr_t)SCARG(uap, ub), sizeof (sb));
	return (error);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */

int
lstat()
{
	register struct lstat_args {
		syscallarg(char *) path;
		syscallarg(struct stat *) ub;
	} *uap = (struct lstat_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	int error;
	struct vnode *vp, *dvp;
	struct stat sb, sb1;
	struct nameidata nd;
	p = u.u_procp;
	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF | LOCKPARENT, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	/*
	 * For symbolic links, always return the attributes of its containing
	 * directory, except for mode, size, inode number, and links.
	 */
	vp = nd.ni_vp;
	dvp = nd.ni_dvp;
	if (vp->v_type != VLNK) {
		if (dvp == vp)
			vrele(dvp);
		else
			vput(dvp);
		error = vn_stat(vp, &sb, p);
		vput(vp);
		if (error)
			return (error);
	} else {
		error = vn_stat(dvp, &sb, p);
		vput(dvp);
		if (error) {
			vput(vp);
			return (error);
		}
		error = vn_stat(vp, &sb1, p);
		vput(vp);
		if (error)
			return (error);
		sb.st_mode &= ~S_IFDIR;
		sb.st_mode |= S_IFLNK;
		sb.st_nlink = sb1.st_nlink;
		sb.st_size = sb1.st_size;
		sb.st_blocks = sb1.st_blocks;
		sb.st_ino = sb1.st_ino;
	}
	error = copyout((caddr_t)&sb, (caddr_t)SCARG(uap, ub), sizeof (sb));
	return (error);
}

/*
 * Get configurable pathname variables.
 */
/* ARGSUSED */
int
pathconf()
{
	register struct pathconf_args {
		syscallarg(char *) path;
		syscallarg(int) name;
	} *uap = (struct pathconf_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	error = VOP_PATHCONF(nd.ni_vp, SCARG(uap, name), retval);
	vput(nd.ni_vp);
	return (error);
}

/*
 * Return target name of a symbolic link.
 */
/* ARGSUSED */
int
readlink()
{
	register struct readlink_args {
		syscallarg(char *) path;
		syscallarg(char *) buf;
		syscallarg(int) count;
	} *uap = (struct readlink_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct iovec aiov;
	struct uio auio;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VLNK)
		error = EINVAL;
	else {
		aiov.iov_base = SCARG(uap, buf);
		aiov.iov_len = SCARG(uap, count);
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_USERSPACE;
		auio.uio_procp = p;
		auio.uio_resid = SCARG(uap, count);
		error = VOP_READLINK(vp, &auio, p->p_ucred);
	}
	vput(vp);
	*retval = SCARG(uap, count) - auio.uio_resid;
	return (error);
}

/*
 * Change flags of a file given a path name.
 */
/* ARGSUSED */
int
chflags()
{
	register struct chflags_args {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} *uap = (struct chflags_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	VATTR_NULL(&vattr);
	vattr.va_flags = SCARG(uap, flags);
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	vput(vp);
	return (error);
}

/*
 * Change flags of a file given a file descriptor.
 */
/* ARGSUSED */

int
fchflags()
{
	register struct fchflags_args {
		syscallarg(int) fd;
		syscallarg(int) flags;
	} *uap = (struct fchflags_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	p = u.u_procp;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	vp = (struct vnode *)fp->f_data;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	VATTR_NULL(&vattr);
	vattr.va_flags = SCARG(uap, flags);
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * Change mode of a file given path name.
 */
/* ARGSUSED */
int
chmod()
{
	register struct chmod_args {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} *uap = (struct chmod_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	VATTR_NULL(&vattr);
	vattr.va_mode = SCARG(uap, mode) & ALLPERMS;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	vput(vp);
	return (error);
}

/*
 * Change mode of a file given a file descriptor.
 */
/* ARGSUSED */
int
fchmod()
{
	register struct fchmod_args {
		syscallarg(int) fd;
		syscallarg(int) mode;
	} *uap = (struct fchmod_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	p = u.u_procp;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	vp = (struct vnode *)fp->f_data;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	VATTR_NULL(&vattr);
	vattr.va_mode = SCARG(uap, mode) & ALLPERMS;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * Set ownership given a path name.
 */
/* ARGSUSED */
int
chown()
{
	register struct chown_args {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} *uap = (struct chown_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	VATTR_NULL(&vattr);
	vattr.va_uid = SCARG(uap, uid);
	vattr.va_gid = SCARG(uap, gid);
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	vput(vp);
	return (error);
}

/*
 * Set ownership given a file descriptor.
 */
/* ARGSUSED */
int
fchown()
{
	register struct fchown_args {
		syscallarg(int) fd;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} *uap = (struct fchown_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	p = u.u_procp;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	vp = (struct vnode *)fp->f_data;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	VATTR_NULL(&vattr);
	vattr.va_uid = SCARG(uap, uid);
	vattr.va_gid = SCARG(uap, gid);
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * Set the access and modification times of a file.
 */
/* ARGSUSED */
int
utimes()
{
	register struct utimes_args {
		syscallarg(char *) path;
		syscallarg(struct timeval *) tptr;
	} *uap = (struct utimes_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct timeval tv[2];
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	VATTR_NULL(&vattr);
	if (SCARG(uap, tptr) == NULL) {
		microtime(&tv[0]);
		tv[1] = tv[0];
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else if ((error = copyin((caddr_t)SCARG(uap, tptr), (caddr_t)tv,
	    sizeof (tv))))
  		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	vattr.va_atime.tv_sec = tv[0].tv_sec;
	vattr.va_atime.tv_nsec = tv[0].tv_usec * 1000;
	vattr.va_mtime.tv_sec = tv[1].tv_sec;
	vattr.va_mtime.tv_nsec = tv[1].tv_usec * 1000;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	vput(vp);
	return (error);
}

/*
 * Truncate a file given its path name.
 */
/* ARGSUSED */
int
truncate()
{
	register struct truncate_args {
		syscallarg(char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	}  *uap = (struct truncate_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0 &&
	    (error = VOP_ACCESS(vp, VWRITE, p->p_ucred, p)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
	vput(vp);
	return (error);
}

/*
 * Truncate a file given a file descriptor.
 */
/* ARGSUSED */
int
ftruncate()
{
    register struct ftruncate_args {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} *uap = (struct ftruncate_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	p = u.u_procp;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	if ((fp->f_flag & FWRITE) == 0)
		return (EINVAL);
	vp = (struct vnode *)fp->f_data;
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, fp->f_cred, p);
	}
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * Sync an open file.
 */
/* ARGSUSED */
int
fsync()
{
	register struct fsync_args {
		syscallarg(int) fd;
	} *uap = (struct fsync_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct file *fp;
	int error;

	p = u.u_procp;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	error = VOP_FSYNC(vp, fp->f_cred, MNT_WAIT, fp->f_flag, p);
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * Rename files.  Source and destination must either both be directories,
 * or both not be directories.  If target is a directory, it must be empty.
 */
/* ARGSUSED */

int
rename()
{
	register struct rename_args {
		syscallarg(char *) from;
		syscallarg(char *) to;
	} *uap = (struct rename_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *tvp, *fvp, *tdvp;
	struct nameidata fromnd, tond;
	int error;

	p = u.u_procp;
	NDINIT(&fromnd, DELETE, WANTPARENT | SAVESTART, UIO_USERSPACE,
	    SCARG(uap, from), p);
	if ((error = namei(&fromnd)))
		return (error);
	fvp = fromnd.ni_vp;
	NDINIT(&tond, RENAME, LOCKPARENT | LOCKLEAF | NOCACHE | SAVESTART,
	    UIO_USERSPACE, SCARG(uap, to), p);
	if ((error = namei(&tond))) {
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
		goto out1;
	}
	tdvp = tond.ni_dvp;
	tvp = tond.ni_vp;
	if (tvp != NULL) {
		if (fvp->v_type == VDIR && tvp->v_type != VDIR) {
			error = ENOTDIR;
			goto out;
		} else if (fvp->v_type != VDIR && tvp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}
	}
	if (fvp == tdvp)
		error = EINVAL;
	/*
	 * If source is the same as the destination (that is the
	 * same inode number with the same name in the same directory),
	 * then there is nothing to do.
	 */
	if (fvp == tvp && fromnd.ni_dvp == tdvp &&
	    fromnd.ni_cnd.cn_namelen == tond.ni_cnd.cn_namelen &&
	    !bcmp(fromnd.ni_cnd.cn_nameptr, tond.ni_cnd.cn_nameptr,
	      fromnd.ni_cnd.cn_namelen))
		error = -1;
out:
	if (!error) {
		VOP_LEASE(tdvp, p, p->p_ucred, LEASE_WRITE);
		if (fromnd.ni_dvp != tdvp)
			VOP_LEASE(fromnd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
		if (tvp)
			VOP_LEASE(tvp, p, p->p_ucred, LEASE_WRITE);
		error = VOP_RENAME(fromnd.ni_dvp, fromnd.ni_vp, &fromnd.ni_cnd,
				   tond.ni_dvp, tond.ni_vp, &tond.ni_cnd);
	} else {
		VOP_ABORTOP(tond.ni_dvp, &tond.ni_cnd);
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
	}
	vrele(tond.ni_startdir);
	FREE(tond.ni_cnd.cn_pnbuf, M_NAMEI);
out1:
	if (fromnd.ni_startdir)
		vrele(fromnd.ni_startdir);
	FREE(fromnd.ni_cnd.cn_pnbuf, M_NAMEI);
	if (error == -1)
		return (0);
	return (error);
}

/*
 * Make a directory file.
 */
/* ARGSUSED */
int
mkdir()
{
    register struct mkdir_args {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} *uap = (struct mkdir_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if (vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(vp);
		return (EEXIST);
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VDIR;
	vattr.va_mode = (SCARG(uap, mode) & ACCESSPERMS) &~ p->p_fd->fd_cmask;
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_MKDIR(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (!error)
		vput(nd.ni_vp);
	return (error);
}

/*
 * Remove a directory file.
 */
/* ARGSUSED */
int
rmdir()
{
    register struct rmdir_args {
		syscallarg(char *) path;
	} *uap = (struct rmdir_args *) u.u_ap;

	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	/*
	 * No rmdir "." please.
	 */
	if (nd.ni_dvp == vp) {
		error = EINVAL;
		goto out;
	}
	/*
	 * The root of a mounted filesystem cannot be deleted.
	 */
	if (vp->v_flag & VROOT)
		error = EBUSY;
out:
	if (!error) {
		VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
		VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
		error = VOP_RMDIR(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	} else {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
	}
	return (error);
}

/*
 * Read a block of directory entries in a file system independent format.
 */
int
getdirentries()
{
	register struct getdirentries_args {
		syscallarg(int)		fd;
		syscallarg(char *)	buf;
		syscallarg(u_int)	count;
		syscallarg(long *)	basep;
	} *uap = (struct getdirentries_args *) u.u_ap;
	register struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	long loff;
	int error, eofflag;

	p = u.u_procp;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);
	vp = (struct vnode *)fp->f_data;
unionread:
	if (vp->v_type != VDIR)
		return (EINVAL);
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, count);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	auio.uio_resid = SCARG(uap, count);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	loff = auio.uio_offset = fp->f_offset;
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag,
			    (int *)0, (u_long **)0);
	fp->f_offset = auio.uio_offset;
	VOP_UNLOCK(vp, 0, p);
	if (error)
		return (error);

#ifdef UNION
{
	extern struct vnodeops *union_vnodeops;
	extern struct vnode *union_dircache (struct vnode*, struct proc*);

	if ((SCARG(uap, count) == auio.uio_resid) &&
	    (vp->v_op == union_vnodeops)) {
		struct vnode *lvp;

		lvp = union_dircache(vp, p);
		if (lvp != NULLVP) {
			struct vattr va;

			/*
			 * If the directory is opaque,
			 * then don't show lower entries
			 */
			error = VOP_GETATTR(vp, &va, fp->f_cred, p);
			if (va.va_flags & OPAQUE) {
				vput(lvp);
				lvp = NULL;
			}
		}

		if (lvp != NULLVP) {
			error = VOP_OPEN(lvp, FREAD, fp->f_cred, p);
			if (error) {
				vput(lvp);
				return (error);
			}
			VOP_UNLOCK(lvp, 0, p);
			fp->f_data = (caddr_t) lvp;
			fp->f_offset = 0;
			error = vn_close(vp, FREAD, fp->f_cred, p);
			if (error)
				return (error);
			vp = lvp;
			goto unionread;
		}
	}
}
#endif /* UNION */

	if ((SCARG(uap, count) == auio.uio_resid) &&
	    (vp->v_flag & VROOT) &&
	    (vp->v_mount->mnt_flag & MNT_UNION)) {
		struct vnode *tvp = vp;
		vp = vp->v_mount->mnt_vnodecovered;
		VREF(vp);
		fp->f_data = (caddr_t) vp;
		fp->f_offset = 0;
		vrele(tvp);
		goto unionread;
	}
	error = copyout((caddr_t)&loff, (caddr_t)SCARG(uap, basep),
	    sizeof(long));
	*retval = SCARG(uap, count) - auio.uio_resid;
	return (error);
}

/*
 * Set the mode mask for creation of filesystem nodes.
 */
int
umask()
{
    register struct umask_args {
		syscallarg(int) newmask;
	} *uap = (struct umask_args *) u.u_ap;

	register struct proc *p;
	register struct filedesc *fdp;
	register_t *retval;

	p = u.u_procp;
	fdp = p->p_fd;
	*retval = fdp->fd_cmask;
	fdp->fd_cmask = SCARG(uap, newmask) & ALLPERMS;
	return (0);
}

/*
 * Void all references to file by ripping underlying filesystem
 * away from vnode.
 */
/* ARGSUSED */
int
revoke()
{
	register struct revoke_args {
		syscallarg(char *) path;
	} *uap = (struct revoke_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	register struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	p = u.u_procp;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)))
		goto out;
	if (p->p_ucred->cr_uid != vattr.va_uid &&
	    (error = suser1(p->p_ucred, &p->p_acflag)))
		goto out;
	if (vp->v_usecount > 1 || (vp->v_flag & VALIASED))
		VOP_REVOKE(vp, REVOKEALL);
out:
	vrele(vp);
	return (error);
}

/*
 * Convert a user file descriptor to a kernel file entry.
 */
int
getvnode(fdp, fd, fpp)
	struct filedesc *fdp;
	struct file **fpp;
	int fd;
{
	return (getfiledesc(fdp, fd, fpp, DTYPE_VNODE));
}
