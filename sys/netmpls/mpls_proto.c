/*	$NetBSD: mpls_proto.c,v 1.3 2012/02/01 16:49:36 christos Exp $ */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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
__KERNEL_RCSID(0, "$NetBSD: mpls_proto.c,v 1.3 2012/02/01 16:49:36 christos Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>

#include <net/route.h>

#include <netmpls/mpls.h>
#include <netmpls/mpls_var.h>

struct ifqueue mplsintrq;

static int mpls_usrreq(struct socket *, int, struct mbuf *, struct mbuf *, struct mbuf *, struct proc *);
int sysctl_mpls(int *, u_int, void *, size_t *, void *, size_t );

int mpls_defttl = 255;
int mpls_mapttl_inet = 1;
int mpls_mapttl_inet6 = 1;
int mpls_icmp_respond = 0;
int mpls_forwarding = 0;
int mpls_accept = 0;
int mpls_mapprec_inet = 1;
int mpls_mapclass_inet6 = 1;

void
mpls_init(void)
{
	memset(&mplsintrq, 0, sizeof(mplsintrq));
	mplsintrq.ifq_maxlen = 256;
}

extern struct domain mplsdomain;

struct protosw mplssw[] = {
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &mplsdomain,
				.pr_protocol 	= 0,
				.pr_flags		= PR_ATOMIC|PR_ADDR,
				.pr_input 		= 0,
				.pr_output		= 0,
				.pr_ctlinput 	= 0,
				.pr_ctloutput	= 0,
				.pr_usrreq		= mpls_usrreq,
				.pr_init		= mpls_init,
				.pr_fasttimo	= 0,
				.pr_slowtimo	= 0,
				.pr_drain		= 0,
				.pr_sysctl		= sysctl_mpls,
		}
};

struct domain mplsdomain = {
		.dom_family 			= PF_MPLS,
		.dom_name 				= "mpls",
		.dom_init 				= 0,
		.dom_externalize 		= 0,
		.dom_dispose 			= 0,
		.dom_protosw 			= mplssw,
		.dom_protoswNPROTOSW 	= &mplssw[nitems(mplssw)]
};

static int
mpls_usrreq(struct socket *so, int req, struct mbuf *m,
        struct mbuf *nam, struct mbuf *control, struct proc *p)
{
	int error = EOPNOTSUPP;

	if ((req == PRU_ATTACH) &&
	    (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0)) {
		int s = splsoftnet();
		error = soreserve(so, 8192, 8192);
		splx(s);
	}

	return error;
}

int
sysctl_mpls(name, namelen, where, given, new, newlen)
	int	*name;
	u_int	namelen;
	void 	*where;
	size_t	*given;
	void	*new;
	size_t	newlen;
{
	int error;

	switch (name[0]) {
	case MPLSCTL_DEFTTL:
		error = sysctl_int(where, given, new, newlen, &mpls_defttl);
		break;
	case MPLSCTL_FORWARD:
		error = sysctl_int(where, given, new, newlen, &mpls_forwarding);
		break;
	case MPLSCTL_ACCEPT:
		error = sysctl_int(where, given, new, newlen, &mpls_accept);
		break;
	case MPLSCTL_IFQLEN:
		error = sysctl_int(where, given, new, newlen, &mplsintrq.ifq_maxlen);
		break;
#ifdef INET
	case MPLSCTL_MAPTTL_IP:
		error = sysctl_int(where, given, new, newlen, &mpls_mapttl_inet);
		break;
	case MPLSCTL_MAPPREC_IP:
		error = sysctl_int(where, given, new, newlen, &mpls_mapprec_inet);
		break;
	case MPLSCTL_ICMP_RESPOND:
		error = sysctl_int(where, given, new, newlen, &mpls_icmp_respond);
		break;
#endif
#ifdef INET6
	case MPLSCTL_MAPTTL_IP6:
		error = sysctl_int(where, given, new, newlen, &mpls_mapttl_inet6);
		break;
	case MPLSCTL_MAPCLASS_IP6:
		error = sysctl_int(where, given, new, newlen, &mpls_mapclass_inet6);
		break;
#endif
	}
	return (error);
}
