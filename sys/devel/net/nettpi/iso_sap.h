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
struct sap_select {
	uint32_t ss_selector[8]; 			/* selector hash */
	int ss_sid;							/* select id */
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

static __inline int
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

static __inline long
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

static __inline long
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

static __inline long
sap_protocol_select(long protocol)
{
	long select = -1;

	switch (protocol) {
	case SAP_PROTOCOL_TCP:
		select = SAP_PROTOCOL_TCP;
		break;
	case SAP_PROTOCOL_UDP:
		select = SAP_PROTOCOL_UDP;
		break;
	case SAP_PROTOCOL_TP0:
		select = SAP_PROTOCOL_TP0;
		break;
	case SAP_PROTOCOL_TP1:
		select = SAP_PROTOCOL_TP1;
		break;
	case SAP_PROTOCOL_TP2:
		select = SAP_PROTOCOL_TP2;
		break;
	case SAP_PROTOCOL_TP3:
		select = SAP_PROTOCOL_TP3;
		break;
	case SAP_PROTOCOL_TP4:
		select = SAP_PROTOCOL_TP4;
		break;
	case SAP_PROTOCOL_SPP:
		select = SAP_PROTOCOL_SPP;
		break;
	case SAP_PROTOCOL_X25:
		select = SAP_PROTOCOL_X25;
		break;
	case SAP_PROTOCOL_ATM:
		select = SAP_PROTOCOL_ATM;
		break;
	case SAP_PROTOCOL_SPX:
		select = SAP_PROTOCOL_SPX;
		break;
	case SAP_PROTOCOL_SNA:
		select = SAP_PROTOCOL_SNA;
		break;
	case SAP_PROTOCOL_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_PROTOCOL_UNKNOWN;
		break;
	}
	return (select);
}

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
 * sap hash:
 * - generates a sap_id from the sap_type_id, sap_subnet_id,
 * sap_protocol_id and sap_class_id.
 */
static __inline uint32_t
sap_hash(long type, long subnet, long protocol, int clazz)
{
	uint32_t sap_id, type_id, subnet_id, protocol_id, class_id, hashid;
	int len;

	len = ISOLEN;
	if ((protocol == 0) && (clazz == 0)) {
        type_id = sap_type_hash(type);
        subnet_id = sap_subnet_hash(subnet);
		if ((type_id == 0) || (subnet_id == 0)) {
			len = 1;
		}
		hashid = (type_id + (subnet_id + (len - 1))) - len;
	} else {
        type_id = sap_type_hash(type);
        subnet_id = sap_subnet_hash(subnet);
        protocol_id = sap_protocol_hash(protocol);
        class_id = sap_class_hash(clazz);
		if ((type_id == 0) || (subnet_id == 0) || (protocol_id == 0)
				|| (class_id == 0)) {
			len = 1;
		}
		hashid = (type_id + (subnet_id + (len - 1)) + (protocol_id + (len - 2))
				+ (class_id + (len - 3))) - len;
	}
	sap_id = enhanced_double_hash(hashid, len);
	return (sap_id);
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
