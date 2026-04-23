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

static void
nsap_type(struct sockaddr_nsap *snsap, int type)
{
	if (snsap == NULL) {
		return;
	}

	switch (type) {
	case NSAP_TYPE_SIN4:
		snsap->snsap_type = NSAP_TYPE_SIN4;
		break;
	case NSAP_TYPE_SIN6:
		snsap->snsap_type = NSAP_TYPE_SIN6;
		break;
	case NSAP_TYPE_SNS:
		snsap->snsap_type = NSAP_TYPE_SNS;
		break;
	case NSAP_TYPE_SISO:
		snsap->snsap_type = NSAP_TYPE_SISO;
		break;
	case NSAP_TYPE_SX25:
		snsap->snsap_type = NSAP_TYPE_SX25;
		break;
	case NSAP_TYPE_SATM:
		snsap->snsap_type = NSAP_TYPE_SATM;
		break;
	case NSAP_TYPE_SIPX:
		snsap->snsap_type = NSAP_TYPE_SIPX;
		break;
	case NSAP_TYPE_SSNA:
		snsap->snsap_type = NSAP_TYPE_SSNA;
		break;
	case NSAP_TYPE_UNKNOWN:
		/* FALLTHROUGH */
	default:
		snsap->snsap_type = NSAP_TYPE_UNKNOWN;
		break;
	}
}

static void
nsap_subnet(struct sockaddr_nsap *snsap, int subnet)
{
	if (snsap == NULL) {
		return;
	}

	switch (subnet) {
	case NSAP_SUBNET_IPV4:
		snsap->snsap_subnet = NSAP_SUBNET_IPV4;
		break;
	case NSAP_SUBNET_IPV6:
		snsap->snsap_subnet = NSAP_SUBNET_IPV6;
		break;
	case NSAP_SUBNET_IDP:
		snsap->snsap_subnet = NSAP_SUBNET_IDP;
		break;
	case NSAP_SUBNET_CONS:
		snsap->snsap_subnet = NSAP_SUBNET_CONS;
		break;
	case NSAP_SUBNET_CLNS:
		snsap->snsap_subnet = NSAP_SUBNET_CLNS;
		break;
	case NSAP_SUBNET_CLNP:
		snsap->snsap_subnet = NSAP_SUBNET_CLNP;
		break;
	case NSAP_SUBNET_ISIS:
		snsap->snsap_subnet = NSAP_SUBNET_ISIS;
		break;
	case NSAP_SUBNET_ESIS:
		snsap->snsap_subnet = NSAP_SUBNET_ESIS;
		break;
	case NSAP_SUBNET_X25:
		snsap->snsap_subnet = NSAP_SUBNET_X25;
		break;
	case NSAP_SUBNET_ATM:
		snsap->snsap_subnet = NSAP_SUBNET_ATM;
		break;
	case NSAP_SUBNET_IPX:
		snsap->snsap_subnet = NSAP_SUBNET_IPX;
		break;
	case NSAP_SUBNET_SNA:
		snsap->snsap_subnet = NSAP_SUBNET_SNA;
		break;
	case NSAP_SUBNET_UNKNOWN:
		/* FALLTHROUGH */
	default:
		snsap->snsap_subnet = NSAP_SUBNET_UNKNOWN;
		break;
	}
}

struct nsap_iso *
nsap_alloc(long type, long subnet)
{
	struct nsap_iso *nsap;

	MALLOC(nsap, struct nsap_iso *, sizeof(*nsap), M_ISOSAP, M_WAITOK);
	if (nsap == NULL) {
		return (NULL);
	}
	bzero((caddr_t)nsap, sizeof(*nsap));
	nsap->nsi_type_id = nsap_type_id(type);
	nsap->nsi_subnet_id = nsap_subnet_id(subnet);
	return (nsap);
}

void
nsap_free(struct nsap_iso *nsap)
{
	if (nsap != NULL) {
		FREE(nsap, M_ISOSAP);
	}
}

uint32_t
nsap_id_hash(uint32_t x, int len)
{
	return (enhanced_double_hash(x, len));
}

/*
 * nsap type identifier:
 * returns 0 if type is unknown or max
 */
uint32_t
nsap_type_id(long type)
{
	uint32_t type_id;
	int len;

	len = ISOLEN;
	if ((type <= NSAP_TYPE_UNKNOWN) || (type >= NSAP_TYPE_MAX)) {
		len = 1;
	}
	type_id = nsap_id_hash(type, len);
	return (type_id);
}

/*
 * nsap subnet identifier:
 * returns 0 if subnet is unknown or max
 */
uint32_t
nsap_subnet_id(long subnet)
{
	uint32_t subnet_id;
	int len;

	len = ISOLEN;
	if ((subnet <= NSAP_SUBNET_UNKNOWN) || (subnet >= NSAP_SUBNET_MAX)) {
		len = 1;
	}
	subnet_id = nsap_id_hash(subnet, len);
	return (subnet_id);
}

void
nsap_setsockaddr(struct sockaddr_nsap *snsap, void *arg, long type, long subnet)
{
	if (snsap == NULL) {
		return;
	}

	switch (type) {
	case NSAP_TYPE_SIN4:
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)arg;
		if (sin != NULL) {
			snsap->snsap_sin4 = *sin;
			nsap_type(snsap, NSAP_TYPE_SIN4);
			switch (subnet) {
			case NSAP_SUBNET_IPV4:
				nsap_subnet(snsap, NSAP_SUBNET_IPV4);
				break;
			case NSAP_SUBNET_IPV6:
				nsap_subnet(snsap, NSAP_SUBNET_IPV4);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SIN6:
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)arg;
		if (sin6 != NULL) {
			snsap->snsap_sin6 = *sin6;
			nsap_type(snsap, NSAP_TYPE_SIN6);
			switch (subnet) {
			case NSAP_SUBNET_IPV4:
				/* FALLTHROUGH */
			case NSAP_SUBNET_IPV6:
				nsap_subnet(snsap, NSAP_SUBNET_IPV6);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SNS:
	{
		struct sockaddr_ns *sns = (struct sockaddr_ns *)arg;
		if (sns != NULL) {
			snsap->snsap_sns = *sns;
			nsap_type(snsap, NSAP_TYPE_SNS);
			nsap_subnet(snsap, NSAP_SUBNET_IDP);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SISO:
	{
		struct sockaddr_iso *siso = (struct sockaddr_iso *)arg;
		if (siso != NULL) {
			snsap->snsap_siso = *siso;
			nsap_type(snsap, NSAP_TYPE_SISO);
			switch (subnet) {
			case NSAP_SUBNET_CONS:
				nsap_subnet(snsap, NSAP_SUBNET_CONS);
				break;
			case NSAP_SUBNET_CLNS:
				nsap_subnet(snsap, NSAP_SUBNET_CLNS);
				break;
			case NSAP_SUBNET_CLNP:
				nsap_subnet(snsap, NSAP_SUBNET_CLNP);
				break;
			case NSAP_SUBNET_ISIS:
				nsap_subnet(snsap, NSAP_SUBNET_ISIS);
				break;
			case NSAP_SUBNET_ESIS:
				nsap_subnet(snsap, NSAP_SUBNET_ESIS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SX25:
	{
		struct sockaddr_x25 *sx25 = (struct sockaddr_x25 *)arg;
		if (sx25 != NULL) {
			snsap->snsap_sx25 = *sx25;
			nsap_type(snsap, NSAP_TYPE_SX25);
			nsap_subnet(snsap, NSAP_SUBNET_X25);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SATM:
	case NSAP_TYPE_SIPX:
	case NSAP_TYPE_SSNA:
	case NSAP_TYPE_UNKNOWN:
unknown:
		/* FALLTHROUGH */
	default:
		nsap_type(snsap, NSAP_TYPE_UNKNOWN);
		nsap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
		break;
	}
}

/*
 * Only ISO/OSI based services have currently been defined.
 */
void
nsap_service(struct nsap_addr *nsapa, char *addr, u_char addrlen, int class)
{
	if (nsapa != NULL) {
		switch (class) {
		case NSAP_CLASS_CONS:	/* connection oriented */
			bcopy(addr, nsapa->nsapa_service_addr, sizeof(nsapa->nsapa_service_addr));
			nsapa->nsapa_service_addrlen = addrlen;
			nsapa->nsapa_service_class = NSAP_CLASS_CONS;
			break;
		case NSAP_CLASS_CLNS: /* connection-less oriented  */
			bcopy(addr, nsapa->nsapa_service_addr, sizeof(nsapa->nsapa_service_addr));
			nsapa->nsapa_service_addrlen = addrlen;
			nsapa->nsapa_service_class = NSAP_CLASS_CLNS;
			break;
		case NSAP_CLASS_UNKNOWN: /* everything else */
unknown:
			/* FALLTHROUGH */
		default:
			bcopy(addr, nsapa->nsapa_service_addr, sizeof(nsapa->nsapa_service_addr));
			nsapa->nsapa_service_addrlen = addrlen;
			nsapa->nsapa_service_class = NSAP_CLASS_UNKNOWN;
			break;
		}
	} else {
		goto unknown;
	}
}

void
nsap_setaddr(struct nsap_addr *nsapa, void *arg, long type, int class)
{
	if (nsapa == NULL) {
		return;
	}

	switch (type) {
	case NSAP_TYPE_SIN4:
	{
		struct in_addr *in4 = (struct in_addr *)arg;
		if (in4 != NULL) {
			nsapa->nsapa_in4 = *in4;
			nsap_service(nsapa, (struct in_addr *)in4, sizeof(*in4), NSAP_CLASS_CLNS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SIN6:
	{
		struct in6_addr *in6 = (struct in6_addr *)arg;
		if (in6 != NULL) {
			nsapa->nsapa_in6 = *in6;
			nsap_service(nsapa, (struct in6_addr *)in6, sizeof(*in6), NSAP_CLASS_CLNS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SNS:
	{
		struct ns_addr *ns = (struct ns_addr *)arg;
		if (ns != NULL) {
			nsapa->nsapa_ns = *ns;
			nsap_service(nsapa, (struct ns_addr *)ns, sizeof(*ns), NSAP_CLASS_CLNS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SISO:
	{
		struct iso_addr *iso = (struct iso_addr *)arg;
		if (iso != NULL) {
			nsapa->nsapa_iso = *iso;
			switch (class) {
			case NSAP_CLASS_CONS:
				nsap_service(nsapa, (struct iso_addr *)iso, sizeof(*iso), NSAP_CLASS_CONS);
				break;
			case NSAP_CLASS_CLNS:
				nsap_service(nsapa, (struct iso_addr *)iso, sizeof(*iso), NSAP_CLASS_CLNS);
				break;
			case NSAP_CLASS_UNKNOWN:
				goto unknown;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SX25:
	{
		struct x25_addr *x25 = (struct x25_addr *)arg;
		if (x25 != NULL) {
			nsapa->nsapa_x25 = *x25;
			nsap_service(nsapa, (struct x25_addr *)x25, sizeof(*x25), NSAP_CLASS_CONS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SATM:
	case NSAP_TYPE_SIPX:
	case NSAP_TYPE_SSNA:
	case NSAP_TYPE_UNKNOWN:
unknown:
		/* FALLTHROUGH */
	default:
		nsap_service(nsapa, NULL, NULL, NSAP_CLASS_UNKNOWN);
		break;
	}
}

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
		nsap_setsockaddr(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4);
		if (nam->m_len != sizeof(sin)) {
			return (EINVAL);
		}
		if (sin->sin_family != AF_INET) {
			return (EAFNOSUPPORT);
		}
		if (sin->sin_port == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setaddr(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_CLNS);
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6);
		nsap_setsockaddr(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6);
		if (nam->m_len != sizeof(sin6)) {
			return (EINVAL);
		}
		if (sin6->sin6_family != AF_INET6) {
			return (EAFNOSUPPORT);
		}
		if (sin6->sin6_port == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setaddr(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_CLNS);
		break;
	}
	case AF_ISO:
	{
		struct sockaddr_iso *siso = mtod(nam, struct sockaddr_iso);
		nsap_setsockaddr(snsap, (struct sockaddr_iso *)siso, NSAP_TYPE_SISO, subnet);
		if (nam->m_len != sizeof(siso)) {
			return (EINVAL);
		}
		if (siso->siso_family != AF_ISO) {
			return (EAFNOSUPPORT);
		}
		if (siso->siso_nlen == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setaddr(&snsap->snsap_addr, (struct iso_addr *)&siso->siso_addr, NSAP_TYPE_SISO, class);
		break;
	}
	case AF_NS:
	{
		struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns);
		nsap_setsockaddr(snsap, (struct sockaddr_ns *)sns, NSAP_TYPE_SNS, NSAP_SUBNET_IDP);
		if (nam->m_len != sizeof(sns)) {
			return (EINVAL);
		}
		if (sns->sns_family != AF_NS) {
			return (EAFNOSUPPORT);
		}
		if (sns->sns_port == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setaddr(&snsap->snsap_addr, (struct ns_addr *)&sns->sns_addr, NSAP_TYPE_SNS, NSAP_CLASS_CLNS);
		break;
	}
	case AF_CCITT:
	{
		struct sockaddr_x25 *sx25 = mtod(nam, struct sockaddr_x25);
		nsap_setsockaddr(snsap, (struct sockaddr_x25 *)sx25, NSAP_TYPE_SX25, NSAP_SUBNET_X25);
		if (nam->m_len != sizeof(sx25)) {
			return (EINVAL);
		}
		if (sx25->x25_family != AF_CCITT) {
			return (EAFNOSUPPORT);
		}
		if (sx25->x25_len == 0) {
			return (EADDRNOTAVAIL);
		}
		nsap_setaddr(&snsap->snsap_addr, (struct x25_addr *)&sx25->x25_addr, NSAP_TYPE_SX25, NSAP_CLASS_CONS);
		break;
	}
	case AF_NATM:
	case AF_IPX:
	case AF_SNA:
	default:
		nsap_setsockaddr(snsap, NULL, NSAP_TYPE_UNKNOWN, NSAP_SUBNET_UNKNOWN);
		nsap_setaddr(&snsap->snsap_addr, NULL, NSAP_TYPE_UNKNOWN, NSAP_CLASS_UNKNOWN);
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
				nsap_setsockaddr(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4);
				nsap_setaddr(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_IPV6:
				nsap_setsockaddr(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_IPV6);
				nsap_setaddr(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_setsockaddr(snsap, (struct sockaddr_in *)sin, NSAP_TYPE_SIN4, NSAP_SUBNET_UNKNOWN);
				nsap_setaddr(&snsap->snsap_addr, (struct in_addr *)&sin->sin_addr, NSAP_TYPE_SIN4, NSAP_CLASS_UNKNOWN);
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
				nsap_setsockaddr(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_IPV4);
				nsap_setaddr(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_IPV6:
				nsap_setsockaddr(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6);
				nsap_setaddr(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_setsockaddr(snsap, (struct sockaddr_in6 *)sin6, NSAP_TYPE_SIN6, NSAP_SUBNET_UNKNOWN);
				nsap_setaddr(&snsap->snsap_addr, (struct in6_addr *)&sin6->sin6_addr, NSAP_TYPE_SIN6, NSAP_CLASS_UNKNOWN);
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
				nsap_setsockaddr(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_CONS);
				nsap_setaddr(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CONS);
				break;
			case NSAP_SUBNET_CLNS:
				nsap_setsockaddr(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_CLNS);
				nsap_setaddr(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_CLNP:
				nsap_setsockaddr(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_CLNP);
				nsap_setaddr(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_ISIS:
				nsap_setsockaddr(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_ISIS);
				nsap_setaddr(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_ESIS:
				nsap_setsockaddr(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_ESIS);
				nsap_setaddr(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_CLNS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				nsap_setsockaddr(snsap, (struct sockaddr_iso *)sisos, NSAP_TYPE_SISO, NSAP_SUBNET_UNKNOWN);
				nsap_setaddr(&snsap->snsap_addr, (struct iso_addr *)&sisos->siso_addr, NSAP_TYPE_SISO, NSAP_CLASS_UNKNOWN);
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
				nsap_setsockaddr(snsap, (struct sockaddr_ns *)sns, NSAP_TYPE_SNS, NSAP_SUBNET_IDP);
				nsap_setaddr(&snsap->snsap_addr, (struct ns_addr *)&sns->sns_addr, NSAP_TYPE_SX25, NSAP_CLASS_CLNS);
			} else {
				nsap_setsockaddr(snsap, (struct sockaddr_ns *)sns, NSAP_TYPE_SNS, NSAP_SUBNET_UNKNOWN);
				nsap_setaddr(&snsap->snsap_addr, (struct ns_addr *)&sns->sns_addr, NSAP_TYPE_SNS, NSAP_CLASS_UNKNOWN);
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
				nsap_setsockaddr(snsap, (struct sockaddr_x25 *)sx25, NSAP_TYPE_SX25, NSAP_SUBNET_X25);
				nsap_setaddr(&snsap->snsap_addr, (struct x25_addr *)&sx25->x25_addr, NSAP_TYPE_SX25, NSAP_CLASS_CONS);
			} else {
				nsap_setsockaddr(snsap, (struct sockaddr_x25 *)sx25, NSAP_TYPE_SX25, NSAP_SUBNET_UNKNOWN);
				nsap_setaddr(&snsap->snsap_addr, (struct x25_addr *)&sx25->x25_addr, NSAP_TYPE_SX25, NSAP_CLASS_UNKNOWN);
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
		nsap_setsockaddr(snsap, NULL, NSAP_TYPE_UNKNOWN, NSAP_SUBNET_UNKNOWN);
		nsap_setaddr(&snsap->snsap_addr, NULL, NSAP_TYPE_UNKNOWN, NSAP_CLASS_UNKNOWN);
		break;
	}
}
