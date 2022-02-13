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
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/null.h>

#include <ufs/ufml/ufml.h>
#include <ufs/ufml/ufml_extern.h>
#include <ufs/ufml/ufml_meta.h>
#include <ufs/ufml/ufml_ops.h>


#define UNOP(up, uop_field)					\
	((up)->ufml_op->uop_field)
#define DO_UOPS(uops, error, ap, uop_field)	\
	error = uops->uop_field(ap)

struct ufmlops default_uops = {
		.uop_archive = uop_archive,
		.uop_extract = uop_extract,
		.uop_compress = uop_compress,
		.uop_decompress = uop_decompress,
		.uop_encrypt = uop_encrypt,
		.uop_decrypt = uop_decrypt,
		.uop_snapshot_write = uop_snapshot_write,
		.uop_snapshot_read = uop_snapshot_read,
		.uop_snapshot_delete = uop_snapshot_delete,
		.uop_snapshot_commit = uop_snapshot_commit
};

int
uop_archive(up, vp, mp, fstype, archive)
	struct ufml_node *up;
	struct vnode *vp;
	struct mount *mp;
	int fstype, archive;
{
	struct uop_archive_args ap;
	int error;

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = archive;

	if(UNOP(up, uop_archive) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_archive);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = archive;

	if(UNOP(up, uop_extract) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_extract);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = compress;

	if(UNOP(up, uop_compress) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_compress);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = compress;

	if(UNOP(up, uop_decompress) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_decompress);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = encrypt;
	if(UNOP(up, uop_encrypt) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_encrypt);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;
	ap.a_type = encrypt;

	if(UNOP(up, uop_decrypt) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_decrypt);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(UNOP(up, uop_snapshot_write) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_snapshot_write);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(UNOP(up, uop_snapshot_read) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_snapshot_read);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(UNOP(up, uop_snapshot_delete) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_snapshot_delete);

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

	ap.a_head.a_ops = up->ufml_op;
	ap.a_up = up;
	ap.a_vp = vp;
	ap.a_mp = mp;
	ap.a_fstype = fstype;

	if(UNOP(up, uop_snapshot_commit) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_UOPS(up->ufml_op, mp, &ap, uop_snapshot_commit);

	return (error);
}
