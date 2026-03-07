/*	$KAME: ip6_fw.c,v 1.41 2007/06/14 12:09:43 itojun Exp $	*/

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
 * Copyright (c) 1996 Alex Nash
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 * Obviously, it would be nice if you gave credit where credit is due
 * but requiring it would be too onerous.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

static int ip6_fw_chk(struct ip6_hdr **, struct ifnet *, struct mbuf **);
static int ip6_fw_ctl(int, struct mbuf **);

static void
ip6fw_report(struct ip6_fw *f, struct ip6_hdr *ip6,
	struct ifnet *rif, struct ifnet *oif, int off, int nxt)
{

}

static int
ip6_fw_chk(struct ip6_hdr **pip6, struct ifnet *oif, struct mbuf **m)
{

}

static struct ip6_fw *
check_ip6fw_mbuf(struct mbuf *m)
{

}

static struct ip6_fw *
check_ip6fw_struct(struct ip6_fw *frwl)
{

}

static int
ip6_fw_ctl(int stage, struct mbuf **mm)
{

}

void
ip6_fw_init(void)
{
	struct ip6_fw default_rule;

	ip6_fw_chk_ptr = ip6_fw_chk;
	ip6_fw_ctl_ptr = ip6_fw_ctl;
	LIST_INIT(&ip6_fw_chain);

	bzero(&default_rule, sizeof default_rule);
	default_rule.fw_prot = IPPROTO_IPV6;
	default_rule.fw_number = (u_short)-1;
#ifdef IPV6FIREWALL_DEFAULT_TO_ACCEPT
	default_rule.fw_flg |= IPV6_FW_F_ACCEPT;
#else
	default_rule.fw_flg |= IPV6_FW_F_DENY;
#endif
	if (check_ip6fw_struct(&default_rule) == NULL
			|| add_entry6(&ip6_fw_chain, &default_rule)) {

	}
}
