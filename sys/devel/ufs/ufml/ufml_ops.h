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
	int (*uop_archive)(struct ufml_node *up, struct vnode *vp);
	int (*uop_extract)(struct ufml_node *up, struct vnode *vp);
	int (*uop_compress)(struct ufml_node *up, struct vnode *vp);
	int (*uop_decompress)(struct ufml_node *up, struct vnode *vp);
	int (*uop_encrypt)(struct ufml_node *up, struct vnode *vp);
	int (*uop_decrypt)(struct ufml_node *up, struct vnode *vp);
	int (*uop_snapshot_write)(struct ufml_node *up, struct vnode *vp);
	int (*uop_snapshot_read)(struct ufml_node *up, struct vnode *vp);
	int (*uop_snapshot_delete)(struct ufml_node *up, struct vnode *vp);
	int (*uop_snapshot_commit)(struct ufml_node *up, struct vnode *vp);
};

#define UOP_ARCHIVE(up, vp)			(*((up)->ufml_op->uop_archive))(up, vp)
#define UOP_EXTRACT(up, vp)			(*((up)->ufml_op->uop_extract))(up, vp)
#define UOP_COMPRESS(up, vp)		(*((up)->ufml_op->uop_compress))(up, vp)
#define UOP_DECOMPRESS(up, vp)		(*((up)->ufml_op->uop_decompress))(up, vp)
#define UOP_ENCRYPT(up, vp)			(*((up)->ufml_op->uop_encrypt))(up, vp)
#define UOP_DECRYPT(up, vp)			(*((up)->ufml_op->uop_decrypt))(up, vp)
#define UOP_SNAPSHOT_WRITE(up, vp)	(*((up)->ufml_op->uop_snapshot_write))(up, vp)
#define UOP_SNAPSHOT_READ(up, vp)	(*((up)->ufml_op->uop_snapshot_read))(up, vp)
#define UOP_SNAPSHOT_DELETE(up, vp)	(*((up)->ufml_op->uop_snapshot_delete))(up, vp)
#define UOP_SNAPSHOT_COMMIT(up, vp)	(*((up)->ufml_op->uop_snapshot_commit))(up, vp)

struct uop_generic_args {
	struct ufmlops 				*a_ops;
};

struct uop_archive_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_extract_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_compress_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_decompress_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_encrypt_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_decrypt_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_snapshot_write_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_snapshot_read_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_snapshot_delete_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

struct uop_snapshot_commit_args {
	struct uop_generic_args		a_head;
	struct ufml_node 			*a_up;
	struct vnode 				*a_vp;
};

extern struct ufmlops uops;

#endif /* _UFS_UFML_OPS_H_ */
