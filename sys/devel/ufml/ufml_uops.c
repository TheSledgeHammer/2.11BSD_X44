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

#include <sys/user.h>
#include <sys/malloc.h>

#include "devel/ufml/ufml.h"
#include "devel/ufml/ufml_meta.h"
#include "devel/ufml/ufml_extern.h"
#include "devel/ufml/ufml_ops.h"

struct ufmlops uops;

void
uops_init()
{
	uops_alloc(&uops);
}

void
uops_alloc(uops)
	struct ufmlops *uops;
{
	MALLOC(uops, struct ufmlops *, sizeof(struct ufmlops *), M_UFMLOPS, M_WAITOK);
}

int
uop_archive(up, vp, mp, fstype, archive)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype, archive;
{
	struct uop_archive_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = archive;

	if(uops.uop_archive == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_archive(up, vp, mp, fstype, archive);

	return (error);
}

int
uop_extract(up, vp, mp, fstype, archive)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype, archive;
{
	struct uop_extract_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = archive;

	if(uops.uop_extract == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_extract(up, vp, mp, fstype, archive);

	return (error);
}

int
uop_compress(up, vp, mp, fstype, compress)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype, compress;
{
	struct uop_compress_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = compress;

	if(uops.uop_compress == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_compress(up, vp, mp, fstype, compress);

	return (error);
}

int
uop_decompress(up, vp, mp, fstype, compress)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype, compress;
{
	struct uop_decompress_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = compress;

	if(uops.uop_decompress == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_decompress(up, vp, mp, fstype, compress);

	return (error);
}

int
uop_encrypt(up, vp, mp, fstype, encrypt)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype, encrypt;
{
	struct uop_encrypt_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = encrypt;
	if(uops.uop_encrypt == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_encrypt(up, vp, mp, fstype, encrypt);

	return (error);
}

int
uop_decrypt(up, vp, mp, fstype, encrypt)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype, encrypt;
{
	struct uop_decrypt_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = encrypt;

	if(uops.uop_decrypt == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_decrypt(up, vp, mp, fstype, encrypt);

	return (error);
}

int
uop_snapshot_write(up, vp, mp, fstype)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype;
{
	struct uop_snapshot_write_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(uops.uop_snapshot_write == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_write(up, vp, mp, fstype);

	return (error);
}

int
uop_snapshot_read(up, vp, mp, fstype)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype;
{
	struct uop_snapshot_read_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(uops.uop_snapshot_read == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_read(up, vp, mp, fstype);

	return (error);
}

int
uop_snapshot_delete(up, vp, mp, fstype)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype;
{
	struct uop_snapshot_delete_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(uops.uop_snapshot_delete == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_delete(up, vp, mp, fstype);

	return (error);
}

int
uop_snapshot_commit(up, vp, mp, fstype)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype;
{
	struct uop_snapshot_commit_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(uops.uop_snapshot_commit == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_commit(up, vp, mp, fstype);

	return (error);
}
