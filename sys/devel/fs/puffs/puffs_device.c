/*	$NetBSD: puffs_msgif.c,v 1.8 2006/11/21 01:53:33 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_msgif.c,v 1.8 2006/11/21 01:53:33 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/lock.h>
#include <sys/poll.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_mount.h>
#include <fs/puffs/puffs.h>


int	puffscdopen(dev_t, int, int, struct proc *);
int	puffscdclose(dev_t, int, int, struct proc *);

/*
 * Device routines
 */

dev_type_open(puffscdopen);
dev_type_close(puffscdclose);
//dev_type_ioctl(puffscdioctl);

/* dev */
const struct cdevsw puffs_cdevsw = {
		.d_open = puffscdopen,
		.d_close = puffscdclose,
		.d_read = noread,
		.d_write = nowrite,
		.d_ioctl = noioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = nommap,
		.d_kqfilter = nokqfilter,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

static int puffs_fop_rw(struct file *, struct uio *, struct ucred *);
static int puffs_fop_read(struct file *, struct uio *, struct ucred *);
static int puffs_fop_write(struct file *, struct uio *, struct ucred *);
static int puffs_fop_ioctl(struct file*, u_long, void *, struct proc *);
static int puffs_fop_poll(struct file *, int, struct proc *);
static int puffs_fop_stat(struct file *, struct stat *, struct proc *);
static int puffs_fop_close(struct file *, struct proc *);
static int puffs_fop_kqfilter(struct file *, struct knote *);
static int puffsgetop(struct puffs_mount *, struct puffs_req *, int);
static int puffsputop(struct puffs_mount *, struct puffs_req *);
static int puffssizeop(struct puffs_mount *, struct puffs_sizeop *);

/* fd routines, for cloner */
struct fileops puffs_fileops = {
		.fo_rw = puffs_fop_rw,
		.fo_read = puffs_fop_read,
		.fo_write = puffs_fop_write,
		.fo_ioctl = puffs_fop_ioctl,
		.fo_poll = puffs_fop_poll,
		.fo_stat = puffs_fop_stat,
		.fo_close = puffs_fop_close,
		.fo_kqfilter = puffs_fop_kqfilter,
};

/*
 * puffs instance structures.  these are always allocated and freed
 * from the context of the device node / fileop code.
 */
struct puffs_instance {
	pid_t 	pi_pid;
	int 	pi_idx;
	int 	pi_fd;
	struct puffs_mount *pi_pmp;
	struct selinfo pi_sel;
	struct timespec	pi_atime;
	struct timespec	pi_mtime;
	struct timespec	pi_btime;

	TAILQ_ENTRY(puffs_instance) pi_entries;
};

#define PMP_EMBRYO 	((struct puffs_mount *)-1)	/* before mount	*/
#define PMP_DEAD 	((struct puffs_mount *)-2)	/* goner	*/

static TAILQ_HEAD(, puffs_instance) puffs_ilist = TAILQ_HEAD_INITIALIZER(puffs_ilist);

static struct lock_object pi_lock;

static int get_pi_idx(struct puffs_instance *);

void puffsattach(void);

void
puffsattach(void)
{
	simple_lock_init(&pi_lock, "puffs_fop_lock");
}

void
puffs_destroy(void)
{

}

/* search sorted list of instances for free minor, sorted insert arg */
static int
get_pi_idx(struct puffs_instance *pi_i)
{
	struct puffs_instance *pi;
	int i;

	i = 0;
	TAILQ_FOREACH(pi, &puffs_ilist, pi_entries) {
		if (i == PUFFS_CLONER) {
			return (PUFFS_CLONER);
		}
		if (i != pi->pi_idx) {
			break;
		}
		i++;
	}

	pi_i->pi_pmp = PMP_EMBRYO;

	if (pi == NULL) {
		TAILQ_INSERT_TAIL(&puffs_ilist, pi_i, pi_entries);
	} else {
		TAILQ_INSERT_BEFORE(pi, pi_i, pi_entries);
	}
	return (i);
}

int
puffscdopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct puffs_instance *pi;
	struct file *fp;
	int error, fd, idx;

	/*
	 * XXX: decide on some security model and check permissions
	 */

	if (minor(dev) != PUFFS_CLONER) {
		return (ENXIO);
	}

	fp = falloc();
	if (fp == NULL) {
		return (ENFILE);
	}
	error = ufdalloc(fp);
	if (error != 0) {
		return (error);
	}

	MALLOC(pi, struct puffs_instance *, sizeof(struct puffs_instance),
			M_PUFFS, M_WAITOK | M_ZERO);

	simple_lock(&pi_lock);
	idx = get_pi_idx(pi);
	if (idx == PUFFS_CLONER) {
		simple_unlock(&pi_lock);
		FREE(pi);
		FILE_UNUSE(fp, p);
		ffree(fp);
		return (EBUSY);
	}

	pi->pi_pid = p->p_pid;
	pi->pi_idx = idx;
	simple_unlock(&pi_lock);

	DPRINTF(("puffscdopen: registered embryonic pmp for pid: %d\n",
	    pi->pi_pid));

	return (fdclone(p, fp, fd, FREAD|FWRITE, &puffs_fileops, pi));
}

int
puffscdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	panic("puffscdclose\n");
	return (0);
}

/*
 * Set puffs_mount -pointer.  Called from puffs_mount(), which is the
 * earliest place that knows about this.
 *
 * We only want to make sure that the caller had the right to open the
 * device, we don't so much care about which context it gets in case
 * the same process opened multiple (since they are equal at this point).
 */
int
puffs_setpmp(pid_t pid, int fd, struct puffs_mount *pmp)
{
	struct puffs_instance *pi;
	int rv = 1;

	simple_lock(&pi_lock);
	TAILQ_FOREACH(pi, &puffs_ilist, pi_entries) {
		if (pi->pi_pid == pid && pi->pi_pmp == PMP_EMBRYO) {
			pi->pi_pmp = pmp;
			pi->pi_fd = fd;
			pmp->pmp_sel = &pi->pi_sel;
			rv = 0;
			break;
		}
	}
	simple_unlock(&pi_lock);

	return (rv);
}

/*
 * Remove mount point from list of instances.  Called from unmount.
 */
void
puffs_nukebypmp(struct puffs_mount *pmp)
{
	struct puffs_instance *pi;

	simple_lock(&pi_lock);
	TAILQ_FOREACH(pi, &puffs_ilist, pi_entries) {
		if (pi->pi_pmp == pmp) {
			TAILQ_REMOVE(&puffs_ilist, pi, pi_entries);
			break;
		}
	}
	if (pi) {
		pi->pi_pmp = PMP_DEAD;
	}

#ifdef DIAGNOSTIC
	else {
		panic("puffs_nukebypmp: invalid puffs_mount\n");
	}
#endif /* DIAGNOSTIC */

	simple_unlock(&pi_lock);

	DPRINTF(("puffs_nukebypmp: nuked %p\n", pi));
}

static int
puffs_fop_rw(struct file *fp, struct uio *uio, struct ucred *cred)
{
	enum uio_rw rw = uio->uio_rw;
	int error;

	if (rw == UIO_READ) {
		error = puffs_fop_read(fp, uio, cred);
	} else {
		error = puffs_fop_write(fp, uio, cred);
	}
	return (error);
}

static int
puffs_fop_read(struct file *fp, struct uio *uio, struct ucred *cred)
{
	return (ENODEV);
}

static int
puffs_fop_write(struct file *fp, struct uio *uio, struct ucred *cred)
{
	return (ENODEV);
}

static int
puffs_fop_ioctl(struct file *fp, u_long cmd, void *data, struct proc *p)
{
	struct puffs_mount *pmp;
	int rv;

	pmp = (struct puffs_mount *)fp->f_data;
	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD) {
		printf("puffs_fop_ioctl: puffs %p, not mounted\n", pmp);
		return (ENOENT);
	}

	switch (cmd) {
	case PUFFSGETOP:
		rv = puffsgetop(pmp, data, fp->f_flag & FNONBLOCK);
		break;

	case PUFFSPUTOP:
		rv =  puffsputop(pmp, data);
		break;

	case PUFFSSIZEOP:
		rv = puffssizeop(pmp, data);
		break;

	case PUFFSSTARTOP:
		rv = puffs_start2(pmp, data);
		break;

	/* already done in sys_ioctl() */
	case FIONBIO:
		rv = 0;
		break;

	default:
		rv = EINVAL;
		break;
	}

	return (rv);
}

/*
 * Poll query interface.  The question is only if an event
 * can be read from us (and by read I mean ioctl... ugh).
 */
#define PUFFPOLL_EVSET (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI)
static int
puffs_fop_poll(struct file *fp, int events, struct proc *p)
{
	struct puffs_mount *pmp;
	int revents;

	pmp = (struct puffs_mount *)fp->f_data;
	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD) {
		printf("puffs_fop_ioctl: puffs %p, not mounted\n", pmp);
		return (ENOENT);
	}

	revents = events & (POLLOUT | POLLWRNORM | POLLWRBAND);
	if ((events & PUFFPOLL_EVSET) == 0) {
		return (revents);
	}

	/* check queue */
	simple_lock(&pmp->pmp_lock);
	if (!TAILQ_EMPTY(&pmp->pmp_req_touser)) {
		revents |= PUFFPOLL_EVSET;
	} else {
		selrecord(p, pmp->pmp_sel);
	}
	simple_unlock(&pmp->pmp_lock);

	return (revents);
}

static int
puffs_fop_stat(struct file *fp, struct stat *sb, struct proc *p)
{
	struct puffs_instance *pi;

	pi = (struct puffs_instance *)fp->f_data;
	(void)memset(sb, 0, sizeof(*sb));
	sb->st_dev = makedev(cdevsw_lookup_major(&puffs_cdevsw), pi->pi_idx);
	sb->st_atime = pi->pi_atime;
	sb->st_mtime = pi->pi_mtime;
	sb->st_ctime = sb->st_birthtime = pi->pi_btime;
	sb->st_uid = geteuid();
	sb->st_gid = getegid();
	sb->st_mode = S_IFCHR;
	return (0);
}

static int
puffs_fop_close(struct file *fp, struct proc *p)
{
	struct puffs_instance *pi;
	struct puffs_mount *pmp;
	struct mount *mp;
	int gone;

	DPRINTF(("puffs_fop_close: device closed, force filesystem unmount\n"));

	simple_lock(&pi_lock);
	pmp = (struct puffs_mount *)fp->f_data;
	/*
	 * First check if the fs was never mounted.  In that case
	 * remove the instance from the list.  If mount is attempted later,
	 * it will simply fail.
	 */
	if (pmp == PMP_EMBRYO) {
		pi = (struct puffs_instance *)fp->f_data;
		TAILQ_REMOVE(&puffs_ilist, pi, pi_entries);
		simple_unlock(&pi_lock);
		FREE(pi);
		return (0);
	}

	/*
	 * Next, analyze unmount was called and the instance is dead.
	 * In this case we can just free the structure and go home, it
	 * was removed from the list by puffs_nukebypmp().
	 */
	if (pmp == PMP_DEAD) {
		/* would be nice, but don't have a reference to it ... */
		/* KASSERT(pmp_status == PUFFSTAT_DYING); */
		simple_unlock(&pi_lock);
		pi = (struct puffs_instance*) fp->f_data;
		FREE(pi);
		return (0);
	}

	/*
	 * So we have a reference.  Proceed to unwrap the file system.
	 */
	mp = PMPTOMP(pmp);
	simple_unlock(&pi_lock);

	/*
	 * Free the waiting callers before proceeding any further.
	 * The syncer might be jogging around in this file system
	 * currently.  If we allow it to go to the userspace of no
	 * return while trying to get the syncer lock, well ...
	 * synclk: I feel happy, I feel fine.
	 * lockmgr: You're not fooling anyone, you know.
	 */
	puffs_userdead(pmp);

	/*
	 * Detach from VFS.  First do necessary XXX-dance (from
	 * sys_unmount() & other callers of dounmount()
	 *
	 * XXX Freeze syncer.  Must do this before locking the
	 * mount point.  See dounmount() for details.
	 *
	 * XXX2: take a reference to the mountpoint before starting to
	 * wait for syncer_lock.  Otherwise the mointpoint can be
	 * wiped out while we wait.
	 */
#ifdef notyet
	simple_lock(&mp->mnt_slock);
	mp->mnt_wcnt++;
	simple_unlock(&mp->mnt_slock);

	lockmgr(&syncer_lock, LK_EXCLUSIVE, NULL, p->p_pid);

	simple_lock(&mp->mnt_slock);
	mp->mnt_wcnt--;
	if (mp->mnt_wcnt == 0) {
		wakeup(&mp->mnt_wcnt);
	}
	gone = mp->mnt_iflag & IMNT_GONE;
	simple_unlock(&mp->mnt_slock);
	if (gone) {
		lockmgr(&syncer_lock, LK_RELEASE, NULL, p->p_pid);
		return (0);
	}

	/*
	 * microscopic race condition here (although not with the current
	 * kernel), but can't really fix it without starting a crusade
	 * against vfs_busy(), so let it be, let it be, let it be
	 */

	/*
	 * The only way vfs_busy() will fail for us is if the filesystem
	 * is already a goner.
	 * XXX: skating on the thin ice of modern calling conventions ...
	 */
	if (vfs_busy(mp, 0, NULL, p)) {
		lockmgr(&syncer_lock, LK_RELEASE, NULL, p->p_pid);
		return (0);
	}
#endif
	/* Once we have the mount point, unmount() can't interfere */
	dounmount(mp, MNT_FORCE, p);

	return (0);
}

static void
filt_puffsdetach(struct knote *kn)
{
	struct puffs_instance *pi;

	pi = kn->kn_hook;
	simple_lock(&pi_lock);
	SIMPLEQ_REMOVE(&pi->pi_sel.si_klist, kn, knote, kn_selnext);
	simple_unlock(&pi_lock);
}

static int
filt_puffsioctl(struct knote *kn, long hint)
{
	struct puffs_instance *pi;
	struct puffs_mount *pmp;
	int error;

	pi = kn->kn_hook;
	error = 0;
	simple_lock(&pi_lock);
	pmp = pi->pi_pmp;
	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD) {
		error = 1;
	}
	simple_unlock(&pi_lock);
	if (error) {
		return (0);
	}

	simple_lock(&pmp->pmp_lock);
	kn->kn_data = pmp->pmp_req_touser_waiters;
	simple_unlock(&pmp->pmp_lock);

	return (kn->kn_data != 0);
}

static const struct filterops puffsioctl_filtops =
	{ 1, NULL, filt_puffsdetach, filt_puffsioctl };

static int
puffs_fop_kqfilter(struct file *fp, struct knote *kn)
{
	struct puffs_instance *pi;
	struct klist *klist;

	pi = (struct puffs_instance *)fp->f_data;
	if (kn->kn_filter != EVFILT_READ) {
		return (1);
	}

	klist = &pi->pi_sel.si_klist;
	kn->kn_fop = &puffsioctl_filtops;
	kn->kn_hook = pi;

	simple_lock(&pi_lock);
	SIMPLEQ_INSERT_HEAD(klist, kn, kn_selnext);
	simple_unlock(&pi_lock);

	return (0);
}

/*
 * ioctl handlers
 */

static int
puffsgetop(struct puffs_mount *pmp, struct puffs_req *preq, int nonblock)
{
	struct puffs_park *park;
	int error;

	simple_lock(&pmp->pmp_lock);
 again:
	if (pmp->pmp_status != PUFFSTAT_RUNNING) {
		simple_unlock(&pmp->pmp_lock);
		return ENXIO;
	}
	if (TAILQ_EMPTY(&pmp->pmp_req_touser)) {
		if (nonblock) {
			simple_unlock(&pmp->pmp_lock);
			return EWOULDBLOCK;
		}
		ltsleep(&pmp->pmp_req_touser, PUSER, "puffs2", 0, &pmp->pmp_lock);
		goto again;
	}

	park = TAILQ_FIRST(&pmp->pmp_req_touser);
	if (preq->preq_auxlen < park->park_copylen) {
		simple_unlock(&pmp->pmp_lock);
		return E2BIG;
	}
	TAILQ_REMOVE(&pmp->pmp_req_touser, park, park_entries);
	pmp->pmp_req_touser_waiters--;
	simple_unlock(&pmp->pmp_lock);

	preq->preq_id = park->park_id;
	preq->preq_opclass = park->park_opclass;
	preq->preq_optype = park->park_optype;
	preq->preq_cookie = park->park_cookie;
	preq->preq_auxlen = park->park_copylen;

	if ((error = copyout(park->park_kernbuf, preq->preq_aux,
	    park->park_copylen)) != 0) {
		/*
		 * ok, user server is probably trying to cheat.
		 * stuff op back & return error to user
		 */
		 simple_lock(&pmp->pmp_lock);
		 TAILQ_INSERT_HEAD(&pmp->pmp_req_touser, park, park_entries);
		 simple_unlock(&pmp->pmp_lock);
		 return error;
	}

	if (PUFFSOP_WANTREPLY(park->park_opclass)) {
		simple_lock(&pmp->pmp_lock);
		TAILQ_INSERT_TAIL(&pmp->pmp_req_replywait, park, park_entries);
		simple_unlock(&pmp->pmp_lock);
	} else {
		free(park->park_kernbuf);
		free(park);
	}

	return (0);
}

static int
puffsputop(struct puffs_mount *pmp, struct puffs_req *preq)
{
	struct puffs_park *park;
	size_t copylen;
	int error;

	simple_lock(&pmp->pmp_lock);
	TAILQ_FOREACH(park, &pmp->pmp_req_replywait, park_entries) {
		if (park->park_id == preq->preq_id) {
			TAILQ_REMOVE(&pmp->pmp_req_replywait, park,
			    park_entries);
			break;
		}
	}
	simple_unlock(&pmp->pmp_lock);

	if (park == NULL) {
		return (EINVAL);
	}

	/*
	 * check size of incoming transmission.  allow to allocate a
	 * larger kernel buffer only if it was specified by the caller
	 * by setting preq->preq_auxadj.  Else, just copy whatever the
	 * kernel buffer size is unless.
	 *
	 * However, don't allow ludicrously large buffers
	 */
	copylen = preq->preq_auxlen;
	if (copylen > pmp->pmp_req_maxsize) {
#ifdef DIAGNOSTIC
		printf("puffsputop: outrageous user buf size: %zu\n", copylen);
#endif
		error = EFAULT;
		goto out;
	}

	if (park->park_buflen < copylen
			&& (park->park_flags & PUFFS_REQFLAG_ADJBUF)) {
		free(park->park_kernbuf);
		park->park_kernbuf = malloc(copylen, M_PUFFS, M_WAITOK);
		park->park_buflen = copylen;
	}

	error = copyin(preq->preq_aux, park->park_kernbuf, copylen);

	/*
	 * if copyin botched, inform both userspace and the vnodeop
	 * desperately waiting for information
	 */
 out:
	if (error) {
		park->park_rv = error;
	} else {
		park->park_rv = preq->preq_rv;
	}
	wakeup(park);

	return (error);
}

/* this is probably going to die away at some point? */
static int
puffssizeop(struct puffs_mount *pmp, struct puffs_sizeop *psop_user)
{
	struct puffs_sizepark *pspark;
	void *kernbuf;
	size_t copylen;
	int error;

	/* locate correct op */
	simple_lock(&pmp->pmp_lock);
	TAILQ_FOREACH(pspark, &pmp->pmp_req_sizepark, pkso_entries) {
		if (pspark->pkso_reqid == psop_user->pso_reqid) {
			TAILQ_REMOVE(&pmp->pmp_req_sizepark, pspark, pkso_entries);
			break;
		}
	}
	simple_unlock(&pmp->pmp_lock);

	if (pspark == NULL) {
		return (EINVAL);
	}

	error = 0;
	copylen = MIN(pspark->pkso_bufsize, psop_user->pso_bufsize);

	/*
	 * XXX: uvm stuff to avoid bouncy-bouncy copying?
	 */
	if (PUFFS_SIZEOP_UIO(pspark->pkso_reqtype)) {
		kernbuf = malloc(copylen, M_PUFFS, M_WAITOK | M_ZERO);
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_UIO_IN) {
			error = copyin(psop_user->pso_userbuf, kernbuf, copylen);
			if (error) {
				printf("psop ERROR1 %d\n", error);
				goto escape;
			}
		}
		error = uiomove(kernbuf, copylen, pspark->pkso_uio);
		if (error) {
			printf("uiomove from kernel %p, len %d failed: %d\n", kernbuf,
					(int) copylen, error);
			goto escape;
		}

		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_UIO_OUT) {
			error = copyout(kernbuf, psop_user->pso_userbuf, copylen);
			if (error) {
				printf("psop ERROR2 %d\n", error);
				goto escape;
			}
		}
 escape:
		free(kernbuf);
	} else if (PUFFS_SIZEOP_BUF(pspark->pkso_reqtype)) {
		copylen = MAX(pspark->pkso_bufsize, psop_user->pso_bufsize);
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_BUF_IN) {
			error = copyin(psop_user->pso_userbuf, pspark->pkso_copybuf,
					copylen);
		} else {
			error = copyout(pspark->pkso_copybuf, psop_user->pso_userbuf,
					copylen);
		}
	}
#ifdef DIAGNOSTIC
	else {
		panic("puffssizeop: invalid reqtype %d\n", pspark->pkso_reqtype);
	}
#endif /* DIAGNOSTIC */

	return (error);
}
