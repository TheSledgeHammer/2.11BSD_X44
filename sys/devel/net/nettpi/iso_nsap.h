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

#ifndef _NETTPI_ISO_NSAP_H_
#define _NETTPI_ISO_NSAP_H_

/*
 * Service Access Point: (In-Kernel Only)
 * - Code is based on code from ISODE. (see isoaddrs.h)
 * - For ISODE Git:
 * 		- refer to following: https://github.com/Wildboar-Software/isode
 * Planned:
 * - replace parts of the current tpi pcb for selecting different protocol stacks.
 * - setup user library
 */

/* sockaddr union structure */
union sockaddr_union {
	struct sockaddr_in sin4;	/* ipv4 */
	struct sockaddr_in6 sin6;	/* ipv6 */
	struct sockaddr_iso siso;	/* iso */
	struct sockaddr_ns sns; 	/* xns */
	struct sockaddr_x25 sx25;	/* x25 */
	// sna, ipx, atm
};

/* addr union structure */
union addr_union {
	struct in_addr 	in4;		/* ipv4 */
	struct in6_addr in6;		/* ipv6 */
	struct iso_addr	iso;		/* iso */
	struct ns_addr	ns;			/* xns */
	struct x25_addr	x25;		/* x25 */
	// sna, ipx, atm
};

/* NSAP: Network Service Access Point */
#define ISOLEN 64 /* ISO MAX LEN */

union nsap_service {
	char ns_addr[ISOLEN];		/* address */
	unsigned char ns_addrlen; 	/* address length */
	int ns_class;				/* class */
};

/* nsap_service class types */
enum nsap_classes {
	NSAP_CLASS_UNKNOWN,
	/* iso (native) */
	/* Connection Oriented */
	NSAP_CLASS_CONS,
	/* Connection-less Oriented */
	NSAP_CLASS_CLNS,
};

/* nsap addr */
struct nsap_addr {
	union nsap_service nsapa_service;
#define nsapa_service_addr 	nsapa_service.ns_addr
#define nsapa_service_len 	nsapa_service.ns_addrlen
#define nsapa_service_class nsapa_service.ns_class
	union addr_union nsapa_addr;
#define nsapa_in4 	nsapa_addr.in4
#define nsapa_in6 	nsapa_addr.in6
#define nsapa_ns 	nsapa_addr.ns
#define nsapa_iso 	nsapa_addr.iso
#define nsapa_x25 	nsapa_addr.x25
};

/* sockaddr nsap */
struct sockaddr_nsap {
	long snsap_type; 	/* stack type */
	long snsap_subnet;	/* subnet type */
	/* sockaddr's */
	union sockaddr_union snsap_sockaddr;
#define snsap_sin4 	snsap_sockaddr.sin4
#define snsap_sin6 	snsap_sockaddr.sin6
#define snsap_sns 	snsap_sockaddr.sns
#define snsap_siso 	snsap_sockaddr.siso
#define snsap_sx25 	snsap_sockaddr.sx25
	/* addr's */
	union addr_union snsap_addr;
#define snsap_addr_in4 	snsap_addr.nsapa_in4
#define snsap_addr_in6 	snsap_addr.nsapa_in6
#define snsap_addr_ns 	snsap_addr.nsapa_ns
#define snsap_addr_iso 	snsap_addr.nsapa_iso
#define snsap_addr_x25 	snsap_addr.nsapa_x25
};

/* sockaddr_nsap stack types */
enum nsap_types {
	NSAP_TYPE_UNKNOWN,
	NSAP_TYPE_SIN4,
	NSAP_TYPE_SIN6,
	NSAP_TYPE_SNS,
	NSAP_TYPE_SISO,
	NSAP_TYPE_SX25,
	NSAP_TYPE_SATM,
	NSAP_TYPE_SIPX,
	NSAP_TYPE_SSNA,

	/* should alway be last */
	NSAP_TYPE_MAX
};

/* sockaddr_nsap subnet types */
/* Network Layer Protocols */
enum nsap_subnets {
	NSAP_SUBNET_UNKNOWN,
	/* inet (v4 and v6) */
	NSAP_SUBNET_IPV4,
	NSAP_SUBNET_IPV6,
	/* iso (native) */
	NSAP_SUBNET_CONS,
	NSAP_SUBNET_CLNS,
	NSAP_SUBNET_CLNP,
	NSAP_SUBNET_ISIS,
	NSAP_SUBNET_ESIS,
	/* xns */
	NSAP_SUBNET_IDP,
	/* x25 */
	NSAP_SUBNET_X25,
	/* atm */
	NSAP_SUBNET_ATM,
	/* ipx */
	NSAP_SUBNET_IPX,
	/* sna */
	NSAP_SUBNET_SNA,

	/* should alway be last */
	NSAP_SUBNET_MAX
};

/* NSAP addr (ISO/OSI equivalent) */
struct nsap_iso {
	struct sockaddr_nsap niiso_snsap; 	/*  sockaddr_nsap (BSD-style) */
	//struct nsap_addr niiso_addr; 		/* nsap_addr (BSD-style) */

	uint32_t niiso_type_id;			/* type id (not nsap_types) */
	uint32_t niiso_subnet_id;		/* subnet id (not nsap_subnets) */

};

/* ISODE Based code */
struct nsap_info {
	struct nsap_iso ni_nsap;
	unsigned char ni_prefix[ISOLEN];
	int ni_plen;
	unsigned char ni_class[ISOLEN];
	int ni_stack;
};

/* SAP Selector (Used by TSAP, SSAP and PSAP) */
union sap_type {
	unsigned char selector[8];
	int port;
};

/* TSAP: Transport Service Access Point */
/* TSAP addr (ISO/OSI equivalent) */
struct tsap_iso {
	struct nsap_iso tiiso_addr[ISOLEN];
	int tiiso_naddr;
	int tiiso_selectlen;
	union sap_type tiiso_type;
#define	tiiso_selector tiiso_type.selector
#define	tiiso_port tiiso_type.port
};

/* SSAP: Session Service Access Point */
/* SSAP addr (ISO/OSI equivalent) */
struct ssap_iso {
	struct tsap_iso siiso_addr[ISOLEN];
	union sap_type siiso_type;
#define	siiso_selector siiso_type.selector
#define	siiso_port siiso_type.port
};

/* PSAP: Presentation Service Access Point */
/* PSAP addr (ISO/OSI equivalent) */
struct psap_iso {
	struct ssap_iso piiso_addr[ISOLEN];
	union sap_type piiso_type;
#define	piiso_selector piiso_type.selector
#define	piiso_port piiso_type.port
};

#endif /* _NETTPI_ISO_NSAP_H_ */
