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

#ifndef _NETTPI_ISO_SAP_H_
#define _NETTPI_ISO_SAP_H_

#define ISOLEN 64 /* ISO MAX LEN */

struct sap_service {
	char ns_addr[ISOLEN];		/* address */
	unsigned char ns_addrlen; 	/* address length */
	int ns_class;				/* class */
};

/*
 * sap select is setup per network stack.
 * type, subnet, protocol and class are defined
 * by what a given network stack offers.
 * Example:
 * - INET/6 only offers CLNS class.
 * - ISO offers both CONS and CLNS classes.
 */
#define SAP_TABLE_MAX 9

struct sap_select {
	uint32_t ss_selector[SAP_TABLE_MAX];/* selector id cksum */
	int ss_sid;							/* sap select id */
	int ss_af;							/* address family */
	long ss_type[SAP_TYPE_MAX];			/* sockaddr array */
	long ss_subnet[SAP_SUBNET_MAX];		/* network protocol array */
	long ss_protocol[SAP_PROTOCOL_MAX];	/* transport protocol array */
	int ss_class[SAP_CLASS_MAX];		/* connection orientation array */
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

/* sap stack types (labeled by sockaddr) */
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

/* sap protocols */
enum sap_protocols {
	SAP_PROTOCOL_UNKNOWN,
	/* inet (v4 and v6) */
	SAP_PROTOCOL_TCP,
	SAP_PROTOCOL_UDP,
	/* iso */
	SAP_PROTOCOL_TP0,
	SAP_PROTOCOL_TP1,
	SAP_PROTOCOL_TP2,
	SAP_PROTOCOL_TP3,
	SAP_PROTOCOL_TP4,
	/* xns */
	SAP_PROTOCOL_SPP,
	/* x25 */
	SAP_PROTOCOL_X25,
	/* atm */
	SAP_PROTOCOL_ATM,
	/* ipx */
	SAP_PROTOCOL_SPX,
	/* sna */
	SAP_PROTOCOL_SNA,

	/* should alway be last */
	SAP_PROTOCOL_MAX
};

extern struct sap_select sap_table[SAP_TABLE_MAX];

int sap_class_select(int);
long sap_type_select(long);
long sap_subnet_select(long);
long sap_protocol_select(long);
long sap_item_lookup(long, long *, int);

void sap_select_init(struct sap_select *, int, int);

int nsap_addr_compare(struct nsap_addr *, struct nsap_addr *);
int sockaddr_nsap_compare(struct sockaddr_nsap *, struct sockaddr_nsap *);
int sap_service_compare(struct sap_service *, struct sap_service *);
int sap_service_check_class(long, long, int);
int sap_select_compare(struct sap_select *, struct sap_select *);

struct sap_select *sap_select_lookup(int, int);
long sap_select_lookup_type(int, int, long);
long sap_select_lookup_subnet(int, int, long);
long sap_select_lookup_protocol(int, int, long);
int sap_select_lookup_class(int, int, int);
uint32_t sap_select_lookup_selector(struct sap_select *, int);
int sap_select_sid_to_af(int);
int sap_select_af_to_sid(int);

/*
 * sap class hash:
 * returns 0 if class is unknown or max
 */
static __inline uint32_t
sap_class_hash(int clazz)
{
	uint32_t class_id;
	int len;

	len = ISOLEN;
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
static __inline uint32_t
sap_type_hash(long type)
{
	uint32_t type_id;
	int len;

	len = ISOLEN;
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
static __inline uint32_t
sap_subnet_hash(long subnet)
{
	uint32_t subnet_id;
	int len;

	len = ISOLEN;
	if ((subnet <= SAP_SUBNET_UNKNOWN) || (subnet >= SAP_SUBNET_MAX)) {
		len = 1;
	}
	subnet_id = enhanced_double_hash(subnet, len);
	return (subnet_id);
}

/*
 * sap protocol hash:
 * returns 0 if protocol is unknown or max
 */
static __inline uint32_t
sap_protocol_hash(long protocol)
{
	uint32_t protocol_id;
	int len;

	len = ISOLEN;
	if ((protocol <= SAP_PROTOCOL_UNKNOWN) || (protocol >= SAP_PROTOCOL_MAX)) {
		len = 1;
	}
	protocol_id = enhanced_double_hash(protocol, len);
	return (protocol_id);
}

/*
 * nsap type identifier:
 * returns 0 if type is unknown or max
 */
#define nsap_type_id(type)	\
	sap_type_hash(type)

/*
 * nsap subnet identifier:
 * returns 0 if subnet is unknown or max
 */
#define nsap_subnet_id(subnet) \
	sap_subnet_hash(subnet)

/*
 * nsap identifier:
 */
#define nsap_id(type, subnet) \
	sap_hash(type, subnet, 0, 0)

/*
 * tsap identifier:
 */
#define tsap_id(type, subnet, protocol, clazz) \
	sap_hash(type, subnet, protocol, clazz)

#endif /* _NETTPI_ISO_SAP_H_ */
