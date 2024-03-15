/*	$NetBSD: scope6.c,v 1.6 2007/12/11 12:30:20 lukem Exp $	*/
/*	$KAME$	*/

/*-
 * Copyright (C) 2000 WIDE Project.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scope6.c,v 1.6 2007/12/11 12:30:20 lukem Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/errno.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>

#include <netinet6/in6_var.h>
#include <netinet6/scope6_var.h>

#ifdef ENABLE_DEFAULT_SCOPE
int ip6_use_defzone = 1;
#else
int ip6_use_defzone = 0;
#endif

static struct scope6_id sid_default;
#define SID(ifp) \
	(((struct in6_ifextra *)(ifp)->if_afdata[AF_INET6])->scope6_id)

void
scope6_init()
{

	memset(&sid_default, 0, sizeof(sid_default));
}

struct scope6_id *
scope6_ifattach(struct ifnet *ifp)
{
	struct scope6_id *sid;

	sid = (struct scope6_id *)malloc(sizeof(*sid), M_IFADDR, M_WAITOK);
	memset(sid, 0, sizeof(*sid));

	/*
	 * XXX: IPV6_ADDR_SCOPE_xxx macros are not standard.
	 * Should we rather hardcode here?
	 */
	sid->s6id_list[IPV6_ADDR_SCOPE_INTFACELOCAL] = ifp->if_index;
	sid->s6id_list[IPV6_ADDR_SCOPE_LINKLOCAL] = ifp->if_index;
#ifdef MULTI_SCOPE
	/* by default, we don't care about scope boundary for these scopes. */
	sid->s6id_list[IPV6_ADDR_SCOPE_SITELOCAL] = 1;
	sid->s6id_list[IPV6_ADDR_SCOPE_ORGLOCAL] = 1;
#endif

	return sid;
}

void
scope6_ifdetach(struct scope6_id *sid)
{

	free(sid, M_IFADDR);
}

int
scope6_set(struct ifnet *ifp, const struct scope6_id *idlist)
{
	int i;
	int error = 0;
	struct scope6_id *sid = NULL;

	sid = SID(ifp);

	if (!sid)	/* paranoid? */
		return (EINVAL);

	/*
	 * XXX: We need more consistency checks of the relationship among
	 * scopes (e.g. an organization should be larger than a site).
	 */

	/*
	 * TODO(XXX): after setting, we should reflect the changes to
	 * interface addresses, routing table entries, PCB entries...
	 */

	for (i = 0; i < 16; i++) {
		if (idlist->s6id_list[i] &&
		    idlist->s6id_list[i] != sid->s6id_list[i]) {
			/*
			 * An interface zone ID must be the corresponding
			 * interface index by definition.
			 */
			if (i == IPV6_ADDR_SCOPE_INTFACELOCAL &&
			    idlist->s6id_list[i] != ifp->if_index)
				return (EINVAL);

			if (i == IPV6_ADDR_SCOPE_LINKLOCAL &&
			    idlist->s6id_list[i] >= if_indexlim) {
				/*
				 * XXX: theoretically, there should be no
				 * relationship between link IDs and interface
				 * IDs, but we check the consistency for
				 * safety in later use.
				 */
				return (EINVAL);
			}

			/*
			 * XXX: we must need lots of work in this case,
			 * but we simply set the new value in this initial
			 * implementation.
			 */
			sid->s6id_list[i] = idlist->s6id_list[i];
		}
	}

	return (error);
}

int
scope6_get(const struct ifnet *ifp, struct scope6_id *idlist)
{
	/* We only need to lock the interface's afdata for SID() to work. */
	const struct scope6_id *sid = SID(ifp);

	if (sid == NULL)	/* paranoid? */
		return EINVAL;

	*idlist = *sid;

	return 0;
}


/* note that ifp argument might be NULL */
void
scope6_setdefault(struct ifnet *ifp)
{

	/*
	 * Currently, this function just sets the default "interfaces"
	 * and "links" according to the given interface.
	 * We might eventually have to separate the notion of "link" from
	 * "interface" and provide a user interface to set the default.
	 */
	if (ifp) {
		sid_default.s6id_list[IPV6_ADDR_SCOPE_INTFACELOCAL] =
			ifp->if_index;
		sid_default.s6id_list[IPV6_ADDR_SCOPE_LINKLOCAL] =
			ifp->if_index;
	} else {
		sid_default.s6id_list[IPV6_ADDR_SCOPE_INTFACELOCAL] = 0;
		sid_default.s6id_list[IPV6_ADDR_SCOPE_LINKLOCAL] = 0;
	}
}

int
scope6_get_default(struct scope6_id *idlist)
{

	*idlist = sid_default;

	return (0);
}

uint32_t
scope6_addr2default(const struct in6_addr *addr)
{
	uint32_t id;

	/*
	 * special case: The loopback address should be considered as
	 * link-local, but there's no ambiguity in the syntax.
	 */
	if (IN6_IS_ADDR_LOOPBACK(addr))
		return (0);

	/*
	 * XXX: 32-bit read is atomic on all our platforms, is it OK
	 * not to lock here?
	 */
	id = sid_default.s6id_list[in6_addrscope(addr)];

	return (id);
}
