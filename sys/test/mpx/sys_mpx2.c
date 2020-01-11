/*
 * sys_mpx2.c
 *
 *  Created on: 19 Dec 2019
 *      Author: marti
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

struct	chan	chans[NCHANS];
struct	schan	schans[NPORTS];
int	mpxline;
struct chan *xcp();
dev_t	mpxdev	= -1;

struct chan
*challoc(index, isport)
{
	register struct chan *cp;
	register int s, i;
	for(i = 0; i < ((isport) ? NPORTS : NCHANS); i++) {
		cp = (isport)? (struct chan *)(schans+i): chans+i;
		if(cp->c_groups[NGROUPS] == NULL) {
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
	register struct chan *cp;
	register i;
	for (i = NGROUPS - 1; i >= 0; i--) {
		if(cp->c_groups[i] == NULL) {
			cp->c_groups[i]++;
			return (i);
		}
	}
	u->u_error = ENXIO;
	return (i);
}


struct chan
*addch(vp, isport)
	struct vnode *vp;
{
	register struct chan *cp, gp;
	register i;

	for(i=0;i < NINDEX; i++) {
		cp = (struct chan *)cp->cy.c_chan[i];
		if (cp == NULL) {
			if ((cp=challoc(i, isport)) != NULL) {
				cp->cy.c_chan[i] = cp;
				cp->c_groups = cp->cy.c_chan;
			}
		}
	}
	vrele(vp);
	return (cp);
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

mpxchan() {
    extern mxopen(), mcread(), sdata(), scontrol();
    struct vnode *vp, *gvp;
    struct vattr *vattr;
    struct tty *tp;
    struct file *fp, *chfp, *gfp;
    struct chan *cp;
    struct group *gp, *ngp;
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
            gvp = gfp;
            gp = &gvp->i_un.i_group;

            if (gp->g_inode != gip) {
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
                if ((ip = ialloc(pipedev)) == NULL)
                    return;
                vattr->va_mode = ((vec.m_arg[1] & 0777) + IFMPC) & ~u->u_cmask;
                vattr->va_flags = IACC | IUPD | ICHG;
                //ip->i_mode = ((vec.m_arg[1] & 0777) + IFMPC) & ~u->u_cmask;
                //ip->i_flag = IACC | IUPD | ICHG;
            } else {
                u.u_dirp = vec.m_name;
                ip = namei(uchar, 1);
                if (ip != NULL) {
                    i = ip->i_mode & IFMT;
                    u->u_error = EEXIST;
                    if (i == IFMPC || i == IFMPB) {
                        i = minor(ip->i_un.i_rdev);
                        gp = groups[i];
                        if (gp && gp->g_inode == ip)
                            u->u_error = EBUSY;
                    }
                    vput(vp);
                    return;
                }
                if (u->u_error)
                    return;
                ip = maknode((vec.m_arg[1] & 0777) + IFMPC);
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
            //ip->i_un.i_rdev = (daddr_t)(mpxdev + i);
            ip->i_count++;
            vrele(vp);
    }
}
