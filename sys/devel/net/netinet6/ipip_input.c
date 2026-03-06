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

#include <sys/cdefs.h>

#include "opt_inet.h"
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip_var.h>
#include <netinet/ip_encap.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/in6_var.h>
#endif

#include <netinet6/ipsec/ipsec.h>
#include <netinet6/ipsec/ipip.h>
#ifdef IPSEC_XFORM
#include <netinet6/ipsec/xform_tdb.h>
#endif

#include <netkey/key.h>
#include <netkey/keydb.h>

#include <machine/stdarg.h>

#include <net/net_osdep.h>

#ifdef INET
/*
 * Really only a wrapper for ipip_input(), for use with IPv4.
 */
void
ipip4_input(struct mbuf *m, ...)
{
	va_list ap;
	int iphlen;

#if 0
	/* If we do not accept IP-in-IP explicitly, drop.  */
	if (!ipip_allow && (m->m_flags & M_IPSEC) == 0) {
		DPRINTF(("ipip4_input: dropped due to policy\n"));
		//ipipstat.ipips_pdrops++;
		m_freem(m);
		return;
	}
#endif
	va_start(ap, m);
	iphlen = va_arg(ap, int);
	va_end(ap);

	ipip_input(m, iphlen, NULL);
}
#endif /* INET */

#ifdef INET6
/*
 * Really only a wrapper for ipip_input(), for use with IPv6.
 */
int
ipip6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
{
#if 0
	/* If we do not accept IP-in-IP explicitly, drop.  */
	if (!ipip_allow && ((*m)->m_flags & M_IPSEC) == 0) {
		DPRINTF(("ipip6_input: dropped due to policy\n"));
		//ipipstat.ipips_pdrops++;
		m_freem(*m);
		return (IPPROTO_DONE);
	}
#endif
	ipip_input(*m, *offp, NULL);
	return (IPPROTO_DONE);
}
#endif /* INET6 */
