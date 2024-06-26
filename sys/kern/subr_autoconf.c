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
 *	California, Lawrence Berkeley Laboratories.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)subr_autoconf.c	8.3 (Berkeley) 5/17/94
 *
 * from: $Header: subr_autoconf.c,v 1.12 93/02/01 19:31:48 torek Exp $ (LBL)
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/hint.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <lib/libkern/libkern.h>

struct devicelist alldevs = TAILQ_HEAD_INITIALIZER(alldevs);
struct deferred_config_head	deferred_config_queue = TAILQ_HEAD_INITIALIZER(deferred_config_queue);
struct deferred_config_head	interrupt_config_queue = TAILQ_HEAD_INITIALIZER(interrupt_config_queue);
struct evcntlist allevents = TAILQ_HEAD_INITIALIZER(allevents);

struct cfattachlist	allattachs = LIST_HEAD_INITIALIZER(allattachs);

static void config_process_deferred(struct deferred_config_head *, struct device *);

__volatile int config_pending;		/* semaphore for mountroot */

/*
 * Autoconfiguration subroutines.
 */
/*
 * ioconf.c exports exactly two names: cfdata and cfroots.  All system
 * devices and drivers are found via these tables.
 */
extern struct cfdata cfdata[];
extern short cfroots[];

#define	ROOT ((struct device *)NULL)

struct matchinfo {
	cfmatch_t 		fn;
	struct	device 	*parent;
	void			*aux;
	struct	cfdata 	*match;
	int				pri;
};

/*
 * Apply the matching function and choose the best.  This is used
 * a few times and we want to keep the code small.
 */
static void
mapply(m, cf)
	register struct matchinfo *m;
	register struct cfdata *cf;
{
	register struct cfattach *ca;
	register struct cfdriver *cd;
	register struct cfops *cops;
	register int pri;

	ca = cf->cf_attach;
	cd = cf->cf_driver;
	cops = ca->ca_ops;
	if (m->fn != NULL) {
		pri = (*m->fn)(m->parent, cf, m->aux);
	} else {
		if (cops->cops_match == NULL) {
			panic("mapply: no match function for '%s' device\n", cd->cd_name);
		}
		pri = (*cops->cops_match)(m->parent, cf, m->aux);
	}
	if (pri > m->pri) {
		m->match = cf;
		m->pri = pri;
	}
}

/*
 * Invoke the "match" routine for a cfdata entry on behalf of
 * an external caller, usually a "submatch" routine.
 */
int
config_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct cfattach *ca;
	struct cfdriver *cd;
	struct cfops *cops;

	ca = cf->cf_attach;
	cd = cf->cf_driver;
	cops = ca->ca_ops;
	if(cd == NULL) {
		return (0);
	}
	return ((*cops->cops_match)(parent, cf, aux));
}

/*
 * Iterate over all potential children of some device, calling the given
 * function (default being the child's match function) for each one.
 * Nonzero returns are matches; the highest value returned is considered
 * the best match.  Return the `found child' if we got a match, or NULL
 * otherwise.  The `aux' pointer is simply passed on through.
 *
 * Note that this function is designed so that it can be used to apply
 * an arbitrary function to all potential children (its return value
 * can be ignored).
 */
struct cfdata *
config_search(fn, parent, aux)
	cfmatch_t fn;
	register struct device *parent;
	void *aux;
{
	register struct cfdata *cf;
	register short *p;
	struct matchinfo m;

	m.fn = fn;
	m.parent = parent;
	m.aux = aux;
	m.match = NULL;
	m.pri = 0;
	for (cf = cfdata; cf->cf_driver; cf++) {
		/*
		 * Skip cf if no longer eligible, otherwise scan through
		 * parents for one matching `parent', and try match function.
		 */
		if (cf->cf_fstate == FSTATE_FOUND)
			continue;
		for (p = cf->cf_parents; *p >= 0; p++)
			if (parent->dv_cfdata == &cfdata[*p])
				mapply(&m, cf);
	}
	return (m.match);
}

/*
 * Find the given root device.
 * This is much like config_search, but there is no parent.
 */
struct cfdata *
config_rootsearch(fn, rootname, aux)
	register cfmatch_t fn;
	register char *rootname;
	register void *aux;
{
	register struct cfdata *cf;
	register short *p;
	struct matchinfo m;

	m.fn = fn;
	m.parent = ROOT;
	m.aux = aux;
	m.match = NULL;
	m.pri = 0;
	/*
	 * Look at root entries for matching name.  We do not bother
	 * with found-state here since only one root should ever be
	 * searched (and it must be done first).
	 */
	for (p = cfroots; *p >= 0; p++) {
		cf = &cfdata[*p];
		if (strcmp(cf->cf_driver->cd_name, rootname) == 0) {
			mapply(&m, cf);
		}
	}
	return (m.match);
}

static char *msgs[3] = { "", " not configured\n", " unsupported\n" };

/*
 * The given `aux' argument describes a device that has been found
 * on the given parent, but not necessarily configured.  Locate the
 * configuration data for that device (using the cd_match configuration
 * driver function) and attach it, and return true.  If the device was
 * not configured, call the given `print' function and return 0.
 */
struct device *
config_found(parent, aux, print)
	struct device *parent;
	void *aux;
	cfprint_t print;
{
	return (config_found_sm(parent, aux, print, NULL));
}

struct device *
config_found_sm(parent, aux, print, submatch)
	struct device *parent;
	void *aux;
	cfprint_t print;
	cfmatch_t submatch;
{
	struct cfdata *cf;

	if ((cf = config_search(submatch, parent, aux)) != NULL) {
		return (config_attach(parent, cf, aux, print));
	}
	if (print) {
		printf(msgs[(*print)(aux, parent->dv_xname)]);
	}
	return (NULL);
}

/*
 * As above, but for root devices.
 */
struct device *
config_rootfound(rootname, aux)
	char *rootname;
	void *aux;
{
	struct cfdata *cf;

	if ((cf = config_rootsearch((cfmatch_t)NULL, rootname, aux)) != NULL) {
		return (config_attach(ROOT, cf, aux, (cfprint_t)NULL));
	}
	printf("root device %s not configured\n", rootname);
	return (NULL);
}

/* just like sprintf(buf, "%d") except that it works from the end */
static char *
number(ep, n)
	register char *ep;
	register int n;
{

	*--ep = 0;
	while (n >= 10) {
		*--ep = (n % 10) + '0';
		n /= 10;
	}
	*--ep = n + '0';
	return (ep);
}

/*
 * Expand the size of the cd_devs array if necessary.
 */
void
config_makeroom(n, cd)
	int n;
	struct cfdriver *cd;
{
	int old, new;
	void **nsp;

	if (n < cd->cd_ndevs) {
		return;
	}

	/*
	 * Need to expand the array.
	 */
	old = cd->cd_ndevs;
	if (old == 0) {
		new = MINALLOCSIZE / sizeof(void*);
	} else {
		new = old * 2;
	}
	while (new <= n) {
		new *= 2;
	}
	cd->cd_ndevs = new;
	nsp = calloc(new, sizeof(void*), M_DEVBUF, M_WAITOK);
	if (nsp == NULL) {
		panic("config_attach: %sing dev array", old != 0 ? "expand" : "creat");
	}
	bzero(nsp + old, (new - old) * sizeof(void*));
	if (old != 0) {
		bcopy(nsp, cd->cd_devs, old * sizeof(void*));
		free(cd->cd_devs, M_DEVBUF);
	}
	cd->cd_devs = nsp;
}

/*
 * Attach a found device.  Allocates memory for device variables.
 */
struct device *
config_attach(parent, cf, aux, print)
	register struct device *parent;
	register struct cfdata *cf;
	register void 			*aux;
	cfprint_t 				print;
{
	register struct device *dev;
	register struct cfattach *ca;
	register struct cfdriver *cd;
	register struct cfops *cops;
	register size_t lname, lunit;
	register char *xunit;
	int myunit;
	char num[10];

	ca = cf->cf_attach;
	cd = cf->cf_driver;
	cops = ca->ca_ops;
	if (ca->ca_devsize < sizeof(struct device))
		panic("config_attach");
	myunit = cf->cf_unit;
	if (cf->cf_fstate == FSTATE_NOTFOUND)
		cf->cf_fstate = FSTATE_FOUND;
	else
		cf->cf_unit++;

	/* compute length of name and decimal expansion of unit number */
	lname = strlen(cd->cd_name);
	xunit = number(&num[sizeof num], myunit);
	lunit = &num[sizeof num] - xunit;
	if (lname + lunit >= sizeof(dev->dv_xname))
		panic("config_attach: device name too long");

	/* get memory for all device vars */
	dev = (struct device *)malloc(ca->ca_devsize, M_DEVBUF, M_WAITOK);	/* XXX cannot wait! */
	if (!dev) {
		panic("config_attach: memory allocation for device softc failed");
	}
	bzero(dev, ca->ca_devsize);
	TAILQ_INSERT_TAIL(&alldevs, dev, dv_list);	/* link up */
	dev->dv_class = cd->cd_class;
	dev->dv_cfdata = cf;
	dev->dv_unit = myunit;
	bcopy(cd->cd_name, dev->dv_xname, lname);
	bcopy(xunit, dev->dv_xname + lname, lunit);
	dev->dv_parent = parent;
	dev->dv_flags = DVF_ACTIVE;	/* always initially active */
	if (parent == ROOT) {
		printf("%s (root)", dev->dv_xname);
	} else {
		printf("%s at %s", dev->dv_xname, parent->dv_xname);
		if (print) {
			(void) (*print)(aux, NULL);
		}
	}

	/* put this device in the devices array */
	config_makeroom(dev->dv_unit, cd);
	if (cd->cd_devs[dev->dv_unit]) {
		panic("config_attach: duplicate %s", dev->dv_xname);
	}
	cd->cd_devs[dev->dv_unit] = dev;

	/*
	 * Before attaching, clobber any unfound devices that are
	 * otherwise identical.
	 */
	for (cf = cfdata; cf->cf_driver; cf++) {
		if (cf->cf_driver == cd && cf->cf_unit == dev->dv_unit) {
			if (cf->cf_fstate == FSTATE_NOTFOUND) {
				cf->cf_fstate = FSTATE_FOUND;
			}
			if (cf->cf_fstate == FSTATE_STAR) {
				cf->cf_unit++;
			}
		}
	}

	/* link to attachments */
	LIST_INSERT_HEAD(&allattachs, ca, ca_list);

	(*cops->cops_attach)(parent, dev, aux);
	config_process_deferred(&deferred_config_queue, dev);
	return (dev);
}

/*
 * Detach a device.  Optionally forced (e.g. because of hardware
 * removal) and quiet.  Returns zero if successful, non-zero
 * (an error code) otherwise.
 *
 * Note that this code wants to be run from a process context, so
 * that the detach can sleep to allow processes which have a device
 * open to run and unwind their stacks.
 */
int
config_detach(dev, flags)
	struct device *dev;
	int flags;
{
	struct cfdata *cf;
	struct cfattach *ca;
	struct cfdriver *cd;
	struct cfops	*cops;
#ifdef DIAGNOSTIC
	struct device *d;
#endif
	int rv = 0, i;

	cf = dev->dv_cfdata;
#ifdef DIAGNOSTIC
	if (cf->cf_fstate != FSTATE_FOUND && cf->cf_fstate != FSTATE_STAR)
		panic("config_detach: bad device fstate");
#endif
	ca = cf->cf_attach;
	cd = cf->cf_driver;
	cops = ca->ca_ops;

	/*
	 * Try to detach the device.  If that's not possible, then
	 * we either panic() (for the forced but failed case), or
	 * return an error.
	 */
	if (rv == 0) {
		if (cops->cops_detach != NULL)
			rv = (*cops->cops_detach)(dev, flags);
		else
			rv = EOPNOTSUPP;
	}
	if (rv != 0) {
		if ((flags & DETACH_FORCE) == 0)
			return (rv);
		else
			panic("config_detach: forced detach of %s failed (%d)", dev->dv_xname, rv);
	}
	/*
	 * The device has now been successfully detached.
	 */
#ifdef DIAGNOSTIC
	/*
	 * Sanity: If you're successfully detached, you should have no
	 * children.  (Note that because children must be attached
	 * after parents, we only need to search the latter part of
	 * the list.)
	 */
	for(d = TAILQ_NEXT(dev, dv_list); d != NULL; d = TAILQ_NEXT(dev, dv_list)) {
		if(d->dv_parent == dev) {
			panic("config_detach: detached device has children");
		}
	}
#endif
	/*
	 * Mark cfdata to show that the unit can be reused, if possible.
	 * Note that we can only re-use a starred unit number if the unit
	 * being detached had the last assigned unit number.
	 */
	for (cf = cfdata; cf->cf_driver; cf++) {
		if (cf->cf_driver == cd) {
			if (cf->cf_fstate == FSTATE_FOUND &&
			    cf->cf_unit == dev->dv_unit)
				cf->cf_fstate = FSTATE_NOTFOUND;
			if (cf->cf_fstate == FSTATE_STAR &&
			    cf->cf_unit == dev->dv_unit + 1)
				cf->cf_unit--;
		}
	}

	/*
	 * Unlink from device list.
	 */
	TAILQ_REMOVE(&alldevs, dev, dv_list);

	/*
	 * Remove from cfdriver's array, tell the world, and free softc.
	 */
	cd->cd_devs[dev->dv_unit] = NULL;
	if ((flags & DETACH_QUIET) == 0) {
		printf("%s detached\n", dev->dv_xname);
	}
	free(dev, M_DEVBUF);

	/*
	 * If the device now has no units in use, deallocate its softc array.
	 */
	for (i = 0; i < cd->cd_ndevs; i++)
		if (cd->cd_devs[i] != NULL)
			break;
	if (i == cd->cd_ndevs) { 			/* nothing found; deallocate */
		free(cd->cd_devs, M_DEVBUF);
		cd->cd_devs = NULL;
		cd->cd_ndevs = 0;
	}

	/*
	 * Unlink from attachment list.
	 */
	LIST_REMOVE(ca, ca_list);

	/*
	 * Return success.
	 */
	return (0);
}

int
config_activate(dev)
	struct device *dev;
{
	struct cfattach *ca;
	struct cfdriver *cd;
	struct cfops *cops;
	int rv, oflags;

	ca = dev->dv_cfdata->cf_attach;
	cd = dev->dv_cfdata->cf_driver;
	cops = ca->ca_ops;
	oflags = dev->dv_flags;
	rv = 0;

	if (cops->cops_activate == NULL) {
		return (EOPNOTSUPP);
	}

	if ((dev->dv_flags & DVF_ACTIVE) == 0 || config_hint_enabled(dev)) {
		dev->dv_flags |= DVF_ACTIVE;
		rv = (*cops->cops_activate)(dev, DVACT_ACTIVATE);
		if (rv) {
			dev->dv_flags = oflags;
		}
	}

	return (rv);
}

int
config_deactivate(dev)
	struct device *dev;
{
	struct cfattach *ca;
	struct cfdriver *cd;
	struct cfops *cops;
	int rv, oflags;

	ca = dev->dv_cfdata->cf_attach;
	cd = dev->dv_cfdata->cf_driver;
	cops = ca->ca_ops;
	oflags = dev->dv_flags;
	rv = 0;

	if (cops->cops_activate == NULL) {
		return (EOPNOTSUPP);
	}

	if ((dev->dv_flags & DVF_ACTIVE) || config_hint_disabled(dev)) {
		dev->dv_flags &= ~DVF_ACTIVE;
		rv = (*cops->cops_activate)(dev, DVACT_DEACTIVATE);
		if (rv) {
			dev->dv_flags = oflags;
		}
	}
	return (rv);
}

void
config_defer(dev, func)
	struct device *dev;
	void (*func)(struct device *);
{
	struct deferred_config *dc;

	if (dev->dv_parent == NULL) {
		panic("config_defer: can't defer config of a root device");
	}

#ifdef DIAGNOSTIC
	for (dc = TAILQ_FIRST(&deferred_config_queue); dc != NULL; dc = TAILQ_NEXT(dc, dc_queue)) {
		if (dc->dc_dev == dev)
			panic("config_defer: deferred twice");
	}
#endif

	dc = malloc(sizeof(*dc), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (dc == NULL)
		panic("config_defer: unable to allocate callback");

	dc->dc_dev = dev;
	dc->dc_func = func;
	TAILQ_INSERT_TAIL(&deferred_config_queue, dc, dc_queue);
	config_pending_incr();
}

/*
 * Defer some autoconfiguration for a device until after interrupts
 * are enabled.
 */
void
config_interrupts(dev, func)
	struct device *dev;
	void (*func)(struct device *);
{
	struct deferred_config *dc;
	/*
	 * If interrupts are enabled, callback now.
	 */
	if (cold == 0) {
		(*func)(dev);
		return;
	}

#ifdef DIAGNOSTIC
	for (dc = TAILQ_FIRST(&interrupt_config_queue); dc != NULL; dc = TAILQ_NEXT(dc, dc_queue)) {
		if (dc->dc_dev == dev)
			panic("config_interrupts: deferred twice");
	}
#endif

	dc = malloc(sizeof(*dc), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (dc == NULL)
		panic("config_interrupts: unable to allocate callback");

	dc->dc_dev = dev;
	dc->dc_func = func;
	TAILQ_INSERT_TAIL(&interrupt_config_queue, dc, dc_queue);
	config_pending_incr();
}

/*
 * Process a deferred configuration queue.
 */
static void
config_process_deferred(queue, parent)
	struct deferred_config_head *queue;
	struct device *parent;
{
	struct deferred_config *dc, *ndc;

	for (dc = TAILQ_FIRST(queue); dc != NULL; dc = ndc) {
		ndc = TAILQ_NEXT(dc, dc_queue);
		if (parent == NULL || dc->dc_dev->dv_parent == parent) {
			TAILQ_REMOVE(queue, dc, dc_queue);
			(*dc->dc_func)(dc->dc_dev);
			free(dc, M_DEVBUF);
			config_pending_decr();
		}
	}
}

/*
 * Manipulate the config_pending semaphore.
 */
void
config_pending_incr(void)
{
	config_pending++;
}

void
config_pending_decr(void)
{
#ifdef DIAGNOSTIC
	if (config_pending == 0)
		panic("config_pending_decr: config_pending == 0");
#endif
	config_pending--;
	if (config_pending == 0)
		wakeup((void *)&config_pending);
}

/*
 * Attach an event.  These must come from initially-zero space (see
 * commented-out assignments below), but that occurs naturally for
 * device instance variables.
 */
void
evcnt_attach(ev, dev, group, name)
	struct evcnt *ev;
	struct device *dev;
	const char *name, *group;
{
	int len;
	len = strlen(group);
#ifdef DIAGNOSTIC
	if (len >= EVCNT_STRING_MAX) {		/* ..._MAX includes NUL */
		panic("evcnt_attach: group length (%s)", ev->ev_group);
	}
#endif
	ev->ev_grouplen = len;

	len = strlen(name);
#ifdef DIAGNOSTIC
	if (len >= EVCNT_STRING_MAX) {		/* ..._MAX includes NUL */
		panic("evcnt_attach: name length (%s)", ev->ev_name);
	}
#endif
	ev->ev_namelen = len;

	ev->ev_dev = dev;
	ev->ev_count = 0;
	ev->ev_name = name;
	ev->ev_group = group;

	TAILQ_INSERT_TAIL(&allevents, ev, ev_list);
}

void
evcnt_detach(ev)
	struct evcnt *ev;
{
	TAILQ_REMOVE(&allevents, ev, ev_list);
}

/*
 * Attach a statically-initialized event.  The type and string pointers
 * are already set up.
 */
void
evcnt_attach_static(ev)
	struct evcnt *ev;
{
	evcnt_attach(ev, NULL, ev->ev_group, ev->ev_name);
}

/*
 * Attach a dynamically-initialized event.  Zero it, set up the type
 * and string pointers and then act like it was statically initialized.
 */
void
evcnt_attach_dynamic(ev, type, parent, group, name)
	struct evcnt *ev;
	int type;
	const struct evcnt *parent;
	const char *group, *name;
{
	bzero(ev, sizeof(*ev));
	ev->ev_type = type;
	ev->ev_parent = parent;
	evcnt_attach(ev, NULL, group, name);
}

/*
 * Hint related configuration routines. These allow for a hints file to
 * enable or disable the specified device.
 */
int
config_hint_enabled(dev)
	struct device *dev;
{
	return (resource_enabled(dev->dv_xname, dev->dv_unit));
}

int
config_hint_disabled(dev)
	struct device *dev;
{
	return (resource_disabled(dev->dv_xname, dev->dv_unit));
}
