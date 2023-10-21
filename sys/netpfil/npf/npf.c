/*	$NetBSD: npf.c,v 1.7.2.7 2013/02/11 21:49:49 riz Exp $	*/

/*-
 * Copyright (c) 2009-2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NPF main: dynamic load/initialisation and unload routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.7.2.7 2013/02/11 21:49:49 riz Exp $");

/*
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/module.h>
*/
#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/percpu.h>
#include <sys/rwlock.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/uio.h>

#include "npf_impl.h"

void		npfattach(int);
static int	npf_fini(void);
static int	npf_dev_open(dev_t, int, int, struct proc *);
static int	npf_dev_close(dev_t, int, int, struct proc *);
static int	npf_dev_ioctl(dev_t, u_long, void *, int, struct proc *);
static int	npf_dev_poll(dev_t, int, struct proc *);
static int	npf_dev_read(dev_t, struct uio *, int);

static int	npfctl_stats(void *);

static struct percpu *npf_stats_percpu;//	__read_mostly;

const struct cdevsw npf_cdevsw = {
		.d_open = npf_dev_open,
		.d_close = npf_dev_close,
		.d_read = npf_dev_read,
		.d_write = nowrite,
		.d_ioctl = npf_dev_ioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = npf_dev_poll,
		.d_mmap = nommap,
		.d_kqfilter = nokqfilter,
		.d_type = D_OTHER
};

static int
npf_init(void)
{

	int error = 0;

	percpu_malloc(npf_stats_percpu, NPF_STATS_SIZE);
	//npf_sysctl = NULL;

	npf_tableset_sysinit();
	npf_session_sysinit();
	npf_nat_sysinit();
	npf_alg_sysinit();
	npf_ext_sysinit();

	/* Load empty configuration. */
	npf_config_init();

#ifdef _MODULE
	/* Attach /dev/npf device. */
	error = devsw_attach("npf", NULL, &bmajor, &npf_cdevsw, &cmajor);
	if (error) {
		/* It will call devsw_detach(), which is safe. */
		(void)npf_fini();
	}
#endif
	return error;
}

static int
npf_fini(void)
{

	/* At first, detach device and remove pfil hooks. */
#ifdef _MODULE
	devsw_detach(NULL, &npf_cdevsw);
#endif
	npf_pfil_unregister();

	/* Flush all sessions, destroy configuration (ruleset, etc). */
	npf_session_tracking(false);
	npf_config_fini();

	/* Finally, safe to destroy the subsystems. */
	npf_ext_sysfini();
	npf_alg_sysfini();
	npf_nat_sysfini();
	npf_session_sysfini();
	npf_tableset_sysfini();

	if (npf_sysctl) {
		sysctl_teardown(&npf_sysctl);
	}
	percpu_free(npf_stats_percpu);

	return 0;
}

void
npfattach(int nunits)
{
	/* Void. */
}

static int
npf_dev_open(dev_t dev, int flag, int mode, struct proc *p)
{

	/* Available only for super-user. */
	if (suser1(p->p_ucred, &p->p_acflag)) {
		return EPERM;
	}
	return 0;
}

static int
npf_dev_close(dev_t dev, int flag, int mode, struct proc *p)
{

	return 0;
}

static int
npf_dev_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct proc *p)
{
	int error;

	/* Available only for super-user. */
	if (suser1(p->p_ucred, &p->p_acflag)) {
		return EPERM;
	}

	switch (cmd) {
	case IOC_NPF_TABLE:
		error = npfctl_table(data);
		break;
	case IOC_NPF_RULE:
		error = npfctl_rule(cmd, data);
		break;
	case IOC_NPF_GETCONF:
		error = npfctl_getconf(cmd, data);
		break;
	case IOC_NPF_STATS:
		error = npfctl_stats(data);
		break;
	case IOC_NPF_SESSIONS_SAVE:
		error = npfctl_sessions_save(cmd, data);
		break;
	case IOC_NPF_SESSIONS_LOAD:
		error = npfctl_sessions_load(cmd, data);
		break;
	case IOC_NPF_SWITCH:
		error = npfctl_switch(data);
		break;
	case IOC_NPF_RELOAD:
		error = npfctl_reload(cmd, data);
		break;
	case IOC_NPF_VERSION:
		*(int *)data = NPF_VERSION;
		error = 0;
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}

static int
npf_dev_poll(dev_t dev, int events, struct proc *p)
{

	return ENOTSUP;
}

static int
npf_dev_read(dev_t dev, struct uio *uio, int flag)
{

	return ENOTSUP;
}

bool
npf_autounload_p(void)
{
	return !npf_pfil_registered_p() && npf_default_pass();
}

/*
 * NPF statistics interface.
 */

void
npf_stats_inc(npf_stats_t st)
{
	uint64_t *stats = percpu_lookup(npf_stats_percpu);
	stats[st]++;
	percpu_putref(npf_stats_percpu);
}

void
npf_stats_dec(npf_stats_t st)
{
	uint64_t *stats = percpu_lookup(npf_stats_percpu);
	stats[st]--;
	percpu_putref(npf_stats_percpu);
}

static void
npf_stats_collect(void *mem, void *arg, struct cpu_info *ci)
{
	uint64_t *percpu_stats = mem, *full_stats = arg;
	int i;

	for (i = 0; i < NPF_STATS_COUNT; i++) {
		full_stats[i] += percpu_stats[i];
	}
}

/*
 * npfctl_stats: export collected statistics.
 */
static int
npfctl_stats(void *data)
{
	uint64_t *fullst, *uptr = *(uint64_t **)data;
	int error;

	fullst = malloc(NPF_STATS_SIZE, M_NPF, M_WAITOK);
	percpu_foreach(npf_stats_percpu, npf_stats_collect, fullst);
	error = copyout(fullst, uptr, NPF_STATS_SIZE);
	kmem_free(fullst, M_NPF);
	return error;
}
