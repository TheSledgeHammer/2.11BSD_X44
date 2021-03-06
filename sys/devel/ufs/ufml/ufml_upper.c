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

#include "devel/ufs/ufml/ufml.h"
#include "devel/ufs/ufml/ufml_meta.h"
#include "devel/ufs/ufml/ufml_extern.h"
#include "devel/ufs/ufml/ufml_ops.h"

/* Check filesystem types to see if the filesystem is supported */
int
ufml_check_fs(vp, type)
	struct vnode *vp;
	enum ufml_fstype type;
{
	int error;
	struct ufml_metadata *meta = VTOUFML(vp)->ufml_meta;

	switch(type) {
	case UFML_FFS:
		meta->ufml_filesystem = UFML_FFS;
		error = 0;
		break;
	case UFML_MFS:
		meta->ufml_filesystem = UFML_MFS;
		error = 0;
		break;
	case UFML_LFS:
		meta->ufml_filesystem = UFML_LFS;
		error = 0;
		break;
	default:
		meta->ufml_filesystem = UFML_UFS;
		error = 0;
		break;
	}

	if(error != 0) {
		return (1);
	}

	return (error);
}

/* Check archive types to see if the archive format is supported */
int
ufml_check_archive(vp, type)
	struct vnode *vp;
	enum ufml_archtype type;
{
	int error;
	struct ufml_metadata *meta = VTOUFML(vp)->ufml_meta;

	switch (type) {
	case UFML_AR:
		meta->ufml_archive = UFML_AR;
		error = 0;
		break;
	case UFML_CPIO:
		meta->ufml_archive = UFML_CPIO;
		error = 0;
		break;
	default:
		meta->ufml_archive = UFML_TAR;
		error = 0;
		break;
	}

	if (error != 0) {
		return (1);
	}

	return (error);
}

/* Check compression types to see if the compression format is supported */
int
ufml_check_compression(vp, type)
	struct vnode *vp;
	enum ufml_comptype type;
{
	int error;
	struct ufml_metadata *meta = VTOUFML(vp)->ufml_meta;

	switch (type) {
	case UFML_GZIP:
		meta->ufml_compress = UFML_GZIP;
		error = 0;
		break;
	case UFML_LZIP:
		meta->ufml_compress = UFML_LZIP;
		error = 0;
		break;
	case UFML_LZMA:
		meta->ufml_compress = UFML_LZMA;
		error = 0;
		break;
	case UFML_XZ:
		meta->ufml_compress = UFML_XZ;
		error = 0;
		break;
	default:
		meta->ufml_compress = UFML_BZIP2;
		error = 0;
		break;
	}

	if (error != 0) {
		return (1);
	}

	return (error);
}

/* Check encryption types to see if the encryption algorithm is supported */
int
ufml_check_encrypt(vp, type)
	struct vnode *vp;
	enum ufml_enctype type;
{
	struct ufml_metadata *meta = VTOUFML(vp)->ufml_meta;

	meta->ufml_filesystem = UFML_TWOFISH;

	if (type != UFML_TWOFISH) {
		return (1);
	}

	return (0);
}
