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

uint32_t nsap_valid_ids[NSAP_TYPE_MAX][NSAP_SUBNET_MAX];
static long nsap_id_cnt;

#define NSAPHASH(table, type, subnet) 	&(table)->nsit_hashtbl[nsap_id((type), (subnet))]

static void nsap_class(struct sap_service *, int);
static void nsap_type(struct sockaddr_nsap *, long);
static void nsap_subnet(struct sockaddr_nsap *, long);
static uint32_t nsap_id_hash(uint32_t, int);
static int nsap_id_compare(uint32_t, uint32_t);
static void nsap_rehash(struct nsapisotable *, struct nsap_iso *, long, long);
static void nsap_insert(struct nsapisotable *, struct nsap_iso *, long, long);
static void nsap_remove(struct nsap_iso *);
static void nsap_setup_id_table(void);

void
nsap_init(struct nsapisotable *table)
{
	table->nsit_hashtbl = hashinit(ISOLEN, M_ISOSAP, &table->nsit_hash);
	nsap_setup_id_table();
}

struct nsap_iso *
nsap_alloc(struct nsapisotable *table, long type, long subnet)
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

void
nsap_set_nsap_id(struct nsapisotable *table, long type, long subnet)
{
	table->nsit_id = nsap_valid_ids[type][subnet];
}

void
nsap_attach(struct nsapisotable *table, struct nsap_iso *naddr, long type, long subnet)
{
	struct nsap_iso *nsap;
	struct nsapisohead *head;

	nsap = nsap_alloc(table, type, subnet);
	if (nsap == NULL) {
		return;
	}
	nsap_set_nsap_id(table, type, subnet);
	nsap_insert(table, nsap, type, subnet);
	naddr = nsap;
}

void
nsap_detach(struct nsapisotable *table, struct nsap_iso *nsap, long type, long subnet)
{
	struct nsapisohead *head;

	if (table->nsit_id != nsap_valid_ids[type][subnet]) {
		return;
	}

	head = NSAPHASH(table, type, subnet);
	if (LIST_EMPTY(head)) {
		nsap_free(nsap);
		return;
	}
	if (nsap != NULL) {
		nsap_remove(nsap);
	}
}

static void
nsap_class(struct sap_service *service, int clazz)
{
	if (service == NULL) {
		return;
	}
	service->ns_class = sap_class_select(clazz);
}

static void
nsap_type(struct sockaddr_nsap *snsap, long type)
{
	if (snsap == NULL) {
		return;
	}
	snsap->snsap_type = sap_type_select(type);
}

static void
nsap_subnet(struct sockaddr_nsap *snsap, long subnet)
{
	if (snsap == NULL) {
		return;
	}
	snsap->snsap_subnet = sap_subnet_select(subnet);
}

static uint32_t
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

/*
 * nsap identifier:
 * - generates a nsap_id from the nsap_type_id and nsap_subnet_id.
 * - returns 0 if either nsap_type_id or nsap_subnet_id equals 0
 */
uint32_t
nsap_id(long type, long subnet)
{
	uint32_t type_id, subnet_id, hashid, nsap_id;
	int len;

	len = ISOLEN;
	type_id = nsap_type_id(type);
	subnet_id = nsap_subnet_id(subnet);
	if ((type_id == 0) || (subnet_id == 0)) {
		len = 1;
	}
	hashid = ((type_id + (subnet_id + (len-1))) - len);
	nsap_id = nsap_id_hash(hashid, len);
	return (tsap_id);
}

static int
nsap_id_compare(uint32_t a, uint32_t b)
{
	if (a != b) {
		return (1);
	}
	return (0);
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
nsap_service(struct sap_service *service, char *addr, u_char addrlen, int class)
{
	switch (class) {
	case NSAP_CLASS_CONS: /* connection oriented */
		bcopy(addr, service->ns_addr, sizeof(service->ns_addr));
		service->ns_addrlen = addrlen;
		nsap_class(service, NSAP_CLASS_CONS);
		break;
	case NSAP_CLASS_CLNS: /* connection-less oriented  */
		bcopy(addr, service->ns_addr, sizeof(service->ns_addr));
		service->ns_addrlen = addrlen;
		nsap_class(service, NSAP_CLASS_CLNS);
		break;
	case NSAP_CLASS_UNKNOWN: /* everything else */
		/* FALLTHROUGH */
	default:
		bcopy(addr, service->ns_addr, sizeof(service->ns_addr));
		service->ns_addrlen = addrlen;
		nsap_class(service, NSAP_CLASS_UNKNOWN);
		break;
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
			nsap_service(&nsapa->nsapa_service, (struct in_addr *)in4, sizeof(*in4), NSAP_CLASS_CLNS);
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
			nsap_service(&nsapa->nsapa_service, (struct in6_addr *)in6, sizeof(*in6), NSAP_CLASS_CLNS);
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
			nsap_service(&nsapa->nsapa_service, (struct ns_addr *)ns, sizeof(*ns), NSAP_CLASS_CLNS);
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
				nsap_service(&nsapa->nsapa_service, (struct iso_addr *)iso, sizeof(*iso), NSAP_CLASS_CONS);
				break;
			case NSAP_CLASS_CLNS:
				nsap_service(&nsapa->nsapa_service, (struct iso_addr *)iso, sizeof(*iso), NSAP_CLASS_CLNS);
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
			nsap_service(&nsapa->nsapa_service, (struct x25_addr *)x25, sizeof(*x25), NSAP_CLASS_CONS);
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
		nsap_service(&nsapa->nsapa_service, NULL, NULL, NSAP_CLASS_UNKNOWN);
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

struct nsap_iso *
nsap_lookup(struct nsapisotable *table, struct sockaddr_nsap *snsap, struct nsap_addr *nsapa, long type, long subnet, int class)
{
	struct nsapisohead *head;
	struct nsap_iso *nsap;
	struct nsap_addr *snsapa;

	if (nsap_valid_ids[type][subnet] == 0) {
		goto unknown;
	}

	head = NSAPHASH(table, type, subnet);
	LIST_FOREACH(nsap, head, nsi_hash) {
		if (nsap_id_compare(nsap->nsi_type_id, nsap_type_id(type)) != 0) {
			continue;
		}
		if (nsap_id_compare(nsap->nsi_subnet_id, nsap_subnet_id(subnet)) != 0) {
			continue;
		}
		/* get sockaddr_nsap */
		if (nsap->nsi_snsap == snsap) {
			/* get nsap_addr */
			snsapa = &nsap->nsi_snsap->snsap_addr;
			if (snsapa == nsapa) {
				/* check service_class against class */
				if (snsapa->nsapa_service_class == class) {
					/* check type and subnet against class */
					switch (snsapa->nsapa_service_class) {
					case NSAP_CLASS_CONS:
						if ((type != (NSAP_TYPE_SISO | NSAP_TYPE_SX25))
								&& (subnet != (NSAP_SUBNET_CONS | NSAP_SUBNET_X25))) {
							goto unknown;
						}
						break;
					case NSAP_CLASS_CLNS:
						if ((type == (NSAP_TYPE_SISO | NSAP_TYPE_SX25))
								&& (subnet == (NSAP_SUBNET_CONS | NSAP_SUBNET_X25))) {
							goto unknown;
						}
						break;
					}
				}
			}
			/* validate type */
			if (snsap->snsap_type == type) {
				if ((nsap_id_compare(nsap->nsi_type_id, nsap_type_id(snsap->snsap_type)) != 0)
						|| (nsap_id_compare(nsap->nsi_type_id, nsap_type_id(type)) != 0)) {
					goto unknown;
				}
			}
			/* validate subnet */
			if (snsap->snsap_subnet == subnet) {
				if ((nsap_id_compare(nsap->nsi_subnet_id, nsap_subnet_id(snsap->snsap_subnet)) != 0)
						|| (nsap_id_compare(nsap->nsi_subnet_id, nsap_subnet_id(subnet)) != 0)) {
					goto unknown;
				}
			}
			goto out;
		}
	}

unknown:
	return (NULL);

out:
	nsap_rehash(table, nsap, type, subnet);
	return (nsap);
}

static void
nsap_rehash(struct nsapisotable *table, struct nsap_iso *nsap, long type, long subnet)
{
	struct nsapisohead *head;

	head = NSAPHASH(table, type, subnet);
	if (nsap != LIST_FIRST(head)) {
		LIST_REMOVE(nsap, nsi_hash);
		LIST_INSERT_HEAD(head, nsap, nsi_hash);
	}
}

static void
nsap_insert(struct nsapisotable *table, struct nsap_iso *nsap, long type, long subnet)
{
	struct nsapisohead *head;

	head = NSAPHASH(table, type, subnet);
	LIST_INSERT_HEAD(head, nsap, nsi_hash);
}

static void
nsap_remove(struct nsap_iso *nsap)
{
	LIST_REMOVE(nsap, nsi_hash);
}

static void
nsap_setup_id_table(void)
{
    long type, subnet, cnt;
    uint32_t nsap_ids[NSAP_TYPE_MAX][NSAP_SUBNET_MAX];

    type = subnet = cnt = 0;

    /* zeroed out nsap_id table */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
        	nsap_ids[type][subnet] = 0;
        }
    }

    /* add nsap_id's to table */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
            nsap_ids[NSAP_TYPE_SIN4][NSAP_SUBNET_IPV4] = nsap_id(NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4);
            nsap_ids[NSAP_TYPE_SIN4][NSAP_SUBNET_IPV6] = nsap_id(NSAP_TYPE_SIN4, NSAP_SUBNET_IPV6); /* nsap id should match sin6 with ipv4 */
            nsap_ids[NSAP_TYPE_SIN6][NSAP_SUBNET_IPV4] = nsap_id(NSAP_TYPE_SIN6, NSAP_SUBNET_IPV4); /* nsap id should match sin4 with ipv6 */
            nsap_ids[NSAP_TYPE_SIN6][NSAP_SUBNET_IPV6] = nsap_id(NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6);
            nsap_ids[NSAP_TYPE_SNS][NSAP_SUBNET_IDP] = nsap_id(NSAP_TYPE_SNS, NSAP_SUBNET_IDP);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CONS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CONS);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CLNS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CLNS);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CLNP] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CLNP);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_ISIS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_ISIS);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_ESIS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_ESIS);
            nsap_ids[NSAP_TYPE_SX25][NSAP_SUBNET_X25] = nsap_id(NSAP_TYPE_SX25, NSAP_SUBNET_X25);
            nsap_ids[NSAP_TYPE_SATM][NSAP_SUBNET_ATM] = nsap_id(NSAP_TYPE_SATM, NSAP_SUBNET_ATM);
            nsap_ids[NSAP_TYPE_SIPX][NSAP_SUBNET_IPX] = nsap_id(NSAP_TYPE_SIPX, NSAP_SUBNET_IPX);
            nsap_ids[NSAP_TYPE_SSNA][NSAP_SUBNET_SNA] = nsap_id(NSAP_TYPE_SSNA, NSAP_SUBNET_SNA);
        }
    }

    /* remove invalid nsap_ids from table (i.e. any with 0) */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
            if (nsap_ids[type][subnet] != 0) {
            	nsap_valid_ids[type][subnet] = nsap_ids[type][subnet];
                cnt++;
            }
        }
    }
    nsap_id_cnt = cnt;
}
