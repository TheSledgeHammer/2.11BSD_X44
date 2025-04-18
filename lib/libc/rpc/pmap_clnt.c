/*	$NetBSD: pmap_clnt.c,v 1.12 1999/03/25 01:16:11 lukem Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char *sccsid = "@(#)pmap_clnt.c 1.37 87/08/11 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)pmap_clnt.c	2.2 88/08/01 4.0 RPCSRC";
#else
__RCSID("$NetBSD: pmap_clnt.c,v 1.12 1999/03/25 01:16:11 lukem Exp $");
#endif
#endif

/*
 * pmap_clnt.c
 * Client interface to pmap rpc service.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "namespace.h"

#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>

#ifdef __weak_alias
__weak_alias(pmap_set,_pmap_set);
__weak_alias(pmap_unset,_pmap_unset);
#endif

static const struct timeval timeout = { 5, 0 };
static const struct timeval tottimeout = { 60, 0 };

/*
 * Set a mapping between program,version and port.
 * Calls the pmap service remotely to do the mapping.
 */
bool_t
pmap_set(program, version, protocol, port)
	u_long program;
	u_long version;
	int protocol;
	u_short port;
{
	struct sockaddr_in myaddress;
	int socket = -1;
	CLIENT *client;
	struct pmap parms;
	bool_t rslt;

	if (get_myaddress(&myaddress) != 0)
		return (FALSE);
	client = clntudp_bufcreate(&myaddress, PMAPPROG, PMAPVERS,
	    timeout, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (client == (CLIENT *)NULL)
		return (FALSE);
	parms.pm_prog = program;
	parms.pm_vers = version;
	parms.pm_prot = protocol;
	parms.pm_port = port;
	if (CLNT_CALL(client, PMAPPROC_SET, (xdrproc_t)xdr_pmap, 
	    &parms, (xdrproc_t)xdr_bool, &rslt, tottimeout) != RPC_SUCCESS) {
		clnt_perror(client, __UNCONST("Cannot register service"));
		rslt = FALSE;
	}
	CLNT_DESTROY(client);
	return (rslt);
}

/*
 * Remove the mapping between program,version and port.
 * Calls the pmap service remotely to do the un-mapping.
 */
bool_t
pmap_unset(program, version)
	u_long program;
	u_long version;
{
	struct sockaddr_in myaddress;
	int socket = -1;
	CLIENT *client;
	struct pmap parms;
	bool_t rslt;

	if (get_myaddress(&myaddress) != 0)
		return (FALSE);
	client = clntudp_bufcreate(&myaddress, PMAPPROG, PMAPVERS,
	    timeout, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (client == (CLIENT *)NULL)
		return (FALSE);
	parms.pm_prog = program;
	parms.pm_vers = version;
	parms.pm_port = parms.pm_prot = 0;
	CLNT_CALL(client, PMAPPROC_UNSET, (xdrproc_t)xdr_pmap, &parms,
	    (xdrproc_t)xdr_bool, &rslt, tottimeout);
	CLNT_DESTROY(client);
	return (rslt);
}
