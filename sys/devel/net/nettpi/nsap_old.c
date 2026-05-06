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

#include "iso_nsap.h"

/*
 * nsap_connect:
 * - Assumes sockaddr for the selected protocol family has been defined prior to nsap_connect.
 */
int
nsap_connect(struct mbuf *nam, struct sockaddr_nsap *snsap, long subnet, int class, int af)
{
	switch (af) {
	case AF_INET:
	{
		struct sockaddr_in *sin = mtod(nam, struct sockaddr_in);
		nsap_setup_snsap(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4);
		if (nam->m_len != sizeof(sin)) {
			return (EINVAL);
		}
		if (sin->sin_family != AF_INET) {
			return (EAFNOSUPPORT);
		}
		if (sin->sin_port == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setup_nsapa(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_CLNS);
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6);
		nsap_setup_snsap(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6);
		if (nam->m_len != sizeof(sin6)) {
			return (EINVAL);
		}
		if (sin6->sin6_family != AF_INET6) {
			return (EAFNOSUPPORT);
		}
		if (sin6->sin6_port == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setup_nsapa(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_CLNS);
		break;
	}
	case AF_ISO:
	{
		struct sockaddr_iso *siso = mtod(nam, struct sockaddr_iso);
		nsap_setup_snsap(snsap, (struct sockaddr_iso *)siso, NSAP_TYPE_SISO, subnet);
		if (nam->m_len != sizeof(siso)) {
			return (EINVAL);
		}
		if (siso->siso_family != AF_ISO) {
			return (EAFNOSUPPORT);
		}
		if (siso->siso_nlen == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setup_nsapa(&snsap->snsap_addr, (struct iso_addr *)&siso->siso_addr, NSAP_TYPE_SISO, class);
		break;
	}
	case AF_NS:
	{
		struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns);
		nsap_setup_snsap(snsap, (struct sockaddr_ns *)sns, NSAP_TYPE_SNS, NSAP_SUBNET_IDP);
		if (nam->m_len != sizeof(sns)) {
			return (EINVAL);
		}
		if (sns->sns_family != AF_NS) {
			return (EAFNOSUPPORT);
		}
		if (sns->sns_port == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setup_nsapa(&snsap->snsap_addr, (struct ns_addr *)&sns->sns_addr, NSAP_TYPE_SNS, NSAP_CLASS_CLNS);
		break;
	}
	case AF_CCITT:
	{
		struct sockaddr_x25 *sx25 = mtod(nam, struct sockaddr_x25);
		nsap_setup_snsap(snsap, (struct sockaddr_x25 *)sx25, NSAP_TYPE_SX25, NSAP_SUBNET_X25);
		if (nam->m_len != sizeof(sx25)) {
			return (EINVAL);
		}
		if (sx25->x25_family != AF_CCITT) {
			return (EAFNOSUPPORT);
		}
		if (sx25->x25_len == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setup_nsapa(&snsap->snsap_addr, (struct x25_addr *)&sx25->x25_addr, NSAP_TYPE_SX25, NSAP_CLASS_CONS);
		break;
	}
	case AF_NATM:
	case AF_IPX:
	case AF_SNA:
	default:
		nsap_setup_snsap(snsap, NULL, NSAP_TYPE_UNKNOWN, NSAP_SUBNET_UNKNOWN);
		nsap_setup_nsapa(&snsap->snsap_addr, NULL, NSAP_TYPE_UNKNOWN, NSAP_CLASS_UNKNOWN);
		return (EAFNOSUPPORT);
	}
	return (0);
}

void
nsap_disconnect(struct sockaddr_nsap *snsap, int subnet, int af)
{
	switch (af) {
	case AF_INET:
	{
		struct sockaddr_in *sin = &snsap->snsap_sin4;
		if (sin != NULL) {
			switch (subnet) {
			case NSAP_SUBNET_IPV4:
				nsap_setup_snsap(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_IPV6:
				nsap_setup_snsap(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_IPV6);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_setup_snsap(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_UNKNOWN);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 *sin6 = &snsap->snsap_sin6;
		if (sin6 != NULL) {
			switch (subnet) {
			case NSAP_SUBNET_IPV4:
				nsap_setup_snsap(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_IPV4);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_IPV6:
				nsap_setup_snsap(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_setup_snsap(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_UNKNOWN);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case AF_ISO:
	{
		struct sockaddr_iso *sisos = &snsap->snsap_siso;
		if (sisos != NULL) {
			switch (subnet) {
			case NSAP_SUBNET_CONS:
				nsap_setup_snsap(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_CONS);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CONS);
				break;
			case NSAP_SUBNET_CLNS:
				nsap_setup_snsap(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_CLNS);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_CLNP:
				nsap_setup_snsap(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_CLNP);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_ISIS:
				nsap_setup_snsap(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_ISIS);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_ESIS:
				nsap_setup_snsap(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_ESIS);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_setup_snsap(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_UNKNOWN);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case AF_NS:
	{
		struct sockaddr_ns *sns = &snsap->snsap_sns;
		if (sns != NULL) {
			if (subnet == NSAP_SUBNET_IDP) {
				nsap_setup_snsap(snsap, (struct sockaddr_ns *)sns, NSAP_TYPE_SNS, NSAP_SUBNET_IDP);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct ns_addr *)&sns->sns_addr, NSAP_TYPE_SX25, NSAP_CLASS_CLNS);
			} else {
				nsap_setup_snsap(snsap, (struct sockaddr_ns *)sns, NSAP_TYPE_SNS, NSAP_SUBNET_UNKNOWN);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct ns_addr *)&sns->sns_addr, NSAP_TYPE_SNS, NSAP_CLASS_UNKNOWN);
			}
		} else {
			goto unknown;
		}
		break;
	}
	case AF_CCITT:
	{
		struct sockaddr_x25 *sx25 = &snsap->snsap_sx25;
		if (sx25 != NULL) {
			if (subnet == NSAP_SUBNET_X25) {
				nsap_setup_snsap(snsap, (struct sockaddr_x25 *)sx25, NSAP_TYPE_SX25, NSAP_SUBNET_X25);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct x25_addr *)&sx25->x25_addr, NSAP_TYPE_SX25, NSAP_CLASS_CONS);
			} else {
				nsap_setup_snsap(snsap, (struct sockaddr_x25 *)sx25, NSAP_TYPE_SX25, NSAP_SUBNET_UNKNOWN);
				nsap_setup_nsapa(&snsap->snsap_addr, (struct x25_addr *)&sx25->x25_addr, NSAP_TYPE_SX25, NSAP_CLASS_UNKNOWN);
			}
		} else {
			goto unknown;
		}
		break;
	}
	case AF_NATM:
	case AF_IPX:
	case AF_SNA:
	default:
unknown:
		nsap_setup_snsap(snsap, NULL, NSAP_TYPE_UNKNOWN, NSAP_SUBNET_UNKNOWN);
		nsap_setup_nsapa(&snsap->snsap_addr, NULL, NSAP_TYPE_UNKNOWN, NSAP_CLASS_UNKNOWN);
		break;
	}
}

/*
 * tsap_connect:
 * - Makes the same assumptions as nsap_connect.
 */
int
tsap_connect(struct mbuf *nam, struct sockaddr_nsap *snsap, long subnet, int class, int af)
{
	int error;

	if (snsap == NULL) {
		return (EINVAL);
	}
	switch (class) {
	case NSAP_CLASS_CONS: {
		switch (af) {
		case AF_INET:
			printf(
					"tsap_connect: Cannot connect IPV4 while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_INET6:
			printf(
					"tsap_connect: Cannot connect IPV6 while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_NS:
			printf(
					"tsap_connect: Cannot connect IDP while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_ISO:
			if (subnet != NSAP_SUBNET_CONS) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_CONS, NSAP_CLASS_CONS,
			AF_ISO);
			break;
		case AF_CCITT:
			if (subnet != NSAP_SUBNET_X25) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_X25, NSAP_CLASS_CONS,
			AF_CCITT);
			break;
		case AF_NATM:
			printf(
					"tsap_connect: Cannot connect ATM while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_IPX:
			printf(
					"tsap_connect: Cannot connect IPX while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_SNA:
			printf(
					"tsap_connect: Cannot connect SNA while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		default:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN,
			NSAP_CLASS_CONS, AF_UNSPEC);
			break;
		}
		break;
	}
	case NSAP_CLASS_CLNS: {
		switch (af) {
		case AF_INET:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, subnet, NSAP_CLASS_CLNS, AF_INET);
			break;
		case AF_INET6:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, subnet, NSAP_CLASS_CLNS, AF_INET6);
			break;
		case AF_ISO:
			if ((subnet != NSAP_SUBNET_CLNS) || (subnet != NSAP_SUBNET_CLNP)
					|| (subnet != NSAP_SUBNET_ISIS)
					|| (subnet != NSAP_SUBNET_ESIS)) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, subnet, NSAP_CLASS_CLNS, AF_ISO);
			break;
		case AF_NS:
			if (subnet != NSAP_SUBNET_IDP) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_IDP, NSAP_CLASS_CLNS,
			AF_NS);
			break;
		case AF_CCITT:
			printf(
					"tsap_connect: Cannot connect X.25 while in a connection-less oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_NATM:
			if (subnet != NSAP_SUBNET_ATM) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_ATM, NSAP_CLASS_CLNS,
			AF_NATM);
			break;
		case AF_IPX:
			if (subnet != NSAP_SUBNET_IPX) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_IPX, NSAP_CLASS_CLNS,
			AF_IPX);
			break;
		case AF_SNA:
			if (subnet != NSAP_SUBNET_SNA) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_SNA, NSAP_CLASS_CLNS,
			AF_SNA);
			break;
		default:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN,
			NSAP_CLASS_CLNS, AF_UNSPEC);
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
tsap_disconnect(struct sockaddr_nsap *snsap, long subnet, int class, int af)
{
	if (snsap == NULL) {
		printf("tsap_disconnect: tsap is empty or has already been disconnected\n");
		return;
	}
	switch (class) {
	case NSAP_CLASS_CONS: {
		switch (af) {
		case AF_INET:
			printf(
					"tsap_disconnect: Cannot disconnect IPV4 while in a connection oriented mode\n");
			break;
		case AF_INET6:
			printf(
					"tsap_disconnect: Cannot disconnect IPV6 while in a connection oriented mode\n");
			break;
		case AF_NS:
			printf(
					"tsap_disconnect: Cannot disconnect IDP while in a connection oriented mode\n");
			break;
		case AF_ISO:
			if (subnet != NSAP_SUBNET_CONS) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_CONS, AF_ISO);
			break;
		case AF_CCITT:
			if (subnet != NSAP_SUBNET_X25) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_X25, AF_CCITT);
			break;
		case AF_NATM:
			printf(
					"tsap_disconnect: Cannot disconnect ATM while in a connection oriented mode\n");
			break;
		case AF_IPX:
			printf(
					"tsap_disconnect: Cannot disconnect IPX while in a connection oriented mode\n");
			break;
		case AF_SNA:
			printf(
					"tsap_disconnect: Cannot disconnect SNA while in a connection oriented mode\n");
			break;
		default:
			nsap_disconnect(snsap, NSAP_SUBNET_UNKNOWN, AF_UNSPEC);
			break;
		}
		break;
	}
	case NSAP_CLASS_CLNS: {
		switch (af) {
		case AF_INET:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				break;
			}
			nsap_disconnect(snsap, subnet, AF_INET);
			break;
		case AF_INET6:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				break;
			}
			nsap_disconnect(snsap, subnet, AF_INET6);
			break;
		case AF_ISO:
			if ((subnet != NSAP_SUBNET_CLNS) || (subnet != NSAP_SUBNET_CLNP)
					|| (subnet != NSAP_SUBNET_ISIS)
					|| (subnet != NSAP_SUBNET_ESIS)) {
				break;
			}
			nsap_disconnect(snsap, subnet, AF_ISO);
			break;
		case AF_NS:
			if (subnet != NSAP_SUBNET_IDP) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_IDP, AF_NS);
			break;
		case AF_CCITT:
			printf(
					"tsap_disconnect: Cannot disconnect X.25 while in a connection-less oriented mode\n");
			break;
		case AF_NATM:
			if (subnet != NSAP_SUBNET_ATM) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_ATM, AF_NATM);
			break;
		case AF_IPX:
			if (subnet != NSAP_SUBNET_IPX) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_IPX, AF_IPX);
			break;
		case AF_SNA:
			if (subnet != NSAP_SUBNET_SNA) {
				break;
			}
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
