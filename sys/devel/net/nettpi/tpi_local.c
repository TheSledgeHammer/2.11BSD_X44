/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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

/* Transport Local Interfaces */

#include <sys/cdefs.h>

#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/null.h>

#include <sys/socket.h>

#include <tpi_pcb.h>

/* Local Hash Table */
struct tpipcbhead *
tpi_local_hash(struct tpipcbtable *table, void *laddr, uint16_t lport)
{
	struct tpipcbhead *tplhash;
	uint32_t nethash;

	nethash = tpi_pcbnethash(laddr, lport);
	tplhash = &table->tppt_lhashtbl[nethash & table->tppt_lhash];
	return (tplhash);
}

void
tpi_local_insert(struct tpipcbtable *table, void *laddr, uint16_t lport)
{
	struct tpi_local *tpl;

    MALLOC(tpl, struct tpi_local *, sizeof(*tpl), M_PCB, M_WAITOK);
    if (tpl == NULL) {
        return;
    }
    tpl->tpl_laddr = laddr;
    tpl->tpl_lport = lport;
    LIST_INSERT_HEAD(tpi_local_hash(table, tpl->tpl_laddr, tpl->tpl_lport), &tpl->tpl_head, tpph_lhash);
}

struct tpi_local *
tpi_local_lookup(struct tpipcbtable *table, void *laddr, uint16_t lport)
{
	struct tpipcbhead *head;
	struct tpipcb_hdr *tpph;
	struct tpi_local  *tpl;

	head = tpi_local_hash(table, laddr, lport);
	LIST_FOREACH(tpph, head, tpph_lhash) {
		tpl = (struct tpi_local *)tpph;
		if ((tpl->tpl_laddr == laddr) && (tpl->tpl_lport == lport)) {
			return (tpl);
		}
	}
	return (NULL);
}

void
tpi_local_remove(struct tpipcbtable *table, void *laddr, uint16_t lport)
{
	struct tpi_local  *tpl;

	tpl = tpi_local_lookup(table, laddr, lport);
	if (tpl != NULL) {
		LIST_REMOVE(&tpl->tpl_head, tpph_lhash);
	}
}

int
tpi_local_compare(struct tpi_local *tpl_a, struct tpi_local *tpl_b)
{
	if (tpl_a != tpl_b) {
		return (1);
	}
	return (bcmp(tpl_a, tpl_b, sizeof(tpl_a)));
}

void
tpi_local_set_lsockaddr(struct tpi_local *tpl, void *lsockaddr, void *laddr, uint16_t lport)
{
	tpl->tpl_lsockaddr = lsockaddr;
	tpl->tpl_lport = lport;
	tpl->tpl_laddr = laddr;
}

void *
tpi_local_get_lsockaddr(struct tpi_local *tpl)
{
	void *lsockaddr = tpl->tpl_lsockaddr;
	if (lsockaddr != NULL) {
		return (lsockaddr);
	}
	return (NULL);
}
