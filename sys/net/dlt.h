/*	$NetBSD: dlt.h,v 1.7 2003/11/16 09:02:42 dyoung Exp $	*/

/*
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)bpf.h	8.2 (Berkeley) 1/9/95
 * @(#) Header: bpf.h,v 1.36 97/06/12 14:29:53 leres Exp  (LBL)
 */

#ifndef _NET_DLT_H_
#define _NET_DLT_H_

/*
 * Data-link level type codes.
 */
#define DLT_NULL	0	/* no link-layer encapsulation */
#define DLT_EN10MB	1	/* Ethernet (10Mb) */
#define DLT_EN3MB	2	/* Experimental Ethernet (3Mb) */
#define DLT_AX25	3	/* Amateur Radio AX.25 */
#define DLT_PRONET	4	/* Proteon ProNET Token Ring */
#define DLT_CHAOS	5	/* Chaos */
#define DLT_IEEE802	6	/* IEEE 802 Networks */
#define DLT_ARCNET	7	/* ARCNET */
#define DLT_SLIP	8	/* Serial Line IP */
#define DLT_PPP		9	/* Point-to-point Protocol */
#define DLT_FDDI	10	/* FDDI */
#define DLT_ATM_RFC1483	11	/* LLC/SNAP encapsulated atm */
#define DLT_RAW		12	/* raw IP */
#define DLT_SLIP_BSDOS	13	/* BSD/OS Serial Line IP */
#define DLT_PPP_BSDOS	14	/* BSD/OS Point-to-point Protocol */
#define DLT_HIPPI	15	/* HIPPI */
#define DLT_HDLC	16	/* HDLC framing */

#define DLT_PFSYNC	18	/* Packet filter state syncing */
#define DLT_ATM_CLIP	19	/* Linux Classical-IP over ATM */
#define DLT_ENC		109	/* Encapsulated packets for IPsec */
#define DLT_LINUX_SLL	113	/* Linux cooked sockets */
#define DLT_LTALK	114	/* Apple LocalTalk hardware */
#define DLT_PFLOG	117	/* Packet filter logging, by pcap people */
#define DLT_CISCO_IOS	118	/* Registered for Cisco-internal use */

/* NetBSD-specific types */
#define	DLT_PPP_SERIAL	50	/* PPP over serial (async and sync) */
#define	DLT_PPP_ETHER	51	/* XXX - deprecated! PPP over Ethernet; session only, w/o ether header */

/* Axent Raptor / Symantec Enterprise Firewall */
#define DLT_SYMANTEC_FIREWALL	99

#define DLT_C_HDLC		104	/* Cisco HDLC */
#define DLT_IEEE802_11		105	/* IEEE 802.11 wireless */
#define DLT_FRELAY		107	/* Frame Relay */
#define DLT_LOOP		108	/* OpenBSD DLT_LOOP */
#define DLT_ECONET		115	/* Acorn Econet */
#define DLT_PRISM_HEADER	119	/* 802.11 header plus Prism II info. */
#define DLT_AIRONET_HEADER 	120	/* 802.11 header plus Aironet info. */
#define DLT_HHDLC		121	/* Reserved for Siemens HiPath HDLC */
#define DLT_IP_OVER_FC		122	/* RFC 2625 IP-over-Fibre Channel */
#define DLT_SUNATM		123	/* Solaris+SunATM */
#define DLT_RIO                 124     /* RapidIO */
#define DLT_PCI_EXP             125     /* PCI Express */
#define DLT_AURORA              126     /* Xilinx Aurora link layer */
#define DLT_IEEE802_11_RADIO 	127	/* 802.11 header plus radio info. */
#define DLT_TZSP                128     /* Tazmen Sniffer Protocol */
#define DLT_ARCNET_LINUX	129	/* ARCNET */
#define DLT_JUNIPER_MLPPP       130	/* Juniper-private data link types. */
#define DLT_JUNIPER_MLFR        131
#define DLT_JUNIPER_ES          132
#define DLT_JUNIPER_GGSN        133
#define DLT_JUNIPER_MFR         134
#define DLT_JUNIPER_ATM2        135
#define DLT_JUNIPER_SERVICES    136
#define DLT_JUNIPER_ATM1        137
#define DLT_APPLE_IP_OVER_IEEE1394	138	/* Apple IP-over-IEEE 1394 */

/*
 * 139 through 142 are reserved for SS7.
 */

#define DLT_DOCSIS		143	/* Reserved for DOCSIS MAC frames. */
#define DLT_LINUX_IRDA		144	/* Linux-IrDA packets */

/* Reserved for IBM SP switch and IBM Next Federation switch. */
#define DLT_IBM_SP		145
#define DLT_IBM_SN		146

#define DLT_IEEE802_11_RADIO_AVS	163	/* 802.11 plus AVS header */
#define DLT_JUNIPER_MONITOR     164	/* Juniper-private data link type */

/*
 * NetBSD-specific generic "raw" link type.  The upper 16-bits indicate
 * that this is the generic raw type, and the lower 16-bits are the
 * address family we're dealing with.
 */
#define	DLT_RAWAF_MASK		    0x02240000
#define	DLT_RAWAF(af)		      (DLT_RAWAF_MASK | (af))
#define	DLT_RAWAF_AF(x)		    ((x) & 0x0000ffff)
#define	DLT_IS_RAWAF(x)		    (((x) & 0xffff0000) == DLT_RAWAF_MASK)

#endif /* _NET_DLT_H_ */
