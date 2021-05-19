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

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <devel/sys/malloctypes.h>
#include <devel/mpx/mpx.h>

struct fileops mpxops = {
		.fo_read = mpx_read,
		.fo_write = mpx_write,
		.fo_ioctl = mpx_ioctl,
		.fo_poll = mpx_poll,
		.fo_close = mpx_close
};

struct	mpx_chan	chans[NCHANS];
struct	mpx_schan	schans[NPORTS];
struct	mpx_group	*groups[NGROUPS];
int					mpxline;
dev_t 				mpxdev;

char				mcdebugs[NDEBUGS];

#define	MIN(a,b)	((a<b)?a:b)
short	cmask[16]	={
	01,	02,	04,
	010,	020,	040,
	0100,	0200,	0400,
	01000,	02000,	04000,
	010000,	020000,	040000, 0100000
};

#define	HIQ			100
#define	LOQ			20
#define	FP			((struct file *)cp)

int
mpx_read(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
    vn_read(fp, uio, cred);
    return (0);
}

int
mpx_write(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
    struct mpx *mpx = (struct mpx *)fp->f_data;

    vn_write(fp, uio, cred);
    return (0);
}

int
mpx_ioctl(fp, cmd, data, p)
	struct file *fp;
	u_long cmd;
	register caddr_t data;
	struct proc *p;
{
	struct mpx *mpx = fp->f_data;
	MPX_LOCK(mpx);

	switch (cmd) {
	case FIONBIO:
		break;
	case FIOASYNC:
	case FIONREAD:
		if (!(fp->f_flag & FREAD)) {
			*(int *)data = 0;
			MPX_UNLOCK(mpx);
			return (0);
		}
		break;
	case FIOSETOWN:
		MPX_UNLOCK(mpx);
		error = fsetown(&mpx->mpx_pgid, (*(int *)data));
		goto out_unlocked;

	case FIOGETOWN:
		return fgetown(mpx->mpx_pgid, data);
	}

	MPX_UNLOCK(mpx);

out_unlocked:
	return (error);
}

int
mpx_poll(fp, event, p)
	struct file *fp;
	int event;
	struct proc *p;
{
	return (0);
}

int
mpx_close(fp, p)
	struct file *fp;
	struct proc *p;
{
	return (0);
}

void
mpx_init()
{
	struct mpx *mpx;
	MALLOC(mpx, struct mpx *, sizeof(struct mpx *), M_MPX, M_WAITOK);
	LIST_INIT(mpx->mpx_head);
}


struct mpx_group *
mpx_get_group(dev)
	dev_t dev;
{
	register dev_t d;
	d = minor(dev);
	if (d >= NGROUPS) {
		return (ENXIO);
	}
	return (groups[d]);
}

struct mpx_chan *
mpx_create_channel(index, isport)
	int index;
	int isport;
{
	register int i;
	register struct mpx_chan *cp;
	for(i = 0; i < ((isport)?NPORTS:NCHANS); i++) {
		cp = (isport)?(struct mpx_chan *)schans[i] : chans[i];
		if(cp->mpc_group == NULL) {
			cp->mpc_index = index;
			cp->mpc_flags = 0;
			return (cp);
		}
	}
	return (NULL);
}

struct mpx_group *
mpx_create_group()
{
	struct mpx_group *gp;
	int i;
	for(i = 0; i < NGROUPS; i++) {
		gp = &groups[i];
		return (gp);
	}
	return (NULL);
}

int
mpx_grouptable()
{
	register int i;
	int error;

	for (i = (NGROUPS-1); i >= 0; i--) {
		if (groups[i] == NULL) {
			groups[i]++;
			return (i);
		}
	}
	return (ENXIO);
}

struct mpx_chan *
mpx_add_channel(vp, isport)
	struct vnode *vp;
	int isport;
{
	register struct mpx_chan *cp;
	register struct mpx_group *gp;
	int i;

	vn_lock(vp);
	for(i = 0; i < 16; i++) {
		cp = (struct mpx_chan *)gp->mpg_chans[i];
		if(cp == NULL) {
			cp = mpx_create_channel(i, isport);
			if(cp != NULL) {
				gp->mpg_chans[i] = cp;
				cp->mpc_group = gp;
			}
			break;
		}
		cp = NULL;
	}

	vrele(vp);
	return (cp);
}

void
mpxchan()
{
	struct	vnode		*vp, *gvp;
	struct	vattr		*vap;
	struct	tty			*tp;
	struct	file		*fp, *chfp, *gfp;
	struct	mpx_chan	*cp;
	struct	mpx_group	*gp, *ngp;
	struct	mpx_args	vec;

	struct	a {
		int	cmd;
		int	*argvec;
	} *uap = (struct a *)u->u_ap;
	dev_t	dev;
	register int i;

	/*
	 * Common setup code.
	 */
	copyin((caddr_t) uap->argvec, (caddr_t) &vec, sizeof vec);
	gp = NULL;
	gfp = NULL;
	cp = NULL;
	vap = (struct vattr *)vp->v_data;

	switch (uap->cmd) {

	case NPGRP:
		if (vec->m_arg[1] < 0)
			break;
		break;
	case CHAN:
	case JOIN:
	case EXTR:
	case ATTACH:
	case DETACH:
	case CSIG:
		gfp = getf(vec.m_arg[1]);
		if (gfp == NULL)
			return;
		gvp = gfp->f_data;
		gp = &gvp->v_mpxgroup;
		if (gp->mpg_vnode != gip) {
			u.u_error = ENXIO;
			return;
		}
	}

	switch (uap->cmd) {

	/*
	 * Create an MPX file.
	 */

	case MPX:
	case MPXN:
		if (mpxdev < 0) {
			for (i = 0; linesw[i].l_open; i++) {
				if (linesw[i].l_read == mcread) {
					mpxline = i;
					for (i = 0; cdevsw[i].d_open; i++) {
						if (cdevsw[i].d_open == mxopen) {
							mpxdev = (dev_t) (i << 8);
						}
					}
				}
			}
			if (mpxdev < 0) {
				u.u_error = ENXIO;
				return;
			}
		}
		if (uap->cmd == MPXN) {
			if ((vp = valloc(pipedev)) == NULL)
				return;
			vap->va_mode = ((vec.m_arg[1] & 0777) + IFMPC) & ~u.u_cmask;
			vp->v_flag = IACC | IUPD | ICHG;
		} else {
			u->u_dirp = vec.m_name;
			vp = namei(uchar, 1);
			if (vp != NULL) {
				i = vap->va_mode & IFMT;
				u.u_error = EEXIST;
				if (i == IFMPC || i == IFMPB) {
					i = minor(ip->i_un.i_rdev);
					gp = groups[i];
					if (gp && gp->mpg_vnode == vp)
						u.u_error = EBUSY;
				}
				vput(vp);
				return;
			}
			if (u.u_error)
				return;
			vp = maknode((vec.m_arg[1] & 0777) + IFMPC);
			if (vp == NULL)
				return;
		}
		if ((i = gpalloc()) < 0) {
			vput(vp);
			return;
		}
		if ((fp = falloc()) == NULL) {
			iput(vp);
			groups[i] = NULL;
			return;
		}
		ip->i_un.i_rdev = (daddr_t) (mpxdev + i);
		vp->vu_count++;
		vrele(vp);

		gp = &vp->v_mpxgroup;
		groups[i] = gp;
		gp->mpg_vnode = vp;
		gp->mpg_state = INUSE | ISGRP;
		gp->mpg_group = NULL;
		gp->mpg_file = fp;
		gp->mpg_index = 0;
		gp->mpg_rotmask = 1;
		gp->mpg_rot = 0;
		gp->mpg_datq = 0;
		for (i = 0; i < NINDEX;)
			gp->mpg_chans[i++] = NULL;

		fp->f_flag = FREAD | FWRITE | FMP;
		fp->f_vnode = vp;
		fp->f_un.f_chan = NULL;
		return;

		/*
		 * join file descriptor (arg 0) to group (arg 1)
		 * return channel number
		 */

	case JOIN:
		if ((fp = getf(vec.m_arg[0])) == NULL)
			return;
		ip = fp->f_inode;
		switch (ip->i_mode & IFMT) {

		case IFMPC:
			if ((fp->f_flag & FMP) != FMP) {
				u.u_error = ENXIO;
				return;
			}
			ngp = &ip->i_un.i_group;
			if (mtree(ngp, gp) == NULL)
				return;
			fp->f_count++;
			u.u_r.r_val1 = cpx(ngp);
			return;

		case IFCHR:
			dev = (dev_t) ip->i_un.i_rdev;
			tp = cdevsw[major(dev)].d_ttys;
			if (tp == NULL) {
				u.u_error = ENXIO;
				return;
			}
			tp = &tp[minor(dev)];
			if (tp->t_chan) {
				u.u_error = ENXIO;
				return;
			}
			if ((cp = mpx_add_channel(gvp, 1)) == NULL) {
				u.u_error = ENXIO;
				return;
			}
			tp->t_chan = cp;
			cp->mpc_fy = fp;
			fp->f_count++;
			cp->mpc_ttyp = tp;
			cp->mpc_line = tp->t_line;
			cp->mpc_flags = XGRP + PORT;
			u.u_r.r_val1 = cpx(cp);
			return;

		default:
			u.u_error = ENXIO;
			return;

		}

		/*
		 * Attach channel (arg 0) to group (arg 1).
		 */
	case ATTACH:
		cp = xcp(gp, vec.m_arg[0]);
		if (cp == NULL || (cp->mpc_flags & ISGRP)) {
			u.u_error = ENXIO;
			return;
		}
		u.u_r.r_val1 = cpx(cp);
		wakeup((caddr_t) cp);
		return;

	case DETACH:
		cp = xcp(gp, vec.m_arg[0]);
		if (cp == NULL) {
			u.u_error = ENXIO;
			return;
		}
		mpx_detach(cp);
		return;

		/*
		 * Extract channel (arg 0) from group (arg 1).
		 */

	case EXTR:
		cp = xcp(gp, vec.m_arg[0]);
		if (cp == NULL) {
			u.u_error = ENXIO;
			return;
		}
		if (cp->mpc_flags & ISGRP) {
			mxfalloc(((struct mpx_group*) cp)->mpg_file);
			return;
		}
		if ((fp = cp->mpc_fy) != NULL) {
			mxfalloc( fp);
			return;
		}
		if ((fp = falloc()) == NULL)
			return;
		fp->f_inode = gip;
		gip->i_count++;
		fp->f_un.f_chan = cp;
		fp->f_flag = (vec.m_arg[2]) ? (FMPY) : (FMPX);
		cp->c_fy = fp;
		return;

		/*
		 * Make new chan on group (arg 1).
		 */

	case CHAN:
		if ((gfp->f_flag & FMP) == FMP)
			cp = mpx_add_channel(gvp, 0);
		if (cp == NULL) {
			u.u_error = ENXIO;
			return;
		}
		cp->mpc_flags = XGRP;
		cp->mpc_file = NULL;
		cp->c_ttyp = cp->c_ottyp = (struct tty*) cp;
		cp->c_line = cp->c_oline = mpxline;
		u.u_r.r_val1 = cpx(cp);
		return;

		/*
		 * Connect fd (arg 0) to channel fd (arg 1).
		 * (arg 2 <  0) => fd to chan only
		 * (arg 2 >  0) => chan to fd only
		 * (arg 2 == 0) => both directions
		 */

	case CONNECT:
		if ((fp = getf(vec.m_arg[0])) == NULL)
			return;
		if ((chfp = getf(vec.m_arg[1])) == NULL)
			return;
		vp = fp->f_vnode;
		i = vp->v_mode & IFMT;
		if (i != IFCHR) {
			u.u_error = ENXIO;
			return;
		}
		dev = (dev_t) ip->i_un.i_rdev;
		tp = cdevsw[major(dev)].d_ttys;
		if (tp == NULL) {
			u.u_error = ENXIO;
			return;
		}
		tp = &tp[minor(dev)];
		if (!(chfp->f_flag & FMPY)) {
			u.u_error = ENXIO;
			return;
		}
		cp = chfp->f_un.f_chan;
		if (cp == NULL || (cp->mpc_flags & PORT)) {
			u.u_error = ENXIO;
			return;
		}
		i = vec.m_arg[2];
		if (i >= 0) {
			cp->c_ottyp = tp;
			cp->c_oline = tp->t_line;
		}
		if (i <= 0) {
			tp->t_chan = cp;
			cp->c_ttyp = tp;
			cp->c_line = tp->t_line;
		}
		u.u_r.r_val1 = 0;
		return;

	case NPGRP: {
		register struct proc *pp;

		if (gp != NULL) {
			cp = xcp(gp, vec.m_arg[0]);
			if (cp == NULL) {
				u.u_error = ENXIO;
				return;
			}
		}
		pp = u.u_procp;
		pp->p_pgrp = pp->p_pid;
		if (vec.m_arg[2])
			pp->p_pgrp = vec.m_arg[2];
		if (gp != NULL)
			cp->mpc_pgrp = pp->p_pgrp;
		u.u_r.r_val1 = pp->p_pgrp;
		return;
	}

	case CSIG:
		cp = xcp(gp, vec.m_arg[0]);
		if (cp == NULL) {
			u.u_error = ENXIO;
			return;
		}
		signal(cp->c_pgrp, vec.m_arg[2]);
		u.u_r.r_val1 = vec.m_arg[2];
		return;

	case DEBUG:
		i = vec.m_arg[0];
		if (i < 0 || i > NDEBUGS)
			return;
		mcdebugs[i] = vec.m_arg[1];
		if (i == ALL)
			for (i = 0; i < NDEBUGS; i++)
				mcdebugs[i] = vec.m_arg[1];
		return;

	default:
		u.u_error = ENXIO;
		return;
	}
}

void
mpx_detach(cp)
	register struct mpx_chan *cp;
{
	register struct mpx_group *master,*sub;
	register int index;

	if (cp->mpc_flags & ISGRP) {
		sub = (struct group*) cp;
		master = sub->mpg_group;
		index = sub->mpg_index;
		closef(sub->mpg_file);
		master->mpg_chans[index] = NULL;
		return;
	} else if ((cp->mpc_flags & PORT) && cp->mpc_ttyp != NULL) {
		struct mpx_group *cpg = mpx_get_chan_group(cp);
		closef(cpg->mpg_file);
		chdrain(cp);
		chfree(cp);
		return;
	}
	if (cp->mpc_file && (cp->mpc_flags & WCLOSE) == 0) {
		cp->mpc_flags |= WCLOSE;
		chwake(cp);
	} else {
		chdrain(cp);
		chfree(cp);
	}
}

int
mxfalloc(fp)
	register struct file *fp;
{
	register int i;

	if (fp == NULL) {
		u.u_error = ENXIO;
		return (-1);
	}
	i = ufalloc();
	if (i < 0)
		return (i);
	u.u_ofile[i] = fp;
	fp->f_count++;
	u.u_r.r_val1 = i;
	return (i);
}

/*
 * Grow a branch on a tree.
 */
void *
mtree(sub,master)
	register struct mpx_group *sub, *master;
{
	register int i;
	int mtresiz, stresiz;

	if ((mtresiz = mup(master, sub)) == NULL) {
		u.u_error = ENXIO;
		return (NULL);
	}
	if ((stresiz = mdown(sub, master)) <= 0) {
		u.u_error = ENXIO;
		return (NULL);
	}
	if (sub->mpg_group != NULL) {
		u.u_error = ENXIO;
		return (NULL);
	}
	if (stresiz + mtresiz > NLEVELS) {
		u.u_error = ENXIO;
		return (NULL);
	}
	for (i = 0; i < NINDEX; i++) {
		if (master->mpg_chans[i] != NULL)
			continue;
		master->mpg_chans[i] = (struct mpx_chan*) sub;
		sub->mpg_group = master;
		sub->mpg_index = i;
		return (1);
	}
	u.u_error = ENXIO;
	return (NULL);
}

int
mup(master, sub)
	struct mpx_group *master, *sub;
{
	register struct mpx_group *top;
	register int depth;

	depth = 1;
	top = master;
	while (top->mpg_group) {
		depth++;
		top = top->mpg_group;
	}
	if (top == sub)
		return (NULL);
	return (depth);
}

int
mdown(sub, master)
	struct mpx_group *sub, *master;
{
	register int maxdepth, i, depth;

	if (sub == (struct mpx_group*) NULL || (sub->mpg_state & ISGRP) == 0)
		return (0);
	if (sub == master)
		return (-1);
	maxdepth = 0;
	for (i = 0; i < NINDEX; i++) {
		if ((depth = mdown(sub->mpg_chans[i], master)) == -1)
			return (-1);
		maxdepth = (depth > maxdepth) ? depth : maxdepth;
	}
	return (maxdepth + 1);
}

/*
 * Mcread and mcwrite move data on an mpx file.
 * Transfer addr and length is controlled by mxread/mxwrite.
 * Kernel-to-Kernel and other special transfers are not
 * yet in.
 */
int
mcread(cp)
	register struct mpx_chan *cp;
{
	register struct clist *q;
	register char *np;

	q = (cp->mpc_ctlx.c_cc) ? &cp->mpc_ctlx : &cp->cx.datq;
	mxmove( q, B_READ);

	if ((cp->mpc_flags & NMBUF) && q == &cp->mpc_ctlx) {
		np = mxnmbuf;
		while (nmsize--)
		passc(*np++);
		cp->mpc_flags &= ~NMBUF;
		rele(mpxip);
	}
	if (cp->mpc_flags & PORT)
		return (cp->mpc_ctlx.c_cc + cp->c_ttyp->t_rawq.c_cc);
	else
		return (cp->mpc_ctlx.c_cc + cp->cx.datq.c_cc);

}

caddr_t
mcwrite(cp)
register struct mpx_chan *cp;
{
	register struct clist *q;
	int s;

	q = &cp->cy.datq;
	while (u.u_count) {
		s = spl6();
		if (q->c_cc > HIQ || (cp->mpc_flags & EOTMARK)) {
			cp->mpc_flags |= SIGBLK;
			splx(s);
			break;
		}
		splx(s);
		mxmove(q, B_WRITE);
	}
	wakeup((caddr_t) q);
	return ((caddr_t) q);
}

/*
 * Msread and mswrite move bytes
 * between user and non-multiplexed channel.
 */
void
msread(fmp, cp)
	register struct mpx_chan *cp;
{
	register struct clist *q;
	int s;

	q = (fmp & FMPX) ? &cp->cx.datq : &cp->cy.datq;
	s = spl6();
	while (q->c_cc == 0) {
		if (cp->mpc_flags & EOTMARK) {
			cp->mpc_flags &= ~EOTMARK;
			if (msgenab(cp))
				scontrol(cp, M_UBLK, 0);
			else {
				wakeup((caddr_t) cp);
				wakeup((caddr_t) q);
			}
			goto out;
		}
		if (cp->mpc_flags & WCLOSE) {
			u.u_error = ENXIO;
			goto out;
		}
		sleep((caddr_t) q, TTIPRI);
	}
	splx(s);
	while (mxmove(q, B_READ) > 0)
		;
	mxrstrt(cp, q, SIGBLK);
	return;
	out: splx(s);
}

void
mswrite(fmp, cp)
	register struct mpx_chan *cp;
{
	register struct clist *q;
	register int cc;

	q = (fmp & FMPX) ? &cp->cy.datq : &cp->cx.datq;
	while (u.u_count) {
		spl6();
		if (cp->mpc_flags & WCLOSE) {
			signal(cp->mpc_pgrp, SIGPIPE);
			spl0();
			return;
		}
		if (q->c_cc >= HIQ || (cp->mpc_flags & FBLOCK)) {
			if (cp->mpc_flags & WCLOSE) {
				signal(cp->mpc_pgrp, SIGPIPE);
				spl0();
				return;
			}
			sdata( cp);
			cp->c_flags |= BLOCK;
			sleep((caddr_t) q + 1, TTOPRI);
			spl0();
			continue;
		}
		spl0();
		cc = mxmove(q, B_WRITE);
		if (cc < 0)
			break;
	}
	if (fmp & FMPX) {
		if (cp->mpc_flags & YGRP)
			sdata( cp);
		else
			wakeup((caddr_t) q);
	} else {
		if (cp->mpc_flags & XGRP)
			sdata( cp);
		else
			wakeup((caddr_t) q);
	}
}

/*
 * move chars between clist and user space.
 */
int
mxmove(q, dir)
	register struct clist *q;
	register dir;
{
	register int cc;
	char cbuf[HIQ];

	cc = MIN(u.u_count, sizeof cbuf);
	if (dir == B_READ)
		cc = q_to_b(q, cbuf, cc);
	if (cc <= 0)
		return (cc);
	IOMOVE((caddr_t) cbuf, (unsigned) cc, dir);
	if (dir == B_WRITE)
		cc = b_to_q(cbuf, cc, q);
	return (cc);
}

void
mxrstrt(cp, q, b)
	register struct mpx_chan *cp;
	register struct clist *q;
	register b;
{
	int s;

	s = spl6();
	if ((cp->mpc_flags & b) && q->c_cc < LOQ) {
		cp->mpc_flags &= ~b;
		if (b & ALT)
			wakeup((caddr_t) q + 1);
		else
			mcstart(cp, (caddr_t) q);
	}
	if (cp->mpc_flags & WFLUSH)
		wakeup((caddr_t) q + 2);
	splx(s);
}

/*
 * called from driver start or xint routines
 * to wakeup output sleeper.
 */
void
mcstart(cp, q)
	register struct mpx_chan *cp;
	register caddr_t q;
{

	if (cp->mpc_flags & (BLKMSG)) {
		cp->mpc_flags &= ~BLKMSG;
		scontrol(cp, M_UBLK, 0);
	} else
		wakeup((caddr_t) q);
}

void
mxwcontrol(cp)
	register struct mpx_chan *cp;
{
	short cmd;
	struct ttiocb vec;
	int s;

	IOMOVE((caddr_t) &cmd, sizeof cmd, B_WRITE);
	if (u.u_error)
		return;
	switch (cmd) {
	/*
	 * not ready to queue this up yet.
	 */
	case M_EOT:
		s = spl6();
		while (cp->mpc_flags & EOTMARK)
			if (msgenab(cp)) {
				scontrol(cp, M_BLK, 0);
				goto out;
			} else
				sleep((caddr_t) cp, TTOPRI);
		cp->mpc_flags |= EOTMARK;
		out: wakeup((caddr_t) &cp->cy.datq);
		splx(s);
		break;
	case M_IOCTL:
		break;
	case M_IOANS:
		if (cp->mpc_flags & SIOCTL) {
			IOMOVE((caddr_t) &vec, sizeof vec, B_WRITE);
			b_to_q((caddr_t) &vec, sizeof vec, &cp->mpc_ctly);
			cp->mpc_flags &= ~SIOCTL;
			wakeup((caddr_t) cp);
		}
		break;
	case M_BLK:
		cp->mpc_flags |= FBLOCK;
		break;
	case M_UBLK:
		cp->mpc_flags &= ~FBLOCK;
		chwake(cp);
		break;
	default:
		u.u_error = ENXIO;
	}
}

/*ARGSUSED*/
void
mxioctl(dev, cmd, addr, flag)
	dev_t 	dev;
	caddr_t addr;
{
	struct mpx_group *gp;
	int fmp;
	struct file *fp;
	struct {
		short 			c_ctl;
		short 			c_cmd;
		struct ttiocb 	c_vec;
	} ctlbuf;

	if ((gp = getmpx(dev)) == NULL || (fp = getf(u.u_arg[0])) == NULL) {
		return;
	}

	fmp = fp->f_flag & FMP;
	if (fmp == FMP) {
		switch (cmd) {

		case MXLSTN:
			if (mpxip == NULL) {
				mpxip = gp->mpg_vnode;
			} else {
				u.u_error = ENXIO;
				return;
			}
			break;

		case MXNBLK:
			gp->mpg_state |= ENAMSG;
			break;

		default:
			u.u_error = ENXIO;
			return;
		}
	} else {
		ctlbuf.c_ctl = M_IOCTL;
		ctlbuf.c_cmd = cmd;
		copyin(addr, (caddr_t) &ctlbuf.c_vec, sizeof(struct ttiocb));
		sioctl(fp->f_un.f_chan, (char*) &ctlbuf, sizeof ctlbuf);
		copyout((caddr_t) &ctlbuf, addr, sizeof(struct ttiocb));
	}
}

void
chdrain(cp)
	register struct mpx_chan *cp;
{
	register struct tty *tp;
	int wflag;

	chwake(cp);

	wflag = (cp->mpc_flags&WCLOSE)==0;
	tp = cp->mpc_ttyp;
	if (tp == NULL)		/* prob not required */
		return;
	if ((cp->mpc_flags&PORT) && tp->t_chan == cp) {
		cp->mpc_ttyp = NULL;
		tp->t_chan = NULL;
		return;
	}
	if (wflag)
		wflush(cp,&cp->cx.datq); else
		flush(&cp->cx.datq);
	if (!(cp->mpc_flags&YGRP)) {
		flush(&cp->cy.datq);
	}
}

void
chwake(cp)
	register struct mpx_chan *cp;
{
	register char *p;

	wakeup((caddr_t)cp);
	flush(&cp->mpc_ctlx);
	p = (char *)&cp->cx.datq;
	wakeup((caddr_t)p); wakeup((caddr_t)++p); wakeup((caddr_t)++p);
	p = (char *)&cp->cy.datq;
	wakeup((caddr_t)p); wakeup((caddr_t)++p); wakeup((caddr_t)++p);
}

void
chfree(cp)
	register struct mpx_chan *cp;
{
	register struct mpx_group *gp;
	register i;

	gp = cp->mpc_group;
	if (gp==NULL)
		return;
	i = cp->mpc_index;
	if (cp == gp->mpg_chans[i]) {
		gp->mpg_chans[i] = NULL;
	}
	cp->mpc_group = NULL;
}

void
flush(q)
	register struct clist *q;
{
	while(q->c_cc) {
		getc(q);
	}
}

void
wflush(cp,q)
	register struct mpx_chan *cp;
	register struct clist *q;
{
	register s;

	s = spl6();
	while (q->c_cc) {
		if (cp->mpc_flags & WCLOSE) {
			flush(q);
			goto out;
		}
		cp->mpc_flags |= WFLUSH;
		cp->mpc_group = sdata(cp);
		sleep((caddr_t) q + 2, TTOPRI);
	}
out:
	cp->mpc_flags &= ~WFLUSH;
	splx(s);
}

void
scontrol(cp, event, value)
	register struct mpx_chan *cp;
	short event,value;
{
	register struct clist *q;
	int s;

	q = &cp->mpc_ctlx;
	s = spl6();
	if (sdata(cp) == NULL)
		return;
	putw(event, q);
	putw(value, q);
	splx(s);
}

void
sioctl(cp, vec, cc)
	register struct mpx_chan *cp;
	char *vec;
	int cc;
{
	register s;
	register struct clist *q;

	s = spl6();
	q = &cp->cx.datq;
	while (q->c_cc) {
		cp->mpc_flags |= BLOCK;
		if (sdata(cp) == NULL) {
			u.u_error = ENXIO;
			return;
		}
		sleep((caddr_t) q + 1, TTOPRI);
	}

	b_to_q(vec, cc, &cp->mpc_ctlx);
	cp->mpc_flags |= SIOCTL;
	while (cp->mpc_flags & SIOCTL) {
		if (cp->mpc_ctlx.c_cc)
			if (sdata(cp) == NULL) {
				u.u_error = ENXIO;
				return;
			}
		sleep((caddr_t) cp, TTOPRI);
	}

	q_to_b(&cp->mpc_ctly, vec, cp->mpc_ctly.c_cc);
	splx(s);
}

struct mpx_group *
sdata(gp)
	register struct mpx_group *gp;
{
	register struct mpx_group *ngp;
	register int s;

	ngp = gp->mpg_group;
	if (ngp == NULL || (ngp->mpg_state & ISGRP) == 0)
		return (NULL);

	s = spl6();
	do {
		ngp->mpg_datq |= cmask[gp->mpg_index];
		wakeup((caddr_t) &ngp->mpg_datq);
		gp = ngp;
	} while (ngp == ngp->mpg_group);
	splx(s);
	return ((int) gp);
}



struct mpx_chan *
xcp(gp, x)
	register struct mpx_group *gp;
	register short x;
{
	register int i;

	while (gp->mpg_group)
		gp = gp->mpg_group;
	for (i = 0; i < NLEVELS; i++) {
		if ((x & 017) >= NINDEX)
			break;
		if (gp == NULL || (gp->mpg_state & ISGRP) == 0)
			return ((struct mpx_chan*) NULL);
		gp = (struct mpx_group*) gp->mpg_chans[x & 017];
		x >>= 4;
	}
	return ((struct mpx_chan*) gp);
}

int
cpx(cp)
	register struct mpx_chan *cp;
{
	register x;
	register struct mpx_group *gp;

	x = (-1 << 4) + cp->mpc_index;
	gp = cp->mpc_group;
	while (gp->mpg_group) {
		x <<= 4;
		x |= gp->mpg_index;
		gp = gp->mpg_group;
	}
	return (x);
}

struct mpx_chan *
nextcp(gp)
	register struct mpx_group *gp;
{
	register struct mpx_group *lgp, *ngp;

	do {
		while ((gp->mpg_datq & cmask[gp->mpg_rot]) == 0) {
			gp->mpg_rot = (gp->mpg_rot + 1) % NINDEX;
		}
		lgp = gp;
		gp = (struct mpx_group*) gp->mpg_chans[gp->mpg_rot];
	} while (gp != NULL && (gp->mpg_state & ISGRP));

	lgp->mpg_datq &= ~cmask[lgp->mpg_rot];
	lgp->mpg_rot = (lgp->mpg_rot + 1) % NINDEX;

	while (ngp == lgp->mpg_group) {
		ngp->mpg_datq &= ~cmask[lgp->mpg_index];
		if (ngp->mpg_datq)
			break;
		lgp = ngp;
	}
	return ((struct mpx_chan*) gp);
}

int
msgenab(cp)
	register struct mpx_chan *cp;
{
	register struct mpx_group *gp;

	for (gp = cp->mpc_group; gp; gp = gp->mpg_group)
		if (gp->mpg_state & ENAMSG)
			return (1);
	return (0);
}
