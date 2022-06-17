/*
 * mpx1.c
 *
 *  Created on: 17 June 2022
 *      Author: marti
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
#include <sys/null.h>

#include <vm/include/vm_extern.h>

#include <devel/mpx/mpx.h>
#include <devel/sys/malloctypes.h>

int
mpx()
{
	/*
	register struct mpx_args {
		syscallarg(int *) fdp;
	} *uap = (struct mpx_args *) u.u_ap;
	*/
	struct file *rf, *wf;
	struct mpx *rmpx, *wmpx;
	struct mpxpair *mm;
	int fd, error;

	mm = mpx_paircreate();
	if(mm == NULL) {
		return (ENOMEM);
	}
	rmpx = &mm->mpp_rmpx;
	wmpx = &mm->mpp_wmpx;

	rf = falloc();
	if (rf != NULL) {
		u.u_r.r_val1 = fd;
		rf->f_flag = FREAD;
		rf->f_type = DTYPE_PIPE;
		rf->f_mpx = rmpx;
		rf->f_ops = &mpxops;
		error = ufdalloc(rf);
		if(error != 0) {
			goto free2;
		}
	} else {
		u.u_error = ENFILE;
		goto free2;
	}
	wf = falloc();
	if (wf != NULL) {
		u.u_r.r_val2 = fd;
		wf->f_flag = FWRITE;
		wf->f_type = DTYPE_PIPE;
		wf->f_mpx = wmpx;
		wf->f_ops = &mpxops;
		error = ufdalloc(wf);
		if(error != 0) {
			goto free3;
		}
	} else {
		u.u_error = ENFILE;
		goto free3;
	}

	rmpx->mpx_peer = wmpx;
	wmpx->mpx_peer = rmpx;

	FILE_UNUSE(rf, u.u_procp);
	FILE_UNUSE(wf, u.u_procp);
	return (0);

free3:
	FILE_UNUSE(rf, u.u_procp);
	ffree(rf);
	fdremove(u.u_r.r_val1);

free2:
	mpxclose(NULL, rmpx);
	mpxclose(NULL, wmpx);
	return (u.u_error);
}
