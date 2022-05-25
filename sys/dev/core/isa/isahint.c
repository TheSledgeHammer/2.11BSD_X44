/*-
 * Copyright (c) 1999 Doug Rabson
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/isa/isahint.c,v 1.8.2.1 2001/03/21 11:18:25 nyan Exp $
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/user.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

static void isahint_register(struct isa_attach_args *, const char *, int);

int
isahint_match(iba, cf)
	struct isabus_attach_args *iba;
	struct cfdata *cf;
{
	const char *buf = iba->iba_busname;
	const char *resname = cf->cf_driver->cd_name;
	int i;

	for (i = resource_query_string(-1, resname, buf); i != -1; i = resource_query_string(i, resname, buf)) {
		return (0);
	}
	return (1);
}

/* must run after isa_attch */
void
isahint_attach(sc)
	struct isa_softc 		*sc;
{
	struct isa_attach_args 	ia;
	struct isa_subdev 		*is;

	if (TAILQ_EMPTY(&sc->sc_subdevs)) {
		return;
	}

	TAILQ_FOREACH(is, &sc->sc_subdevs, id_bchain) {
		if((is->id_dev = config_found_sm(&sc->sc_dev, &ia, isaprint, isasubmatch)) != NULL) {
			isahint_register(&ia, sc->sc_dev.dv_xname, sc->sc_dev.dv_unit);
		}
	}
}

static void
isahint_register(ia, name, unit)
	struct isa_attach_args 	*ia;
	const char 				*name;
	int 					unit;
{
	resource_int_value(name, unit, "port", &ia->ia_iobase);
	resource_int_value(name, unit, "portsize", &ia->ia_iosize);
	resource_int_value(name, unit, "maddr", &ia->ia_maddr);
	resource_int_value(name, unit, "msize", &ia->ia_msize);
	resource_int_value(name, unit, "irq", &ia->ia_irq);
	resource_int_value(name, unit, "drq", &ia->ia_drq);
	resource_int_value(name, unit, "drq2", &ia->ia_drq2);
}
