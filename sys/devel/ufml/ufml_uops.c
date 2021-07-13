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

#include "devel/ufs/ufml/ufml.h"
#include "devel/ufs/ufml/ufml_meta.h"
#include "devel/ufs/ufml/ufml_extern.h"
#include "devel/ufs/ufml/ufml_ops.h"

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
uop_archive(up, vp)
	struct ufml_node *up;
	struct vnode *vp;
{
	struct uop_archive_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_up = up;
	ap.a_vp = vp;

	if(uops.uop_archive == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_archive(up, vp);

	return (error);
}

int
uop_extract(vp)
	struct vnode *vp;
{
	struct uop_extract_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_extract == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_extract(vp);

	return (error);
}

int
uop_compress(vp)
	struct vnode *vp;
{
	struct uop_compress_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_compress == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_compress(vp);

	return (error);
}

int
uop_decompress(vp)
	struct vnode *vp;
{
	struct uop_decompress_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_decompress == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_decompress(vp);

	return (error);
}

int
uop_encrypt(vp)
	struct vnode *vp;
{
	struct uop_encrypt_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_encrypt == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_encrypt(vp);

	return (error);
}

int
uop_decrypt(vp)
	struct vnode *vp;
{
	struct uop_decrypt_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_decrypt == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_decrypt(vp);

	return (error);
}

int
uop_snapshot_write(vp)
	struct vnode *vp;
{
	struct uop_snapshot_write_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_snapshot_write == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_write(vp);

	return (error);
}

int
uop_snapshot_read(vp)
	struct vnode *vp;
{
	struct uop_snapshot_read_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_snapshot_read == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_read(vp);

	return (error);
}

int
uop_snapshot_delete(vp)
	struct vnode *vp;
{
	struct uop_snapshot_delete_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_snapshot_delete == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_delete(vp);

	return (error);
}

int
uop_snapshot_commit(vp)
	struct vnode *vp;
{
	struct uop_snapshot_commit_args ap;
	int error;

	ap.a_head.a_ops = &uops;
	ap.a_vp = vp;

	if(uops.uop_snapshot_commit == NULL) {
		return (EOPNOTSUPP);
	}

	error = uops.uop_snapshot_commit(vp);

	return (error);
}
