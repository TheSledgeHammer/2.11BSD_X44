/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

struct nsapisotable tsapisotable;

/* TSAP's */
void
tsap_init(struct tsap_iso *tsap)
{
	nsap_init(&tsapisotable);
}

void
tsap_attach(struct tsap_iso *tsap, long type, long subnet, long protocol, int class)
{
	nsap_attach(&tsapisotable, &tsap->tsi_nsaps, type, subnet);

	tsap_select(&tsap->tsi_select, protocol, type, subnet, class);
}

void
tsap_detach(struct tsap_iso *tsap, long type, long subnet)
{
	nsap_detach(&tsapisotable, tsap->tsi_nsaps, type, subnet);
}

void
tsap_select(struct sap_select *select, long proto, long type, long subnet, int class)
{
	bzero(select, sizeof(*select));

    select->ss_type = sap_type_select(type);
    select->ss_subnet = sap_subnet_select(subnet);
    select->ss_class = sap_class_select(class);
    select->ss_protocol = sap_protocol_select(proto);
}

/*
 * tsap_connect:
 * - Makes the same assumptions as nsap_connect.
 */
int
tsap_connect(struct mbuf *nam, int class, int af)
{
	struct sockaddr_nsap *snsap;
	int error;

	snsap = mtod(nam, struct sockaddr_nsap *);
	if (snsap == NULL) {
		return (EINVAL);
	}
	switch (class) {
	case NSAP_CLASS_CONS:
	{
		switch (af) {
		case AF_INET:
			printf("tsap_connect: Cannot connect IPV4 while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_INET6:
			printf("tsap_connect: Cannot connect IPV6 while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_NS:
			printf("tsap_connect: Cannot connect IDP while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_ISO:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_CONS,	NSAP_CLASS_CONS, AF_ISO);
			break;
		case AF_CCITT:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_X25, NSAP_CLASS_CONS, AF_CCITT);
			break;
		case AF_NATM:
			printf("tsap_connect: Cannot connect ATM while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_IPX:
			printf("tsap_connect: Cannot connect IPX while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_SNA:
			printf("tsap_connect: Cannot connect SNA while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		default:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN, NSAP_CLASS_CONS, AF_UNSPEC);
			break;
		}
		break;
	}
	case NSAP_CLASS_CLNS:
	{
		switch (af) {
		case AF_INET:
			error = nsap_connect(nam, snsap, (NSAP_SUBNET_IPV4 | NSAP_SUBNET_IPV6), NSAP_CLASS_CLNS, AF_INET);
			break;
		case AF_INET6:
			error = nsap_connect(nam, snsap, (NSAP_SUBNET_IPV4 | NSAP_SUBNET_IPV6), NSAP_CLASS_CLNS, AF_INET6);
			break;
		case AF_ISO:
			error = nsap_connect(nam, snsap, (NSAP_SUBNET_CLNS | NSAP_SUBNET_CLNP | NSAP_SUBNET_ISIS | NSAP_SUBNET_ESIS),
					NSAP_CLASS_CLNS, AF_ISO);
			break;
		case AF_NS:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_IDP, NSAP_CLASS_CLNS, AF_NS);
			break;
		case AF_CCITT:
			printf("tsap_connect: Cannot connect X.25 while in a connection-less oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_NATM:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_ATM, NSAP_CLASS_CLNS, AF_NATM);
			break;
		case AF_IPX:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_IPX, NSAP_CLASS_CLNS, AF_IPX);
			break;
		case AF_SNA:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_SNA, NSAP_CLASS_CLNS, AF_SNA);
			break;
		default:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN, NSAP_CLASS_CLNS, AF_UNSPEC);
			break;
		}
		break;
	}
	default:
		error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN, NSAP_CLASS_UNKNOWN, AF_UNSPEC);
		break;
	}
	return (error);
}

void
tsap_disconnect(void *v, int class, int af)
{
	struct sockaddr_nsap *snsap;

	snsap = (struct sockaddr_nsap *)v;
	if (snsap == NULL) {
		return;
	}

	switch (class) {
	case NSAP_CLASS_CONS:
	{
		switch (af) {
		case AF_INET:
			printf("tsap_disconnect: Cannot disconnect IPV4 while in a connection oriented mode\n");
			break;
		case AF_INET6:
			printf("tsap_disconnect: Cannot disconnect IPV6 while in a connection oriented mode\n");
			break;
		case AF_NS:
			printf("tsap_disconnect: Cannot disconnect IDP while in a connection oriented mode\n");
			break;
		case AF_ISO:
			nsap_disconnect(snsap, NSAP_SUBNET_CONS, AF_ISO);
			break;
		case AF_CCITT:
			nsap_disconnect(snsap, NSAP_SUBNET_X25, AF_CCITT);
			break;
		case AF_NATM:
			printf("tsap_disconnect: Cannot disconnect ATM while in a connection oriented mode\n");
			break;
		case AF_IPX:
			printf("tsap_disconnect: Cannot disconnect IPX while in a connection oriented mode\n");
			break;
		case AF_SNA:
			printf("tsap_disconnect: Cannot disconnect SNA while in a connection oriented mode\n");
			break;
		default:
			nsap_disconnect(snsap, NSAP_SUBNET_UNKNOWN, AF_UNSPEC);
			break;
		}
		break;
	}
	case NSAP_CLASS_CLNS:
	{
		switch (af) {
		case AF_INET:
			nsap_disconnect(snsap, (NSAP_SUBNET_IPV4 | NSAP_SUBNET_IPV6), AF_INET);
			break;
		case AF_INET6:
			nsap_disconnect(snsap, (NSAP_SUBNET_IPV4 | NSAP_SUBNET_IPV6), AF_INET6);
			break;
		case AF_ISO:
			nsap_disconnect(snsap, (NSAP_SUBNET_CLNS | NSAP_SUBNET_CLNP | NSAP_SUBNET_ISIS | NSAP_SUBNET_ESIS),
					AF_ISO);
			break;
		case AF_NS:
			nsap_disconnect(snsap, NSAP_SUBNET_IDP, AF_NS);
			break;
		case AF_CCITT:
			printf("tsap_disconnect: Cannot disconnect X.25 while in a connection-less oriented mode\n");
			break;
		case AF_NATM:
			nsap_disconnect(snsap, NSAP_SUBNET_ATM, AF_NATM);
			break;
		case AF_IPX:
			nsap_disconnect(snsap, NSAP_SUBNET_IPX, AF_IPX);
			break;
		case AF_SNA:
			nsap_disconnect(snsap, NSAP_SUBNET_SNA, AF_SNA);
			break;
		default:
			nsap_disconnect(snsap, NSAP_SUBNET_UNKNOWN, AF_UNSPEC);
			break;
		}
		break;
	}
	default:
		nsap_disconnect(snsap, NSAP_SUBNET_UNKNOWN, AF_UNSPEC);
		break;
	}
}
