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

#define TSAPHASH_LOCAL(tsiso, type, subnet) 	&(tsiso)->tsi_localhashtbl[tsap_id((type), (subnet))]
#define TSAPHASH_FOREIGN(tsiso, type, subnet) 	&(tsiso)->tsi_foriegnhashtbl[tsap_id((type), (subnet))]

uint32_t tsap_valid_ids[NSAP_TYPE_MAX][NSAP_SUBNET_MAX];
static long tsap_id_cnt;

static struct nsap_iso *tsap_hashlookup(struct tsap_iso *, struct sockaddr_nsap *, struct nsap_addr *, long, long, int, int);
static void tsap_rehash(struct tsap_iso *, struct nsap_iso *, long, long, int);
static void tsap_insert(struct tsap_iso *, struct nsap_iso *, long, long, int);
static void tsap_remove(struct nsap_iso *, long, long, int);
static void tsap_setup_tsap_id_table(void);

/* TSAP's */
void
tsap_init(struct tsap_iso *tsap)
{
	tsap->tsi_foriegnhashtbl = hashinit(ISOLEN, M_PCB, &tsap->tsi_foreignhash);
	tsap->tsi_localhashtbl = hashinit(ISOLEN, M_PCB, &tsap->tsi_localhash);
	tsap_setup_tsap_id_table();
}

/*
 * tsap identifier:
 * - generates a tsap_id from the nsap_type_id and nsap_subnet_id.
 * - returns 0 if either nsap_type_id or nsap_subnet_id equals 0
 */
uint32_t
tsap_id(long type, long subnet)
{
	uint32_t type_id, subnet_id, hashid, tsap_id;
	int len;

	len = ISOLEN;
	type_id = nsap_type_id(type);
	subnet_id = nsap_subnet_id(subnet);
	if ((type_id == 0) || (subnet_id == 0)) {
		len = 1;
	}
	hashid = ((type_id + (subnet_id + (len-1))) - len);
	tsap_id = nsap_id_hash(hashid, len);
	return (tsap_id);
}

int
tsap_alloc(struct tsap_iso *tsap, long type, long subnet)
{
	struct nsap_iso *nsap;

	nsap = nsap_alloc(type, subnet);
	if (nsap == NULL) {
		return (ENOBUFS);
	}
	tsap_init_tsap_id(type, subnet);
	if (tsap->tsi_id == 0) {
		nsap_free(nsap);
		return (EAFNOSUPPORT);
	}
	tsap->tsi_nsaps = nsap;
	tsap_insert(tsap, nsap, type, subnet, TSAPLOOKUP_LOCAL);
	tsap_insert(tsap, nsap, type, subnet, TSAPLOOKUP_FOREIGN);
	return (0);
}

void
tsap_init_tsap_id(struct tsap_iso *tsap, long type, long subnet)
{
	tsap->tsi_id = tsap_valid_ids[type][subnet];
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

struct nsap_iso *
tsap_lookup_foreign(struct tsap_iso *tsap, struct sockaddr_nsap *snsap, struct nsap_addr *nsapa, long type, long subnet, int class)
{
	/* validate tsap id */
	if (tsap->tsi_id != tsap_valid_ids[type][subnet]) {
		return (NULL);
	}
	return (tsap_hashlookup(tsap, snsap, nsapa, type, subnet, class, TSAPLOOKUP_FOREIGN));
}

struct nsap_iso *
tsap_lookup_local(struct tsap_iso *tsap, struct sockaddr_nsap *snsap, struct nsap_addr *nsapa, long type, long subnet, int class)
{
	/* validate tsap id */
	if (tsap->tsi_id != tsap_valid_ids[type][subnet]) {
		return (NULL);
	}
	return (tsap_hashlookup(tsap, snsap, nsapa, type, subnet, class, TSAPLOOKUP_LOCAL));
}

static struct nsap_iso *
tsap_hashlookup(struct tsap_iso *tsap, struct sockaddr_nsap *snsap, struct nsap_addr *nsapa, long type, long subnet, int class, int which)
{
	struct nsapisohead *head;
	struct nsap_iso *nsap;
	struct nsap_addr *snsapa;

	switch (which) {
	case TSAPLOOKUP_FOREIGN:
		head = TSAPHASH_FOREIGN(tsap, type, subnet);
		LIST_FOREACH(nsap, head, nsi_fhash) {
			if (nsap->nsi_type_id != nsap_type_id(type)) {
				continue;
			}
			if (nsap->nsi_subnet_id != nsap_subnet_id(subnet)) {
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
				/* validate type and subnet */
				if (snsap->snsap_type == type) {
					if ((nsap->nsi_type_id != nsap_type_id(snsap->snsap_type))
							|| (nsap->nsi_type_id != nsap_type_id(type))) {
						goto unknown;
					}
				}
				if (snsap->snsap_subnet == subnet) {
					if ((nsap->nsi_subnet_id
							!= nsap_subnet_id(snsap->snsap_subnet))
							|| (nsap->nsi_subnet_id != nsap_subnet_id(subnet))) {
						goto unknown;
					}
				}
				goto out;
			}
		}
		break;

	case TSAPLOOKUP_LOCAL:
		head = TSAPHASH_LOCAL(tsap, type, subnet);
		LIST_FOREACH(nsap, head, nsi_lhash) {
			if (nsap->nsi_type_id != nsap_type_id(type)) {
				continue;
			}
			if (nsap->nsi_subnet_id != nsap_subnet_id(subnet)) {
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
				/* validate type and subnet */
				if (snsap->snsap_type == type) {
					if ((nsap->nsi_type_id != nsap_type_id(snsap->snsap_type))
							|| (nsap->nsi_type_id != nsap_type_id(type))) {
						goto unknown;
					}
				}
				if (snsap->snsap_subnet == subnet) {
					if ((nsap->nsi_subnet_id != nsap_subnet_id(snsap->snsap_subnet))
							|| (nsap->nsi_subnet_id != nsap_subnet_id(subnet))) {
						goto unknown;
					}
				}
				goto out;
			}
		}
		break;
	}

unknown:
	return (NULL);

out:
	tsap_rehash(tsap, nsap, type, subnet, which);
	return (nsap);
}

static void
tsap_rehash(struct tsap_iso *tsap, struct nsap_iso *nsap, long type, long subnet, int which)
{
	struct nsapisohead *head;

	switch (which) {
	case TSAPLOOKUP_FOREIGN:
		head = TSAPHASH_FOREIGN(tsap, type, subnet);
		if (nsap != LIST_FIRST(head)) {
			LIST_REMOVE(nsap, nsi_fhash);
			LIST_INSERT_HEAD(head, nsap, nsi_fhash);
		}
		break;
	case TSAPLOOKUP_LOCAL:
		head = TSAPHASH_LOCAL(tsap, type, subnet);
		if (nsap != LIST_FIRST(head)) {
			LIST_REMOVE(nsap, nsi_lhash);
			LIST_INSERT_HEAD(head, nsap, nsi_lhash);
		}
		break;
	}
}

static void
tsap_insert(struct tsap_iso *tsap, struct nsap_iso *nsap, long type, long subnet, int which)
{
	struct nsapisohead *head;

	head = NULL;
	switch (which) {
	case TSAPLOOKUP_FOREIGN:
		head = TSAPHASH_FOREIGN(tsap, type, subnet);
		LIST_INSERT_HEAD(head, nsap, nsi_fhash);
		break;
	case TSAPLOOKUP_LOCAL:
		head = TSAPHASH_LOCAL(tsap, type, subnet);
		LIST_INSERT_HEAD(head, nsap, nsi_lhash);
		break;
	}
}

static void
tsap_remove(struct nsap_iso *nsap, long type, long subnet, int which)
{

	switch (which) {
	case TSAPLOOKUP_FOREIGN:
		LIST_REMOVE(nsap, nsi_fhash);
		break;
	case TSAPLOOKUP_LOCAL:
		LIST_REMOVE(nsap, nsi_lhash);
		break;
	}
}

static void
tsap_setup_tsap_id_table(void)
{
    long type, subnet, cnt;
    uint32_t tsap_ids[NSAP_TYPE_MAX][NSAP_SUBNET_MAX];

    type = subnet = cnt = 0;
    /* generate tsap_id table */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
            tsap_ids[type][subnet] = 0;
            tsap_ids[NSAP_TYPE_SIN4][NSAP_SUBNET_IPV4] = tsap_id(NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4);
            tsap_ids[NSAP_TYPE_SIN4][NSAP_SUBNET_IPV6] = tsap_id(NSAP_TYPE_SIN4, NSAP_SUBNET_IPV6); /* tsap id should match sin6 with ipv4 */
            tsap_ids[NSAP_TYPE_SIN6][NSAP_SUBNET_IPV4] = tsap_id(NSAP_TYPE_SIN6, NSAP_SUBNET_IPV4); /* tsap id should match sin4 with ipv6 */
            tsap_ids[NSAP_TYPE_SIN6][NSAP_SUBNET_IPV6] = tsap_id(NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6);
            tsap_ids[NSAP_TYPE_SNS][NSAP_SUBNET_IDP] = tsap_id(NSAP_TYPE_SNS, NSAP_SUBNET_IDP);
            tsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CONS] = tsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CONS);
            tsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CLNS] = tsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CLNS);
            tsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CLNP] = tsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CLNP);
            tsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_ISIS] = tsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_ISIS);
            tsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_ESIS] = tsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_ESIS);
            tsap_ids[NSAP_TYPE_SX25][NSAP_SUBNET_X25] = tsap_id(NSAP_TYPE_SX25, NSAP_SUBNET_X25);
            tsap_ids[NSAP_TYPE_SATM][NSAP_SUBNET_ATM] = tsap_id(NSAP_TYPE_SATM, NSAP_SUBNET_ATM);
            tsap_ids[NSAP_TYPE_SIPX][NSAP_SUBNET_IPX] = tsap_id(NSAP_TYPE_SIPX, NSAP_SUBNET_IPX);
            tsap_ids[NSAP_TYPE_SSNA][NSAP_SUBNET_SNA] = tsap_id(NSAP_TYPE_SSNA, NSAP_SUBNET_SNA);
            //printf("tsap_ids[type%ld][subnet%ld]: %u\n", type, subnet, tsap_ids[type][subnet]);
        }
    }
    /* remove invalid tsap_ids (i.e. any with 0) table */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
            if (tsap_ids[type][subnet] != 0) {
            	tsap_valid_ids[type][subnet] = tsap_ids[type][subnet];
                cnt++;
                //printf("valid_ids[type%ld][subnet%ld]: %u\n", type, subnet, valid_ids[type][subnet]);
            }
        }
    }
    tsap_id_cnt = cnt;
    //printf("cnt: %d\n", cnt);
}
