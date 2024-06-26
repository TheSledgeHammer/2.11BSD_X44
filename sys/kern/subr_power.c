/*	$NetBSD: kern_hook.c,v 1.8 2019/10/16 18:29:49 christos Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2002, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Luke Mewburn.
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

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: kern_hook.c,v 1.8 2019/10/16 18:29:49 christos Exp $"); */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/power.h>
#include <sys/null.h>

int	powerhook_debug = 0;

void *
hook_establish(struct hook_head *list, void (*fn)(void *), void *arg)
{
	struct hook_desc *hd;
	hd = malloc(sizeof(*hd), M_DEVBUF, M_NOWAIT);
	if (hd == NULL) {
		return (NULL);
	}

	hd->hk_fn = fn;
	hd->hk_arg = arg;
	TAILQ_INSERT_HEAD(list, hd, hk_list);

	return (hd);
}

void
hook_disestablish(struct hook_head *list, void *vhook)
{
#ifdef DIAGNOSTIC
	struct hook_desc *hd;

	TAILQ_FOREACH(hd, list, hk_list) {
		 if (hd == vhook) {
			 goto found;
		 }
	}
	if (hd == NULL) {
		panic("hook_disestablish: hook %p not established", vhook);
	}
#endif
found:
	TAILQ_REMOVE(list, (struct hook_desc *)vhook, hk_list);
	free(vhook, M_DEVBUF);
}

void
dohooks(struct hook_head *list, int flags)
{
	struct hook_desc *hd;

	if ((flags & HOOK_REMOVE) == 0) {
		TAILQ_FOREACH(hd, list, hk_list) {
			(*hd->hk_fn)(hd->hk_arg);
		}
	} else {
		while ((hd = TAILQ_FIRST(list)) != NULL) {
			TAILQ_REMOVE(list, hd, hk_list);
			(*hd->hk_fn)(hd->hk_arg);
			if ((flags & HOOK_FREE) != 0) {
				free(hd, M_DEVBUF);
			}
		}
	}
}

void
hook_destroy(struct hook_head *list)
{
	struct hook_desc *hd;

	while ((hd = TAILQ_FIRST(list)) != NULL) {
		TAILQ_REMOVE(list, hd, hk_list);
		free(hd, M_DEVBUF);
	}
}

/*
 * "Shutdown hook" types, functions, and variables.
 *
 * Should be invoked immediately before the
 * system is halted or rebooted, i.e. after file systems unmounted,
 * after crash dump done, etc.
 *
 * Each shutdown hook is removed from the list before it's run, so that
 * it won't be run again.
 */

static struct hook_head shutdownhook_list = TAILQ_HEAD_INITIALIZER(shutdownhook_list);

void *
shutdownhook_establish(void (*fn)(void *), void *arg)
{
	return (hook_establish(&shutdownhook_list, fn, arg));
}

void
shutdownhook_disestablish(void *vhook)
{
	hook_disestablish(&shutdownhook_list, vhook);
}

void
doshutdownhooks(void)
{
	struct hook_desc *hd;
	while ((hd == TAILQ_FIRST(&shutdownhook_list)) != NULL) {
		TAILQ_REMOVE(&shutdownhook_list, hd, hk_list);
		(*hd->hk_fn)(hd->hk_arg);
		free(hd, M_DEVBUF);
	}
}

/*
 * "Power hook" types, functions, and variables.
 * The list of power hooks is kept ordered with the last registered hook
 * first.
 * When running the hooks on power down the hooks are called in reverse
 * registration order, when powering up in registration order.
 */
struct powerhook_desc {
	TAILQ_ENTRY(powerhook_desc) sfd_list;
	void						(*sfd_fn)(int, void *);
	void						*sfd_arg;
	char						sfd_name[16];
};
static TAILQ_HEAD(powerhook_head, powerhook_desc) powerhook_list =
    TAILQ_HEAD_INITIALIZER(powerhook_list);

void *
powerhook_establish(const char *name, void (*fn)(int, void *), void *arg)
{
	struct powerhook_desc *ndp;

	ndp = (struct powerhook_desc *)
	    malloc(sizeof(*ndp), M_DEVBUF, M_NOWAIT);
	if (ndp == NULL)
		return (NULL);

	ndp->sfd_fn = fn;
	ndp->sfd_arg = arg;
	strlcpy(ndp->sfd_name, name, sizeof(ndp->sfd_name));
	TAILQ_INSERT_HEAD(&powerhook_list, ndp, sfd_list);

	printf("%s: WARNING: powerhook_establish is deprecated\n", name);
	return (ndp);
}

void
powerhook_disestablish(void *vhook)
{
#ifdef DIAGNOSTIC
	struct powerhook_desc *dp;

	TAILQ_FOREACH(dp, &powerhook_list, sfd_list) {
		if (dp == vhook) {
			goto found;
		}
	}
	panic("powerhook_disestablish: hook %p not established", vhook);
 found:
#endif

	TAILQ_REMOVE(&powerhook_list, (struct powerhook_desc* )vhook, sfd_list);
	free(vhook, M_DEVBUF);
}

/*
 * Run power hooks.
 */
void
dopowerhooks(int why)
{
	struct powerhook_desc *dp;
	const char *why_name;
	static const char * pwr_names[] = {PWR_NAMES};
	why_name = why < __arraycount(pwr_names) ? pwr_names[why] : "???";

	if (why == PWR_RESUME || why == PWR_SOFTRESUME) {
		TAILQ_FOREACH_REVERSE(dp, &powerhook_list, powerhook_head,
				sfd_list) {
			if (powerhook_debug) {
				printf("dopowerhooks %s: %s (%p)\n", why_name, dp->sfd_name,
						dp);
			}
			(*dp->sfd_fn)(why, dp->sfd_arg);
		}
	} else {
		TAILQ_FOREACH(dp, &powerhook_list, sfd_list) {
			if (powerhook_debug) {
				printf("dopowerhooks %s: %s (%p)\n", why_name, dp->sfd_name,
						dp);
			}
			(*dp->sfd_fn)(why, dp->sfd_arg);
		}
	}

	if (powerhook_debug) {
		printf("dopowerhooks: %s done\n", why_name);
	}
}
