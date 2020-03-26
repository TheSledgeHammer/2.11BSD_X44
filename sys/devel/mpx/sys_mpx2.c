/*
 * sys_mpx2.c
 *
 *  Created on: 19 Dec 2019
 *      Author: marti
 *
 * Equivalent to mx1.c
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "mpx/mx.h"

struct	mpx_chan	chans[NCHANS];
struct	mpx_schan	schans[NPORTS];
struct  mpx_groups 	*groups[NGROUPS];
int	mpxline;
struct mpx_chan *xcp();
dev_t mpxdev = -1;

struct mpx_chan *
challoc(index, isport)
{
	register struct mpx_chan *cp;
	register int s, i;
	for(i = 0; i < ((isport) ? NPORTS : NCHANS); i++) {
		cp = (isport)? (struct chan *)(schans+i): chans+i;
		if(cp->c_group == NULL) {
			cp->c_index = index;
			cp->c_pgrp = 0;
			cp->c_flags = 0;
			splx(s);
			return(cp);
		}
	}
	splx(s);
	return(NULL);
}

int
gpalloc()
{
	register struct mpx_chan *cp;
	register i;
	for (i = NGROUPS - 1; i >= 0; i--) {
		if(groups[i] == NULL) {
			groups[i]++;
			return (i);
		}
	}
	u->u_error = ENXIO;
	return (i);
}


struct chan
*addch(vp, isport)
	struct vnode *vp;
	int isport;
{
	register struct mpx_chan *cp;
	register struct mpx_group *gp;
	register i;

	gp = &vp->v_un.vu_group;
	for(i=0;i < NINDEX; i++) {
		cp = (struct chan *)gp->g_chans[i];
		if (cp == NULL) {
			if ((cp=challoc(i, isport)) != NULL) {
				cp->cy.c_chan[i] = cp;
				cp->c_group = gp;
			}
			break;
		}
		cp = NULL;
	}
	vrele(vp); /* XXX */
	return (cp);
}

/* Not Finished(WIP) */
mpxchan() {
    extern mxopen(), mcread(), sdata(), scontrol();
    struct vnode *vp, *gvp;
    struct vattr *vattr;
    struct tty *tp;
    struct file *fp, *chfp, *gfp;
    struct mpx_chan *cp;
    struct mpx_group *gp, *ngp;
    struct mx_args vec;
    struct a {
        int cmd;
        int *argvec;
    } *uap;
    dev_t dev;
    register int i;

    /*
     * Common setup code.
     */

    uap = (struct a *) u->u_ap;
    (void) copyin((caddr_t) uap->argvec, (caddr_t) & vec, sizeof vec);
    gp = NULL;
    gfp = NULL;
    cp = NULL;

    switch (uap->cmd) {

        case NPGRP:
            if (vec->m_arg[1] < 0) {

            }
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
            //gvp = gfp
            gp = &gvp->v_un.vu_group;
            if (gp->g_vnode != gvp) {
                u->u_error = ENXIO;
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
                                mpxdev = (dev_t)(i << 8);
                            }
                        }
                    }
                }
                if (mpxdev < 0) {
                    u->u_error = ENXIO;
                    return;
                }
            }
            if (uap->cmd == MPXN) {
                if ((vp = ialloc(pipedev)) == NULL)
                    return;
                vattr->va_mode = ((vec.m_arg[1] & 0777) + IFMPC) & ~u->u_cmask;
                vattr->va_flags = IACC | IUPD | ICHG;

            } else {
                u.u_dirp = vec.m_name;
                vp = namei(uchar, 1);
                if (vp != NULL) {
                    i = vp->i_mode & IFMT;
                    u->u_error = EEXIST;
                    if (i == IFMPC || i == IFMPB) {
                        i = minor(vattr->va_rdev);
                        gp = groups[i];
                        if (gp && gp->g_vnode == vp)
                            u->u_error = EBUSY;
                    }
                    vput(vp);
                    return;
                }
                if (u->u_error)
                    return;
                vp = MAKEIMODE((vec.m_arg[1] & 0777) + IFMPC);
                if (vp == NULL)
                    return;
            }
            if ((i = gpalloc()) < 0) {
                vput(vp);
                return;
            }
            if ((fp = falloc()) == NULL) {
                vput(vp);
                groups[i] = NULL;
                return;
            }
            vattr->va_rdev = (daddr_t)(mpxdev + i);

            //inode count
            vrele(vp);

            gp = &vp->v_un->vu_group;
            groups[i] = gp;
            gp->g_state = INUSE|ISGRP;
            gp->g_group = NULL;
            gp->g_file = fp;
            gp->g_index = 0;
            gp->g_rotmask = 1;
            gp->g_rot = 0;
            gp->g_datq = 0;
            for(i=0;i<NINDEX;)
            	gp->g_chans[i++] = NULL;
            fp->f_flag = FREAD|FWRITE|FMP;
            fp->f_un.f_Chan = NULL;
            return;
    }
}

void
detach(cp)
register struct mpx_chan *cp;
{
	register struct mpx_group *master,*sub;
	register index;

	if (cp->c_flags&ISGRP) {
		sub = (struct group *)cp;
		master = sub->g_group;	index = sub->g_index;
		closef(sub->g_file);
		master->g_chans[index] = NULL;
		return;
	} else if ((cp->c_flags&PORT) && cp->c_ttyp != NULL) {
		closef(cp->c_fy);
		chdrain(cp);
		chfree(cp);
		return;
	}
	if (cp->c_fy && (cp->c_flags&WCLOSE)==0) {
		cp->c_flags |= WCLOSE;
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
	if(fp == NULL) {
		u->u_error = ENXIO;
		return(-1);
	}
	i = ufalloc();
	if (i < 0)
		return(i);
	u->u_ofile[i] = fp;
	fp->f_count++;
	u->u_r->r_val1 = i;
	return(i);
}

int
mtree(sub, master)
register struct mpx_group *sub, *master;
{
	register i;
	int mtresiz, stresiz;

	if ((mtresiz=mup(master,sub)) == NULL) {
		u->u_error = ENXIO;
		return(NULL);
	}
	if ((stresiz=mdown(sub,master)) <= 0) {
		u->u_error = ENXIO;
		return(NULL);
	}
	if (sub->g_group != NULL) {
		u->u_error = ENXIO;
		return(NULL);
	}
	if (stresiz+mtresiz > NLEVELS) {
		u->u_error = ENXIO;
		return(NULL);
	}
	for (i=0;i<NINDEX;i++) {
		if (master->g_chans[i] != NULL)
			continue;
		master->g_chans[i] = (struct mpx_chan *)sub;
		sub->g_group = master;
		sub->g_index = i;
		return(1);
	}
	u->u_error = ENXIO;
	return(NULL);
}

int
mup(master, sub)
struct mpx_group *master, *sub;
{
	register struct mpx_group *top;
	register int depth;

	depth = 1;  top = master;
	while (top->g_group) {
		depth++;
		top = top->g_group;
	}
	if(top == sub)
		return(NULL);
	return(depth);
}

int
mdown(sub, master)
struct mpx_group *sub, *master;
{
	register int maxdepth, i, depth;

	if(sub == (struct mpx_group *)NULL || (sub->g_state&ISGRP) == 0)
		return(0);
	if(sub == master)
		return(-1);
	maxdepth = 0;
	for(i=0; i<NINDEX; i++) {
		if((depth=mdown(sub->g_chans[i],master)) == -1)
			return(-1);
		maxdepth = (depth>maxdepth) ? depth: maxdepth;
	}
	return(maxdepth+1);
}
