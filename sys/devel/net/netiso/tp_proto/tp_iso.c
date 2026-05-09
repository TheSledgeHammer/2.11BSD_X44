/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)tp_iso.c	8.2 (Berkeley) 9/22/94
 */

#include <sys/socket.h>

#include <netiso/tp_pcb.h>
#include <netiso/tp_protosw.h>

#include <netiso/tp_proto/tp_iso.h>

struct tp_protosw tpiso_protosw = {
 	.tp_afamily = AF_ISO,
	.tp_xsapattach = iso_sapattach,
	.tp_xsapdetach = iso_sapdetach,
 	.tp_putnetaddr = iso_putnetaddr,
 	.tp_getnetaddr = iso_getnetaddr,
 	.tp_cmpnetaddr = iso_cmpnetaddr,
 	.tp_putsufx = iso_putsufx,
 	.tp_getsufx = iso_getsufx,
 	.tp_recycle_suffix = iso_recycle_tsuffix,
 	/*
 	.tp_mtu =
 	.tp_pcbbind =
 	.tp_pcbconn =
 	.tp_pcbdisc =
 	.tp_pcbdetach =
 	.tp_pcballoc =
 	.tp_output =
 	.tp_dgoutput =
 	.tp_ctloutput =
 	.tp_pcblist =
 	*/
};

void
iso_sapattach(struct tp_xsap_router *router)
{
	tp_xsap_attach(router, AF_ISO);
}

void
iso_sapdetach(struct tp_xsap_router *router)
{
	tp_xsap_detach(router, AF_ISO);
}
