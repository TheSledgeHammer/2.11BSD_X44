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
 *
 *	@(#)advvm_mcell.c 1.0 	22/05/23
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/extent.h>
#include <sys/fnv_hash.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_mcell.h>

#define MCELL_MASK 		32							/* hash mask */

struct advcell_list advcell_table;

void
advvm_mcell_init(adfst)
	advvm_fileset_t 	*adfst;
{
	LIST_INIT(&advcell_table);

	advvm_mcell_add(adfst);
}

unsigned long
advvm_mcell_hash(adfst)
	advvm_fileset_t *adfst;
{
	Fnv32_t hash = fnv_32_buf(&adfst, sizeof(*adfst), FNV1_32_INIT) % MCELL_MASK;
	return (hash);
}

void
advvm_mcell_add(adfst)
	advvm_fileset_t 	*adfst;
{
	struct advcell_list  *cell;
	advvm_cell_t		*admc;

	KASSERT(adfst != NULL);
	cell = &advcell_table[mcell_hash(adfst)];
	admc = (struct advvm_cell *)malloc(sizeof(struct advvm_cell), M_ADVVM, M_WAITOK);
	admc->ac_tag = advvm_filset_allocate_tag_directory(adfst->fst_name, adfst->fst_id);
	admc->ac_fdir = advvm_filset_allocate_file_directory(admc->ac_tag, admc->ac_tag->tag_name);

	LIST_INSERT_HEAD(cell, admc, ac_next);
}

advvm_cell_t *
advvm_mcell_find(adfst)
	advvm_fileset_t 	*adfst;
{
	struct advcell_list  *cell;
	advvm_cell_t		*admc;

	KASSERT(adfst != NULL);
	cell = &advcell_table[mcell_hash(adfst)];
	LIST_FOREACH(admc, cell, ac_next) {
		if (admc->ac_tag == adfst->fst_tags) {
			return (admc);
		}
	}
	return (NULL);
}

void
advvm_mcell_remove(adfst)
	advvm_fileset_t 	*adfst;
{
	struct advcell_list  *cell;
	advvm_cell_t		*admc;

	KASSERT(adfst != NULL);
	cell = &advcell_table[mcell_hash(adfst)];
	LIST_FOREACH(admc, cell, ac_next) {
		if (admc->ac_tag == adfst->fst_tags) {
			LIST_REMOVE(admc, ac_next);
		}
	}
}

advvm_file_dir_t *
advvm_mcell_get_fdir(adfst)
	advvm_fileset_t 	*adfst;
{
	advvm_cell_t *admc;

	admc = advvm_mcell_find(adfst);
	if (admc != NULL) {
		return (admc->ac_fdir);
	}
	return (NULL);
}
