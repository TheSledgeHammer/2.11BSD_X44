/*
 * other.c
 *
 *  Created on: Mar 1, 2026
 *      Author: marti
 */


void
mld6_init(void)
{

	ip6_initpktopts(&ip6_opts);
	ip6_opts.ip6po_hbh = hbh;
}

int
rip6_output(struct mbuf *m, ...)
{
	if (control) {
		if ((error = ip6_setpktopts(control, &opt, in6p->in6p_outputopts, priv,
				so->so_proto->pr_protocol)) != 0) {
			goto bad;
		}
	}


	 bad:

	if (m)
		m_freem(m);

	freectl:

	if (control) {
		ip6_clearpktopts(&opt, -1);
		m_freem(control);
	}
}

int
udp6_output()
{
	if (control) {
		if ((error = ip6_setpktopts(control, &opt, in6p->in6p_outputopts, priv,
				IPPROTO_UDP)) != 0) {
			goto release;
		}
		optp = &opt;
	} else {
		optp = in6p->in6p_outputopts;
	}

	releaseopt:

	if (control) {
		ip6_clearpktopts(&opt, -1);
		m_freem(control);
	}
}

struct socket *
syn_cache_get(src, dst, th, hlen, tlen, so, m)
{
	if (in6p && in6totcpcb(in6p)->t_family == AF_INET6 && sotoinpcb(oso)) {
			struct in6pcb *oin6p = sotoin6pcb(oso);

			/* Inherit socket options from the listening socket. */
			in6p->in6p_flags |= (oin6p->in6p_flags & IN6P_CONTROLOPTS);
			if (oin6p->in6p_outputopts)
				in6p->in6p_outputopts =
				    ip6_copypktopts(oin6p->in6p_outputopts, M_NOWAIT);
		}
}
