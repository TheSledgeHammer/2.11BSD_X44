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

#ifndef _NETINET6_IPIP_H_
#define _NETINET6_IPIP_H_

#ifdef _KERNEL

#ifdef IPSEC_XFORM

extern int ipip_allow;

#define	M_IPSEC	(M_AUTHIPHDR|M_AUTHIPDGM|M_DECRYPTED)

void ipip_input(struct mbuf *, int, struct ifnet *);
int ipip_output(struct mbuf *, u_char *, struct mbuf *, struct ipsecrequest *, int);

void ipip4_input(struct mbuf *, ...);
int ipip4_output(struct mbuf *, struct ipsecrequest *);

#ifdef INET6
int ipip6_input(struct mbuf **, int *, int);
int ipip6_output(struct mbuf **, int *, int);
#endif

#endif /* IPSEC_XFORM */

#endif /* _KERNEL */
#endif /* _NETINET6_IPIP_H_ */
