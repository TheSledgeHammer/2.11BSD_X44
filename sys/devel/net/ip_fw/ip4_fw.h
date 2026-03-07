/*	$KAME: ip6_fw.h,v 1.13 2007/06/14 12:09:44 itojun Exp $	*/

/*
 * Copyright (C) 1998, 1999, 2000 and 2001 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1993 Daniel Boulet
 * Copyright (c) 1994 Ugen J.S.Antsilevich
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 * Obviously, it would be nice if you gave credit where credit is due
 * but requiring it would be too onerous.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 *
 */

#ifndef _IP4_FW_H
#define _IP4_FW_H

#include <net/if.h>

union ip4_fw_if {
	struct in_addr fu_via_ip4; 			/* Specified by IPv4 address */
	struct { 							/* Specified by interface name */
		char name[IP_FW_IFNLEN];
		short unit; 					/* -1 means match any unit */
	} fu_via_if;
};

struct ip4_fw {
	u_long_t		fw_pcnt;			/* Packet counters */
	u_long_t		fw_bcnt;			/* Byte counters */
	struct in_addr	fw_src;				/* Source IPv4 address */
	struct in_addr	fw_dst;				/* Destination IPv4 address */
	struct in_addr	fw_smsk;			/* Mask for source IPv4 address */
	struct in_addr	fw_dmsk;			/* Mask for destination IPv4 address */
    u_short 		fw_number;			/* Rule number */
    u_short 		fw_flg;				/* Flags word */
    u_int 			fw_ipflg;			/* IPv4 flags word */
    u_short 		fw_pts[IP_FW_MAX_PORTS];	/* Array of port numbers to match */
	u_char			fw_ipopt;			/* IPv4 options set */
	u_char			fw_ipnopt;			/* IPv4 options unset */
	u_char			fw_tcpf;			/* TCP flags set/unset */
	u_char			fw_tcpnf;			/* TCP flags set/unset */
    unsigned 		fw_icmptypes[IPV4_FW_ICMPTYPES_DIM]; /* ICMP types bitmap */
    long 			timestamp;			/* timestamp (tv_sec) of last match */
	union ip4_fw_if	fw_in_if;			/* Incoming interfaces */
	union ip4_fw_if	fw_out_if;			/* Outgoing interfaces */
	union {
		u_short 	fu_divert_port; 	/* Divert/tee port (options IP6DIVERT) */
		u_short 	fu_skipto_rule; 	/* SKIPTO command rule number */
		u_short 	fu_reject_code; 	/* REJECT response code */
	} fw_un;
    u_char 			fw_prot;			/* IPv4 protocol */
    u_char 			fw_nports;			/* N'of src ports and # of dst ports */
										/* in ports array (dst ports follow */
										/* src ports; max of 10 ports in all; */
										/* count of 0 means match all ports) */
};

/*
 * Values for "flags" field .
 */
#define IP_FW_F_IN		0x0001	/* Check inbound packets		*/
#define IP_FW_F_OUT		0x0002	/* Check outbound packets		*/
#define IP_FW_F_IIFACE	0x0004	/* Apply inbound interface test		*/
#define IP_FW_F_OIFACE	0x0008	/* Apply outbound interface test	*/

#define IP_FW_F_COMMAND 0x0070	/* Mask for type of chain entry:	*/
#define IP_FW_F_DENY	0x0000	/* This is a deny rule			*/
#define IP_FW_F_REJECT	0x0010	/* Deny and send a response packet	*/
#define IP_FW_F_ACCEPT	0x0020	/* This is an accept rule		*/
#define IP_FW_F_COUNT	0x0030	/* This is a count rule			*/
#define IP_FW_F_DIVERT	0x0040	/* This is a divert rule		*/
#define IP_FW_F_TEE		0x0050	/* This is a tee rule			*/
#define IP_FW_F_SKIPTO	0x0060	/* This is a skipto rule		*/
#define IP_FW_F_PIPE	0x0070	/* This is a 'pipe' rule (dummynet)	*/

#define IP_FW_F_PRN		0x0080	/* Print if this rule matches		*/

#define IP_FW_F_SRNG	0x0100	/* The first two src ports are a min	*
				 	 	 	 	 * and max range (stored in host byte	*
				 	 	 	 	 * order).				*/

#define IP_FW_F_DRNG	0x0200	/* The first two dst ports are a min	*
				 	 	 	 	 * and max range (stored in host byte	*
				 	 	 	 	 * order).				*/

#define IP_FW_F_IIFNAME	0x0400	/* In interface by name/unit (not IP)	*/
#define IP_FW_F_OIFNAME	0x0800	/* Out interface by name/unit (not IP)	*/

#define IP_FW_F_INVSRC	0x1000	/* Invert sense of src check		*/
#define IP_FW_F_INVDST	0x2000	/* Invert sense of dst check		*/

#define IP_FW_F_FRAG	0x4000	/* Fragment				*/

#define IP_FW_F_ICMPBIT 0x8000	/* ICMP type bitmap is valid		*/

#define IP_FW_F_MASK	0xFFFF	/* All possible flag bits mask		*/

/*
 * For backwards compatibility with rules specifying "via iface" but
 * not restricted to only "in" or "out" packets, we define this combination
 * of bits to represent this configuration.
 */

#define IF_FW_F_VIAHACK		(IP_FW_F_IN|IP_FW_F_OUT|IP_FW_F_IIFACE|IP_FW_F_OIFACE)

/*
 * Definitions for REJECT response codes.
 * Values less than 256 correspond to ICMP unreachable codes.
 */
#define IP_FW_REJECT_RST	0x0100		/* TCP packets: send RST */

/*
 * Definitions for IP option names.
 */
#define IP_FW_IPOPT_LSRR	0x01
#define IP_FW_IPOPT_SSRR	0x02
#define IP_FW_IPOPT_RR		0x04
#define IP_FW_IPOPT_TS		0x08

/*
 * Definitions for TCP flags.
 */
#define IP_FW_TCPF_FIN		TH_FIN
#define IP_FW_TCPF_SYN		TH_SYN
#define IP_FW_TCPF_RST		TH_RST
#define IP_FW_TCPF_PSH		TH_PUSH
#define IP_FW_TCPF_ACK		TH_ACK
#define IP_FW_TCPF_URG		TH_URG
#define IP_FW_TCPF_ESTAB	0x40

struct ip4_fw_head;
LIST_HEAD (ip4_fw_head, ip_fw_chain) ip4_fw_chain;
extern struct ip4_fw_head ip4_fw_chain;
extern int fw4_verbose;
extern char err_prefix4[];

typedef	int ip4_fw_chk_t(struct ip **, struct ifnet *, struct mbuf **);
typedef	int ip4_fw_ctl_t(int, struct mbuf **);
extern	ip4_fw_chk_t *ip4_fw_chk_ptr;
extern	ip4_fw_ctl_t *ip4_fw_ctl_ptr;

int add_entry4(struct ip4_fw_head *, struct ip4_fw *);
int del_entry4(struct ip4_fw_head *, u_short);
int zero_entry4(struct mbuf *);

#endif /* _IP4_FW_H */
