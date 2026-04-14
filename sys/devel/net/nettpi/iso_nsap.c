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

#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

void
nsap_setsockaddr(struct sockaddr_nsap *snsap, void *arg, int type, int subnet)
{
	if (snsap == NULL) {
		return;
	}

	switch (type) {
	case NSAP_TYPE_SIN4:
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)arg;
		if (sin != NULL) {
			sin->sin_family = AF_INET;
			sin->sin_len = sizeof(*sin);
			*snsap->snsap_sin4 = sin;
			snsap->snsap_type = NSAP_TYPE_SIN4;
			switch (subnet) {
			case NSAP_SUBNET_TCP:
				snsap->snsap_subnet = NSAP_SUBNET_TCP;
				break;
			case NSAP_SUBNET_UDP:
				snsap->snsap_subnet = NSAP_SUBNET_UDP;
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				snsap->snsap_subnet = NSAP_SUBNET_UNKNOWN;
				break;
			}
		}
		break;
	}
	case NSAP_TYPE_SIN6:
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)arg;
		if (sin6 != NULL) {
			sin6->sin6_family = AF_INET6;
			sin6->sin6_len = sizeof(*sin6);
			*snsap->snsap_sin6 = sin6;
			snsap->snsap_type = NSAP_TYPE_SIN6;
			switch (subnet) {
			case NSAP_SUBNET_TCP:
				snsap->snsap_subnet = NSAP_SUBNET_TCP;
				break;
			case NSAP_SUBNET_UDP:
				snsap->snsap_subnet = NSAP_SUBNET_UDP;
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				snsap->snsap_subnet = NSAP_SUBNET_UNKNOWN;
				break;
			}
		}
		break;
	}
	case NSAP_TYPE_SNS:
	{
		struct sockaddr_ns *sns = (struct sockaddr_ns *)arg;
		if (sns != NULL) {
			sns->sns_family = AF_NS;
			sns->sns_len = sizeof(*sns);
			*snsap->snsap_sns = sns;
			snsap->snsap_type = NSAP_TYPE_SNS;
			snsap->snsap_subnet = NSAP_SUBNET_IDP;
		}
		break;
	}
	case NSAP_TYPE_SISO:
	{
		struct sockaddr_iso *siso = (struct sockaddr_iso *)arg;
		if (siso != NULL) {
			siso->siso_family = AF_ISO;
			siso->siso_len = sizeof(*siso);
			*snsap->snsap_siso = siso;
			snsap->snsap_type = NSAP_TYPE_SISO;
			switch (subnet) {
			case NSAP_SUBNET_CONS:
				snsap->snsap_subnet = NSAP_SUBNET_CONS;
				break;
			case NSAP_SUBNET_CLNS:
				snsap->snsap_subnet = NSAP_SUBNET_CLNS;
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				snsap->snsap_subnet = NSAP_SUBNET_UNKNOWN;
				break;
			}
		}
		break;
	}
	case NSAP_TYPE_SX25:
	{
		struct sockaddr_x25 *sx25 = (struct sockaddr_x25 *)arg;
		if (sx25 != NULL) {
			sx25->x25_family = AF_CCITT;
			sx25->x25_len = sizeof(*sx25);
			*snsap->snsap_sx25 = sx25;
			snsap->snsap_type = NSAP_TYPE_SX25;
			snsap->snsap_subnet = NSAP_SUBNET_X25;
		}
		break;
	}
	case NSAP_TYPE_SATM:
	case NSAP_TYPE_SIPX:
	case NSAP_TYPE_SSNA:
	case NSAP_TYPE_UNKNOWN:
		/* FALLTHROUGH */
	default:
		snsap->snsap_type = NSAP_TYPE_UNKNOWN;
		snsap->snsap_subnet = NSAP_SUBNET_UNKNOWN;
		break;
	}
}

/*
 * Only ISO/OSI based services have currently been defined.
 * - All others fall into the class type of unknown (i.e. TCP, UDP, ATM, X25, IPX, SNA, etc)
 */
void
nsap_service(struct nsap_addr *nsapa, char *addr, u_char len, int class)
{
	if (nsapa != NULL) {
		nsapa->nsapa_addr = addr;
		nsapa->nsapa_len = len;
		nsapa->nsapa_class = class;
	}
}

void
nsap_setaddr(struct nsap_addr *nsapa, void *arg, int type, char *addr, u_char addrlen, int class)
{
	if (nsapa == NULL) {
		return;
	}

	switch (type) {
	case NSAP_TYPE_SIN4:
	{
		struct in_addr *in4 = (struct in_addr *)arg;
		if (in4 != NULL) {
			*nsapa->nsapa_in4 = in4;
			//nsap_service(addr, addrlen, NSAP_CLASS_UNKNOWN); /* not defined! */
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SIN6:
	{
		struct in6_addr *in6 = (struct in6_addr *)arg;
		if (in6 != NULL) {
			*nsapa->nsapa_in6 = in6;
			//nsap_service(addr, addrlen, NSAP_CLASS_UNKNOWN); /* not defined! */
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SNS:
	{
		struct ns_addr *ns = (struct ns_addr *)arg;
		if (ns != NULL) {
			*nsapa->nsapa_ns = ns;
			//nsap_service(addr, addrlen, NSAP_CLASS_UNKNOWN); /* not defined! */
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SISO:
	{
		struct iso_addr *iso = (struct iso_addr *)arg;
		if (iso != NULL) {
			*nsapa->nsapa_iso = iso;
			switch (class) {
			case NSAP_CLASS_CONS:
				nsap_service(addr, addrlen, NSAP_CLASS_CONS);
				break;
			case NSAP_CLASS_CLNS:
				nsap_service(addr, addrlen, NSAP_CLASS_CLNS);
				break;
			case NSAP_CLASS_UNKNOWN:
				/* FALLTHROUGH */
			default:
				goto unknown;
			}
		}
		break;
	}
	case NSAP_TYPE_SX25:
	{
		struct x25_addr *x25 = (struct x25_addr *)arg;
		if (x25 != NULL) {
			*nsapa->nsapa_x25 = x25;
			//nsap_service(addr, addrlen, NSAP_CLASS_UNKNOWN); /* not defined! */
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SATM:
	case NSAP_TYPE_SIPX:
	case NSAP_TYPE_SSNA:
	case NSAP_TYPE_UNKNOWN:
		/* FALLTHROUGH */
	default:
	unknown:
		nsap_service(NULL, NULL, NSAP_CLASS_UNKNOWN);
		break;
	}
}


