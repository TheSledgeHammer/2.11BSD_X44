/*
 * Copyright (c) 1996-1999
 * Kazutaka YOKOTA (yokota@zodiac.mech.utsunomiya-u.ac.jp)
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
 * $FreeBSD: src/sys/dev/kbd/atkbdc.c,v 1.5.2.2 2002/03/31 11:02:02 murray Exp $
 * from kbdio.c,v 1.13 1998/09/25 11:55:46 yokota Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/user.h>

#include <vm/include/vm_param.h>

#include <dev/core/isa/isavar.h>
#include <dev/core/isa/isareg.h>

#include <devel/dev/kbio.h>
#include <devel/dev/consio.h>
#include <devel/dev/fbio.h>
#include <devel/dev/kbd/kbdreg.h>
#include <devel/dev/kbd/kbdtables.h>
#include <devel/dev/kbd/atkbdcreg.h>

#define ATKBDC_PORT0		IO_KBD
#define ATKBDC_PORT1		(IO_KBD + KBD_STATUS_PORT)

int
atkbdc_configure(void)
{
	bus_space_tag_t 	tag;
	bus_space_handle_t 	h0;
	bus_space_handle_t 	h1;

	int port0 = ATKBDC_PORT0;
	int port1 = ATKBDC_PORT1;

    bus_space_map(tag, port0, IO_KBDSIZE, 0, &h0);
    bus_space_map(tag, port1, IO_KBDSIZE, 0, &h1);

	h0 = (bus_space_handle_t) port0;
	h1 = (bus_space_handle_t) port1;

	return (atkbdc_setup(atkbdc_softc[0], tag, h0, h1));
}
