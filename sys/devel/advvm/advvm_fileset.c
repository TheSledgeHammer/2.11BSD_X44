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

//#include <devel/advvm/dm/dm.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_mcell.h>
#include <devel/advvm/advvm_fileset.h>

struct advdomain_list 	domain_list[MAXDOMAIN];

void
advvm_fileset_init(adfst, name, id)
	advvm_fileset_t *adfst;
	char *name;
	uint32_t id;
{
	adfst->fst_name = advvm_strcat(name, "_fileset");
	adfst->fst_id = id;
}

void
advvm_fileset_set_domain(adom, adfst)
	advvm_domain_t *adom;
	advvm_fileset_t *adfst;
{
	adfst->fst_domain = adom;
	adfst->fst_domain_name = adom->dom_name;
	adfst->fst_domain_id = adom->dom_id;
}

void
advvm_fileset_create(adom, adfst)
	advvm_domain_t 		*adom;
	advvm_fileset_t 	*adfst;
{
	advvm_fileset_set_domain(adom, adfst);
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

	advvm_fileset_create(adom, adfst);

	bucket = &domain_list[advvm_hash(adom)];

	advvm_mcell_add(adfst);

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
		advvm_extent_destroy(adfst->fst_storage);
	}
	advvm_free((advvm_fileset_t *)adfst);
}
