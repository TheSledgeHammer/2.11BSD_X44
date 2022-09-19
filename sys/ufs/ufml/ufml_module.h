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

#ifndef _UFS_UFML_MODULE_H_
#define _UFS_UFML_MODULE_H_

#include <sys/queue.h>

/* ufml modules (WIP) */
/*
 * Modules are the framework for adding, removing and modifying support
 * of the various types (i.e. filesystem, archive, compression & encryption)
 * or adding new types.
 * Each module has a name, id and a sublist
 * of what that module supports.
 */
struct ufml_mmodlist;
LIST_HEAD(ufml_mmodlist, ufml_module);
struct ufml_module {
	LIST_ENTRY(ufml_module) ufml_modentry;	/* module list entry */
	char					*ufml_modname;	/* module name */
	int						ufml_modid;		/* module id */
};

/* module names */
#define UFML_FSMOD_NAME		"ufml_filesystem_module"
#define UFML_ARCHMOD_NAME	"ufml_archive_module"
#define UFML_COMPMOD_NAME	"ufml_compression_module"
#define UFML_ENCMOD_NAME	"ufml_encryption_module"

/* module identifier */
#define UFML_FSMOD_ID 		0x01
#define UFML_ARCHMOD_ID 	0x02
#define UFML_COMPMOD_ID 	0x03
#define UFML_ENCMOD_ID 		0x04

#endif /* SYS_UFS_UFML_UFML_MODULE_H_ */
