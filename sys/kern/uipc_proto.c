/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)uipc_proto.c	7.2 (Berkeley) 12/30/87
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/null.h>
#include <sys/un.h>

/*
 * Definitions of protocols supported in the UNIX domain.
 */
extern	struct domain unixdomain;		/* or at least forward */

struct protosw unixsw[] = {
		{
				.pr_type		= SOCK_STREAM,
				.pr_domain		= &unixdomain,
				.pr_protocol	= PF_UNIX,
				.pr_flags		= PR_CONNREQUIRED|PR_WANTRCVD|PR_RIGHTS,
				.pr_usrreq		= uipc_usrreq,
				.pr_attach		= 0,
				.pr_detach		= 0,
		},
		{
				.pr_type		= SOCK_SEQPACKET,
				.pr_domain		= &unixdomain,
				.pr_protocol	= PF_UNIX,
				.pr_flags		= PR_ATOMIC|PR_CONNREQUIRED|PR_WANTRCVD|PR_RIGHTS,
				.pr_usrreq		= uipc_usrreq,
				.pr_attach		= 0,
				.pr_detach		= 0,
		},
		{
				.pr_type		= SOCK_DGRAM,
				.pr_domain		= &unixdomain,
				.pr_protocol	= PF_UNIX,
				.pr_flags		= PR_ATOMIC|PR_ADDR|PR_RIGHTS,
				.pr_usrreq		= uipc_usrreq,
				.pr_attach		= 0,
				.pr_detach		= 0,
		},
};

struct domain unixdomain = {
  .dom_family = AF_UNIX,
  .dom_name = "unix",
  .dom_init = 0,
  .dom_externalize = unp_externalize,
  .dom_dispose = unp_dispose,
  .dom_protosw = unixsw,
  .dom_protoswNPROTOSW = &unixsw[sizeof(unixsw)/sizeof(unixsw[0])]
};
