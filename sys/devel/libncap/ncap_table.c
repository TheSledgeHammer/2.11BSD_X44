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
 *
 *	@(#)ncap_table.c 1.0 	18/11/26
 */

#include <sys/file.h>
#include <sys/ioctl.h>

#include <net/bpf.h>

#include <net/nbpf/nbpf.h>
#include <net/nbpf/nbpfdesc.h>

#include "ncap.h"

static int
ncap_table_set(ncap_t *n, ncap_table_t *t, nbpf_ioctl_table_t *nct)
{
    int error;

    error = ioctl(n->fd, BIOCSETTBLIO, (caddr_t)nct);
    if (error < 0) {
    	(void)snprintf(n->errbuf, NCAP_ERRBUF_SIZE, "BIOCSETTBLIO: %s", ncap_strerror(errno));
        return (error);
    }
    nct->nct_tid = t->tid;
    nct->nct_type = t->type;
    nct->nct_alen = t->alen;
    nct->nct_addr = t->addr;
    nct->nct_mask = t->mask;
    return (0);
}

static int
ncap_table_get(ncap_t *n, ncap_table_t *t, nbpf_ioctl_table_t *nct)
{
    int error;

    error = ioctl(n->fd, BIOCGETTBLIO, (caddr_t)nct);
    if (error < 0) {
    	(void)snprintf(n->errbuf, NCAP_ERRBUF_SIZE, "BIOCGETTBLIO: %s", ncap_strerror(errno));
        return (error);
    }
    t->tid = nct->nct_tid;
    t->type = nct->nct_type;
    t->alen = nct->nct_alen;
    t->addr = nct->nct_addr;
    t->mask = nct->nct_mask;
    return (0);
}

static int
ncap_table_cntl(ncap_t *n, nbpf_ioctl_table_t *nct)
{
    int error;

    error = ioctl(n->fd, BIOCTBLCNTL, (caddr_t)nct);
    if (error < 0) {
    	(void)snprintf(n->errbuf, NCAP_ERRBUF_SIZE, "BIOCTBLCNTL: %s", ncap_strerror(errno));
        return (error);
    }
    return (0);
}

static int
ncap_table_cmds(ncap_t *n, ncap_table_t *t, nbpf_ioctl_table_t *nct, int cmd)
{
	int error;

	t->cmd = cmd;
    error = ncap_table_get(n, t, nct);
    if (error != 0) {
    	if (t->cmd == NBPF_CMD_TABLE_LOOKUP) {
    		return (error);
    	}
    	memset(nct, 0, sizeof(nbpf_ioctl_table_t));
    }

	switch (t->cmd) {
	case NBPF_CMD_TABLE_LOOKUP:
		nct->nct_cmd = t->cmd;
		if ((nct->nct_tid != t->tid) || (nct->nct_type != t->type)
				|| (nct->nct_alen != t->alen) /*|| (nct->nct_mask != mask) */
				|| (nct->nct_addr != t->addr)) {
			error = -1;
			break;
		}
		break;
	case NBPF_CMD_TABLE_ADD:
		/* FALLTHROUGH */
	case NBPF_CMD_TABLE_REMOVE:
		nct->nct_cmd = t->cmd;
		error = ncap_table_set(n, t, nct);
		break;
	default:
		error = -1;
		break;
	}
	return (error);
}

int
ncap_table_lookup(ncap_t *n, ncap_table_t *t)
{
    nbpf_ioctl_table_t nct;
    int error;

    error = ncap_table_cmds(n, t, &nct, NBPF_CMD_TABLE_LOOKUP);
    if (error != 0) {
        return (error);
    }
    return (ncap_table_cntl(n, &nct));
}

int
ncap_table_add(ncap_t *n, ncap_table_t *t)
{
    nbpf_ioctl_table_t nct;
    int error;

    error = ncap_table_cmds(n, t, &nct, NBPF_CMD_TABLE_ADD);
    if (error != 0) {
        return (error);
    }
    return (ncap_table_cntl(n, &nct));
}

int
ncap_table_remove(ncap_t *n, ncap_table_t *t)
{
    nbpf_ioctl_table_t nct;
    int error;

    error = ncap_table_cmds(n, t, &nct, NBPF_CMD_TABLE_REMOVE);
    if (error != 0) {
        return (error);
    }
    return (ncap_table_cntl(n, &nct));
}
