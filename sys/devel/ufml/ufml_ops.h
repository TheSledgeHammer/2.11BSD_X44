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

#ifndef _UFS_UFML_OPS_H_
#define _UFS_UFML_OPS_H_

struct ufmlops {
	int (*uop_archive)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype, int type);
	int (*uop_extract)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype, int type);
	int (*uop_compress)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype, int type);
	int (*uop_decompress)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype, int type);
	int (*uop_encrypt)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype, int type);
	int (*uop_decrypt)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype, int type);
	int (*uop_snapshot_write)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype);
	int (*uop_snapshot_read)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype);
	int (*uop_snapshot_delete)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype);
	int (*uop_snapshot_commit)(struct ufml_node *up, struct vnode *vp, struct mount *mp, int fstype);
};

#define UOP_ARCHIVE(up, vp, mp, fstype, type)		(*((up)->ufml_op->uop_archive))(up, vp, mp, fstype, type)
#define UOP_EXTRACT(up, vp, mp, fstype, type)		(*((up)->ufml_op->uop_extract))(up, vp, mp, fstype, type)
#define UOP_COMPRESS(up, vp, mp, fstype, type)		(*((up)->ufml_op->uop_compress))(up, vp, mp, fstype, type)
#define UOP_DECOMPRESS(up, vp, mp, fstype, type)	(*((up)->ufml_op->uop_decompress))(up, vp, mp, fstype, type)
#define UOP_ENCRYPT(up, vp, mp, fstype, type)		(*((up)->ufml_op->uop_encrypt))(up, vp, mp, fstype, type)
#define UOP_DECRYPT(up, vp, mp, fstype, type)		(*((up)->ufml_op->uop_decrypt))(up, vp, mp, fstype, type)
#define UOP_SNAPSHOT_WRITE(up, vp, mp, fstype)		(*((up)->ufml_op->uop_snapshot_write))(up, vp, mp, fstype)
#define UOP_SNAPSHOT_READ(up, vp, mp, fstype)		(*((up)->ufml_op->uop_snapshot_read))(up, vp, mp, fstype)
#define UOP_SNAPSHOT_DELETE(up, vp, mp, fstype)		(*((up)->ufml_op->uop_snapshot_delete))(up, vp, mp, fstype)
#define UOP_SNAPSHOT_COMMIT(up, vp, mp, fstype)		(*((up)->ufml_op->uop_snapshot_commit))(up, vp, mp, fstype)

struct uop_generic_args {
	struct ufmlops 				*a_ops;
};

struct uop_archive_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
	int							a_type;
};

struct uop_extract_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
	int							a_type;
};

struct uop_compress_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
	int							a_type;
};

struct uop_decompress_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
	int							a_type;
};

struct uop_encrypt_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
	int							a_type;
};

struct uop_decrypt_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
	int							a_type;
};

struct uop_snapshot_write_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
};

struct uop_snapshot_read_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
};

struct uop_snapshot_delete_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
};

struct uop_snapshot_commit_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
	struct mount				*a_mp;
	int							a_fstype;
};

extern struct ufmlops uops;

#endif /* _UFS_UFML_OPS_H_ */
