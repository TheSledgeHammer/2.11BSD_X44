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

/* Transport Foreign Interfaces */

#include <sys/cdefs.h>

#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/null.h>

#include <tpi_pcb.h>

/* Foreign Hash Table */
struct tpipcbhead *
tpi_foreign_hash(struct tpipcbtable *table, void *faddr, uint16_t fport)
{
	struct tpipcbhead *tpfhash;
	uint32_t nethash;

	nethash = tpi_pcbnethash(faddr, fport);
	tpfhash = &table->tppt_fhashtbl[nethash & table->tppt_fhash];
	return (tpfhash);
}

void
tpi_foreign_insert(struct tpipcbtable *table, void *faddr, uint16_t fport)
{
	struct tpi_foreign *tpf;

    MALLOC(tpf, struct tpi_foreign *, sizeof(*tpf), M_PCB, M_WAITOK);
    if (tpf == NULL) {
        return;
    }
    tpf->tpf_faddr = faddr;
    tpf->tpf_fport = fport;
    LIST_INSERT_HEAD(tpi_foreign_hash(table, tpf->tpf_faddr, tpf->tpf_fport), &tpf->tpf_head, tpph_fhash);
}

struct tpi_foreign *
tpi_foreign_lookup(struct tpipcbtable *table, void *faddr, uint16_t fport)
{
	struct tpipcbhead *head;
	struct tpipcb_hdr *tpph;
	struct tpi_foreign *tpf;

	head = tpi_foreign_hash(table, faddr, fport);
	LIST_FOREACH(tpph, head, tpph_fhash) {
		tpf = (struct tpi_foreign *)tpph;
		if ((tpf->tpf_faddr == faddr) && (tpf->tpf_fport == fport)) {
			return (tpf);
		}
	}
	return (NULL);
}

void
tpi_foreign_remove(struct tpipcbtable *table, void *faddr, uint16_t fport)
{
	struct tpi_foreign  *tpf;

	tpf = tpi_foreign_lookup(table, faddr, fport);
    if (tpf != NULL) {
        LIST_REMOVE(&tpf->tpf_head, tpph_fhash);
    }
}

int
tpi_foreign_compare(struct tpi_foreign *tpf_a, struct tpi_foreign *tpf_b)
{
	if (tpf_a != tpf_b) {
		return (1);
	}
	return (bcmp(tpf_a, tpf_b, sizeof(tpf_a)));
}

void
tpi_foreign_set_fsockaddr(struct tpi_foreign *tpf, void *fsockaddr, void *faddr, uint16_t fport)
{
	tpf->tpf_fsockaddr = fsockaddr;
	tpf->tpf_fport = fport;
	tpf->tpf_faddr = faddr;
}

void *
tpi_foreign_get_fsockaddr(struct tpi_foreign *tpf)
{
	void *fsockaddr = tpf->tpf_fsockaddr;
	if (fsockaddr != NULL) {
		return (fsockaddr);
	}
	return (NULL);
}
