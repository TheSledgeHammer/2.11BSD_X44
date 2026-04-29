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

#include "iso_sap.h"

/*
 * Service Access Point: (In-Kernel Only)
 * - Code is based on code from ISODE. (see isoaddrs.h)
 * - For ISODE Git:
 * 		- refer to following: https://github.com/Wildboar-Software/isode
 * Planned:
 * - replace parts of the current tpi pcb for selecting different protocol stacks.
 * - setup user library
 */

/* malloctypes */
#define M_ISOSAP 	103 /* netiso service access points */

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
	struct ns_addr	ns;		/* xns */
	struct x25_addr	x25;		/* x25 */
	// sna, ipx, atm
};

/* NSAP: Network Service Access Point */
/* nsap class types */
#define NSAP_CLASS_UNKNOWN 	SAP_CLASS_UNKNOWN
#define NSAP_CLASS_CONS		SAP_CLASS_CONS
#define NSAP_CLASS_CLNS		SAP_CLASS_CLNS
#define NSAP_CLASS_MAX		SAP_CLASS_MAX

/* nsap addr */
struct nsap_addr {
	struct sap_service nsapa_service;
#define nsapa_service_addr 	nsapa_service.ns_addr
#define nsapa_service_addrlen 	nsapa_service.ns_addrlen
#define nsapa_service_class 	nsapa_service.ns_class
	union addr_union u_addr;
#define nsapa_in4 	 u_addr.in4
#define nsapa_in6 	 u_addr.in6
#define nsapa_ns 	 u_addr.ns
#define nsapa_iso 	 u_addr.iso
#define nsapa_x25 	 u_addr.x25
};

/* sockaddr nsap */
struct sockaddr_nsap {
	long snsap_type; 	/* stack type */
	long snsap_subnet;	/* subnet type */
	/* sockaddr's */
	union sockaddr_union u_sockaddr;
#define snsap_sin4 	u_sockaddr.sin4
#define snsap_sin6 	u_sockaddr.sin6
#define snsap_sns 	u_sockaddr.sns
#define snsap_siso 	u_sockaddr.siso
#define snsap_sx25 	u_sockaddr.sx25
	/* addr's */
	struct nsap_addr snsap_addr;
#define snsap_addr_in4 	snsap_addr.nsapa_in4
#define snsap_addr_in6 	snsap_addr.nsapa_in6
#define snsap_addr_ns 	snsap_addr.nsapa_ns
#define snsap_addr_iso 	snsap_addr.nsapa_iso
#define snsap_addr_x25 	snsap_addr.nsapa_x25
};

/* nsap stack types (labeled by sockaddr) */
#define NSAP_TYPE_UNKNOWN 	SAP_TYPE_UNKNOWN
#define NSAP_TYPE_SIN4		SAP_TYPE_SIN4
#define NSAP_TYPE_SIN6		SAP_TYPE_SIN6
#define NSAP_TYPE_SNS		SAP_TYPE_SNS
#define NSAP_TYPE_SISO		SAP_TYPE_SISO
#define NSAP_TYPE_SX25		SAP_TYPE_SX25
#define NSAP_TYPE_SATM		SAP_TYPE_SATM
#define NSAP_TYPE_SIPX		SAP_TYPE_SIPX
#define NSAP_TYPE_SSNA		SAP_TYPE_SSNA
#define NSAP_TYPE_MAX		SAP_TYPE_MAX

/* nsap subnet types (protocols) */
#define NSAP_SUBNET_UNKNOWN SAP_SUBNET_UNKNOWN
#define NSAP_SUBNET_IPV4 	SAP_SUBNET_IPV4
#define NSAP_SUBNET_IPV6 	SAP_SUBNET_IPV6
#define NSAP_SUBNET_CONS 	SAP_SUBNET_CONS
#define NSAP_SUBNET_CLNS 	SAP_SUBNET_CLNS
#define NSAP_SUBNET_CLNP 	SAP_SUBNET_CLNP
#define NSAP_SUBNET_ISIS 	SAP_SUBNET_ISIS
#define NSAP_SUBNET_ESIS 	SAP_SUBNET_ESIS
#define NSAP_SUBNET_IDP 	SAP_SUBNET_IDP
#define NSAP_SUBNET_X25 	SAP_SUBNET_X25
#define NSAP_SUBNET_ATM 	SAP_SUBNET_ATM
#define NSAP_SUBNET_IPX 	SAP_SUBNET_IPX
#define NSAP_SUBNET_SNA 	SAP_SUBNET_SNA
#define NSAP_SUBNET_MAX		SAP_SUBNET_MAX

/* NSAP addr (ISO/OSI equivalent) */
struct nsap_iso {
	LIST_ENTRY(nsap_iso) nsi_hash;	/* nsap */
	uint32_t nsi_type_id;		/* type id (not nsap_types) */
	uint32_t nsi_subnet_id;		/* subnet id (not nsap_subnets) */
	struct sockaddr_nsap *nsi_snsap;/* sockaddr_nsap (BSD-style) */
#define nsi_nsapa nsi_snsap->snsap_addr /* nsap_addr (BSD-style) */
#define nsi_iso   nsi_snsap->snsap_siso /* ISO/OSI sockaddr */
#define nsi_selectlen nsi_iso.siso_nlen
};

LIST_HEAD(nsapisohead, nsap_iso);

/* NSAP Table: */
struct nsapisotable {
	struct nsapisohead *nsit_hashtbl;
	u_long nsit_hash;
	uint32_t nsit_id;		/* nsap id */
};

/* TSAP: Transport Service Access Point */
/* tsap protocols */
#define TSAP_PROTOCOL_UNKNOWN 	SAP_PROTOCOL_UNKNOWN
#define TSAP_PROTOCOL_TCP 		SAP_PROTOCOL_TCP
#define TSAP_PROTOCOL_UDP 		SAP_PROTOCOL_UDP
#define TSAP_PROTOCOL_TP0 		SAP_PROTOCOL_TP0
#define TSAP_PROTOCOL_TP1 		SAP_PROTOCOL_TP1
#define TSAP_PROTOCOL_TP2 		SAP_PROTOCOL_TP2
#define TSAP_PROTOCOL_TP3 		SAP_PROTOCOL_TP3
#define TSAP_PROTOCOL_TP4 		SAP_PROTOCOL_TP4
#define TSAP_PROTOCOL_SPP 		SAP_PROTOCOL_SPP
#define TSAP_PROTOCOL_X25 		SAP_PROTOCOL_X25
#define TSAP_PROTOCOL_ATM 		SAP_PROTOCOL_ATM
#define TSAP_PROTOCOL_SPX 		SAP_PROTOCOL_SPX
#define TSAP_PROTOCOL_SNA 		SAP_PROTOCOL_SNA
#define TSAP_PROTOCOL_MAX 		SAP_PROTOCOL_MAX

/* TSAP addr (ISO/OSI equivalent) */
struct tsap_iso {
	struct nsap_iso tsi_nsaps[ISOLEN];
	struct sap_select tsi_select;
#define tsi_sid			tsi_select.ss_sid
#define tsi_af			tsi_select.ss_af
#define tsi_type		tsi_select.ss_type
#define tsi_subnet		tsi_select.ss_subnet
#define tsi_class		tsi_select.ss_class
#define tsi_protocol	tsi_select.ss_protocol
#define tsi_selector	tsi_select.ss_selector
};

/* NSAP */
extern uint32_t nsap_valid_ids[NSAP_TYPE_MAX][NSAP_SUBNET_MAX];

struct nsap_iso *nsap_alloc(struct nsapisotable *, long, long);
void nsap_free(struct nsap_iso *);
void nsap_service(struct sap_service *, char *, u_char, int);
void nsap_setsockaddr(struct sockaddr_nsap *, void *, long, long);
void nsap_setaddr(struct nsap_addr *, void *, long, int);

void nsap_init(struct nsapisotable *);
void nsap_attach(struct nsapisotable *, struct nsap_iso *, long, long);
void nsap_detach(struct nsapisotable *, struct nsap_iso *, long, long);
int nsap_connect(struct mbuf *, struct sockaddr_nsap *, long, int, int);
void nsap_disconnect(struct sockaddr_nsap *, int, int);
struct nsap_iso *nsap_lookup(struct nsapisotable *, struct sockaddr_nsap *, struct nsap_addr *, long, long);

/* TSAP */
void tsap_init(struct tsap_iso *);
void tsap_attach(struct tsap_iso *, int);
void tsap_detach(struct tsap_iso *, int);
//void tsap_select(struct tsap_iso *, long, long, long, int);
int tsap_connect(struct mbuf *, struct sockaddr_nsap *, long, int, int);
void tsap_disconnect(struct sockaddr_nsap *, int, int);
int tsap_acknowledge(struct tsap_iso *, struct sockaddr_nsap *, struct nsap_addr *, long);

#ifdef notyet
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
struct tsap_iso2 {
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
#endif

#endif /* _NETTPI_ISO_NSAP_H_ */
