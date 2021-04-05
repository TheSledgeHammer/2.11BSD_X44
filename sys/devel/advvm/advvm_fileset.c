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

struct advdomain_list 	domain_list[MAXDOMAIN];

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
void
advvm_filset_set_tag_directory(tag, name, id)
  advvm_tag_dir_t *tag;
  char            *name;
  uint32_t        id;
{
	if (tag == NULL) {
		advvm_malloc((struct advvm_tag_directory*) tag,
				sizeof(struct advvm_tag_directory*));
	}
	tag->tag_name = name;
	tag->tag_id = id;
}

void
advvm_filset_set_file_directory(fdir, tag, name, disk)
  advvm_file_dir_t  *fdir;
  advvm_tag_dir_t   *tag;
  char              *name;
  struct dkdevice   *disk;
{
  if (fdir == NULL) {
	  advvm_malloc((struct advvm_file_directory*) fdir, sizeof(struct advvm_file_directory*));
  }

  fdir->fdr_tag = tag;
  fdir->fdr_name = name;
  fdir->fdr_disk = disk;
}
*/

void
advvm_fileset_create(adfst, tag, fdir, name, id)
  advvm_fileset_t 	*adfst;
  advvm_tag_dir_t 	*tag;
  advvm_file_dir_t 	*fdir;
  char 				*name;
  uint32_t 			id;
{
	register advvm_domain_t *adom;

	advvm_malloc((advvm_fileset_t*) adfst, sizeof(advvm_fileset_t*));
	adfst->fst_domain;
	adfst->fst_name = name;
	adfst->fst_id = id;
	adfst->fst_tags = tag;
	adfst->fst_file_directory = fdir;
	advvm_storage_create(adfst->fst_storage, adom->dom_start, adom->dom_end, NULL, NULL, adom->dom_flags); /* XXX to complete */
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
	struct advdomain_list 	*bucket;
	register advvm_tag_dir_t *tags;
	
	if(adom == NULL) {
		return;
	}
	if(adfst == NULL) {
		return;
	}
/*
	if(adfst->fst_tags == NULL) {
		panic("advvm_fileset_insert: no tag directory set");
		return;
	} else if(adfst->fst_file_directory == NULL) {
		panic("advvm_fileset_insert: no file directory set");
		return;
	} else {
		advvm_fileset_create(adfst, adfst->fst_tags, adfst->fst_file_directory, name, id);
	}
	*/
	advvm_fileset_create(adfst, adfst->fst_tags, adfst->fst_file_directory, name, id);
	advvm_fileset_set_domain(adfst, adom);
	
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
