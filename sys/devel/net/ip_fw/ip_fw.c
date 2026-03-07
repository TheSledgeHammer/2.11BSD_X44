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

/*
 * Implement IPv4/v6 packet firewall
 */

#include <sys/cdefs.h>

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <sys/null.h>

#include "ip_fw.h"
#include "ip4_fw.h"
#include "ip6_fw.h"

#include <net/net_osdep.h>

#ifndef M_IP6FW
#define M_IP6FW	M_TEMP
#endif
#ifndef M_IP4FW
#define M_IP4FW	M_TEMP
#endif

#ifndef LOG_SECURITY
#define LOG_SECURITY LOG_AUTH
#endif
static int fw6_debug = 1;
#ifdef IPV6FIREWALL_VERBOSE
static int fw6_verbose = 1;
#else
static int fw6_verbose = 0;
#endif

struct ip4_fw_head ip4_fw_chain;
struct ip6_fw_head ip6_fw_chain;

char err_prefix4[] = "ip4_fw_ctl:";
char err_prefix6[] = "ip6_fw_ctl:";

int
add_entry4(struct ip4_fw_head *chainptr, struct ip4_fw *frwl)
{
	struct ip4_fw *ftmp = NULL;
	struct ip_fw_chain *fwc = NULL, *fcp, *fcpl = NULL;
	u_short nbr = 0;
	int s;

	fwc = malloc(sizeof *fwc, M_IP4FW, M_NOWAIT);
	ftmp = malloc(sizeof *ftmp, M_IP4FW, M_NOWAIT);
	if (!fwc || !ftmp) {
		dprintf(("%s malloc said no\n", err_prefix4));
		if (fwc) {
			free(fwc, M_IP4FW);
		}
		if (ftmp) {
			free(ftmp, M_IP4FW);
		}
		return (ENOSPC);
	}

	bcopy(frwl, ftmp, sizeof(struct ip4_fw));
	ftmp->fw_in_if.fu_via_if.name[IP_FW_IFNLEN - 1] = '\0';
	ftmp->fw_pcnt = 0L;
	ftmp->fw_bcnt = 0L;
	fwc->rule4 = ftmp;

	s = splnet();

	if (!LIST_FIRST(chainptr)) {
		LIST_INSERT_HEAD(chainptr, fwc, chain4);
		splx(s);
		return (0);
	} else if (ftmp->fw_number == (u_short) -1) {
		if (fwc) {
			free(fwc, M_IP4FW);
		}
		if (ftmp) {
			free(ftmp, M_IP4FW);
		}
		splx(s);
		dprintf(("%s bad rule number\n", err_prefix4));
		return (EINVAL);
	}

	/* If entry number is 0, find highest numbered rule and add 100 */
	if (ftmp->fw_number == 0) {
		LIST_FOREACH(fcp, chainptr, chain4) {
			if (fcp->rule4->fw_number != (u_short)-1) {
				nbr = fcp->rule4->fw_number;
			} else {
				break;
			}
		}
		if (nbr < (u_short)-1 - 100) {
			nbr += 100;
		}
		ftmp->fw_number = nbr;
	}

	/* Got a valid number; now insert it, keeping the list ordered */
	LIST_FOREACH(fcp, chainptr, chain4) {
		if (fcp->rule4->fw_number > ftmp->fw_number) {
			if (fcpl) {
				LIST_INSERT_AFTER(fcpl, fwc, chain4);
			} else {
				LIST_INSERT_HEAD(chainptr, fwc, chain4);
			}
			break;
		} else {
			fcpl = fcp;
		}
	}

	splx(s);
	return (0);
}

int
del_entry4(struct ip4_fw_head *chainptr, u_short number)
{
	struct ip_fw_chain *fcp;
	int s;

	s = splnet();

	fcp = LIST_FIRST(chainptr);
	if (number != (u_short)-1) {
		for (; fcp; fcp = LIST_NEXT(fcp, chain4)) {
			if (fcp->rule4->fw_number == number) {
				LIST_REMOVE(fcp, chain4);
				splx(s);
				free(fcp->rule4, M_IP4FW);
				free(fcp, M_IP4FW);
				return 0;
			}
		}
	}

	splx(s);
	return (EINVAL);
}

int
zero_entry4(struct mbuf *m)
{
	struct ip4_fw *frwl;
	struct ip_fw_chain *fcp;
	int s;

	if (m && m->m_len != 0) {
		if (m->m_len != sizeof(struct ip4_fw)) {
			return (EINVAL);
		}
		frwl = mtod(m, struct ip4_fw *);
	} else {
		frwl = NULL;
	}

	/*
	 *	It's possible to insert multiple chain entries with the
	 *	same number, so we don't stop after finding the first
	 *	match if zeroing a specific entry.
	 */
	s = splnet();
	LIST_FOREACH(fcp, &ip6_fw_chain, chain4) {
		if (!frwl || frwl->fw_number == fcp->rule4->fw_number) {
			fcp->rule4->fw_bcnt = fcp->rule4->fw_pcnt = 0;
			fcp->rule4->timestamp = 0;
		}
	}
	splx(s);

	if (fw4_verbose) {
		if (frwl) {
			log(LOG_SECURITY | LOG_NOTICE, "ip4fw: Entry %d cleared.\n",
					frwl->fw_number);
		} else {
			log(LOG_SECURITY | LOG_NOTICE, "ip4fw: Accounting cleared.\n");
		}
	}
	return (0);
}

int
add_entry6(struct ip6_fw_head *chainptr, struct ip6_fw *frwl)
{
	struct ip6_fw *ftmp = NULL;
	struct ip_fw_chain *fwc = NULL, *fcp, *fcpl = NULL;
	u_short nbr = 0;
	int s;

	fwc = malloc(sizeof *fwc, M_IP6FW, M_NOWAIT);
	ftmp = malloc(sizeof *ftmp, M_IP6FW, M_NOWAIT);
	if (!fwc || !ftmp) {
		dprintf(("%s malloc said no\n", err_prefix6));
		if (fwc) {
			free(fwc, M_IP6FW);
		}
		if (ftmp) {
			free(ftmp, M_IP6FW);
		}
		return (ENOSPC);
	}

	bcopy(frwl, ftmp, sizeof(struct ip6_fw));
	ftmp->fw_in_if.fu_via_if.name[IP_FW_IFNLEN - 1] = '\0';
	ftmp->fw_pcnt = 0L;
	ftmp->fw_bcnt = 0L;
	fwc->rule6 = ftmp;

	s = splnet();

	if (!LIST_FIRST(chainptr)) {
		LIST_INSERT_HEAD(chainptr, fwc, chain6);
		splx(s);
		return (0);
	} else if (ftmp->fw_number == (u_short) -1) {
		if (fwc) {
			free(fwc, M_IP6FW);
		}
		if (ftmp) {
			free(ftmp, M_IP6FW);
		}
		splx(s);
		dprintf(("%s bad rule number\n", err_prefix6));
		return (EINVAL);
	}

	/* If entry number is 0, find highest numbered rule and add 100 */
	if (ftmp->fw_number == 0) {
		LIST_FOREACH(fcp, chainptr, chain6) {
			if (fcp->rule6->fw_number != (u_short)-1) {
				nbr = fcp->rule6->fw_number;
			} else {
				break;
			}
		}
		if (nbr < (u_short)-1 - 100) {
			nbr += 100;
		}
		ftmp->fw_number = nbr;
	}

	/* Got a valid number; now insert it, keeping the list ordered */
	LIST_FOREACH(fcp, chainptr, chain6) {
		if (fcp->rule6->fw_number > ftmp->fw_number) {
			if (fcpl) {
				LIST_INSERT_AFTER(fcpl, fwc, chain6);
			} else {
				LIST_INSERT_HEAD(chainptr, fwc, chain6);
			}
			break;
		}  else {
			fcpl = fcp;
		}
	}

	splx(s);
	return (0);
}

int
del_entry6(struct ip6_fw_head *chainptr, u_short number)
{
	struct ip_fw_chain *fcp;
	int s;

	s = splnet();

	fcp = LIST_FIRST(chainptr);
	if (number != (u_short)-1) {
		for (; fcp; fcp = LIST_NEXT(fcp, chain6)) {
			if (fcp->rule6->fw_number == number) {
				LIST_REMOVE(fcp, chain6);
				splx(s);
				free(fcp->rule6, M_IP6FW);
				free(fcp, M_IP6FW);
				return 0;
			}
		}
	}

	splx(s);
	return (EINVAL);
}

int
zero_entry6(struct mbuf *m)
{
	struct ip6_fw *frwl;
	struct ip_fw_chain *fcp;
	int s;

	if (m && m->m_len != 0) {
		if (m->m_len != sizeof(struct ip6_fw)) {
			return (EINVAL);
		}
		frwl = mtod(m, struct ip6_fw *);
	} else {
		frwl = NULL;
	}

	/*
	 *	It's possible to insert multiple chain entries with the
	 *	same number, so we don't stop after finding the first
	 *	match if zeroing a specific entry.
	 */
	s = splnet();
	LIST_FOREACH(fcp, &ip6_fw_chain, chain6) {
		if (!frwl || frwl->fw_number == fcp->rule6->fw_number) {
			fcp->rule6->fw_bcnt = fcp->rule6->fw_pcnt = 0;
			fcp->rule6->timestamp = 0;
		}
	}
	splx(s);

	if (fw6_verbose) {
		if (frwl) {
			log(LOG_SECURITY | LOG_NOTICE, "ip6fw: Entry %d cleared.\n",
					frwl->fw_number);
		} else {
			log(LOG_SECURITY | LOG_NOTICE, "ip6fw: Accounting cleared.\n");
		}
	}
	return (0);
}
