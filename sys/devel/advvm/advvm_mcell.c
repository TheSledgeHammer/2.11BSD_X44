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

#include <sys/null.h>
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
}

unsigned long
advvm_mcell_hash(adfst)
	advvm_fileset_t *adfst;
{
	Fnv32_t hash = fnv_32_buf(&adfst, sizeof(*adfst), FNV1_32_INIT) % MCELL_MASK;
	return (hash);
}

struct advcell_list  *
advvm_mcell_table(adfst)
	advvm_fileset_t 	*adfst;
{
	struct advcell_list  *cell;

	cell = &advcell_table[mcell_hash(adfst)];
	return (cell);
}

void
advvm_mcell_add(adfst)
	advvm_fileset_t 	*adfst;
{
	struct advcell_list  *cell;
	advvm_cell_t		*admc;

	KASSERT(adfst != NULL);
	cell = advvm_mcell_table(adfst);
	admc = (struct advvm_cell *)malloc(sizeof(struct advvm_cell), M_ADVVM, M_WAITOK);
	admc->ac_tag = advvm_mcell_allocate_tag_directory(adfst->fst_name, adfst->fst_id);
	admc->ac_fdir = advvm_mcell_allocate_file_directory(admc->ac_tag, admc->ac_tag->tag_name);

	adfst->fst_mcell = cell;
	adfst->fst_tag = admc->ac_tag;
	adfst->fst_fdir = admc->ac_fdir;
	LIST_INSERT_HEAD(cell, admc, ac_next);
}

advvm_cell_t *
advvm_mcell_find(adfst)
	advvm_fileset_t 	*adfst;
{
	struct advcell_list  *cell;
	advvm_cell_t		*admc;

	KASSERT(adfst != NULL);
	cell = advvm_mcell_table(adfst);
	LIST_FOREACH(admc, cell, ac_next) {
		if (admc == adfst->fst_mcell) {
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
	cell = advvm_mcell_table(adfst);
	LIST_FOREACH(admc, cell, ac_next) {
		if (admc == adfst->fst_mcell) {
			LIST_REMOVE(admc, ac_next);
		}
	}
}

advvm_tag_dir_t *
advvm_mcell_get_tag(adfst)
	advvm_fileset_t 	*adfst;
{
	advvm_cell_t *admc;
	admc = advvm_mcell_find(adfst);
	if (admc != NULL) {
		if (admc->ac_tag == adfst->fst_tag) {
			return (admc->ac_tag);
		}
	}
	return (NULL);
}

advvm_file_dir_t *
advvm_mcell_get_fdir(adfst)
	advvm_fileset_t 	*adfst;
{
	advvm_cell_t *admc;

	admc = advvm_mcell_find(adfst);
	if (admc != NULL) {
		if (admc->ac_fdir == adfst->fst_fdir) {
			return (admc->ac_fdir);
		}
	}
	return (NULL);
}

/*
 * tag directory hash: from tag name & tag id pair
 */
uint32_t
advvm_mcell_tag_hash(tag_name, tag_id)
	const char  *tag_name;
	uint32_t    tag_id;
{
	uint32_t hash1 = triple32(strlen(tag_name) + sizeof(tag_name)) % MCELL_MASK;
	uint32_t hash2 = triple32(tag_id) % MCELL_MASK;
	return (hash1 ^ hash2);
}

/*
 * file directory hash: from tag & file directory name pair
 */
uint32_t
advvm_mcell_fdir_hash(tag_dir, fdr_name)
	advvm_tag_dir_t   *tag_dir;
	const char        *fdr_name;
{
	uint32_t hash1 = advvm_mcell_tag_hash(tag_dir->tag_name, tag_dir->tag_id);
	uint32_t hash2 = triple32(strlen(fdr_name) + sizeof(fdr_name)) % MCELL_MASK;
	return (hash1 ^ hash2);
}

/*
 * set fileset's tag directory name and id.
 */
advvm_tag_dir_t *
advvm_mcell_allocate_tag_directory(name, id)
	char            *name;
	uint32_t        id;
{
	advvm_tag_dir_t *tag;

	tag = (struct advvm_tag_directory *)malloc(sizeof(struct advvm_tag_directory *), M_ADVVM, M_WAITOK);
	tag->tag_name = name;
	tag->tag_id = id;

	return (tag[advvm_mcell_tag_hash(name, id)]);
}

/*
 * set fileset's file directory: from the fileset's tag directory and file directory
 * name.
 */
advvm_file_dir_t *
advvm_mcell_allocate_file_directory(tag, name)
	advvm_tag_dir_t *tag;
	char            *name;
{
	advvm_file_dir_t *fdir;

	fdir = (struct advvm_file_dir_t *)malloc(sizeof(struct advvm_file_dir_t *), M_ADVVM, M_WAITOK);
	fdir->fdr_tag = tag;
	fdir->fdr_name = name;

	return (fdir[advvm_mcell_fdir_hash(tag, name)]);
}
