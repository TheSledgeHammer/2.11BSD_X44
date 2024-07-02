/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "nbpf.h"

typedef struct nbpf_propdb  nbpf_propdb_t;

struct nbpf_propdb_list;
LIST_HEAD(nbpf_propdb_list, nbpf_propdb);
struct nbpf_propdb {
	nbpf_tableset_t			*nbp_tblset;
	nbpf_table_t			*nbp_tbl;

	u_int					nbp_tid;
	int 					nbp_type;
	int						nbp_alen;
	nbpf_addr_t				nbp_addr;
	nbpf_netmask_t			nbp_mask;
	LIST_ENTRY(nbpf_propdb) nbp_next;
};

struct prop_object {
	nbpf_tableset_t			*nbp_tblset;
	nbpf_table_t			*nbp_tbl;
};

struct nbpf_propdb_list nbpf_propdb;

nbpf_propdb_t *
nbpf_propdb_create(void)
{
	nbpf_propdb_t 	*prop;

	LIST_INIT(&nbpf_propdb);

	prop = (nbpf_propdb_t *)nbpf_malloc(sizeof(nbpf_propdb_t), M_NBPF, M_WAITOK);
	if (prop == NULL) {
		return (NULL);
	}
	return (prop);
}

void
nbpf_propdb_destroy(prop)
	nbpf_propdb_t 	*prop;
{
	if (prop == NULL) {
		return;
	}
	free(prop, M_NBPF);
}

void
nbpf_propdb_insert(prop, tset, tbl, tid, type, hsize, alen, addr, mask)
	nbpf_propdb_t *prop;
	nbpf_tableset_t *tset;
	nbpf_table_t 	*tbl;
	u_int tid;
	size_t hsize;
	int type, alen;
	nbpf_addr_t addr;
	nbpf_netmask_t mask;
{
		/*
	nbpf_tableset_t *tset;
	nbpf_table_t 	*tbl;
	int error;

	error = nbpf_mktable(tset, tbl, tid, type, hsize);
	if (error != 0) {
		return;
	}
	*/
	prop->nbp_tblset = tset;
	prop->nbp_tbl = tbl;
	prop->nbp_tid = tid;
	prop->nbp_type = type;
	prop->nbp_alen = alen;
	prop->nbp_addr = addr;
	prop->nbp_mask = mask;

	LIST_INSERT_HEAD(&nbpf_propdb, prop, nbp_next);
}

nbpf_propdb_t *
nbpf_propdb_lookup(tid, type, alen)
	u_int tid;
	int type, alen;
{
	register nbpf_propdb_t *prop;

	LIST_FOREACH(prop, &nbpf_propdb, nbp_next) {
		if ((prop->nbp_tid == tid) && (prop->nbp_type == type) && (prop->nbp_alen == alen)) {
			return (prop);
		}
	}
	return (NULL);
}

void
nbpf_propdb_delete(tid, type, alen)
	u_int tid;
	int type, alen;
{
	register nbpf_propdb_t *prop;

	LIST_FOREACH(prop, &nbpf_propdb, nbp_next) {
		if ((prop->nbp_tid == tid) && (prop->nbp_type == type) && (prop->nbp_alen == alen)) {
			LIST_REMOVE(prop, nbp_next);
		}
	}
}

#define NBPFIOCTBLADD	_IOWR('N', 103, struct nbpf_table)

int
nbpf_table_ioctl(cmd, addr)
	int cmd;
	caddr_t addr;
{
	nbpf_propdb_t prop;
	int error;

	prop = (nbpf_propdb_t *)addr;

	tset = propdb_table_lookup

	switch (cmd) {
	case NBPF_CMD_TABLE_LOOKUP:
		error = nbpf_table_lookup(prop->nbp_tblset, prop->nbp_tid, prop->nbp_alen, prop->nbp_addr);
		break;
	case NBPF_CMD_TABLE_ADD:
		error = nbpf_table_insert(prop->nbp_tblset, prop->nbp_tid, prop->nbp_alen, prop->nbp_addr, prop->nbp_mask);
		break;
	case NBPF_CMD_TABLE_REMOVE:
		error = nbpf_table_remove(prop->nbp_tblset, prop->nbp_tid, prop->nbp_alen, prop->nbp_addr, prop->nbp_mask);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

