/* $NetBSD: wsevent.c,v 1.16 2003/08/07 16:31:29 agc Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)event.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Internal "wscons_event" queue interface for the keyboard and mouse drivers.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/poll.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wseventvar.h>

/*
 * Initialize a wscons_event queue.
 */
void
wsevent_init(ev)
	struct wseventvar *ev;
{
	if (ev->q != NULL) {
#ifdef DIAGNOSTIC
		printf("wsevent_init: already init\n");
#endif
		return;
	}
	ev->get = ev->put = 0;
	ev->q = malloc((u_long)WSEVENT_QSIZE * sizeof(struct wscons_event), M_DEVBUF, M_WAITOK);
}

/*
 * Tear down a wscons_event queue.
 */
void
wsevent_fini(ev)
	struct wseventvar *ev;
{
	if (ev->q == NULL) {
#ifdef DIAGNOSTIC
		printf("wsevent_fini: already fini\n");
#endif
		return;
	}
	free(ev->q, M_DEVBUF);
	ev->q = NULL;
}

/*
 * User-level interface: read, poll.
 * (User cannot write an event queue.)
 */
int
wsevent_read(ev, uio, flags)
	register struct wseventvar *ev;
	struct uio *uio;
	int flags;
{
	int s, n, cnt, error;

	/*
	 * Make sure we can return at least 1.
	 */
	if (uio->uio_resid < sizeof(struct wscons_event))
		return (EMSGSIZE); /* ??? */
	s = splwsevent();
	while (ev->get == ev->put) {
		if (flags & IO_NDELAY) {
			splx(s);
			return (EWOULDBLOCK);
		}
		ev->wanted = 1;
		error = tsleep((caddr_t) ev, PWSEVENT | PCATCH, "wsevent_read", 0);
		if (error) {
			splx(s);
			return (error);
		}
	}
	/*
	 * Move wscons_event from tail end of queue (there is at least one
	 * there).
	 */
	if (ev->put < ev->get) {
		cnt = WSEVENT_QSIZE - ev->get; /* events in [get..QSIZE) */
	} else {
		cnt = ev->put - ev->get; /* events in [get..put) */
	}
	splx(s);
	n = howmany(uio->uio_resid, sizeof(struct wscons_event));
	if (cnt > n) {
		cnt = n;
	}
	error = uiomove(&ev->q[ev->get], cnt * sizeof(struct wscons_event), uio);
	n -= cnt;

	/*
	 * If we do not wrap to 0, used up all our space, or had an error,
	 * stop.  Otherwise move from front of queue to put index, if there
	 * is anything there to move.
	 */
	if ((ev->get = (ev->get + cnt) % WSEVENT_QSIZE) != 0 || n == 0 || error
			|| (cnt = ev->put) == 0) {
		return (error);
	}
	if (cnt > n) {
		cnt = n;
	}
	error = uiomove(&ev->q[0], cnt * sizeof(struct wscons_event), uio);
	ev->get = cnt;
	return (error);
}

int
wsevent_poll(ev, events, p)
	register struct wseventvar *ev;
	int events;
	struct proc *p;
{
	int revents = 0;
	int s = splwsevent();

	if(ev->revoked) {
		return (POLLHUP);
	}

	if (events & (POLLIN | POLLRDNORM)) {
		if (ev->get != ev->put)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			ev->selected = TRUE;
			selrecord(p, &ev->sel);
	}

	splx(s);
	return (revents);
}

static void
filt_wseventrdetach(kn)
	struct knote *kn;
{
	struct wseventvar *ev = kn->kn_hook;
	int s;

	s = splwsevent();
	SIMPLEQ_REMOVE(&ev->sel.si_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_wseventread(kn, hint)
	struct knote *kn;
	long hint;
{
	struct wseventvar *ev = kn->kn_hook;

	if (ev->get == ev->put)
		return (0);

	if (ev->get < ev->put)
		kn->kn_data = ev->put - ev->get;
	else
		kn->kn_data = (WSEVENT_QSIZE - ev->get) + ev->put;

	kn->kn_data *= sizeof(struct wscons_event);

	return (1);
}

static const struct filterops wsevent_filtops =
	{ 1, NULL, filt_wseventrdetach, filt_wseventread };

int
wsevent_kqfilter(ev, kn)
	struct wseventvar *ev;
	struct knote *kn;
{
	struct klist *klist;
	int s;

	if(ev->revoked) {
		return (ENODEV);
	}

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &ev->sel.si_klist;
		kn->kn_fop = &wsevent_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = ev;

	s = splwsevent();
	SIMPLEQ_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

/*
 * Wakes up all listener of the 'ev' queue.
 */
void
wsevent_wakeup(struct wseventvar *ev)
{
	selnotify(&ev->sel, 0);
	if (ev->wanted) {
		ev->wanted = 0;
		wakeup((caddr_t)ev);
	}
	if (ev->async) {
		psignal(ev->io, SIGIO);
	}
}

#define EVARRAY(ev, idx) (&(ev)->q[(idx)])

int
wsevent_inject(ev, events, nevents)
	struct wseventvar *ev;
	struct wscons_event *events;
    size_t nevents;
{
	size_t avail, i;
	struct timeval tv;
	struct timespec ts;

	avail = wsevent_avail(ev);

	/* Fail if there is all events will not fit in the queue. */
	if (avail < nevents) {
		return (ENOSPC);
	}

	/* Use the current time for all events. */
	microtime(&tv);
	TIMEVAL_TO_TIMESPEC(&tv, &ts);

	/* Inject the events. */
	for (i = 0; i < nevents; i++) {
		struct wscons_event *we;

		we = EVARRAY(ev, ev->put);
		we->type = events[i].type;
		we->value = events[i].value;
		we->time = ts;

		ev = wsevent_put(ev, 1);
	}
	wsevent_wakeup(ev);
	return (0);
}

/* Calculate number of free slots in the queue. */
size_t
wsevent_avail(ev)
	struct wseventvar *ev;
{
	size_t avail;
	if (WSEVENT_EMPTYQ(ev)) {
		avail = ev->get - ev->put;
	} else {
		avail = WSEVENT_QSIZE - (ev->put - ev->get);
	}
	KASSERT(avail <= WSEVENT_QSIZE);
	return (avail);
}

struct wseventvar *
wsevent_put(ev, size)
	struct wseventvar *ev;
	size_t size;
{
	ev->put = (ev->put + size) % WSEVENT_QSIZE;
	return (ev);
}

struct wseventvar *
wsevent_get(ev, cnt)
	struct wseventvar *ev;
	size_t cnt;
{
	ev->get = (ev->get + cnt) % WSEVENT_QSIZE;
	return (ev);
}
