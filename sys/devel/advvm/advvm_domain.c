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
 *	@(#)advvm_domain.c 1.0 	20/02/21
 */

#include <sys/extent.h>
#include <sys/tree.h>
#include <sys/null.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_domain.h>

struct advdomain_list 	domain_list[MAXDOMAIN];

void
advvm_domain_init(adom)
	advvm_domain_t *adom;
{
	adom->dom_name = NULL;
	adom->dom_id = 0;
	adom->dom_refcnt = 0;
	TAILQ_INIT(&adom->dom_volumes);
	TAILQ_INIT(&adom->dom_filesets);

	simple_lock_init(adom->dom_lock, "advvm_domain_lock");
}

void
advvm_domain_create(adom, name, id, start, end, flags)
	advvm_domain_t *adom;
	char *name;
	uint32_t id;
	u_long start, end;
	int flags;
{
	adom->dom_name = name;
	adom->dom_id = id;		/* generate a random uuid */
	adom->dom_start = start;
	adom->dom_end = end;
	adom->dom_flags = flags;
	advvm_storage_create(adom->dom_storage, adom->dom_name, adom->dom_start, adom->dom_end, NULL, NULL, adom->dom_flags);
}

uint32_t
advvm_hash(adom)
	advvm_domain_t *adom;
{
	uint32_t hash1 = triple32(sizeof(adom)) % MAXDOMAIN;
	uint32_t hash2 = triple32(sizeof(adom->dom_storage)) % MAXDOMAIN;
	return (hash1 ^ hash2);
}

advvm_domain_t *
advvm_domain_find(name, id)
	char *name;
	uint32_t id;
{
	struct advdomain_list	*bucket;
	register advvm_domain_t *adom;

	bucket = &domain_list[advvm_hash(adom)];
	simple_lock(adom->dom_lock);
	TAILQ_FOREACH(adom, bucket, dom_entries) {
		if(adom->dom_name == name && adom->dom_id == id) {
			simple_unlock(adom->dom_lock);
			return (adom);
		}
	}
	simple_unlock(adom->dom_lock);
	return (NULL);
}

void
advvm_domain_insert(adom)
	advvm_domain_t *adom;
{
	struct advdomain_list	*bucket;

	if(adom == NULL) {
		return;
	}

	bucket = &domain_list[advvm_hash(adom)];
	simple_lock(adom->dom_lock);
	TAILQ_INSERT_HEAD(bucket, adom, dom_entries);
	simple_unlock(adom->dom_lock);
	adom->dom_refcnt++;
}

void
advvm_domain_remove(name, id)
	char *name;
	uint32_t id;
{
	struct advdomain_list *bucket;
	advvm_domain_t *adom;

	bucket = &domain_list[advvm_hash(adom)];
	simple_lock(adom->dom_lock);
	TAILQ_FOREACH(adom, bucket, dom_entries) {
		if(adom->dom_name == name && adom->dom_id == id) {
			TAILQ_REMOVE(bucket, adom, dom_entries);
			advvm_domain_destroy(adom);
			simple_unlock(adom->dom_lock);
			adom->dom_refcnt--;
		}
	}
	simple_unlock(adom->dom_lock);
}

/* destroy and free an AdvVM domain */
void
advvm_domain_destroy(adom)
	advvm_domain_t *adom;
{
	if(adom->dom_storage) {
		advvm_storage_delete(adom->dom_storage);
	}
	advvm_free((advvm_domain_t *)adom);
}
