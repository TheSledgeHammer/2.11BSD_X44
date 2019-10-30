
/* sys_pipe using vnodes...? */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/fs.h>
#include <sys/mount.h>
#include <sys/uio.h>

extern int vn_ioctl();
int	pipe_rw(), pipe_select(), pipe_close();
struct	fileops	pipeops = { pipe_rw, vn_ioctl, pipe_select, pipe_close };

/* vnode attributes
 * struct vattr
 */


void
pipe()
{
   	register struct vnode *vp;
	register struct file *rf, *wf;
	static struct mount *mp;
	struct vnode vtmp;
	int r;

	if (!mp || !mp->mnt_vnodecovered || mp->m_dev != pipedev) {
		for (mp = &mount[0];;++mp) {
			if (mp == &mount[NMOUNT]) {
				mp = &mount[0];		/* use root */
				break;
			}
			if (mp->mnt_vnodecovered == NULL || mp->m_dev != pipedev)
				continue;
			break;
			if (mp->m_filsys.fs_ronly) {
				u->u_error = EROFS;
				return;
			}
		}
	}
}

int
pipe_rw(fp, uio, flag)
	register struct file *fp;
	register struct uio *uio;
	int flag;
{

	if (uio->uio_rw == UIO_READ)
		return (readp(fp, uio, flag));
	return (writep(fp, uio, flag));
}

int
readp(fp, uio, flag)
	register struct file *fp;
	register struct	uio *uio;
	int flag;
{
    register struct vnode *vp;
	int error;

loop:
	VOP_LOCK(vp);

	VOP_UNLOCK(vp);
	return (error);
}

int
writep(fp, uio, flag)
	struct file *fp;
	register struct	uio *uio;
	int flag;
{
    register struct vnode *vp;
    register int c;
	int error = 0;

	c = uio->uio_resid;
	VOP_LOCK(vp);
}

int
pipe_select(fp, which)
	struct file *fp;
	int which;
{
    register struct vnode *vp = (struct vnode *)fp->f_data;
	register struct proc *p;
	register int retval = 0;
	extern int selwait;


	VOP_LOCK(vp);


	VOP_UNLOCK(vp);
	return(retval);
}

int
pipe_close(fp)
	struct	file *fp;
{
    register struct vnode *vp = (struct vnode *)fp->f_data;

    VOP_LOCK(vp);
#ifdef	DIAGNOSTIC
	if	((vp->v_flag & VPIPE) == 0)
		panic("pipe_close !VPIPE");
#endif

    vput(vp);
    return(0);
}
