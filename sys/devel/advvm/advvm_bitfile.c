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

#include <sys/extent.h>
#include <sys/tree.h>

#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_fileset.h>
#include <devel/advvm/advvm_bitfile.h>


#define HASH_MASK 	31	/* hash mask XXX: Better Hash Mask random number?? */

struct dkdevice 	disktable[HASH_MASK]; /* disk lookup hash table */

/* tag directory hash: from tag name & tag id pair */
uint32_t
advvm_tag_hash(tag_name, tag_id)
	const char  *tag_name;
	uint32_t    tag_id;
{
	uint32_t hash1 = triple32(sizeof(tag_name)) % HASH_MASK;
	uint32_t hash2 = triple32(tag_id) % HASH_MASK;
	return (hash1 ^ hash2);
}

/* file directory hash: from tag & file directory name pair */
uint32_t
advvm_fdr_hash(tag_dir, fdr_name)
	advvm_tag_dir_t   *tag_dir;
	const char        *fdr_name;
{
	uint32_t hash1 = advvm_tag_hash(tag_dir->tag_name, tag_dir->tag_id);
	uint32_t hash2 = triple32(sizeof(fdr_name)) % HASH_MASK;
	return (hash1 ^ hash2);
}

uint32_t
advvm_disk_hash(fdr, disk)
	advvm_file_dir_t 	 *fdr;
	struct dkdevice  	 *disk;
{
	uint32_t hash1 = advvm_fdr_hash(fdr->fdr_tag, fdr->fdr_name);
	uint32_t hash2 = triple32(sizeof(disk)) % HASH_MASK;
	return (hash1 ^ hash2);
}

void
advvm_filset_set_tag_directory(tag, name, id)
  advvm_tag_dir_t *tag;
  char            *name;
  uint32_t        id;
{
	if (tag == NULL) {
		advvm_malloc((struct advvm_tag_directory*) tag, sizeof(struct advvm_tag_directory*));
	}
	tag->tag_name = name;
	tag->tag_id = id;
}

void
advvm_filset_set_file_directory(fdir, tag, name)
  advvm_file_dir_t  *fdir;
  advvm_tag_dir_t   *tag;
  char              *name;
{
  if (fdir == NULL) {
	  advvm_malloc((struct advvm_file_directory*) fdir, sizeof(struct advvm_file_directory*));
  }
  register struct dkdevice *disk;

  disk = &disktable[advvm_fdr_hash(tag, name)];

  fdir->fdr_tag = tag;
  fdir->fdr_name = name;
  fdir->fdr_disk = disk;
}

void
advvm_bitfile_setup(adfst, tag_name, tag_id, fdr_name)
	advvm_fileset_t  *adfst;
	char 			*tag_name, *fdr_name;
	uint32_t  		tag_id;
{
	register advvm_tag_dir_t   	*tag;
	register advvm_file_dir_t  	*fdir;

	tag = adfst->fst_tags;
	fdir = adfst->fst_file_directory;

	if(tag) {
		advvm_filset_set_tag_directory(tag, tag_name, tag_id);
		if(fdir) {
			advvm_filset_set_file_directory(fdir, tag, fdr_name);
		} else {
			panic("advvm_bitfile_setup: empty file directory");
		}
	} else {
		panic("advvm_bitfile_setup: empty tag directory");
	}
}
