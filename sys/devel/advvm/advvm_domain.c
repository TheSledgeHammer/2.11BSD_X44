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
#include <devel/sys/malloctypes.h>

#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_domain.h>

void
advvm_volume_init(adom)
	domain_t adom;
{

	adom->dom_name = NULL;
	adom->dom_id = 0;
	TAILQ_INIT(&adom->dom_volumes);
	TAILQ_INIT(&adom->dom_filesets);
}

void
volume_alloc(adom)
	domain_t adom;
{
	adom = (struct domain *)malloc(adom, sizeof(domain_t), M_ADVVM, M_WAITOK);
}

advvm_volume_create(adom, name)
	domain_t adom;
	char *name;
{
	adom->dom_name = name;
	adom->dom_id = 0;/* generate a random uuid */

	TAILQ_INSERT_HEAD(&domains, adom, dom_entries);
}

advvm_volume_destroy(adom)
	domain_t adom;
{
	TAILQ_REMOVE(&domains, adom, dom_entries);
}

advvm_volume_add()
{

}

advvm_volume_delete()
{

}


