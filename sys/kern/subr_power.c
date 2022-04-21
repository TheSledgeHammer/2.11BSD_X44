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

struct hook_desc {
	TAILQ_ENTRY(hook_desc) 	hk_list;
	hook_func_t				hk_fn;
	void					*hk_arg;
};
typedef TAILQ_HEAD(hook_head, hook_desc) hook_list_t;

struct hook_list {
	hook_list_t	 			hl_list;
};

#define	__arraycount(__x)	(sizeof(__x) / sizeof(__x[0]))
int	powerhook_debug = 0;

static void *
hook_establish(hook_list_t *list, hook_func_t fn, void *arg)
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

static void
hook_disestablish(hook_list_t *list, void *vhook)
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

static void
hook_destroy(hook_list_t *list)
{
	struct hook_desc *hd;

	while ((hd = TAILQ_FIRST(list)) != NULL) {
		TAILQ_REMOVE(list, hd, hk_list);
		free(hd, M_DEVBUF);
	}
}

static void
do_hooks_shutdown(hook_list_t *list, int why)
{
	struct hook_desc *hd;
    while((hd == TAILQ_FIRST(list)) != NULL) {
        TAILQ_REMOVE(list, hd, hk_list);
		(*hd->hk_fn)(why, hd->hk_arg);
		free(hd, M_DEVBUF);
    }
}

static void
do_hooks_power(hook_list_t *list, int why)
{
    struct hook_desc *hd;
    const char *why_name;
	static const char * pwr_names[] = {PWR_NAMES};
	why_name = why < __arraycount(pwr_names) ? pwr_names[why] : "???";
	
	if (why == PWR_RESUME || why == PWR_SOFTRESUME) {
			TAILQ_FOREACH_REVERSE(hd, list, hook_head, hk_list) {
				if (powerhook_debug) {
					printf("dopowerhooks %s: (%p)\n", why_name, hd);
				}
				(*hd->hk_fn)(why, hd->hk_arg);
			}
		}  else {
			TAILQ_FOREACH(hd, list, hk_list) {
				if (powerhook_debug) {
					printf("dopowerhooks %s: (%p)\n", why_name, hd);
				}
				(*hd->hk_fn)(why, hd->hk_arg);
			}
		}

		if (powerhook_debug) {
			printf("dopowerhooks: %s done\n", why_name);
		}
}

static void
do_hooks(hook_list_t *list, int why, int type)
{
	struct hook_desc *hd;
	switch (type) {
	case HKLIST_SHUTDOWN:
        do_hooks_shutdown(list, why);
		return;

	case HKLIST_POWER:
        do_hooks_power(list, why);
		return;
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

static hook_list_t shutdownhook_list = TAILQ_HEAD_INITIALIZER(shutdownhook_list);

void *
shutdownhook_establish(hook_func_t fn, void *arg)
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
	do_hooks(&shutdownhook_list, PWR_SHUTDOWN, HKLIST_SHUTDOWN);
}

/*
 * "Power hook" types, functions, and variables.
 * The list of power hooks is kept ordered with the last registered hook
 * first.
 * When running the hooks on power down the hooks are called in reverse
 * registration order, when powering up in registration order.
 */

static hook_list_t powerhook_list = TAILQ_HEAD_INITIALIZER(powerhook_list);

void *
powerhook_establish(hook_func_t fn, void *arg)
{
	return (hook_establish(&powerhook_list, fn, arg));
}

void
powerhook_disestablish(void *vhook)
{
	hook_disestablish(&powerhook_list, vhook);
}

/*
 * Run power hooks.
 */
void
dopowerhooks(int why)
{
	do_hooks(&powerhook_list, why, HKLIST_POWER);
}
