/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)tuba_table.h	8.2 (Berkeley) 7/13/94
 */

#ifndef _NETISO_TUBA_TABLE_H_
#define _NETISO_TUBA_TABLE_H_

struct tuba_cache {
    struct radix_node      	tc_nodes[2];/* convenient lookup */
    int	                    tc_refcnt;
	int	                    tc_time;	/* last looked up */
	int	                    tc_flags;
    int	                    tc_index;
	u_short	                tc_sum;		/* cksum of nsap inc. length */
	u_short	                tc_ssum;	/* swab(tc_sum) */
	struct sockaddr_iso    	tc_siso;	/* for responding */
	struct iso_addr        	tc_addr;
	struct callout			tc_callout; /* callouts */
};

#define ADDCARRY(x)  (x >= 65535 ? x -= 65535 : x)
#define REDUCE(a, b) \
	{ union { u_short s[2]; long l;} l_util; long x; \
	l_util.l = (b); x = l_util.s[0] + l_util.s[1]; ADDCARRY(x); \
	if (x == 0) x = 0xffff; a = x;}

#define SWAB(a, b) \
		{ union { u_char c[2]; u_short s;} s; u_char t; \
		s.s = (b); t = s.c[0]; s.c[0] = s.c[1]; s.c[1] = t; a = s.s;}

#ifdef _KERNEL
extern int tuba_table_size;
extern struct tuba_cache **tuba_table;
extern struct radix_node_head *tuba_tree;
extern struct inpcbtable tubainptable;
extern struct isopcbtable tubaisoptable;

void tuba_init(void);
void tuba_table_init(void);
int tuba_cksum(u_long *, int *, struct sockaddr_iso **, u_long);
int tuba_pcbattach(struct socket *, int);
void tuba_pcbdetach(struct isopcb *);
void tuba_refcnt(struct isopcb *, int);

int tuba4_pcbconnect(void *, struct mbuf *);
void tuba4_mbuf(struct mbuf *, struct sockaddr_iso *, struct sockaddr_iso *, int,
		int, size_t);
int tuba4_tcp_ouput(struct mbuf *, struct tcpcb *);
void tuba4_tcp_input(struct mbuf *, struct sockaddr_iso *, struct sockaddr_iso *,
		struct ip *, struct tcphdr *, int, int, u_long, u_long);

int tuba6_pcbconnect(void *, struct mbuf *);
void tuba6_mbuf(struct mbuf *, struct sockaddr_iso *, struct sockaddr_iso *, int,
		int, size_t);
int tuba6_tcp_output(struct mbuf *, struct tcpcb *);
void tuba6_tcp_input(struct mbuf *, struct sockaddr_iso *, struct sockaddr_iso *,
		struct ip6_hdr *, struct tcphdr *, int, int, u_long, u_long);

#endif
#endif /* _NETISO_TUBA_TABLE_H_ */
