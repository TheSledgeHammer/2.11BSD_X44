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

struct sap_select {
	unsigned char selector[8]; /* unused */
	long ss_type;		/* sockaddr (from nsap) */
	long ss_subnet;		/* network protocol (from nsap) */
	long ss_protocol;	/* transport protocol (from tsap) */
	int ss_class;		/* connection orientation (from nsap) */
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

#endif /* _NETTPI_ISO_SAP_H_ */
