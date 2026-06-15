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

#ifndef _NET_IF_SAP_H_
#define _NET_IF_SAP_H_

/* malloctypes */
#define M_SAP 	103 	/* service access point */

#define SAPLEN 64 		/* SAP MAX LEN */

#define SAP_TABLE_MAX 9
struct sap_select {
	uint32_t ss_selector[SAP_TABLE_MAX];	/* selector id cksum */
	int ss_sid;								/* sap select id */
	int ss_af;								/* address family */
	long ss_type[SAP_TYPE_MAX];				/* sockaddr array */
	long ss_subnet[SAP_SUBNET_MAX];			/* network protocol array */
	long ss_subtran[SAP_SUBTRAN_MAX];		/* transport protocol array */
	int ss_class[SAP_CLASS_MAX];			/* connection orientation array */
};

/* service access point */
struct sap_addr {
	char saa_addr[SAPLEN];		/* address */
	unsigned char saa_addrlen; 	/* address length */
};

struct sockaddr_sap {
	long sasap_type;				/* stack type (address family) */
	long sasap_subnet;			/* subnet type (network layer protocols) */
	long sasap_subtran;			/* subtran type (transport layer protocols) */
	int sasap_class;				/* class type (connection or connection-less) */
	struct sap_addr sasap_addr;
};

struct sap_head;
LIST_HEAD(sap_head, sap_node);
/* sap tree */
struct sap_tree {
	struct radix_node_head *st_tree[9];	/* radix head per af */
	struct sap_head *st_hashtbl;		/* sap hashtable */
	u_long st_hash;						/* sap hash */
};

/* sap node */
struct sap_node {
	LIST_ENTRY(sap_node) st_hash;		/* sap entry */
	struct radix_node st_nodes[9];		/* radix node per af */
	struct sockaddr_sap *st_smask[9];	/* radix mask per af */
	struct sockaddr_sap *st_sasap;		/* sockaddr_sap  */
	struct sap_addr *st_sapa;			/* sap_addr  */
	uint32_t st_type_id;				/* type id */
	uint32_t st_subnet_id;				/* subnet id */
	uint32_t st_subtran_id;				/* subtran id */
	uint32_t st_class_id;				/* class id */
};

/* sap select id's */
enum sap_sids {
	SAP_SID_UNKNOWN,
	SAP_SID_INET4,
	SAP_SID_INET6,
	SAP_SID_NS,
	SAP_SID_ISO,
	SAP_SID_X25,
	SAP_SID_ATM,
	SAP_SID_IPX,
	SAP_SID_SNA,
	SAP_SID_MAX
};

/* sap class types */
enum sap_classes {
	SAP_CLASS_UNKNOWN,
	/* iso (native) */
	/* Connection Oriented */
	SAP_CLASS_CONS,
	/* Connection-less Oriented */
	SAP_CLASS_CLNS,

	/* should alway be last */
	SAP_CLASS_MAX
};

/* sap stack types */
/* uses sockaddr names */
enum sap_types {
	SAP_TYPE_UNKNOWN,
	SAP_TYPE_SIN4,
	SAP_TYPE_SIN6,
	SAP_TYPE_SNS,
	SAP_TYPE_SISO,
	SAP_TYPE_SX25,
	SAP_TYPE_SATM,
	SAP_TYPE_SIPX,
	SAP_TYPE_SSNA,

	/* should alway be last */
	SAP_TYPE_MAX
};

/* sap subnet types */
/* Network Layer Protocols */
enum sap_subnets {
	SAP_SUBNET_UNKNOWN,
	/* inet (v4 and v6) */
	SAP_SUBNET_IPV4,
	SAP_SUBNET_IPV6,
	/* iso (native) */
	SAP_SUBNET_CONS,
	SAP_SUBNET_CLNS,
	SAP_SUBNET_CLNP,
	SAP_SUBNET_ISIS,
	SAP_SUBNET_ESIS,
	/* xns */
	SAP_SUBNET_IDP,
	/* x25 */
	SAP_SUBNET_X25,
	/* atm */
	SAP_SUBNET_ATM,
	/* ipx */
	SAP_SUBNET_IPX,
	/* sna */
	SAP_SUBNET_SNA,

	/* should alway be last */
	SAP_SUBNET_MAX
};

/* sap subtran type */
/* Transport Layer Protocols */
enum sap_subtran {
	SAP_SUBTRAN_UNKNOWN,
	/* inet (v4 and v6) */
	SAP_SUBTRAN_TCP,
	SAP_SUBTRAN_UDP,
	/* iso */
	SAP_SUBTRAN_TP0,
	SAP_SUBTRAN_TP1,
	SAP_SUBTRAN_TP2,
	SAP_SUBTRAN_TP3,
	SAP_SUBTRAN_TP4,
	/* xns */
	SAP_SUBTRAN_SPP,
	/* x25 */
	SAP_SUBTRAN_X25,
	/* atm */
	SAP_SUBTRAN_ATM,
	/* ipx */
	SAP_SUBTRAN_SPX,
	/* sna */
	SAP_SUBTRAN_SNA,

	/* should alway be last */
	SAP_SUBTRAN_MAX
};

/* sockaddr_sap radix masks */
#define sap_smask(sap, sid) 	((sap)->st_smask[(sid)])
#define sap_smask_unknown(sap) 	sap_smask(sap, SAP_SID_UNKNOWN)
#define sap_smask_inet4(sap) 	sap_smask(sap, SAP_SID_INET4)
#define sap_smask_inet6(sap)	sap_smask(sap, SAP_SID_INET6)
#define sap_smask_ns(sap)		sap_smask(sap, SAP_SID_NS)
#define sap_smask_iso(sap)		sap_smask(sap, SAP_SID_ISO)
#define sap_smask_x25(sap)		sap_smask(sap, SAP_SID_X25)
#define sap_smask_atm(sap)		sap_smask(sap, SAP_SID_ATM)
#define sap_smask_ipx(sap)		sap_smask(sap, SAP_SID_IPX)
#define sap_smask_sna(sap)		sap_smask(sap, SAP_SID_SNA)

extern struct sap_tree sap_radix_tree;
extern struct sap_select sap_table[SAP_TABLE_MAX];

void sap_init(struct sap_tree *);

int sockaddr_sap_compare(struct sockaddr_sap *, struct sockaddr_sap *);
void sap_addr_init(struct sap_addr *, char *, u_char);
int sap_addr_compare(struct sap_addr *, struct sap_addr *);
int sap_acknowledge(struct sockaddr_sap *, struct sap_addr *, long, long, long, int, int);

struct sap_node *sap_lookup(struct sap_tree *, struct sockaddr_sap *, long, long, long, int, int);
void sap_insert(struct sap_tree *, struct sap_node *, struct sockaddr_sap *, long, long, long, int, int);
void sap_remove(struct sap_tree *, struct sockaddr_sap *, long, long, long, int, int);

void sap_insert_af(struct sap_tree *, struct sap_node *, int, int);
void sap_remove_af(struct sap_tree *, struct sap_node *, int, int);
int sap_id_check(struct sap_node *, long, long, long, int, int);

void sap_select_init(struct sap_select *, int, int);
int sap_select_compare(struct sap_select *, struct sap_select *);
struct sap_select *sap_select_lookup(int, int);
long sap_select_lookup_type(int, int, long);
long sap_select_lookup_subnet(int, int, long);
long sap_select_lookup_subtran(int, int, long);
int sap_select_lookup_class(int, int, int);
uint32_t sap_select_lookup_selector(struct sap_select *, int);
int sap_select_sid_to_af(int);
int sap_select_af_to_sid(int);

#endif /* _NET_IF_SAP_H_ */
