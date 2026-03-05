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
__KERNEL_RCSID(0, "$NetBSD: xform_ipip.c,v 1.9.16.1 2007/12/01 17:32:29 bouyer Exp $");

/*
 * IP-inside-IP processing
 */

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>

#include <netinet6/ipsec/ipsec.h>
#include <devel/net/netinet6/ipip.h>
#include <netinet6/ipsec/xform_tdb.h>

#include <machine/stdarg.h>

#include <net/pfkeyv2.h>
#include <netkey/key.h>
#include <netkey/keydb.h>

#include <net/net_osdep.h>

#define IPEV4 		IPVERSION
#define IPEV6 		(IPV6_VERSION >> 4)
#define TPV(tp)		(((tp) >> 4) & 0xff)

static int ipe4_output_af(struct mbuf *, struct ipsecrequest *, struct secasindex *, struct mbuf **, int, int, int *, int);
#ifdef INET
static int ipv4_init(struct mbuf *, struct secasvar *, int, int, int *, struct in_addr *, struct in_addr *, int);
#endif
#ifdef INET6
static int ipv6_init(struct mbuf *, struct secasvar *, int, int, int *, struct in6_addr *, struct in6_addr *, int);
#endif

int
ipip_output(m, nexthdrp, md, isr, af)
	struct mbuf *m;
	u_char *nexthdrp;
	struct mbuf *md;
	struct ipsecrequest *isr;
	int af;
{
	struct mbuf *mprev;
	struct secasvar *sav;
	int skip, length, offset;

	sav = isr->sav;

	switch (af) {
#ifdef INET
	case AF_INET:
		skip = sizeof(struct ip);
		length = htons(m->m_pkthdr.len);
		offset = 0;
		*nexthdrp = IPPROTO_IPIP;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		skip = sizeof(struct ip6_hdr);
		length = htons(m->m_pkthdr.len);
		offset = 0;
		*nexthdrp = IPPROTO_IPV6;
		break;
#endif
	default:
		ipseclog((LOG_ERR, "ipip_output: unsupported af %d\n", af));
		return (0);
	}

	for (mprev = m; mprev && mprev->m_next != md; mprev = mprev->m_next) {
		;
	}
	if (mprev == NULL || mprev->m_next != md) {
		ipseclog((LOG_DEBUG, "ipip%d_output: md is not in chain\n", af));
		m_freem(m);
		return (EINVAL);
	}

	/* make the packet over-writable */
	mprev->m_next = NULL;
	if ((md = ipsec_copypkt(md)) == NULL) {
		m_freem(m);
		return (ENOBUFS);
	}
	mprev->m_next = md;

	return (ipe4_output_af(m, isr, &sav->sah->saidx, &mprev, skip, length, &offset, af));
}

static int
ipe4_init(sav, xsp)
	struct secasvar *sav;
	struct xformsw *xsp;
{
	struct tdb *tdb;

	tdb = tdb_init(sav, NULL, NULL, NULL, NULL, IPPROTO_IPIP);
	if (tdb == NULL) {
		ipseclog((LOG_ERR, "ipe4_init: no memory for tunnel descriptor block\n"));
		return (EINVAL);
	}
	return (0);
}

static int
ipe4_zeroize(sav)
	struct secasvar *sav;
{
	return (tdb_zeroize(sav->tdb_tdb));
}

static int
ipe4_input(m, sav, skip, length, offset)
	struct mbuf *m;
	struct secasvar *sav;
	int skip, length, offset;
{
	/* This is a rather serious mistake, so no conditional printing. */
	printf("ipe4_input: should never be called\n");
	if (m) {
		m_freem(m);
	}
	return (EOPNOTSUPP);
}

static int
ipe4_output(m, isr, mp, skip, length, offset)
	struct mbuf *m;
	struct ipsecrequest *isr;
	struct mbuf **mp;
	int skip, length, offset;
{
	struct secasvar *sav;
	struct secasindex *saidx;
	int error, af;

	sav = isr->sav;
	saidx = &sav->sah->saidx;
	af = saidx->dst.sa.sa_family;
	switch (af) {
#ifdef INET
	case AF_INET:
		skip = sizeof(struct ip);
		length = htons(m->m_pkthdr.len);
		error = ipe4_output_af(m, isr, saidx, mp, skip, length, &offset, AF_INET);
		if (error != 0) {
			return (error);
		}
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		skip = sizeof(struct ip6_hdr);
		length = htons(m->m_pkthdr.len);
		error = ipe4_output_af(m, isr, saidx, mp, skip, length, &offset, AF_INET6);
		if (error != 0) {
			return (error);
		}
		break;
#endif /* INET6 */
	default:
		error = EAFNOSUPPORT; /* XXX diffs from openbsd */
		return (error);
	}

	*mp = m;
	return (0);
}

static int
ipe4_output_af(m, isr, saidx, mp, skip, length, offset, af)
	struct mbuf *m;
	struct ipsecrequest *isr;
	struct secasindex *saidx;
	struct mbuf **mp;
	int skip, length, *offset, af;
{
	u_int8_t tp;
	int error;
#ifdef INET
	struct in_addr *src4, *dst4;
#endif
#ifdef INET6
	struct in6_addr *src6, *dst6;
#endif

	/* XXX Deal with empty TDB source/destination addresses. */

	m_copydata(m, 0, 1, &tp);
	tp = TPV(tp); /* Get the IP version number. */

	switch (af) {
#ifdef INET
	case AF_INET:
		src4 = saidx->src.sin.sin_addr;
		dst4 = saidx->dst.sin.sin_addr;
		if (TPV(tp) == IPEV4) {
			error = ipv4_init(m, isr->sav, skip, length, offset, src4, dst4, AF_INET);
		}
#ifdef INET6
		else if (TPV(tp) == IPEV6) {
			error = ipv4_init(m, isr->sav, skip, length, offset, src4, dst4, AF_INET6);
		}
#endif /* INET6 */
		else {
			goto nofamily;
		}
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		src6 = saidx->src.sin6.sin6_addr;
		dst6 = saidx->dst.sin6.sin6_addr;
#ifdef INET
		if (TPV(tp) == IPEV4) {
			error = ipv6_init(m, isr->sav, skip, length, offset, src6, dst6, AF_INET);
		} else
#endif /* INET */
		if (TPV(tp) == IPEV6) {
			error = ipv6_init(m, isr->sav, skip, length, offset, src6, dst6, AF_INET6);
		}
		else {
			goto nofamily;
		}
		break;
#endif /* INET6 */
	default:
nofamily:
		ipseclog((LOG_ERR, "ipe4_output: unsupported protocol family %u\n", saidx->dst.sa.sa_family));
		//ipipstat.ipips_family++;
		error = EAFNOSUPPORT; /* XXX diffs from openbsd */
		goto bad;
	}

	//ipipstat.ipips_opackets++;
	//*mp = m;
	return (0);

bad:
	if (m) {
		m_freem(m);
	}
	*mp = NULL;
	return (error);
}

#ifdef INET
static int
ipv4_init(m, sav, skip, length, offset, src, dst, af)
	struct mbuf *m;
	struct secasvar *sav;
	int skip, length, *offset, af;
	struct in_addr *src, *dst;
{
	struct ip *ipo;
	u_int8_t itos, otos;
	int error;

	if (af != AF_INET || src->s_addr == INADDR_ANY || dst->s_addr == INADDR_ANY) {
		char *addr4 = (char *)ip_sprintf(dst);
		ipseclog((LOG_ERR, "ipe4_output: unspecified tunnel endpoint "
				"address in SA %s/%08lx\n", addr4, (u_long)ntohl(sav->spi)));
		error = EINVAL;
		goto bad;
	}

	M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
	if (m == 0) {
		ipseclog((LOG_ERR, "ipe4_output: M_PREPEND failed\n"));
		//ipipstat.ipips_hdrops++;
		error = ENOBUFS;
		goto bad;
	}

	ipo = mtod(m, struct ip *);
	ipo->ip_v = IPVERSION;
	ipo->ip_hl = 5;
	ipo->ip_len = (u_int16_t)length;
	ipo->ip_ttl = ip_defttl;
	ipo->ip_sum = 0;
	ipo->ip_src = src;
	ipo->ip_dst = dst;
	ipo->ip_id = ip_newid();

	switch (af) {
	case AF_INET:
		/* Save ECN notification */
		*offset = offsetof(struct ip, ip_tos);
		m_copydata(m, skip + offset, sizeof(u_int8_t), (caddr_t)&itos);

		ipo->ip_p = IPPROTO_IPIP;

		/*
		 * We should be keeping tunnel soft-state and
		 * send back ICMPs if needed.
		 */
		*offset = offsetof(struct ip, ip_off);
		m_copydata(m, skip + *offset, sizeof(u_int16_t), (caddr_t)&ipo->ip_off);
		ipo->ip_off &= ~IP_OFF_CONVERT(IP_DF | IP_MF | IP_OFFMASK);
		break;
#ifdef INET6
	case AF_INET6:
		u_int32_t itos32;

		/* Save ECN notification. */
		*offset = offsetof(struct ip6_hdr, ip6_flow);
		m_copydata(m, skip + *offset, sizeof(u_int32_t), (caddr_t)&itos32);
		itos = ntohl(itos32) >> 20;
		ipo->ip_p = IPPROTO_IPV6;
		ipo->ip_off = 0;
		break;
#endif /* INET6 */
	default:
		goto bad;
	}

	otos = 0;
	ip_ecn_ingress(ECN_ALLOWED, &otos, &itos);
	ipo->ip_tos = otos;
	return (0);

bad:
	if (m) {
		m_freem(m);
	}
	return (error);
}
#endif /* INET */

#ifdef INET6
static int
ipv6_init(m, sav, skip, length, offset, src, dst, af)
	struct mbuf *m;
	struct secasvar *sav;
	int skip, length, *offset, af;
	struct in6_addr *src, *dst;
{
	struct ip6_hdr *ip6o, *ip6;
	u_int8_t itos, otos;
	int error;

	if (IN6_IS_ADDR_UNSPECIFIED(dst) ||
			af != AF_INET6 ||
			IN6_IS_ADDR_UNSPECIFIED(src)) {
		char *addr6 = (char *)ip6_sprintf(dst);
		ipseclog((LOG_ERR, "ipe4_output: unspecified tunnel endpoint "
				"address in SA %s/%08lx\n", addr6, (u_long) ntohl(sav->spi)));
		error = ENOBUFS;
		goto bad;
	}

	/* scoped address handling */
	ip6 = mtod(m, struct ip6_hdr *);
	if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
		ip6->ip6_src.s6_addr16[1] = 0;
	}
	if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_dst)) {
		ip6->ip6_dst.s6_addr16[1] = 0;
	}

	M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
	if (m == 0) {
		ipseclog((LOG_ERR, "ipe4_output: M_PREPEND failed\n"));
		//ipipstat.ipips_hdrops++;
		error = ENOBUFS;
		goto bad;
	}

	/* Initialize IPv6 header */
	ip6o = mtod(m, struct ip6_hdr *);
	ip6o->ip6_flow = 0;
	ip6o->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6o->ip6_vfc |= IPV6_VERSION;
	ip6o->ip6_plen = length;
	ip6o->ip6_hlim = ip_defttl;
	ip6o->ip6_dst = dst;
	ip6o->ip6_src = src;

	switch (af) {
#ifdef INET
	case AF_INET:
		/* Save ECN notification */
		*offset = offsetof(struct ip, ip_tos);
		m_copydata(m, skip + offset, sizeof(u_int8_t), (caddr_t)&itos);

		/* This is really IPVERSION. */
		ip6o->ip6_nxt = IPPROTO_IPIP;
		break;
#endif /* INET */
	case AF_INET6:
		u_int32_t itos32;

		/* Save ECN notification. */
		*offset = offsetof(struct ip6_hdr, ip6_flow);
		m_copydata(m, skip + offset, sizeof(u_int32_t), (caddr_t)&itos32);
		itos = ntohl(itos32) >> 20;

		ip6o->ip6_nxt = IPPROTO_IPV6;
		break;
	default:
		goto bad;
	}

	otos = 0;
	ip_ecn_ingress(ECN_ALLOWED, &otos, &itos);
	ip6o->ip6_flow |= htonl((u_int32_t) otos << 20);
	return (0);

bad:
	if (m) {
		m_freem(m);
	}
	return (error);
}
#endif /* INET6 */

static struct xformsw ipe4_xformsw = {
		.xf_type = XF_IP4,
		.xf_flags = 0,
		.xf_name = "Kame IPsec IPv4 Simple Encapsulation",
		.xf_init = ipe4_init,
		.xf_zeroize = ipe4_zeroize,
		.xf_input = ipe4_input,
		.xf_output = ipe4_output,
};

extern struct domain inetdomain;
static struct ipprotosw ipe4_protosw[] = {
		{

		},
		{

		},
};

/*
 * Check the encapsulated packet to see if we want it
 */
static int
ipe4_encapcheck(m, off, proto, arg)
	const struct mbuf *m;
	int off, proto;
	void *arg;
{
	/*
	 * Only take packets coming from IPSEC tunnels; the rest
	 * must be handled by the gif tunnel code.  Note that we
	 * also return a minimum priority when we want the packet
	 * so any explicit gif tunnels take precedence.
	 */
	return ((m->m_flags & M_IPSEC) != 0 ? 1 : 0);
}

void
ipe4_attach(void)
{
	xform_register(&ipe4_xformsw);
	/* attach to encapsulation framework */
	/* XXX save return cookie for detach on module remove */
	(void) encap_attach_func(AF_INET, -1,
		ipe4_encapcheck, (struct protosw*) &ipe4_protosw[0], NULL);
#ifdef INET6
	(void) encap_attach_func(AF_INET6, -1,
		ipe4_encapcheck, (struct protosw*) &ipe4_protosw[1], NULL);
#endif
}
