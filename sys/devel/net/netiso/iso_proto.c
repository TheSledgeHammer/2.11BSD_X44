/*	$NetBSD: iso_proto.c,v 1.14 2003/08/07 16:33:37 agc Exp $	*/

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
 *	@(#)iso_proto.c	8.2 (Berkeley) 2/9/95
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */
/*
 * iso_proto.c : protocol switch tables in the ISO domain
 *
 * ISO protocol family includes TP, CLTP, CLNP, 8208
 * TP and CLNP are implemented here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iso_proto.c,v 1.14 2003/08/07 16:33:37 agc Exp $");

#include "opt_iso.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/radix.h>

#include <netiso/iso.h>
#include <netiso/clnp.h>
#include <netiso/esis.h>
#include <netiso/idrp_var.h>
#include <netiso/iso_pcb.h>
#include <netiso/cltp_var.h>
#ifdef TUBA
#include <netiso/tuba_table.h>
#endif

extern	struct domain isodomain;

void
iso_init(void)
{
#ifdef RADIX_ART
	rtable_art_init(AF_ISO, sizeof(struct iso_addr));
#endif
#ifdef TUBA
	tuba_table_init();
#endif
}

struct protosw isosw[] = {
		{
				.pr_type		= 0,
				.pr_domain		= &isodomain,
				.pr_protocol 	= 0,
				.pr_flags		= 0,
				.pr_input 		= 0,
				.pr_output		= 0,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= 0,
				.pr_usrreq		= 0,
				.pr_init		= iso_init,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		/* CLTP protocol */
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &isodomain,
				.pr_protocol 	= ISOPROTO_CLTP,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= 0,
				.pr_output		= cltp_output,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= 0,
				.pr_usrreq		= cltp_usrreq,
				.pr_init		= cltp_init,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		/* CLNP protocol */
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &isodomain,
				.pr_protocol 	= ISOPROTO_CLNP,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= 0,
				.pr_output		= clnp_output,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= 0,
				.pr_usrreq		= clnp_usrreq,
				.pr_init		= clnp_init,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= clnp_slowtimo,
				.pr_drain		= clnp_drain,
				.pr_sysctl		= 0,
		},
		/* ES-IS protocol */
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &isodomain,
				.pr_protocol 	= ISOPROTO_ESIS,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= esis_input,
				.pr_output		= 0,
				.pr_ctlinput 	= esis_ctlinput,
				.pr_ctloutput	= 0,
				.pr_usrreq		= esis_usrreq,
				.pr_init		= esis_init,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		/* ISOPROTO_INTRAISIS */
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &isodomain,
				.pr_protocol 	= ISOPROTO_INTRAISIS,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= isis_input,
				.pr_output		= 0,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= 0,
				.pr_usrreq		= esis_usrreq,
				.pr_init		= 0,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		/* ISOPROTO_IDRP */
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &isodomain,
				.pr_protocol 	= ISOPROTO_IDRP,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= idrp_input,
				.pr_output		= 0,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= 0,
				.pr_usrreq		= idrp_usrreq,
				.pr_init		= idrp_init,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
#ifdef TUBA
		/* ISOPROTO_TUBA */
		{
				.pr_type		= SOCK_STREAM,
				.pr_domain		= &isodomain,
				.pr_protocol 	= ISOPROTO_TCP,
				.pr_flags		= PR_CONNREQUIRED|PR_WANTRCVD,
				.pr_input 		= tuba_tcpinput,
				.pr_output		= 0,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= tuba_ctloutput,
				.pr_usrreq		= tuba_usrreq,
				.pr_init		= iso_init,
				.pr_fasttimo	= tuba_fasttimo,
				.pr_slowtimo	= tuba_fasttimo,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		}
#endif
};

struct domain isodomain = {
		.dom_family 			= PF_ISO,
		.dom_name 				= "iso-domain",
		.dom_init 				= 0,
		.dom_externalize 		= 0,
		.dom_dispose 			= 0,
		.dom_protosw 			= isosw,
		.dom_protoswNPROTOSW 	= &isosw[sizeof(isosw)/sizeof(isosw[0])],
		.dom_next 				= 0,
		.dom_rtattach			= rn_inithead,
		.dom_rtoffset			= 48,
		.dom_maxrtkey			= sizeof(struct sockaddr_iso),
		.dom_ifattach 			= 0,
		.dom_ifdetach 			= 0,
};
