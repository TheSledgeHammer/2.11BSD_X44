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
 *	@(#)advvm_bitfile.c 1.0 	7/01/21
 */

#define TAG_HASH 32	/* tag directory hash */
#define FDR_HASH 64	/* file directory hash */

/* tag directory hash: from tag name & tag id pair */
uint32_t
advvm_tag_hash(tag_name, tag_id)
	const char  *tag_name;
	uint32_t    tag_id;
{
	uint32_t hash1 = triple32(sizeof(tag_name)) % TAG_HASH;
	uint32_t hash2 = triple32(tag_id) % TAG_HASH;
	return (hash1 ^ hash2);
}

/* file directory hash: from tag & file directory name pair */
uint32_t
advvm_fdr_hash(tag_dir, fdr_name)
	advvm_tag_dir_t   *tag_dir;
	const char        *fdr_name;
{
	uint32_t hash1 = advvm_tag_hash(tag_dir->tag_name, tag_dir->tag_id);
	uint32_t hash2 = triple32(fdr_name) % TAG_HASH;
	return (hash1 ^ hash2);
}
