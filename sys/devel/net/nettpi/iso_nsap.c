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

/*
 * Future Implementations:
 * - May benefit from using Radix Tree for lookups combined with a list.
 * 	- Use One node per address family.
 * 	e.g. radix_node_head root[8].
 */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "if_sap.h"
#include "iso_nsap.h"

static struct nsap_iso *
nsap_create(void)
{
	struct nsap_iso *nsap;

	MALLOC(nsap, struct nsap_iso *, sizeof(*nsap), M_IFSAP, M_WAITOK);
	if (nsap == NULL) {
		return (NULL);
	}
	bzero((caddr_t)nsap, sizeof(*nsap));
	sap_init(nsap->nsi_tree);
	return (nsap);
}

static void
nsap_destroy(struct nsap_iso *nsap)
{
	if (nsap != NULL) {
		FREE(nsap, M_IFSAP);
	}
}

void
nsap_attach(struct nsap_iso *nsap, int sid, int af)
{
	struct nsap_iso *nsiiso;
	struct nsapisohead *head;
	struct sap_tree *tree;

	nsiiso = nsap_create();
	if (nsiiso != NULL) {
		tree = nsiiso->nsi_tree;
		sap_insert_af(tree, nsiiso, sid, af);
		nsap = nsiiso;
	} else {
		nsap = NULL;
	}
}

void
nsap_detach(struct nsap_iso *nsap, int sid, int af)
{
	struct sap_tree *tree;

	if (nsap != NULL) {
		tree = nsap->nsi_tree;
		if (!LIST_EMPTY(tree->st_hashtbl)) {
			sap_remove_af(tree, nsap, sid, af);
		} else {
			nsap_destroy(nsap);
		}
	}
}

void
nsap_iso_attach(struct nsap_iso *nsap)
{
    nsap_attach(nsap, SAP_SID_ISO, AF_ISO);
    nsap_attach(nsap, SAP_SID_INET4, AF_INET);
    nsap_attach(nsap, SAP_SID_INET6, AF_INET6);
    nsap_attach(nsap, SAP_SID_NS, AF_NS);
}

void
nsap_iso_detach(struct nsap_iso *nsap)
{
	nsap_detach(nsap, SAP_SID_ISO, AF_ISO);
	nsap_detach(nsap, SAP_SID_INET4, AF_INET);
	nsap_detach(nsap, SAP_SID_INET6, AF_INET6);
	nsap_detach(nsap, SAP_SID_NS, AF_NS);
}

/*
 * nsap_iso_compare:
 * - compares sockaddr_nsap, nsap_addr, type_id and subnet_id
 * returns -1 if a, 1 if b and 0 if equal
 */
int
nsap_iso_compare(struct nsap_iso *a, struct nsap_iso *b)
{
	int error;

	if (a != b) {
		error = sockaddr_sap_compare(a->nsi_snsap, b->nsi_snsap);
		if (error != 0) {
			return (error);
		}
		error = sap_addr_compare(a->nsi_nsapa, b->nsi_nsapa);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}
