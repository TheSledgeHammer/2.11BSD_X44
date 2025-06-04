/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_generic.c	1.8 (2.11BSD) 2000/2/28
 */

#include <sys/cdefs.h>
#include <sys/param.h>

#include <sys/user.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/poll.h>
#include <sys/event.h>
#include <sys/ktrace.h>
#include <sys/malloc.h>

#include <machine/setjmp.h>
	
static int rwuio(struct uio *, struct iovec *, int);
static int rwuiov(struct uio *, struct iovec *, unsigned int);

/*
 * Read system call.
 */
int
read()
{
	register struct read_args {
		syscallarg(int)	fdes;
		syscallarg(char	*) cbuf;
		syscallarg(unsigned) count;
	} *uap = (struct read_args *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = (caddr_t)SCARG(uap, cbuf);
	aiov.iov_len = SCARG(uap, count);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = u.u_procp;

	return (rwuio(&auio, &aiov, SCARG(uap, fdes)));
}

int
readv()
{
	register struct readv_args {
		syscallarg(int) fdes;
		syscallarg(const struct iovec *) iovp;
		syscallarg(unsigned) iovcnt;
	} *uap = (struct readv_args *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */
	
#ifdef KTRACE
	struct iovec *ktriov;
	u_int iovlen;
	
	if (aiov == NULL) {
		ktriov = NULL;
	}
	
	/*
	 * if tracing, save a copy of iovec
	 */
    iovlen = SCARG(uap, iovcnt) * sizeof(struct iovec);
    if (aiov == NULL) {
		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
		bcopy((caddr_t) auio.uio_iov, (caddr_t) ktriov, iovlen);
	} else {
		if (KTRPOINT(u.u_procp, KTR_GENIO)) {
			ktriov = aiov;
		}
	}
#endif

	if (SCARG(uap, iovcnt) > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return (EINVAL);
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = SCARG(uap, iovcnt);
	auio.uio_rw = UIO_READ;
	auio.uio_procp = u.u_procp;
	u.u_error = copyin((caddr_t)SCARG(uap, iovp), (caddr_t)aiov, SCARG(uap, iovcnt) * sizeof (struct iovec));
	if (u.u_error)
		return (u.u_error);
	return (rwuio(&auio, NULL, SCARG(uap, fdes)));
}

/*
 * Positional read system call.
 */
int
pread()
{
	register struct pread_args {
		syscallarg(int) fdes;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} *uap = (struct pread_args *) u.u_ap;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, nbyte);
	if (aiov.iov_len > SSIZE_MAX) {
		return (EINVAL);
	}

	auio.uio_iov = &iov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = iov.iov_len;
	auio.uio_offset = SCARG(uap, offset);

	return (rwuio(&auio, &aiov, SCARG(uap, fdes)));
}

int
preadv()
{
	register struct preadv_args {
		syscallarg(int) fdes;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(off_t) offset;
	} *uap = (struct preadv_args *) u.u_ap;
	struct uio auio;
	struct iovec aiov[16];

	u.u_error = rwuiov(&auio, &aiov, SCARG(uap, iovcnt));
	if (u.u_error == EINVAL) {
		return (EINVAL);
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = SCARG(uap, iovcnt);
	auio.uio_rw = UIO_READ;
	auio.uio_offset = SCARG(uap, offset);
	u.u_error = copyin((caddr_t)SCARG(uap, iovp), (caddr_t)aiov, SCARG(uap, iovcnt) * sizeof (struct iovec));
	if (u.u_error)
		return (u.u_error);
	return (rwuio(&auio, NULL, SCARG(uap, fdes)));
}

/*
 * Write system call
 */
int
write()
{
	register struct write_args {
		syscallarg(int) fdes;
		syscallarg(char	*) cbuf;
		syscallarg(unsigned) count;
	} *uap = (struct write_args *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_WRITE;
	aiov.iov_base = SCARG(uap, cbuf);
	aiov.iov_len = SCARG(uap, count);

	return (rwuio(&auio, &aiov, SCARG(uap, fdes)));
}

int
writev()
{
	register struct writev_args {
		syscallarg(int)	fdes;
		syscallarg(const struct iovec *) iovp;
		syscallarg(unsigned) iovcnt;
	} *uap = (struct writev_args *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */
	
#ifdef KTRACE
	struct iovec *ktriov;
	u_int iovlen;
	
	if (aiov == NULL) {
		ktriov = NULL;
	}
	
	/*
	 * if tracing, save a copy of iovec
	 */
    iovlen = SCARG(uap, iovcnt) * sizeof(struct iovec);
    if (aiov == NULL) {
		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
		bcopy((caddr_t) auio.uio_iov, (caddr_t) ktriov, iovlen);
	} else {
		if (KTRPOINT(u.u_procp, KTR_GENIO)) {
			ktriov = aiov;
		}
	}
#endif

	if (SCARG(uap, iovcnt) > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return (EINVAL);
	}

	auio.uio_iov = aiov;
	auio.uio_iovcnt = SCARG(uap, iovcnt);
	auio.uio_rw = UIO_WRITE;
	u.u_error = copyin((caddr_t)SCARG(uap, iovp), (caddr_t)aiov, SCARG(uap, iovcnt) * sizeof (struct iovec));
	if (u.u_error)
		return (u.u_error);
	return (rwuio(&auio, NULL, SCARG(uap, fdes)));
}

/*
 * Positional write system call.
 */
int
pwrite()
{
	register struct pwrite_args {
		syscallarg(int) fdes;
		syscallarg(const void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} *uap = (struct pwrite_args *) u.u_ap;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, nbyte);
	if (aiov.iov_len > SSIZE_MAX) {
		return (EINVAL);
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = aiov.iov_len;
	auio.uio_offset = SCARG(uap, offset);

	return (rwuio(&auio, &aiov, SCARG(uap, fdes)));
}

int
pwritev()
{
	register struct pwritev_args {
		syscallarg(int) fdes;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(off_t) offset;
	} *uap = (struct pwritev_args *) u.u_ap;
	struct uio auio;
	struct iovec aiov[16];

	u.u_error = rwuiov(&auio, &aiov, SCARG(uap, iovcnt));
	if (u.u_error == EINVAL) {
		return (EINVAL);
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = SCARG(uap, iovcnt);
	auio.uio_rw = UIO_WRITE;
	auio.uio_offset = SCARG(uap, offset);
	u.u_error = copyin((caddr_t)SCARG(uap, iovp), (caddr_t)aiov, SCARG(uap, iovcnt) * sizeof (struct iovec));
	if (u.u_error)
		return (u.u_error);
	return (rwuio(&auio, NULL, SCARG(uap, fdes)));
}

static int
rwuio(uio, aiov, fdes)
	register struct uio *uio;
	register struct iovec *aiov;
	register int fdes;
{
	register struct file *fp;
	register struct iovec *iov;
	u_int i, count;
	off_t	total;
	
#ifdef KTRACE
    struct iovec *ktriov;
    
    if (aiov == NULL) {
		ktriov = NULL;
	}
#endif

	GETF(fp, fdes);
	if ((fp->f_flag & (uio->uio_rw == UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return (EBADF);
	}
	total = (off_t) 0;
	uio->uio_resid = 0;
	uio->uio_segflg = UIO_USERSPACE;
	for (iov = uio->uio_iov, i = 0; i < uio->uio_iovcnt; i++, iov++)
		total += iov->iov_len;

	uio->uio_resid = total;
	if (uio->uio_resid != total) /* check wraparound */
		u.u_error = EINVAL;
	return (EINVAL);

	count = uio->uio_resid;
	if (setjmp(&u.u_qsave)) {
		/*
		 * The ONLY way we can get here is via the longjump in sleep.  Thus signals
		 * have been checked and u_error set accordingly.  If no bytes have been
		 * transferred then all that needs to be done now is 'return'; the system
		 * call will either be restarted or reported as interrupted.  If bytes have
		 * been transferred then we need to calculate the number of bytes transferred.
		 */
		if (uio->uio_resid == count) {
			return (u.u_error);
		} else {
			u.u_error = 0;
		}
	} else {
		u.u_error = (*fp->f_ops->fo_rw)(fp, uio, u.u_ucred);
	}
#ifdef KTRACE
	if (aiov == NULL) {
		if (ktriov != NULL) {
			if (u.u_error == 0) {
				ktrgenio(u.u_procp, fdes, UIO_READ, ktriov, count, u.u_error);
			}
			FREE(ktriov, M_TEMP);
		}
	} else {
		if (KTRPOINT(u.u_procp, KTR_GENIO) && u.u_error == 0) {
			ktrgenio(u.u_procp, fdes, UIO_READ, ktriov, count, u.u_error);
		}
	}
#endif
	u.u_r.r_val1 = count - uio->uio_resid;
	return (u.u_error);
}

static int
rwuiov(uio, aiov, iovcnt)
	register struct uio *uio;
	register struct iovec *aiov;
	register unsigned int iovcnt;
{
#ifdef KTRACE
	struct iovec *ktriov;
	u_int iovlen;

	if (aiov == NULL) {
		ktriov = NULL;
	}
	/*
	 * if tracing, save a copy of iovec
	 */
	iovlen = iovcnt * sizeof(struct iovec);
	if (aiov == NULL) {
		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
		bcopy((caddr_t) uio->uio_iov, (caddr_t) ktriov, iovlen);
	} else {
		if (KTRPOINT(u.u_procp, KTR_GENIO)) {
			ktriov = aiov;
		}
	}
#endif
	if (iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return (EINVAL);
	}
	return (u.u_error);
}

/*
 * Ioctl system call
 */
int
ioctl()
{
	register struct ioctl_args {
		syscallarg(int)	fdes;		/* fd */
		syscallarg(long) cmd; 		/* com */
		syscallarg(caddr_t)	cmarg; 	/* data */
	} *uap = (struct ioctl_args *)u.u_ap;
	register struct file *fp;
	long com;
	register u_int size;
	caddr_t memp, data;
#define STK_PARAMS	128
	char stkbuf[STK_PARAMS];

	if ((fp = getf(SCARG(uap, fdes))) == NULL)
		return (EBADF);
	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		u.u_error = EBADF;
		return (EBADF);
	}

	switch (com = SCARG(uap, cmd)) {
	case FIONCLEX:
		u.u_pofile[SCARG(uap, fdes)] |= UF_EXCLOSE;
		return (0);
	case FIOCLEX:
		u.u_pofile[SCARG(uap, fdes)] &= ~UF_EXCLOSE;
		return (0);
	}

	/*
	 * Interpret high order word to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
	size = IOCPARM_LEN(com);
	if (size > IOCPARM_MAX) {
		u.u_error = EFAULT | ENOTTY;
		return (u.u_error);
	}
	memp = NULL;
	if (size > sizeof (stkbuf)) {
		memp = (caddr_t)malloc((u_long)size, M_IOCTLOPS, M_WAITOK);
		data = memp;
	} else {
		data = stkbuf;
	}
	if (com & IOC_IN) {
		if (size) {
			if (((u_int) SCARG(uap, cmarg) | size) & 1)
				u.u_error = copyin(SCARG(uap, cmarg), (caddr_t) data, size);
			else
				u.u_error = copyin(SCARG(uap, cmarg), (caddr_t) data, size);
			if (u.u_error)
				if(memp) {
					free(memp, M_IOCTLOPS);
				}
				return (u.u_error);
		} else
			*(caddr_t*) data = SCARG(uap, cmarg);
	} else if ((com & IOC_OUT) && size)
		/*
		 * Zero the buffer on the stack so the user
		 * always gets back something deterministic.
		 */
		bzero((caddr_t) data, size);
	else if (com & IOC_VOID)
		*(caddr_t*) data = SCARG(uap, cmarg);

	switch (com) {

	case FIONBIO:
		u.u_error = fset(fp, FNONBLOCK, *(int*) data);
		break;

	case FIOASYNC:
		u.u_error = fset(fp, FASYNC, *(int*) data);
		break;

	case FIOSETOWN:
		u.u_error = fsetown(fp, *(int*)data);
		break;

	case FIOGETOWN:
		u.u_error = fgetown(fp, (int*)data);
		break;

	default:
		u.u_error = (*fp->f_ops->fo_ioctl)(fp, com, data, u.u_procp);
		/*
		 * Copy any data to user, size was
		 * already set and checked above.
		 */
		if (u.u_error == 0 && (com & IOC_OUT) && size)
			if (((u_int) SCARG(uap, cmarg) | size) & 1)
				u.u_error = copyout(data, SCARG(uap, cmarg), size);
		break;
	}
	if (memp)
		free(memp, M_IOCTLOPS);
	return (u.u_error);
}

int	nselcoll;

struct pselect_args {
	syscallarg(int)					nd;
	syscallarg(fd_set *) 			in;
	syscallarg(fd_set *) 			ou;
	syscallarg(fd_set *) 			ex;
	syscallarg(struct timespec *) 	ts;
	syscallarg(sigset_t *) 			maskp;
};

static int select1(struct pselect_args *, int);
int	selwait, nselcoll;

/*
 * Select system call.
*/
int
select()
{
	register struct select_args {
		syscallarg(int)		nd;
		syscallarg(fd_set *) in;
		syscallarg(fd_set *) ou;
		syscallarg(fd_set *) ex;
		syscallarg(struct timeval *) tv;
	} *uap = (struct select_args *)u.u_ap;

	register struct pselect_args *pselargs = (struct pselect_args *)uap;

	/*
	 * Fake the 6th parameter of pselect.  See the comment below about the
	 * number of parameters!
	*/
	SCARG(pselargs, maskp) = 0;
	u.u_error = select1(pselargs, 0);
	return (select1(pselargs, 0));
}

/*
 * pselect (posix select)
 *
 * N.B.  There is only room for 6 arguments - see user.h - so pselect() is
 *       at the maximum!  See user.h
*/
int
pselect()
{
	register struct pselect_args *uap;
	uap = (struct pselect_args *)u.u_ap;

	u.u_error = select1(uap, 1);
	return (select1(uap, 1));
}

/*
 * Select helper function common to both select() and pselect()
 */

static int
select1(uap, is_pselect)
	register struct pselect_args *uap;
	int	is_pselect;
{
	fd_set ibits[3], obits[3];
	struct timeval atv;
	struct timeval time;
	sigset_t sigmsk;
	unsigned int timo = 0;
	register int error, ni;
	int ncoll, s;

	bzero((caddr_t)ibits, sizeof(ibits));
	bzero((caddr_t)obits, sizeof(obits));

	if (SCARG(uap, nd) > NOFILE)
		SCARG(uap, nd) = NOFILE; /* forgiving, if slightly wrong */
	ni = howmany(SCARG(uap, nd), NFDBITS);

#define	getbits(name, x) 												\
	if (SCARG(uap, name)) { 											\
		error = copyin((caddr_t)SCARG(uap, name), (caddr_t)&ibits[x], 	\
		    (unsigned)(ni * sizeof(fd_mask))); 							\
		if (error) 														\
			goto done; 													\
	}
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);

#undef	getbits

	if (SCARG(uap, maskp)) {
		error = copyin(SCARG(uap, maskp), &sigmsk, sizeof(sigmsk));
		sigmsk &= ~sigcantmask;
		if (error)
			goto done;
	}
	if (SCARG(uap, ts)) {
		error = copyin((caddr_t) SCARG(uap, ts), (caddr_t) &atv, sizeof(atv));
		if (error)
			goto done;
		/*
		 * nanoseconds ('struct timespec') on a PDP-11 are stupid since a 50 or 60 hz
		 * clock is all we have.   Keeping the names and logic made porting easier
		 * though.
		 */
		if (is_pselect) {
			struct timespec *ts = (struct timespec*) &atv;

			if (ts->tv_sec == 0 && ts->tv_nsec < 1000)
				atv.tv_usec = 1;
			else
				atv.tv_usec = ts->tv_nsec / 1000;
		}
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splhigh();
		time.tv_usec = lbolt * mshz;
		timevaladd(&atv, &time);
		splx(s);
	}
retry:
	ncoll = nselcoll;
	u.u_procp->p_flag |= P_SELECT;
	error = selscan(ibits, obits, SCARG(uap, nd), &u.u_r.r_val1);
	if (error || u.u_r.r_val1)
		goto done;
	s = splhigh();
	if (SCARG(uap, ts)) {
		/* this should be timercmp(&time, &atv, >=) */
		if ((time.tv_sec > atv.tv_sec
				|| (time.tv_sec == atv.tv_sec && lbolt * mshz >= atv.tv_usec))) {
			splx(s);
			goto done;
		}
		timo = hzto(&atv);
		if (timo == 0)
			timo = 1;
	}
	if ((u.u_procp->p_flag & P_SELECT) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~P_SELECT;
		splx(s);
		goto retry;
	}
	u.u_procp->p_flag &= ~P_SELECT;
	/*
	 * If doing a pselect() need to set a temporary mask while in tsleep.
	 * Returning from pselect after catching a signal the old mask has to be
	 * restored.  Save it here and set the appropriate flag.
	 */
	if (SCARG(uap, maskp)) {
		u.u_oldmask = u.u_procp->p_sigmask;
		u.u_psflags |= SAS_OLDMASK;
		u.u_procp->p_sigmask = sigmsk;
	}
	error = tsleep(&selwait, PSOCK | PCATCH, "select1", timo);
	if (SCARG(uap, maskp))
		u.u_procp->p_sigmask = u.u_oldmask;
	splx(s);
	if (error == 0)
		goto retry;
done:
	u.u_procp->p_flag &= ~P_SELECT;
	/* select is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
#define	putbits(name, x) 														\
	if (SCARG(uap, name) &&														\
		(error2 = copyout(&obits[x], SCARG(uap, name),(ni*sizeof(fd_mask))))) 	\
			error = error2;

	if (error == 0) {
		int error2;

		putbits(in, 0);
		putbits(ou, 1);
		putbits(ex, 2);
#undef putbits
	}
	return (error);
}

int
selscan(ibits, obits, nfd, retval)
	fd_set *ibits, *obits;
	int nfd, *retval;
{
	register int i, j, flag;
	fd_mask bits;
	struct file *fp;
	int	which, n = 0;

	for (which = 0; which < 3; which++) {
		switch (which) {

		case 0:
			flag = FREAD;
			break;

		case 1:
			flag = FWRITE;
			break;

		case 2:
			flag = 0;
			break;
		}
		for (i = 0; i < nfd; i += NFDBITS) {
			bits = ibits[which].fds_bits[i / NFDBITS];
			while ((j = ffs(bits)) && i + --j < nfd) {
				bits &= ~(1L << j);
				fp = u.u_ofile[i + j];
				if (fp == NULL) {
					return (EBADF);
				}
				if ((*fp->f_ops->fo_poll)(fp, flag, u.u_procp)) {
					FD_SET(i + j, &obits[which]);
					n++;
				}
			}
		}
	}
	*retval = n;
	return (0);
}

int
poll()
{
	register struct poll_args {
		syscallarg(struct pollfd *)	fds;
		syscallarg(u_int) nfds;
		syscallarg(int) timeout;
	} *uap = (struct poll_args *)u.u_ap;
	struct timeval atv;
	struct timeval time;
	sigset_t sigmsk;
	caddr_t	bits;
	char smallbits[NFDSBITS];
	unsigned int timo = 0;
	register int error, ni;
	int ncoll, s;

	ni = howmany(SCARG(uap, nfds), sizeof(struct pollfd));
	bits = smallbits;

	error = copyin(SCARG(uap, fds), bits, ni);
    if (error) {
        goto done;
    }

    if (SCARG(uap, timeout) != INFTIM) {
		struct timespec *ts = (struct timespec*) &atv;

		if (ts->tv_sec == 0 && ts->tv_nsec < 1000) {
			atv.tv_usec = 1;
		} else {
			atv.tv_usec = ts->tv_nsec / 1000;
		}

		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splhigh();
		time.tv_usec = lbolt * mshz;
		timevaladd(&atv, &time);
		splx(s);
	}
retry:
	ncoll = nselcoll;
	u.u_procp->p_flag |= P_SELECT;
    error = pollscan((struct pollfd *)bits, SCARG(uap, nfds), &u.u_r.r_val1);
	if (error || u.u_r.r_val1)
		goto done;
	s = splhigh();
	if (SCARG(uap, timeout) != INFTIM) {
		/* this should be timercmp(&time, &atv, >=) */
		if ((time.tv_sec > atv.tv_sec
				|| (time.tv_sec == atv.tv_sec && lbolt * mshz >= atv.tv_usec))) {
			splx(s);
			goto done;
		}
		timo = hzto(&atv);
		if (timo == 0)
			timo = 1;
	}
	if ((u.u_procp->p_flag & P_SELECT) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~P_SELECT;
		splx(s);
		goto retry;
	}
	u.u_procp->p_flag &= ~P_SELECT;
	error = tsleep(&selwait, PSOCK | PCATCH, "poll", timo);
	splx(s);
	if (error == 0)
		goto retry;
done:
	u.u_procp->p_flag &= ~P_SELECT;
	/* poll is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
	if (error == 0) {
		error = copyout(bits, SCARG(uap, fds), ni);
		if (error == 0) {
			return (error);
		}
	}
	return (error);
}

int
pollscan(fds, nfd, retval)
	struct pollfd *fds;
	u_int nfd;
	int *retval;
{
	int i, n;
	struct filedesc	*fdp;
	struct file	*fp;

	fdp = u.u_fd;
	for (i = 0; i < nfd; i++, fds++) {
		if (fds->fd >= fdp->fd_nfiles) {
			fds->revents = POLLNVAL;
			n++;
		} else if (fds->fd < 0) {
			fds->revents = 0;
		} else {
			fp = fd_getfile(fds->fd);
			if (fp == NULL) {
				fds->revents = POLLNVAL;
				n++;
			} else {
				FILE_USE(fp);
				fds->revents = (*fp->f_ops->fo_poll)(fp, fds->events | POLLERR | POLLHUP, u.u_procp);
				if (fds->revents != 0) {
					n++;
				}
				FILE_UNUSE(fp, u.u_procp);
			}
		}
	}
	*retval = n;
	return (0);
}

/*
 * Record a select request.
 */
void
selrecord(selector, sip)
	struct proc *selector;
	struct selinfo *sip;
{
	struct proc *p;
	pid_t mypid;

	mypid = selector->p_pid;
	if (sip->si_pid == mypid)
		return;
	if (sip->si_pid && (p = pfind(sip->si_pid)) &&
	    (p->p_wchan == (caddr_t)&selwait))
		sip->si_flags |= SI_COLL;
	else
		sip->si_pid = mypid;
}

/*ARGSUSED*/
int
seltrue(dev, flag)
	dev_t dev;
	int flag;
{
	return (flag & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

/*
 * Do a wakeup when a selectable event occurs.
 */
void
selwakeup(p, coll)
	register struct proc *p;
	long coll;
{
	if (coll) {
		nselcoll++;
		wakeup((caddr_t)&selwait);
	}
	if (p) {
		register int s = splhigh();
		if (p->p_wchan == (caddr_t)&selwait) {
			if (p->p_stat == SSLEEP)
				setrun(p);
			else
				unsleep(p);
		} else if (p->p_flag & P_SELECT)
			p->p_flag &= ~P_SELECT;
		splx(s);
	}
}

void
_selwakeup(sip)
	register struct selinfo *sip;
{
	register struct proc *p;

	if (sip->si_pid == 0) {
		return;
	}
	if (sip->si_flags & SI_COLL) {
		sip->si_flags &= ~SI_COLL;
	}
	p = pfind(sip->si_pid);
	sip->si_pid = 0;

	selwakeup(p, sip->si_flags);
}

void
selnotify(sip, knhint)
	struct selinfo *sip;
	long knhint;
{
	if (sip->si_pid != 0) {
		_selwakeup(sip);
	}
	KNOTE(&sip->si_klist, knhint);
}
