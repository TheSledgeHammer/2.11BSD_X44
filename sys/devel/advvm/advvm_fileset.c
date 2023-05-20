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
 *	@(#)advvm_fileset.c 1.0 	24/02/21
 */

#include <sys/extent.h>
#include <sys/tree.h>
#include <sys/fnv_hash.h>
#include <lib/libkern/libkern.h>

#include <devel/advvm/dm/dm.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_fileset.h>

#define HASH_MASK 		32							/* hash mask */
struct advdomain_list 	domain_list[MAXDOMAIN];

void	advvm_mcell_init(advvm_fileset_t *);

void
advvm_fileset_init(adfst, name, id)
	advvm_fileset_t *adfst;
	char *name;
	uint32_t id;
{
	adfst->fst_name = name;
	adfst->fst_id = id;
	adfst->fst_tags = NULL;
}

void
advvm_fileset_set_domain(adfst, adom)
	advvm_fileset_t *adfst;
	advvm_domain_t *adom;
{
	adfst->fst_domain = adom;
	adfst->fst_domain_name = adom->dom_name;
	adfst->fst_domain_id = adom->dom_id;
}

/*
 * tag directory hash: from tag name & tag id pair
 */
uint32_t
advvm_tag_hash(tag_name, tag_id)
	const char  *tag_name;
	uint32_t    tag_id;
{
	uint32_t hash1 = triple32(strlen(tag_name) + sizeof(tag_name)) % HASH_MASK;
	uint32_t hash2 = triple32(tag_id) % HASH_MASK;
	return (hash1 ^ hash2);
}

/*
 * file directory hash: from tag & file directory name pair
 */
uint32_t
advvm_fdir_hash(tag_dir, fdr_name)
	advvm_tag_dir_t   *tag_dir;
	const char        *fdr_name;
{
	uint32_t hash1 = advvm_tag_hash(tag_dir->tag_name, tag_dir->tag_id);
	uint32_t hash2 = triple32(strlen(fdr_name) + sizeof(fdr_name)) % HASH_MASK;
	return (hash1 ^ hash2);
}

/*
 * set fileset's tag directory name and id.
 */
advvm_tag_dir_t *
advvm_filset_allocate_tag_directory(name, id)
	char            *name;
	uint32_t        id;
{
	advvm_tag_dir_t *tag;

	tag = malloc(sizeof(struct advvm_tag_directory *), M_ADVVM, M_WAITOK);
	tag->tag_name = name;
	tag->tag_id = id;

	return (tag[advvm_tag_hash(name, id)]);
}

/*
 * set fileset's file directory: from the fileset's tag directory and file directory
 * name.
 */
advvm_file_dir_t *
advvm_filset_allocate_file_directory(tag, name)
	advvm_tag_dir_t *tag;
	char            *name;
{
	advvm_file_dir_t *fdir;

	fdir = malloc(sizeof(struct advvm_file_dir_t *), M_ADVVM, M_WAITOK);
	fdir->fdr_tag = tag;
	fdir->fdr_name = name;

	return (fdir[advvm_fdir_hash(tag, name)]);
}

void
advvm_fileset_create(adom, adfst)
	advvm_domain_t 		*adom;
	advvm_fileset_t 	*adfst;
{
	adfst->fst_domain = adom;

	advvm_mcell_init(adfst);
	advvm_storage_create(adfst->fst_storage, adfst->fst_name, adom->dom_start, adom->dom_end, NULL, NULL, adom->dom_flags); /* XXX to complete */
}

advvm_fileset_t *
advvm_filset_find(adom, name, id)
	advvm_domain_t 	*adom;
	char 		    *name;
	uint32_t 	    id;
{
	struct advdomain_list 	*bucket;
	advvm_fileset_t         *adfst;
	
	bucket = &domain_list[advvm_hash(adom)];
	TAILQ_FOREACH(adfst, bucket, fst_entries) {
		if (adfst->fst_domain == adom) {
			if (adfst->fst_name == name && adfst->fst_id == id) {
				return (adfst);
			}
		}
	}
	return (NULL);
}

void
advvm_filset_insert(adom, adfst)
	advvm_domain_t    	*adom;
	advvm_fileset_t 	*adfst;
{
	struct advdomain_list 		*bucket;
	
	if(adom == NULL || adfst == NULL) {
		return;
	}

	advvm_fileset_set_domain(adfst, adom);
	advvm_fileset_create(adom, adfst);

	bucket = &domain_list[advvm_hash(adom)];

	TAILQ_INSERT_HEAD(bucket, adfst, fst_entries);
}

void
advvm_filset_remove(adom, name, id)
	advvm_domain_t *adom;
	char 		*name;
	uint32_t 	id;
{
	struct advdomain_list 	*bucket;
	advvm_fileset_t 		*adfst;
	
	bucket = &domain_list[advvm_hash(adom)];
	TAILQ_FOREACH(adfst, bucket, fst_entries) {
		if (adfst->fst_domain == adom) {
			if (adfst->fst_name == name && adfst->fst_id == id) {
				TAILQ_REMOVE(bucket, adfst, fst_entries);
			}
		}
	}
}

/* destroy and free an AdvVM fileset */
void
advvm_fileset_destroy(adfst)
	advvm_fileset_t *adfst;
{
	if(adfst->fst_storage) {
		advvm_storage_delete(adfst->fst_storage);
	}
	advvm_free((advvm_fileset_t *)adfst);
}

/*
 * mcell functions:
 * used to location advvm file directory from fileset id and tag
 */
struct advcell_list;
LIST_HEAD(advcell_list, advvm_cell);
struct advvm_cell {
	advvm_tag_dir_t 		*ac_tag;
	advvm_file_dir_t		*ac_fdir;
	LIST_ENTRY(advvm_cell)	ac_next;
};
typedef struct advvm_cell advvm_cell_t;

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
	Fnv32_t hash = fnv_32_buf(&adfst, sizeof(*adfst), FNV1_32_INIT) % HASH_MASK;
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
