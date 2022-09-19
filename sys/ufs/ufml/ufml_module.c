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
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/buf.h>

#include <ufs/ufml/ufml.h>
#include <ufs/ufml/ufml_extern.h>
#include <ufs/ufml/ufml_module.h>

/* WIP */
static struct ufml_mmodlist ufml_modhead = LIST_INITIALIZER(ufml_modhead);

void
ufml_module_init()
{
	struct ufml_module 	*module;

	ufml_module_insert(module, UFML_FSMOD_NAME, UFML_FSMOD_ID);
	ufml_module_insert(module, UFML_ARCHMOD_NAME, UFML_ARCHMOD_ID);
	ufml_module_insert(module, UFML_COMPMOD_NAME, UFML_COMPMOD_ID);
	ufml_module_insert(module, UFML_ENCMOD_NAME, UFML_ENCMOD_ID);
}

void
ufml_module_insert(module, name, id)
	struct ufml_module 	*module;
	char 				*name;
	int 				id;
{
	module->ufml_modname = name;
	module->ufml_modid = id;
	LIST_INSERT_HEAD(&ufml_modhead, module, ufml_modentry);
}

struct ufml_module *
ufml_module_find(name, id)
	char 		*name;
	int 		id;
{
	struct ufml_module *module;

	LIST_FOREACH(module, &ufml_modhead, ufml_modentry) {
		if(module->ufml_modname == name && module->ufml_modid == id) {
			return (module);
		}
	}
	return (NULL);
}

void
ufml_module_remove(name, id)
	char *name;
	int id;
{
	struct ufml_module *module;

	module = ufml_module_find(name, id);
	if(module != NULL) {
		LIST_REMOVE(module, ufml_modentry);
	}
}
