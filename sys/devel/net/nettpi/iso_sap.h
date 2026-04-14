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
	struct sockaddr sa;
	struct sockaddr_in 	sin4;	/* ipv4 */
	struct sockaddr_in6 sin6;	/* ipv6 */
	struct sockaddr_iso siso;	/* iso */
	struct sockaddr_ns 	sns; 	/* xns */
	struct sockaddr_x25	sx25;	/* x25 */
	// sna, ipx, atm
};

/* addr union structure */
union addr_union {
	struct in_addr 	in4;	/* ipv4 */
	struct in6_addr in6;	/* ipv6 */
	struct iso_addr	iso;	/* iso */
	struct ns_addr	ns;		/* xns */
	struct x25_addr	x25;	/* x25 */
};

/* NSAP: Network Service Access Point */
#define ISOLEN 64
union nsap_service {
	char ns_addr;	/* addr */
	unsigned char ns_addrlen[ISOLEN]; /* length */
	int ns_class;
};

/* nsap addr */
struct nsap_addr {
	union nsap_service nsapa_service;
#define nsapa_addr	nsapa_service.ns_addr
#define nsapa_len	nsapa_service.ns_addrlen
#define nsapa_class nsapa_service.ns_class
	union addr_union nsapa_union;
#define nsapa_in4	nsapa_union.in4
#define nsapa_in6	nsapa_union.in6
#define nsapa_ns	nsapa_union.ns
#define nsapa_iso	nsapa_union.iso
#define nsapa_x25	nsapa_union.x25
};

/* sockaddr nsap */
struct sockaddr_nsap {
	long snsap_type; 	/* stack type */
	long snsap_subnet;	/* subnet type */
	union sockaddr_union snsap_union;
#define snsap_sa	snsap_union.sa
#define snsap_sin4	snsap_union.sin4
#define snsap_sin6	snsap_union.sin6
#define snsap_sns	snsap_union.sns
#define snsap_siso	snsap_union.siso
#define snsap_sx25	snsap_union.sx25
};

struct nsap_info {
	struct sockaddr_nsap ni_snsap; 	/* nsap sockaddr */
	struct addr_nsap ni_addr; 		/* nsap address */
	unsigned char ni_prefix[ISOLEN];
	unsigned char ni_class[ISOLEN];
};

/* stack types */
enum nsap_types {
	NSAP_TYPE_UNKNOWN,
	NSAP_TYPE_SA,
	NSAP_TYPE_SIN4,
	NSAP_TYPE_SIN6,
	NSAP_TYPE_SNS,
	NSAP_TYPE_SISO,
	NSAP_TYPE_SX25,
};

/* subnet types */
enum nsap_subnets {
	NSAP_SUBNET_UNKNOWN,
	/* inet (v4 and v6) */
	NSAP_SUBNET_TCP,
	NSAP_SUBNET_UDP,
	/* iso (native) */
	NSAP_SUBNET_CONS,
	NSAP_SUBNET_CLNS,
	/* xns */
	NSAP_SUBNET_IDP,
	/* x25 */
	NSAP_SUBNET_X25
};

/* SAP Selector (for TSAP, SSAP and PSAP) */
union sap_type {
	unsigned char st_selector[8];
	int st_port;
};

/* TSAP: Transport Service Access Point */
struct tsap_addr {
	struct nsap_addr ta_addr[ISOLEN];
	int ta_naddr;
	int ta_selectlen;
	union sap_type ta_type;
#define	ta_selector	ta_type.st_selector
#define	ta_port		ta_type.st_port
};

#endif /* SYS_DEVEL_NET_NETTPI_ISO_SAP_H_ */
