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
#include <sys/queue.h>

#include <net/radix.h>

#include "if_sap.h"

struct sap_tree sap_radix_tree;
static uint32_t sap_hashids[SAP_TABLE_MAX];

struct sap_select sap_table[] = {
		/* 0 - AF_UNSPEC */
		{
				.ss_sid = 0,
				.ss_af = AF_UNSPEC,
				.ss_type =  { SAP_TYPE_UNKNOWN },
				.ss_subnet = { SAP_SUBNET_UNKNOWN },
				.ss_subtran = { SAP_SUBTRAN_UNKNOWN },
				.ss_class = { SAP_CLASS_UNKNOWN },
		},
		/* 1 - AF_INET */
		{
				.ss_sid = 1,
				.ss_af = AF_INET,
				.ss_type =  { SAP_TYPE_SIN4 },
				.ss_subnet = { SAP_SUBNET_IPV4, SAP_SUBNET_IPV6 },
				.ss_subtran = { SAP_SUBTRAN_TCP,  SAP_SUBTRAN_UDP },
				.ss_class = { SAP_CLASS_CONS, SAP_CLASS_CLNS },
		},
		/* 2 - AF_INET6 */
		{
				.ss_sid = 2,
				.ss_af = AF_INET6,
				.ss_type =  { SAP_TYPE_SIN6 },
				.ss_subnet = { SAP_SUBNET_IPV4, SAP_SUBNET_IPV6 },
				.ss_subtran = { SAP_SUBTRAN_TCP,  SAP_SUBTRAN_UDP },
				.ss_class = { SAP_CLASS_CONS, SAP_CLASS_CLNS },
		},
		/* 3 - AF_NS */
		{
				.ss_sid = 3,
				.ss_af = AF_NS,
				.ss_type =  { SAP_TYPE_SNS },
				.ss_subnet = { SAP_SUBNET_IDP },
				.ss_subtran = { SAP_SUBTRAN_SPP },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 4 - AF_ISO */
		{
				.ss_sid = 4,
				.ss_af = AF_ISO,
				.ss_type =  { SAP_TYPE_SISO },
				.ss_subnet = { SAP_SUBNET_CONS, SAP_SUBNET_CLNS, SAP_SUBNET_CLNP, SAP_SUBNET_ISIS, SAP_SUBNET_ESIS },
				.ss_subtran = { SAP_SUBTRAN_TP0, SAP_SUBTRAN_TP1, SAP_SUBTRAN_TP2, SAP_SUBTRAN_TP3, SAP_SUBTRAN_TP4 },
				.ss_class = { SAP_CLASS_CONS, SAP_CLASS_CLNS },
		},
		/* 5 - AF_CCITT */
		{
				.ss_sid = 5,
				.ss_af = AF_CCITT,
				.ss_type =  { SAP_TYPE_SX25 },
				.ss_subnet = { SAP_SUBNET_X25 },
				.ss_subtran = { SAP_SUBTRAN_X25 },
				.ss_class = { SAP_CLASS_CONS },
		},
		/* 6 - AF_NATM */
		{
				.ss_sid = 6,
				.ss_af = AF_NATM,
				.ss_type =  { SAP_TYPE_SATM },
				.ss_subnet = { SAP_SUBNET_ATM },
				.ss_subtran = { SAP_SUBTRAN_ATM },
				.ss_class = { SAP_CLASS_CONS },
		},
		/* 7 - AF_IPX */
		{
				.ss_sid = 7,
				.ss_af = AF_IPX,
				.ss_type =  { SAP_TYPE_SIPX },
				.ss_subnet = { SAP_SUBNET_IPX },
				.ss_subtran = { SAP_SUBTRAN_SPX },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 8 - AF_SNA */
		{
				.ss_sid = 8,
				.ss_af = AF_SNA,
				.ss_type =  { SAP_TYPE_SSNA },
				.ss_subnet = { SAP_SUBNET_SNA },
				.ss_subtran = { SAP_SUBTRAN_SNA },
				.ss_class = { SAP_CLASS_CLNS },
		},
};

#define SAPHASH(table, type, subnet, subtran, class) \
	&(tree)->st_hashtbl[sap_hash((type), (subnet), (subtran), (class), SAPLEN)]

void
sap_init(struct sap_tree *tree)
{
	tree->st_hashtbl = hashinit(SAPLEN, M_SAP, &tree->st_hash);
}

int
sap_class_select(int clazz)
{
	int select = -1;

	switch (clazz) {
	case SAP_CLASS_CONS:
		select = SAP_CLASS_CONS;
		break;
	case SAP_CLASS_CLNS:
		select = SAP_CLASS_CLNS;
		break;
	case SAP_CLASS_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_CLASS_UNKNOWN;
		break;
	}
	return (select);
}

long
sap_type_select(long type)
{
	long select = -1;

	switch (type) {
	case SAP_TYPE_SIN4:
		select = SAP_TYPE_SIN4;
		break;
	case SAP_TYPE_SIN6:
		select = SAP_TYPE_SIN6;
		break;
	case SAP_TYPE_SNS:
		select = SAP_TYPE_SNS;
		break;
	case SAP_TYPE_SISO:
		select = SAP_TYPE_SISO;
		break;
	case SAP_TYPE_SX25:
		select = SAP_TYPE_SX25;
		break;
	case SAP_TYPE_SATM:
		select = SAP_TYPE_SATM;
		break;
	case SAP_TYPE_SIPX:
		select = SAP_TYPE_SIPX;
		break;
	case SAP_TYPE_SSNA:
		select = SAP_TYPE_SSNA;
		break;
	case SAP_TYPE_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_TYPE_UNKNOWN;
		break;
	}
	return (select);
}

long
sap_subnet_select(long subnet)
{
	long select = -1;

	switch (subnet) {
	case SAP_SUBNET_IPV4:
		select = SAP_SUBNET_IPV4;
		break;
	case SAP_SUBNET_IPV6:
		select = SAP_SUBNET_IPV6;
		break;
	case SAP_SUBNET_IDP:
		select = SAP_SUBNET_IDP;
		break;
	case SAP_SUBNET_CONS:
		select = SAP_SUBNET_CONS;
		break;
	case SAP_SUBNET_CLNS:
		select = SAP_SUBNET_CLNS;
		break;
	case SAP_SUBNET_CLNP:
		select = SAP_SUBNET_CLNP;
		break;
	case SAP_SUBNET_ISIS:
		select = SAP_SUBNET_ISIS;
		break;
	case SAP_SUBNET_ESIS:
		select = SAP_SUBNET_ESIS;
		break;
	case SAP_SUBNET_X25:
		select = SAP_SUBNET_X25;
		break;
	case SAP_SUBNET_ATM:
		select = SAP_SUBNET_ATM;
		break;
	case SAP_SUBNET_IPX:
		select = SAP_SUBNET_IPX;
		break;
	case SAP_SUBNET_SNA:
		select = SAP_SUBNET_SNA;
		break;
	case SAP_SUBNET_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_SUBNET_UNKNOWN;
		break;
	}
	return (select);
}

long
sap_subtran_select(long subtran)
{
	long select = -1;

	switch (subtran) {
	case SAP_SUBTRAN_TCP:
		select = SAP_SUBTRAN_TCP;
		break;
	case SAP_SUBTRAN_UDP:
		select = SAP_SUBTRAN_UDP;
		break;
	case SAP_SUBTRAN_TP0:
		select = SAP_SUBTRAN_TP0;
		break;
	case SAP_SUBTRAN_TP1:
		select = SAP_SUBTRAN_TP1;
		break;
	case SAP_SUBTRAN_TP2:
		select = SAP_SUBTRAN_TP2;
		break;
	case SAP_SUBTRAN_TP3:
		select = SAP_SUBTRAN_TP3;
		break;
	case SAP_SUBTRAN_TP4:
		select = SAP_SUBTRAN_TP4;
		break;
	case SAP_SUBTRAN_SPP:
		select = SAP_SUBTRAN_SPP;
		break;
	case SAP_SUBTRAN_X25:
		select = SAP_SUBTRAN_X25;
		break;
	case SAP_SUBTRAN_ATM:
		select = SAP_SUBTRAN_ATM;
		break;
	case SAP_SUBTRAN_SPX:
		select = SAP_SUBTRAN_SPX;
		break;
	case SAP_SUBTRAN_SNA:
		select = SAP_SUBTRAN_SNA;
		break;
	case SAP_SUBTRAN_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_SUBTRAN_UNKNOWN;
		break;
	}
	return (select);
}

void
sap_type(struct sockaddr_sap *sasap, long type)
{
	if (sasap == NULL) {
		return;
	}
	sasap->sasap_type = sap_type_select(type);
}

void
sap_subnet(struct sockaddr_sap *sasap, long subnet)
{
	if (sasap == NULL) {
		return;
	}
	sasap->sasap_subnet = sap_subnet_select(subnet);
}

void
sap_subtran(struct sockaddr_sap *sasap, long subtran)
{
	if (sasap == NULL) {
		return;
	}
	sasap->sasap_subtran = sap_subtran_select(subtran);
}

void
sap_class(struct sockaddr_sap *sasap, int clazz)
{
	if (sasap == NULL) {
		return;
	}
	sasap->sasap_class = sap_class_select(clazz);
}

void
sap_addr_init(struct sap_addr *sapa, char *addr, u_char addrlen)
{
	bcopy(addr, sapa->saa_addr, sizeof(sapa->saa_addr));
	sapa->saa_addrlen = addrlen;
}

int
sap_acknowledge(struct sockaddr_sap *sasap, struct sap_addr *sapa, long type, long subnet, long subtran, int clazz, int sid)
{
	struct sap_node *sap;
	int error;

	sap = sap_lookup(&sap_radix_tree, sasap, type, subnet, subtran, clazz, sid);
	if (sap != NULL) {
		if (sap->st_sasap != NULL) {
			error = sockaddr_sap_compare(sap->st_sasap, sasap);
			if (error != 0) {
				error = sap_id_check(sap, sasap->sasap_type, sasap->sasap_subnet, sasap->sasap_subtran, sasap->sasap_class);
				if (error != 0) {
					return (error);
				}
			}
		}
		if (sap->st_sapa != NULL) {
			error = sap_addr_compare(sap->st_sapa, sapa);
			if (error != 0) {
				return (error);
			}
		}
	}
	return (0);
}

struct radix_node_head *
sap_radix_node_head(struct sap_tree *tree, int sid)
{
	return (tree->st_tree[sid]);
}

struct radix_node *
sap_radix_node_add(struct sap_tree *tree, struct sap_node *sap, struct sockaddr_sap *sasap, int sid)
{
	struct radix_node_head *rnh;
	struct sockaddr_sap *smask;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	smask = sap_smask(sap, sid);
	if (smask == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_addaddr(sasap, smask, rnh, sap->st_nodes);
	return (rn);
}

struct radix_node *
sap_radix_node_delete(struct sap_tree *tree, struct sap_node *sap, struct sockaddr_sap *sasap, int sid)
{
	struct radix_node_head *rnh;
	struct sockaddr_sap *smask;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	smask = sap_smask(sap, sid);
	if (smask == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_deladdr(sasap, smask, rnh);
	return (rn);
}

struct radix_node *
sap_radix_node_matchaddr(struct sap_tree *tree, struct sockaddr_sap *sasap, int sid)
{
	struct radix_node_head *rnh;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_matchaddr(sasap, rnh);
	return (rn);
}

struct radix_node *
sap_radix_node_lookup(struct sap_tree *tree, struct sap_node *sap, struct sockaddr_sap *sasap, int sid)
{
	struct radix_node_head *rnh;
	struct sockaddr_sap *smask;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	smask = sap_smask(sap, sid);
	if (smask == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_lookup(sasap, smask, rnh);
	return (rn);
}

static struct sap_node *
sap_hashlookup(struct sap_tree *tree, long type, long subnet, long subtran, int clazz)
{
	struct sap_head *head;
	struct sap_node *sap;
	int error;

	head = SAPHASH(tree, type, subnet, subtran, clazz);
	LIST_FOREACH(sap, head, st_hash) {
		if (sap_id_check(sap, type, subnet, subtran, clazz)) {
			return (sap);
		}
	}
	return (NULL);
}

struct sap_node *
sap_lookup(struct sap_tree *tree, struct sockaddr_sap *sasap, long type, long subnet, long subtran, int clazz, int sid)
{
	struct radix_node_head *rnh;
	struct radix_node *rn;
	struct sap_node *sap, *match;
	int error;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}

	rn = sap_radix_node_matchaddr(sasap, sid);
	if (rn && (rn->rn_flags & RNF_ROOT) == 0) {
		match = (struct sap_node *)rn;
		error = sap_id_check(match, type, subnet, subtran, clazz);
	}

	sap = sap_hashlookup(tree, type, subnet, subtran, clazz);
	if (sap != NULL) {
		if (error != 0) {
			match = sap;
		}
	}
	return (match);
}

void
sap_insert(struct sap_tree *tree, struct sap_node *sap, struct sockaddr_sap *sasap, long type, long subnet, long subtran, int clazz, int sid)
{
	struct sap_head *head;
	struct radix_node *rn;

	head = SAPHASH(tree, type, subnet, subtran, clazz);
	if (head == NULL || sap == NULL) {
		return;
	}
	rn = nsap_radix_node_add(tree, sap, sasap, sid);
	if (rn == NULL) {
		return;
	}

	sap->st_sasap = sasap;
	sap->st_type_id = sap_type_hash(type, SAPLEN);
	sap->st_subnet_id = sap_subnet_hash(subnet, SAPLEN);
	sap->st_subtran_id = sap_subtran_hash(subtran, SAPLEN);
	sap->st_class_id = sap_class_hash(clazz, SAPLEN);
	LIST_INSERT_HEAD(head, sap, st_hash);
}

void
sap_remove(struct sap_tree *tree, struct sockaddr_sap *sasap, long type, long subnet, long subtran, int clazz, int sid)
{
	struct sap_head *head;
	struct sap_node *sap, *match;
	struct radix_node *rn;
	int error;

	sap = sap_lookup(tree, sasap, type, subnet, subtran, clazz, sid);
	if (sap != NULL) {
		rn = sap_radix_node_delete(tree, sap, sasap, sid);
		if (rn != NULL) {
			match = (struct sap_node *)rn;
			error = sap_id_check(match, type, subnet, subtran, clazz);
			if (error != 0) {
				match = sap;
			}
		}
		LIST_REMOVE(sap, st_hash);
	}
}

void
sap_insert_af(struct sap_tree *tree, struct sap_node *sap, int sid, int af)
{
	switch (af) {
	case AF_INET:
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		/* IPv6 */
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_insert(tree, sap, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		break;
	case AF_INET6:
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		/* IPv4 */
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_insert(tree, sap, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		break;
	case AF_NS:
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SNS, SAP_SUBNET_IDP, SAP_SUBTRAN_SPP, SAP_CLASS_CLNS, SAP_SID_NS);
		break;
	case AF_ISO:
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP0, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP1, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP2, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP3, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP4, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CLNS, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CLNP, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_ISIS, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		sap_insert(tree, sap, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_ESIS, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		break;
	case AF_CCITT:
		sap_insert(tree, sap, sap_smask_x25(sap), SAP_TYPE_SX25, SAP_SUBNET_X25, SAP_SUBTRAN_X25, SAP_CLASS_CONS, SAP_SID_X25);
		break;
	case AF_NATM:
		sap_insert(tree, sap, sap_smask_atm(sap), SAP_TYPE_SATM, SAP_SUBNET_ATM, SAP_SUBTRAN_ATM, SAP_CLASS_CONS, SAP_SID_ATM);
		break;
	case AF_IPX:
		sap_insert(tree, sap, sap_smask_ipx(sap), SAP_TYPE_SIPX, SAP_SUBNET_IPX, SAP_SUBTRAN_SPX, SAP_CLASS_CLNS, SAP_SID_IPX);
		break;
	case AF_SNA:
		sap_insert(tree, sap, sap_smask_sna(sap), SAP_TYPE_SSNA, SAP_SUBNET_SNA, SAP_SUBTRAN_SNA, SAP_CLASS_CLNS, SAP_SID_SNA);
		break;
	}
}

void
sap_remove_af(struct sap_tree *tree, struct sap_node *sap, int sid, int af)
{
	switch (af) {
	case AF_INET:
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		/* IPv6 */
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET4);
		sap_remove(tree, sap_smask_inet4(sap), SAP_TYPE_SIN4, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET4);
		break;
	case AF_INET6:
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV6, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		/* IPv4 */
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_TCP, SAP_CLASS_CONS, SAP_SID_INET6);
		sap_remove(tree, sap_smask_inet6(sap), SAP_TYPE_SIN6, SAP_SUBNET_IPV4, SAP_SUBTRAN_UDP, SAP_CLASS_CLNS, SAP_SID_INET6);
		break;
	case AF_NS:
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SNS, SAP_SUBNET_IDP, SAP_SUBTRAN_SPP, SAP_CLASS_CLNS, SAP_SID_NS);
		break;
	case AF_ISO:
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP0, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP1, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP2, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP3, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CONS, SAP_SUBTRAN_TP4, SAP_CLASS_CONS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CLNS, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_CLNP, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_ISIS, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		sap_remove(tree, sap_smask_iso(sap), SAP_TYPE_SISO, SAP_SUBNET_ESIS, SAP_SUBTRAN_TP4, SAP_CLASS_CLNS, SAP_SID_ISO);
		break;
	case AF_CCITT:
		sap_remove(tree, sap_smask_x25(sap), SAP_TYPE_SX25, SAP_SUBNET_X25, SAP_SUBTRAN_X25, SAP_CLASS_CONS, SAP_SID_X25);
		break;
	case AF_NATM:
		sap_remove(tree, sap_smask_atm(sap), SAP_TYPE_SATM, SAP_SUBNET_ATM, SAP_SUBTRAN_ATM, SAP_CLASS_CONS, SAP_SID_ATM);
		break;
	case AF_IPX:
		sap_remove(tree, sap_smask_ipx(sap), SAP_TYPE_SIPX, SAP_SUBNET_IPX, SAP_SUBTRAN_SPX, SAP_CLASS_CLNS, SAP_SID_IPX);
		break;
	case AF_SNA:
		sap_remove(tree, sap_smask_sna(sap), SAP_TYPE_SSNA, SAP_SUBNET_SNA, SAP_SUBTRAN_SNA, SAP_CLASS_CLNS, SAP_SID_SNA);
		break;
	}
}

/*
 * sap class hash:
 * returns 0 if class is unknown or max
 */
static uint32_t
sap_class_hash(int clazz, int len)
{
	uint32_t class_id;

	if ((clazz <= SAP_CLASS_UNKNOWN) || (clazz >= SAP_CLASS_MAX)) {
		len = 1;
	}
	class_id = enhanced_double_hash(clazz, len);
	return (class_id);
}

/*
 * sap type hash:
 * returns 0 if type is unknown or max
 */
static uint32_t
sap_type_hash(long type, int len)
{
	uint32_t type_id;

	if ((type <= SAP_TYPE_UNKNOWN) || (type >= SAP_TYPE_MAX)) {
		len = 1;
	}
	type_id = enhanced_double_hash(type, len);
	return (type_id);
}

/*
 * sap subnet hash:
 * returns 0 if subnet is unknown or max
 */
static uint32_t
sap_subnet_hash(long subnet, int len)
{
	uint32_t subnet_id;

	if ((subnet <= SAP_SUBNET_UNKNOWN) || (subnet >= SAP_SUBNET_MAX)) {
		len = 1;
	}
	subnet_id = enhanced_double_hash(subnet, len);
	return (subnet_id);
}

/*
 * sap subtran hash:
 * returns 0 if subtran is unknown or max
 */
static uint32_t
sap_subtran_hash(long subtran, int len)
{
	uint32_t subtran_id;

	if ((subtran <= SAP_SUBTRAN_UNKNOWN) || (subtran >= SAP_SUBTRAN_MAX)) {
		len = 1;
	}
	subtran_id = enhanced_double_hash(subtran, len);
	return (subtran_id);
}

/*
 * Common sap hash:
 * for NSAP ID and TSAP ID
 */
static uint32_t
sap_hash(long type, long subnet, long subtran, int clazz, int len)
{
    uint32_t sap_id, type_id, subnet_id, subtran_id, class_id, hashid;

    if ((subtran == 0) && (clazz == 0)) {
        type_id = sap_type_hash(type, len);
        subnet_id = sap_subnet_hash(subnet, len);
        if ((type_id == 0) || (subnet_id == 0)) {
            len = 1;
        }
        hashid = (type_id + (subnet_id + (len - 1))) - len;
    } else {
        type_id = sap_type_hash(type, len);
        subnet_id = sap_subnet_hash(subnet, len);
        subtran_id = sap_subtran_hash(subtran, len);
        class_id = sap_class_hash(clazz, len);
        if ((type_id == 0) || (subnet_id == 0) || (subtran_id == 0)
                || (class_id == 0)) {
            len = 1;
        }
        hashid = (type_id + (subnet_id + (len - 1)) + (subtran_id + (len - 2)) + (class_id + (len - 2))) - len;
    }
    sap_id = enhanced_double_hash(hashid, len);
    return (sap_id);
}

/*
 * sap_type_id_check:
 * check type id against type
 */
static int
sap_type_id_check(uint32_t id, long type)
{
	if (id > sap_type_hash(type, SAPLEN)) {
		return (-1);
	} else if (id < sap_type_hash(type, SAPLEN)) {
		return (1);
	}
	return (0);
}

/*
 * sap_subnet_id_check:
 * check subnet id against subnet
 */
static int
sap_subnet_id_check(uint32_t id, long subnet)
{
	if (id > sap_subnet_hash(subnet, SAPLEN)) {
		return (-1);
	} else if (id < sap_subnet_hash(subnet, SAPLEN)) {
		return (1);
	}
	return (0);
}

/*
 * sap_subtran_id_check:
 * check subtran id against subtran
 */
static int
sap_subtran_id_check(uint32_t id, long subtran)
{
	if (id > sap_subtran_hash(subtran, SAPLEN)) {
		return (-1);
	} else if (id < sap_subtran_hash(subtran, SAPLEN)) {
		return (1);
	}
	return (0);
}

/*
 * sap_class_id_check:
 * check class id against class
 */
static int
sap_class_id_check(uint32_t id, int clazz)
{
	if (id > sap_class_hash(clazz, SAPLEN)) {
		return (-1);
	} else if (id < sap_class_hash(clazz, SAPLEN)) {
		return (1);
	}
	return (0);
}

int
sap_id_check(struct sap_node *sap, long type, long subnet, long subtran, int clazz)
{
	int error;

	error = sap_type_id_check(sap->st_type_id, type);
	if (error != 0) {
		return (error);
	}
	error = sap_subnet_id_check(sap->st_subnet_id, subnet);
	if (error != 0) {
		return (error);
	}
	error = sap_subtran_id_check(sap->st_subtran_id, subtran);
	if (error != 0) {
		return (error);
	}
	error = sap_class_id_check(sap->st_class_id, clazz);
	if (error != 0) {
		return (error);
	}
	return (0);
}

/*
 * sockaddr_sap functions
 */
static int
sockaddr_sap_compare_type(struct sockaddr_sap *a, struct sockaddr_sap *b)
{
	if (a->sasap_type > b->sasap_type) {
		return (-1);
	} else if (a->sasap_type < b->sasap_type) {
		return (1);
	} else {
		return (0);
	}
}

static int
sockaddr_sap_compare_subnet(struct sockaddr_sap *a, struct sockaddr_sap *b)
{
	if (a->sasap_subnet > b->sasap_subnet) {
		return (-1);
	} else if (a->sasap_subnet < b->sasap_subnet) {
		return (1);
	} else {
		return (0);
	}
}

static int
sockaddr_sap_compare_subtran(struct sockaddr_sap *a, struct sockaddr_sap *b)
{
	if (a->sasap_subtran > b->sasap_subtran) {
		return (-1);
	} else if (a->sasap_subtran < b->sasap_subtran) {
		return (1);
	} else {
		return (0);
	}
}

static int
sockaddr_sap_compare_class(struct sockaddr_sap *a, struct sockaddr_sap *b)
{
	if (a->sasap_class > b->sasap_class) {
		return (-1);
	} else if (a->sasap_class < b->sasap_class) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * sockaddr_nsap_compare:
 * - compares type, subnet, subtran, class and sap_addr.
 * returns -1 if a, 1 if b and 0 if equal
 */
int
sockaddr_sap_compare(struct sockaddr_sap *a, struct sockaddr_sap *b)
{
	struct sap_addr *aa, *bb;
	int error;

	aa = &a->sasap_addr;
	bb = &b->sasap_addr;

	if (a != b) {
		error = sockaddr_sap_compare_type(a, b);
		if (error != 0) {
			return (error);
		}
		error = sockaddr_sap_compare_subnet(a, b);
		if (error != 0) {
			return (error);
		}
		error = sockaddr_sap_compare_subtran(a, b);
		if (error != 0) {
			return (error);
		}
		error = sockaddr_sap_compare_class(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_addr_compare(aa, bb);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}

/*
 * sap_addr functions
 */
static int
sap_addr_compare_addr(struct sap_addr *a, struct sap_addr *b)
{
	if (bcmp(a->ns_addr, b->ns_addr, sizeof(a->ns_addr)) != 0) {
		return (1);
	}
	return (0);
}

static int
sap_addr_compare_addrlen(struct sap_addr *a, struct sap_addr *b)
{
	if (a->ns_addrlen > b->ns_addrlen) {
		return (-1);
	} else if (a->ns_addrlen < b->ns_addrlen) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * sap_addr_compare:
 * - compares addr, addrlen.
 * returns -1 if a, 1 if b and 0 if equal
 */
int
sap_addr_compare(struct sap_addr *a, struct sap_addr *b)
{
	int error;

	if (a != b) {
		error = sap_addr_compare_addr(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_addr_compare_addrlen(a, b);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}


/*
 * sap_select functions
 */
static void
sap_select_setup(struct sap_select *select, long *type, long *subnet, long *subtran, int *class)
{
    bcopy(type, select->ss_type, sizeof(*select->ss_type));
    bcopy(subnet, select->ss_subnet, sizeof(*select->ss_subnet));
    bcopy(subtran, select->ss_subtran, sizeof(*select->ss_subtran));
    bcopy(class, select->ss_class, sizeof(*select->ss_class));
}

static void
sap_select_selector_setup(struct sap_select *select)
{
	int i;
	uint32_t sap_hashtable[SAP_TABLE_MAX];

	for (i = 1; i < SAP_TABLE_MAX; i++) {
		select = &sap_table[i];
		if (select != NULL) {
			sap_hashids[i] = sap_hash(*select->ss_type, *select->ss_subnet, *select->ss_subtran, *select->ss_class);
			tsap_valid_ids[i] = sap_hashids[i];
		}
	}
}

void
sap_select_init(struct sap_select *select, int sid, int af)
{
	bzero(select, sizeof(*select));
	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_select_setup(select, select->ss_type, select->ss_subnet, select->ss_subtran, select->ss_class);
		sap_select_selector_setup(select);
		select->ss_selector[sid] = sap_hashids[sid];
	}
}

struct sap_select *
sap_select_lookup(int sid, int af)
{
	struct sap_select *select;

	select = &sap_table[sid];
	if (select != NULL) {
		if ((select->ss_sid == sid) && (select->ss_af == af)) {
			return (select);
		}
	}
	return (NULL);
}

/*
 * sap item lookup:
 * search sap_array for sap_item
 * returns item if found or -1 if not found.
 */
long
sap_item_lookup(long sap_item, long *sap_array, int nelems)
{
	long item = -1;
	int i;

	for (i = 0; i < nelems; i++) {
		item = sap_array[i];
		if (item == sap_item) {
			break;
		}
	}
	return (item);
}

long
sap_select_lookup_type(int sid, int af, long type)
{
	struct sap_select *select;
	long sap_type;

	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_type = sap_item_lookup(type, select->ss_type, SAP_TYPE_MAX);
		if (sap_type != sap_type_select(type)) {
			sap_type = SAP_TYPE_UNKNOWN;
		}
	}
	return (sap_type);
}

long
sap_select_lookup_subnet(int sid, int af, long subnet)
{
	struct sap_select *select;
	long sap_subnet;

	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_subnet = sap_item_lookup(subnet, select->ss_subnet, SAP_SUBNET_MAX);
		if (sap_subnet != sap_subnet_select(subnet)) {
			sap_subnet = SAP_SUBNET_UNKNOWN;
		}
	}
	return (sap_subnet);
}

long
sap_select_lookup_subtran(int sid, int af, long subtran)
{
	struct sap_select *select;
	long sap_subtran;

	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_subtran = sap_item_lookup(subtran, select->ss_subtran,
				SAP_SUBTRAN_MAX);
		if (sap_subtran != sap_subtran_select(subtran)) {
			sap_subtran = SAP_SUBTRAN_UNKNOWN;
		}
	}
	return (sap_subtran);
}

int
sap_select_lookup_class(int sid, int af, int clazz)
{
	struct sap_select *select;
	long sap_class;

	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_class = sap_item_lookup(clazz, select->ss_class, SAP_CLASS_MAX);
		if (sap_class != sap_class_select(clazz)) {
			sap_class = SAP_CLASS_UNKNOWN;
		}
	}
	return (sap_class);
}

uint32_t
sap_select_lookup_selector(struct sap_select *select, int sid)
{
	if (select != NULL) {
		if (select->ss_selector[sid] == 0) {
			select->ss_selector[sid] = sap_hashids[sid];
		}
		return (select->ss_selector[sid]);
	}
	return (0);
}

/*
 * get sap select af from sid
 */
int
sap_select_sid_to_af(int sid)
{
    int sap_af = sap_table[sid].ss_af;
    return (sap_af);
}

/*
 * get sap select sid from af
 */
int
sap_select_af_to_sid(int af)
{
    int i, sap_sid;

    for (i = 0; i < SAP_TABLE_MAX; i++) {
        if (af == sap_table[i].ss_af) {
            sap_sid = i;
            break;
        }
    }
    return (sap_sid);
}

static int
sap_select_compare_sid(struct sap_select *a, struct sap_select *b)
{
	if (a->ss_sid > b->ss_sid) {
		return (-1);
	} else if (a->ss_sid < b->ss_sid) {
		return (1);
	} else {
		return (0);
	}
}

static int
sap_select_compare_af(struct sap_select *a, struct sap_select *b)
{
	if (a->ss_af > b->ss_af) {
		return (-1);
	} else if (a->ss_af < b->ss_af) {
		return (1);
	} else {
		return (0);
	}
}

static int
sap_select_compare_selector(struct sap_select *a, struct sap_select *b)
{
	if (a->ss_selector > b->ss_selector) {
		return (-1);
	} else if (a->ss_selector < b->ss_selector) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * sap_select_compare:
 * - compares selector, sid and af.
 * returns -1 if a, 1 if b and 0 if equal
 */
int
sap_select_compare(struct sap_select *a, struct sap_select *b)
{
	int error;

	if (a != b) {
		error = sap_select_compare_sid(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare_af(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare_selector(a, b);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}
