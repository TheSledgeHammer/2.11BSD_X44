/*	$NetBSD: acpi_apm.c,v 1.20 2010/10/24 07:53:04 jruoho Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and by Jared McNeill.
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
 * Autoconfiguration support for the Intel ACPI Component Architecture
 * ACPI reference implementation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_apm.c,v 1.20 2010/10/24 07:53:04 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <dev/power/acpi/acpivar.h>
#include <dev/power/apm/apmvar.h>

static void	acpiapm_disconnect(void *);
static void	acpiapm_enable(void *, int);
static int	acpiapm_set_powstate(void *, u_int, u_int);
static int	acpiapm_get_powstat(void *, u_int, struct apm_power_info *);
static bool	apm_per_sensor(const struct sysmon_envsys *,
			       const envsys_data_t *, void *);
static int	acpiapm_get_event(void *, u_int *, u_int *);
static void	acpiapm_cpu_busy(void *);
static void	acpiapm_cpu_idle(void *);
static void	acpiapm_get_capabilities(void *, u_int *, u_int *);

struct apm_accessops acpiapm_accessops = {
		acpiapm_disconnect,
		acpiapm_enable,
		acpiapm_set_powstate,
		acpiapm_get_powstat,
		acpiapm_get_event,
		acpiapm_cpu_busy,
		acpiapm_cpu_idle,
		acpiapm_get_capabilities,
};

#ifdef ACPI_APM_DEBUG
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a)
#endif

#ifndef ACPI_APM_DEFAULT_STANDBY_STATE
#define ACPI_APM_DEFAULT_STANDBY_STATE	(1)
#endif
#ifndef ACPI_APM_DEFAULT_SUSPEND_STATE
#define ACPI_APM_DEFAULT_SUSPEND_STATE	(3)
#endif
#define ACPI_APM_DEFAULT_CAP						      \
	((ACPI_APM_DEFAULT_STANDBY_STATE!=0 ? APM_GLOBAL_STANDBY : 0) |	      \
	 (ACPI_APM_DEFAULT_SUSPEND_STATE!=0 ? APM_GLOBAL_SUSPEND : 0))
#define ACPI_APM_STATE_MIN		(0)
#define ACPI_APM_STATE_MAX		(4)

/* It is assumed that there is only acpiapm instance. */
static int resumed = 0, capability_changed = 0;
static int standby_state = ACPI_APM_DEFAULT_STANDBY_STATE;
static int suspend_state = ACPI_APM_DEFAULT_SUSPEND_STATE;
static int capabilities = ACPI_APM_DEFAULT_CAP;
static int acpiapm_node = CTL_EOL, standby_node = CTL_EOL;

struct acpi_softc;
extern void acpi_enter_sleep_state(int);
static int acpiapm_match(struct device *, struct cfdata *, void *);
static void acpiapm_attach(struct device *, struct device *, void *);


static int
/*ARGSUSED*/
acpiapm_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return apm_match();
}

static void
/*ARGSUSED*/
acpiapm_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct apm_softc *sc = (void *)self;

	sc->sc_dev = self;
	sc->sc_ops = &acpiapm_accessops;
	sc->sc_cookie = parent;
	sc->sc_vers = 0x0102;
	sc->sc_detail = 0;
	sc->sc_hwflags = APM_F_DONT_RUN_HOOKS;
	apm_attach(sc);
}
