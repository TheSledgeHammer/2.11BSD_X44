/*	$NetBSD: xform_ipip.c,v 1.9.16.1 2007/12/01 17:32:29 bouyer Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_ipip.c,v 1.3.2.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$OpenBSD: ip_ipip.c,v 1.25 2002/06/10 18:04:55 itojun Exp $ */

/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * The original version of this code was written by John Ioannidis
 * for BSD/OS in Athens, Greece, in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis.
 *
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 * Copyright (c) 2001, Angelos D. Keromytis.
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 * You may use this code under the GNU public license if you so wish. Please
 * contribute changes back to the authors under this freer than GPL license
 * so that we may further the use of strong encryption without limitations to
 * all.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
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
#include <sys/syslog.h>

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
    struct mbuf *m = *mp;
#if 0
	/* If we do not accept IP-in-IP explicitly, drop.  */
	if (!ipip_allow && (m->m_flags & M_IPSEC) == 0) {
		DPRINTF(("ipip6_input: dropped due to policy\n"));
		//ipipstat.ipips_pdrops++;
		m_freem(m);
		return (IPPROTO_DONE);
	}
#endif
	ipip_input(m, *offp, NULL);
	return (IPPROTO_DONE);
}
#endif /* INET6 */
