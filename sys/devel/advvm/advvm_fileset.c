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
#include <lib/libkern/libkern.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_fileset.h>

#define HASH_MASK 		32							/* hash mask */
struct advdomain_list 	domain_list[MAXDOMAIN];
struct dkdevice 		disktable[HASH_MASK]; 		/* fileset's disk hash lookup */

void
advvm_fileset_init(adfst)
	advvm_fileset_t *adfst;
{
	adfst->fst_name = NULL;
	adfst->fst_id = 0;
	adfst->fst_tags = NULL;
	adfst->fst_file_directory = NULL;
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
	uint32_t hash1 = triple32(sizeof(tag_name)) % HASH_MASK;
	uint32_t hash2 = triple32(tag_id) % HASH_MASK;
	return (hash1 ^ hash2);
}

/*
 * file directory hash: from tag & file directory name pair
 */
uint32_t
advvm_fdr_hash(tag_dir, fdr_name)
	advvm_tag_dir_t   *tag_dir;
	const char        *fdr_name;
{
	uint32_t hash1 = advvm_tag_hash(tag_dir->tag_name, tag_dir->tag_id);
	uint32_t hash2 = triple32(sizeof(fdr_name)) % HASH_MASK;
	return (hash1 ^ hash2);
}

/*
 * set fileset's tag directory name and id.
 */
void
advvm_filset_set_tag_directory(tag, name, id)
  advvm_tag_dir_t *tag;
  char            *name;
  uint32_t        id;
{
	if (tag == NULL) {
		advvm_malloc((struct advvm_tag_directory*) tag,	sizeof(struct advvm_tag_directory*));
	}
	tag->tag_name = name;
	tag->tag_id = id;
}

/*
 * set fileset's file directory: from the fileset's tag directory and file directory
 * name.
 */
void
advvm_filset_set_file_directory(fdir, tag, name)
	advvm_file_dir_t  *fdir;
	advvm_tag_dir_t   *tag;
	char              *name;
{
	if (fdir == NULL) {
		advvm_malloc((struct advvm_file_directory*) fdir,
				sizeof(struct advvm_file_directory*));
	}
	register struct dkdevice *disk;

	disk = &disktable[advvm_fdr_hash(tag, name)];

	fdir->fdr_tag = tag;
	fdir->fdr_name = name;
	fdir->fdr_disk = disk;
}

void
advvm_fileset_create(adom, adfst, fst_name, fst_id, tag, tag_name, tag_id, fdir, fdr_name)
	advvm_domain_t 		*adom;
	advvm_fileset_t 	*adfst;
	advvm_tag_dir_t 	*tag;
	advvm_file_dir_t 	*fdir;
	char 				*fst_name, *tag_name, *fdr_name;
	uint32_t 			fst_id, tag_id;
{
	//advvm_malloc((advvm_fileset_t*) adfst, sizeof(advvm_fileset_t*)); /* setup in advvm_attach */
	advvm_filset_set_tag_directory(tag, tag_name, tag_id);
	advvm_filset_set_file_directory(fdir, tag, fdr_name);

	if(tag == NULL) {
		panic("advvm_fileset_create: no tag directory set");
		return;
	}
	if(fdir == NULL) {
		panic("advvm_fileset_create: no file directory set");
		return;
	}

	adfst->fst_domain = adom;
	adfst->fst_name = name;
	adfst->fst_id = fst_id;
	adfst->fst_tags = tag;
	adfst->fst_file_directory = fdir;
	advvm_storage_create(adfst->fst_storage, adfst->fst_name, adom->dom_start, adom->dom_end, NULL, NULL, adom->dom_flags); /* XXX to complete */
}

advvm_volume_t *
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
advvm_filset_insert(adom, adfst, name, id)
	advvm_domain_t    	*adom;
	advvm_fileset_t 	*adfst;
	char 		        *name;
	uint32_t 	        id;
{
	struct advdomain_list 		*bucket;
	register advvm_tag_dir_t 	*tags;
	register advvm_file_dir_t 	*fdir;
	
	if(adom == NULL || adfst == NULL) {
		return;
	}

	tags = adfst->fst_tags;
	fdir = adfst->fst_file_directory;

	advvm_fileset_set_domain(adfst, adom);
	advvm_fileset_create(adom, adfst, name, id, tags, tags->tag_name, tags->tag_id, fdir, fdir->fdr_tag, fdir->fdr_name);

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
