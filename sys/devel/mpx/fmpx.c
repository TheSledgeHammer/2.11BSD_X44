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
 * File Mpx:
 * - Multiplexing file operations like read and write using mpx.
 * - Each operation has its own channel.
 * - Example of a modified pipe system call using mpx for file read and write
 */

#include <sys/cdefs.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/event.h>
#include <sys/poll.h>
#include <sys/null.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/mpx.h>


/* file mpx channels */
#define FMPX_NCHANS  	2 	/* number of channels */

#define FMPX_READ 		0 	/* read channel */
#define FMPX_WRITE 		1	/* write channel */

/* file mpx creation */
struct mpx *
fmpx_create(fp, idx, data)
	struct file *fp;
	int idx;
	void *data;
{
	int error;

	if (fp->f_mpx == NULL) {
		fp->f_mpx = mpx_allocate(FMPX_NCHANS);
	}
	error = mpx_create(fp->f_mpx, idx, data);
	if (error != 0) {
		mpx_deallocate(fp->f_mpx);
		return (NULL);
	}
	return (fp->f_mpx);
}

/* file mpx read */
int
fmpx_read(fp, data)
	struct file *fp;
	void *data;
{
	register struct mpx *rmpx;
	int error;

	rmpx = fmpx_create(fp, FMPX_READ, data);
	if (rmpx != NULL) {
		error = mpx_get(rmpx, FMPX_READ, data);
		if (error != 0) {
			return (error);
		}
		rmpx->mpx_file = fp;
	}
	return (0);
}

/* file mpx write */
int
fmpx_write(fp, data)
	struct file *fp;
	void *data;
{
	register struct mpx *wmpx;
	int error;

	wmpx = fmpx_create(fp, FMPX_WRITE, data);
	if (wmpx != NULL) {
		error = mpx_put(wmpx, FMPX_WRITE, data);
		if (error != 0) {
			return (error);
		}
		wmpx->mpx_file = fp;
	}
	return (0);
}

/* pipe read using mpx */
static void
pipe_read(rf, rso)
	struct file *rf;
	struct socket *rso;
{
	int error;

	error = fmpx_read(rf, rso);
	if (error != 0) {
		u.u_error = error;
		return;
	}
	rf->f_flag = FREAD;
	rf->f_type = DTYPE_SOCKET;
	rf->f_ops = &socketops;
	rf->f_data = (caddr_t)rso;
}

/* pipe write using mpx */
static void
pipe_write(wf, wso)
	struct file *wf;
	struct socket *wso;
{
	int error;

	error = fmpx_write(wf, wso);
	if (error != 0) {
		u.u_error = error;
		return;
	}
	wf->f_flag = FWRITE;
	wf->f_type = DTYPE_SOCKET;
	wf->f_ops = &socketops;
	wf->f_data = (caddr_t)wso;
}

int
mpx_pipe()
{
	void *uap;
	register_t *retval;
	register struct filedesc *fdp;
	struct file *rf, *wf;
	struct socket *rso, *wso;
	int fd, error;

	uap = u.u_ap;
	fdp = u.u_procp->p_fd;
	error = socreate(AF_UNIX, &rso, SOCK_STREAM, 0);
	if (error) {
		u.u_error = error;
		return (error);
	}
	error = socreate(AF_UNIX, &wso, SOCK_STREAM, 0);
	if (error) {
		goto free1;
	}
	rf = falloc();
	if (rf == NULL) {
		goto free2;
	}
	retval[0] = fd;
	u.u_r.r_val1 = retval[0];
	pipe_read(rf, rso);
	wf = falloc();
	if (wf == NULL) {
		goto free3;
	}
	pipe_write(wf, wso);
	retval[1] = fd;
	u.u_r.r_val2 = retval[1];
	error = unp_connect2(wso, rso);
	if (error) {
		goto free4;
	}
	return (0);

free4:
	ffree(wf);
	fdp->fd_ofiles[retval[1]] = 0;
	ufdsync(fdp);
free3:
	ffree(rf);
	fdp->fd_ofiles[retval[0]] = 0;
	ufdsync(fdp);
free2:
	(void)soclose(wso);
free1:
	(void)soclose(rso);

	u.u_error = error;
	return (error);
}
