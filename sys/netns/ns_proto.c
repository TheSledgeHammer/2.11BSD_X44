/*	$NetBSD: ns_proto.c,v 1.12 2003/08/07 16:33:46 agc Exp $	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1993
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
 *	@(#)ns_proto.c	8.2 (Berkeley) 2/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ns_proto.c,v 1.12 2003/08/07 16:33:46 agc Exp $");

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/radix.h>
#include <net/route.h>

/*
 * NS protocol family: IDP, ERR, PE, SPP, ROUTE.
 */
#include <netns/ns.h>
#include <netns/ns_pcb.h>
#include <netns/ns_if.h>
#include <netns/ns_var.h>
#include <netns/idp.h>
#include <netns/idp_var.h>
#include <netns/ns_error.h>
#include <netns/sp.h>
#include <netns/spidp.h>
#include <netns/spp_timer.h>
#include <netns/spp_var.h>

extern	struct domain nsdomain;

struct protosw nssw[] = {
		{
				.pr_type		= 0,
				.pr_domain		= &nsdomain,
				.pr_protocol 	= 0,
				.pr_flags		= 0,
				.pr_input 		= 0,
				.pr_output		= idp_output,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= 0,
				.pr_usrreq		= 0,
				.pr_init		= ns_init,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &nsdomain,
				.pr_protocol 	= 0,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= 0,
				.pr_output		= 0,
				.pr_ctlinput 	= idp_ctlinput,
				.pr_ctloutput	= idp_ctloutput,
				.pr_usrreq		= idp_usrreq,
				.pr_init		= 0,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		{
				.pr_type		= SOCK_STREAM,
				.pr_domain		= &nsdomain,
				.pr_protocol 	= NSPROTO_SPP,
				.pr_flags		= PR_CONNREQUIRED|PR_WANTRCVD,
				.pr_input 		= spp_input,
				.pr_output		= 0,
				.pr_ctlinput 	= spp_ctlinput,
				.pr_ctloutput	= spp_ctloutput,
				.pr_usrreq		= spp_usrreq,
				.pr_init		= spp_init,
				.pr_fasttimo	= spp_fasttimo,
				.pr_slowtimo	= spp_slowtimo,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		{
				.pr_type		= SOCK_SEQPACKET,
				.pr_domain		= &nsdomain,
				.pr_protocol 	= NSPROTO_SPP,
				.pr_flags		= PR_CONNREQUIRED|PR_WANTRCVD|PR_ATOMIC|PR_LISTEN|PR_ABRTACPTDIS,
				.pr_input 		= spp_input,
				.pr_output		= 0,
				.pr_ctlinput 	= spp_ctlinput,
				.pr_ctloutput	= spp_ctloutput,
				.pr_usrreq		= spp_usrreq_sp,
				.pr_init		= 0,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		{
				.pr_type		= SOCK_RAW,
				.pr_domain		= &nsdomain,
				.pr_protocol 	= NSPROTO_RAW,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= idp_input,
				.pr_output		= idp_output,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= idp_ctloutput,
				.pr_usrreq		= idp_raw_usrreq,
				.pr_init		= 0,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},
		{
				.pr_type		= SOCK_RAW,
				.pr_domain		= &nsdomain,
				.pr_protocol 	= NSPROTO_ERROR,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= 0,
				.pr_output		= idp_output,
				.pr_ctlinput 	= idp_ctlinput,
				.pr_ctloutput	= idp_ctloutput,
				.pr_usrreq		= idp_raw_usrreq,
				.pr_init		= 0,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= 0,
		},

};

struct domain nsdomain = {
		.dom_family 			= PF_NS,
		.dom_name 				= "ns",
		.dom_init 				= 0,
		.dom_externalize 		= 0,
		.dom_dispose 			= 0,
		.dom_protosw 			= nssw,
		.dom_protoswNPROTOSW 	= &nssw[sizeof(nssw)/sizeof(nssw[0])],
		.dom_next 				= 0,
		.dom_rtattach			= rn_inithead,
		.dom_rtoffset			= 16,
		.dom_maxrtkey			= sizeof(struct sockaddr_ns),
		.dom_ifattach 			= 0,
		.dom_ifdetach 			= 0,
};
