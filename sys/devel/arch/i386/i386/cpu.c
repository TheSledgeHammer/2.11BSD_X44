/*	$NetBSD: cpu.c,v 1.16 2019/10/01 18:00:07 chs Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author:
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/user.h>

#include <devel/arch/i386/include/cpu.h>

struct cpu_softc {
	struct device 		*sc_dev;		/* device tree glue */
	struct cpu_info 	*sc_info;		/* pointer to CPU info */
};

static int cpu_match(struct device *, struct cfdata *, void *);
static void cpu_attach(struct device *, struct device *, void *);

CFDRIVER_DECL(NULL, cpu, &cpu_cops, DV_DULL, sizeof(struct cpu_softc));
CFOPS_DECL(cpu, cpu_match, cpu_attach, NULL, NULL);

static int
cpu_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

static void
cpu_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cpu_softc *sc = (struct cpu_softc *)self;
	struct cpu_info *ci;

	sc->sc_dev = self;

	sc->sc_info = ci;
	ci->cpu_dev = self;

	return;
}

void
cpu_init(ci, cpuid, size)
	struct cpu_info *ci;
	int cpuid;
	size_t size;
{
	ci->cpu_cpuid = cpuid;
	ci->cpu_cpumask = 1 << cpuid;
	ci->cpu_size = size;

	ci->cpu_acpi_id = 0xffffffff;
}

u_int
cpu_cpuid(ci)
	struct cpu_info *ci;
{
	return (ci->cpu_cpuid);
}

u_int
cpu_cpumask(ci)
	struct cpu_info *ci;
{
	return (ci->cpu_cpumask);
}

u_int
cpu_acpi_id(ci)
	struct cpu_info *ci;
{
	return (ci->cpu_acpi_id);
}

u_int
cpu_apic_id(ci)
	struct cpu_info *ci;
{
	return (ci->cpu_apic_id);
}
