/*	$NetBSD: npf.c,v 1.7.2.3 2012/07/16 22:13:27 riz Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.7.2.3 2012/07/16 22:13:27 riz Exp $");

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
static int	npf_dev_open(dev_t, int, int, struct proc *);
static int	npf_dev_close(dev_t, int, int, struct proc *);
static int	npf_dev_ioctl(dev_t, u_long, void *, int, struct proc *);
static int	npf_dev_poll(dev_t, int, struct proc *);
static int	npf_dev_read(dev_t, struct uio *, int);

typedef struct npf_core npf_core_t;

struct npf_core {
	npf_ruleset_t *		n_rules;
	npf_tableset_t *	n_tables;
	npf_ruleset_t *		n_nat_rules;
	prop_dictionary_t	n_dict;
	bool				n_default_pass;
};

static void	npf_core_destroy(npf_core_t *);
static int	npfctl_stats(void *);

static struct rwlock	npf_rwlock;//		__cacheline_aligned;
static npf_core_t 		*npf_core;//		__cacheline_aligned;
static struct percpu 	*npf_stats_percpu;//	__read_mostly;

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

//state, nat, table, session, ruleset, log


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

void
npf_core_enter(void)
{
	rw_enter(&npf_rwlock, RW_READER);
}

npf_ruleset_t *
npf_core_ruleset(void)
{
	KASSERT(rw_lock_held(&npf_rwlock));
	return (npf_core->n_rules);
}

npf_ruleset_t *
npf_core_natset(void)
{
	KASSERT(rw_lock_held(&npf_rwlock));
	return (npf_core->n_nat_rules);
}

npf_tableset_t *
npf_core_tableset(void)
{
	KASSERT(rw_lock_held(&npf_rwlock));
	return (npf_core->n_tables);
}

void
npf_core_exit(void)
{
	rw_exit(&npf_rwlock);
}

bool
npf_core_locked(void)
{
	return rw_lock_held(&npf_rwlock);
}

prop_dictionary_t
npf_core_dict(void)
{
	KASSERT(rw_lock_held(&npf_rwlock));
	return (npf_core->n_dict);
}

bool
npf_default_pass(void)
{
	KASSERT(rw_lock_held(&npf_rwlock));
	return (npf_core->n_default_pass);
}
