/*	$OpenBSD: video.c,v 1.44 2020/05/16 10:47:22 mpi Exp $	*/

/*
 * Copyright (c) 2008 Robert Nagy <robert@openbsd.org>
 * Copyright (c) 2008 Marcus Glocker <mglocker@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>

#include "video.h"
//#if NVIDEO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/videoio.h>

#include <dev/video/generic/video_if.h>

#include <vm/include/vm_extern.h>

#include "ioconf.h"

#ifdef VIDEO_DEBUG
#define	DPRINTF(x)	do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

struct video_softc {
	struct device			dev;
	void					*hw_hdl;		/* hardware driver handle */
	struct device			*sc_dev;		/* hardware device struct */
	struct video_hw_if		*hw_if;			/* hardware interface */
	char			 		sc_dying;		/* device detached */
#define VIDEO_OPEN			0x01
	char					sc_open;

	int			 			sc_fsize;
	uint8_t					*sc_fbuffer;
	size_t				 	sc_fbufferlen;
	int			 			sc_vidmode;		/* access mode */
#define		VIDMODE_NONE	0
#define		VIDMODE_MMAP	1
#define		VIDMODE_READ	2
	int			 			sc_frames_ready;

	struct selinfo		 	sc_rsel;		/* read selector */
};

int			videoprobe(struct device *, struct cfdata *, void *);
void 		videoattach(struct device *, struct device *, void *);
int			videodetach(struct device *, int);
int			videoactivate(struct device *, int);
int			videoprint(void *, const char *);
void		video_intr(void *);

dev_type_open(videoopen);
dev_type_close(videoclose);
dev_type_read(videoread);
dev_type_write(videowrite);
dev_type_ioctl(videoioctl);
dev_type_poll(videopoll);
dev_type_mmap(videommap);

const struct cdevsw video_cdevsw = {
	.d_open = videoopen,
	.d_close = videoclose,
	.d_read = videoread,
	.d_write = videowrite,
	.d_ioctl = videoioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = videopoll,
	.d_mmap = videommap,
	.d_discard = nodiscard,
	.d_type = D_OTHER
};

CFOPS_DECL(video, videoprobe, videoattach, videodetach, videoactivate);
CFDRIVER_DECL(NULL, video, DV_DULL);
CFATTACH_DECL(video, &video_cd, &video_cops, sizeof(struct video_softc));

int
videoprobe(struct device *parent, struct cfdata *match, void *aux)
{
	return (1);
}

void
videoattach(struct device *parent, struct device *self, void *aux)
{
	struct video_softc *sc = (void *)self;
	struct video_attach_args *sa = aux;

	printf("\n");
	sc->hw_if = sa->hwif;
	sc->hw_hdl = sa->hdl;
	sc->sc_dev = parent;
	sc->sc_fbufferlen = 0;

	if (sc->hw_if->get_bufsize)
		sc->sc_fbufferlen = (sc->hw_if->get_bufsize)(sc->hw_hdl);
	if (sc->sc_fbufferlen == 0) {
		printf("video: could not request frame buffer size\n");
		return;
	}

	sc->sc_fbuffer = malloc(sc->sc_fbufferlen, M_DEVBUF, M_NOWAIT);
	if (sc->sc_fbuffer == NULL) {
		printf("video: could not allocate frame buffer\n");
		return;
	}
}

int
videoopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	int	unit;
	struct video_softc *sc;

	unit = VIDEOUNIT(dev);
	if (unit >= video_cd.cd_ndevs ||
	    (sc = video_cd.cd_devs[unit]) == NULL ||
	     sc->hw_if == NULL)
		return (ENXIO);

	if (sc->sc_open & VIDEO_OPEN)
		return (EBUSY);
	sc->sc_open |= VIDEO_OPEN;

	sc->sc_vidmode = VIDMODE_NONE;
	sc->sc_frames_ready = 0;

	if (sc->hw_if->open != NULL)
		return (sc->hw_if->open(sc->hw_hdl, flags, &sc->sc_fsize, sc->sc_fbuffer, video_intr, sc));
	else
		return (0);
}

int
videoclose(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct video_softc *sc;
	int r = 0;

	sc = video_cd.cd_devs[VIDEOUNIT(dev)];

	if (sc->hw_if->close != NULL)
		r = sc->hw_if->close(sc->hw_hdl);

	sc->sc_open &= ~VIDEO_OPEN;

	return (r);
}

int
videoread(dev_t dev, struct uio *uio, int ioflag)
{
	struct video_softc *sc;
	int unit, error;
	size_t size;

	unit = VIDEOUNIT(dev);
	if (unit >= video_cd.cd_ndevs ||
	    (sc = video_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if (sc->sc_dying)
		return (EIO);

	if (sc->sc_vidmode == VIDMODE_MMAP)
		return (EBUSY);

	/* start the stream if not already started */
	if (sc->sc_vidmode == VIDMODE_NONE && sc->hw_if->start_read) {
 		error = sc->hw_if->start_read(sc->hw_hdl);
 		if (error)
 			return (error);
		sc->sc_vidmode = VIDMODE_READ;
 	}
 
	DPRINTF(("resid=%zu\n", uio->uio_resid));

	if (sc->sc_frames_ready < 1) {
		/* block userland read until a frame is ready */
		error = tsleep_nsec(sc, PWAIT | PCATCH, "vid_rd", INFSLP);
		if (sc->sc_dying)
			error = EIO;
		if (error)
			return (error);
	}

	/* move no more than 1 frame to userland, as per specification */
	size = ulmin(uio->uio_resid, sc->sc_fsize);
	error = uiomove(sc->sc_fbuffer, size, uio);
	sc->sc_frames_ready--;
	if (error)
		return (error);

	DPRINTF(("uiomove successfully done (%zu bytes)\n", size));

	return (0);
}


int
videowrite(dev_t dev, struct uio *uio, int ioflag)
{
	return (ENXIO);
}

int
videoioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct video_softc *sc;
	int unit, error;

	unit = VIDEOUNIT(dev);
	if (unit >= video_cd.cd_ndevs ||
	    (sc = video_cd.cd_devs[unit]) == NULL || sc->hw_if == NULL)
		return (ENXIO);

	DPRINTF(("video_ioctl(%zu, '%c', %zu)\n",
	    IOCPARM_LEN(cmd), (int) IOCGROUP(cmd), cmd & 0xff));

	error = EOPNOTSUPP;
	switch (cmd) {
	case VIDIOC_QUERYCAP:
		if (sc->hw_if->querycap)
			error = (sc->hw_if->querycap)(sc->hw_hdl,
			    (struct v4l2_capability *)data);
		break;
	case VIDIOC_ENUM_FMT:
		if (sc->hw_if->enum_fmt)
			error = (sc->hw_if->enum_fmt)(sc->hw_hdl,
			    (struct v4l2_fmtdesc *)data);
		break;
	case VIDIOC_ENUM_FRAMESIZES:
		if (sc->hw_if->enum_fsizes)
			error = (sc->hw_if->enum_fsizes)(sc->hw_hdl,
			    (struct v4l2_frmsizeenum *)data);
		break;
	case VIDIOC_ENUM_FRAMEINTERVALS:
		if (sc->hw_if->enum_fivals)
			error = (sc->hw_if->enum_fivals)(sc->hw_hdl,
			    (struct v4l2_frmivalenum *)data);
		break;
	case VIDIOC_S_FMT:
		if (!(flags & FWRITE))
			return (EACCES);
		if (sc->hw_if->s_fmt)
			error = (sc->hw_if->s_fmt)(sc->hw_hdl,
			    (struct v4l2_format *)data);
		break;
	case VIDIOC_G_FMT:
		if (sc->hw_if->g_fmt)
			error = (sc->hw_if->g_fmt)(sc->hw_hdl,
			    (struct v4l2_format *)data);
		break;
	case VIDIOC_S_PARM:
		if (sc->hw_if->s_parm)
			error = (sc->hw_if->s_parm)(sc->hw_hdl,
			    (struct v4l2_streamparm *)data);
		break;
	case VIDIOC_G_PARM:
		if (sc->hw_if->g_parm)
			error = (sc->hw_if->g_parm)(sc->hw_hdl,
			    (struct v4l2_streamparm *)data);
		break;
	case VIDIOC_ENUMINPUT:
		if (sc->hw_if->enum_input)
			error = (sc->hw_if->enum_input)(sc->hw_hdl,
			    (struct v4l2_input *)data);
		break;
	case VIDIOC_S_INPUT:
		if (sc->hw_if->s_input)
			error = (sc->hw_if->s_input)(sc->hw_hdl,
			    (int)*data);
		break;
	case VIDIOC_G_INPUT:
		if (sc->hw_if->g_input)
			error = (sc->hw_if->g_input)(sc->hw_hdl,
			    (int *)data);
		break;
	case VIDIOC_REQBUFS:
		if (sc->hw_if->reqbufs)
			error = (sc->hw_if->reqbufs)(sc->hw_hdl,
			    (struct v4l2_requestbuffers *)data);
		break;
	case VIDIOC_QUERYBUF:
		if (sc->hw_if->querybuf)
			error = (sc->hw_if->querybuf)(sc->hw_hdl,
			    (struct v4l2_buffer *)data);
		break;
	case VIDIOC_QBUF:
		if (sc->hw_if->qbuf)
			error = (sc->hw_if->qbuf)(sc->hw_hdl,
			    (struct v4l2_buffer *)data);
		break;
	case VIDIOC_DQBUF:
		if (!sc->hw_if->dqbuf)
			break;
		/* should have called mmap() before now */
		if (sc->sc_vidmode != VIDMODE_MMAP) {
			error = EINVAL;
			break;
		}
		error = (sc->hw_if->dqbuf)(sc->hw_hdl,
		    (struct v4l2_buffer *)data);
		sc->sc_frames_ready--;
		break;
	case VIDIOC_STREAMON:
		if (sc->hw_if->streamon)
			error = (sc->hw_if->streamon)(sc->hw_hdl,
			    (int)*data);
		break;
	case VIDIOC_STREAMOFF:
		if (sc->hw_if->streamoff)
			error = (sc->hw_if->streamoff)(sc->hw_hdl,
			    (int)*data);
		break;
	case VIDIOC_TRY_FMT:
		if (sc->hw_if->try_fmt)
			error = (sc->hw_if->try_fmt)(sc->hw_hdl,
			    (struct v4l2_format *)data);
		break;
	case VIDIOC_QUERYCTRL:
		if (sc->hw_if->queryctrl)
			error = (sc->hw_if->queryctrl)(sc->hw_hdl,
			    (struct v4l2_queryctrl *)data);
		break;
	case VIDIOC_G_CTRL:
		if (sc->hw_if->g_ctrl)
			error = (sc->hw_if->g_ctrl)(sc->hw_hdl,
			    (struct v4l2_control *)data);
		break;
	case VIDIOC_S_CTRL:
		if (sc->hw_if->s_ctrl)
			error = (sc->hw_if->s_ctrl)(sc->hw_hdl,
			    (struct v4l2_control *)data);
		break;
	default:
		error = (ENOTTY);
	}

	return (error);
}

int
videopoll(dev_t dev, int events, struct proc *p)
{
	int unit = VIDEOUNIT(dev);
	struct video_softc *sc;
	int error, revents = 0;

	if (unit >= video_cd.cd_ndevs ||
	    (sc = video_cd.cd_devs[unit]) == NULL)
		return (POLLERR);

	if (sc->sc_dying)
		return (POLLERR);

	DPRINTF(("%s: events=0x%x\n", __func__, events));

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_frames_ready > 0)
			revents |= events & (POLLIN | POLLRDNORM);
	}
	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM)) {
			/*
			 * Start the stream in read() mode if not already
			 * started.  If the user wanted mmap() mode,
			 * he should have called mmap() before now.
			 */
			if (sc->sc_vidmode == VIDMODE_NONE &&
			    sc->hw_if->start_read) {
				error = sc->hw_if->start_read(sc->hw_hdl);
				if (error)
					return (POLLERR);
				sc->sc_vidmode = VIDMODE_READ;
			}
			selrecord(p, &sc->sc_rsel);
		}
	}

	DPRINTF(("%s: revents=0x%x\n", __func__, revents));

	return (revents);
}

caddr_t
videommap(dev_t dev, off_t off, int prot)
{
	struct video_softc *sc;
	int unit;
	caddr_t p;
	caddr_t pa;

	DPRINTF(("%s: off=%lld, prot=%d\n", __func__, off, prot));

	unit = VIDEOUNIT(dev);
	if (unit >= video_cd.cd_ndevs ||
	    (sc = video_cd.cd_devs[unit]) == NULL)
		return (-1);

	if (sc->sc_dying)
		return (-1);

	if (sc->hw_if->mappage == NULL)
		return (-1);

	p = sc->hw_if->mappage(sc->hw_hdl, off, prot);
	if (p == NULL)
		return (-1);
	if (pmap_extract(pmap_kernel(), (vm_offset_t)p, &pa) == FALSE)
		panic("videommap: invalid page");
	sc->sc_vidmode = VIDMODE_MMAP;

	return (pa);
}

int
video_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	return (cf->cf_driver == &video_cd);
}

/*
 * Called from hardware driver. This is where the MI video driver gets
 * probed/attached to the hardware driver
 */
struct device *
video_attach_mi(struct video_hw_if *rhwp, void *hdlp, struct device *dev)
{
	struct video_attach_args arg;

	arg.hwif = rhwp;
	arg.hdl = hdlp;
	return (config_found_sm(dev, &arg, videoprint, video_submatch));
}

void
video_intr(void *addr)
{
	struct video_softc *sc = (struct video_softc *)addr;

	DPRINTF(("video_intr sc=%p\n", sc));
	if (sc->sc_vidmode != VIDMODE_NONE)
		sc->sc_frames_ready++;
	else
		printf("%s: interrupt but no streams!\n", __func__);
	if (sc->sc_vidmode == VIDMODE_READ)
		wakeup(sc);
	selwakeup1(&sc->sc_rsel);
}

int
videoprint(void *aux, const char *pnp)
{
	if (pnp != NULL)
		printf("video at %s", pnp);
	return (UNCONF);
}

int
videodetach(struct device *self, int flags)
{
	struct video_softc *sc = (struct video_softc *)self;
	int s, maj, mn;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == videoopen)
			break;

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);

	free(sc->sc_fbuffer, M_DEVBUF, sc->sc_fbufferlen);

	return (0);
}

int
videoactivate(struct device *self, int act)
{
	struct video_softc *sc = (struct video_softc *)self;


	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		break;
	}

	return (0);
}

//#endif /* NVIDEO > 0 */
