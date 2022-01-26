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

#ifndef UFS_UFML_EXTERN_H_
#define UFS_UFML_EXTERN_H_

int	ufml_badop(void *);
int ufml_ebadf(void);
int	ufml_lookup(struct vop_lookup_args *);
int	ufml_create(struct vop_create_args *);
int	ufml_mknod(struct vop_mknod_args *);
int	ufml_access(struct vop_access_args *);
int	ufml_getattr(struct vop_getattr_args *);
int	ufml_getattr(struct vop_setattr_args *);
int	ufml_remove(struct vop_remove_args *);
int	ufml_link(struct vop_link_args *);
int	ufml_rename(struct vop_rename_args *);
int	ufml_mkdir(struct vop_mkdir_args *);
int	ufml_rmdir(struct vop_rmdir_args *);
int	ufml_symlink(struct vop_symlink_args *);
int	ufml_abortop(struct vop_abortop_args *);
int	ufml_inactive(struct vop_inactive_args *);
int	ufml_reclaim(struct vop_reclaim_args *);
int	ufml_lock(struct vop_lock_args *);
int	ufml_unlock(struct vop_unlock_args *);
int	ufml_strategy(struct vop_strategy_args *);
int	ufml_print(struct vop_print_args *);
int	ufml_islocked(struct vop_islocked_args *);
int	ufml_bwrite(struct vop_bwrite_args *);

/* ufml_meta.c */
int ufml_check_filesystem(struct ufml_metadata *, int);
int ufml_check_archive(struct ufml_metadata *, int);
int ufml_check_compression(struct ufml_metadata *, int);
int ufml_check_encyrpt(struct ufml_metadata *, int);

/* ufml_archive.c */
int ufml_archive(struct uop_archive_args *);
int ufml_extract(struct uop_extract_args *);

/* ufml_compress.c */
int ufml_compress(struct uop_compress_args *);
int ufml_decompress(struct uop_decompress_args *);

/* ufml_encrypt.c */
int ufml_encrypt(struct uop_encrypt_args *);
int ufml_decrypt(struct uop_decrypt_args *);

/* ufml_snapshot.c */
int ufml_snapshot_write(struct uop_snapshot_write_args *);
int ufml_snapshot_read(struct uop_snapshot_read_args *);
int ufml_snapshot_delete(struct uop_snapshot_delete_args *);
int ufml_snapshot_commit(struct uop_snapshot_commit_args *);

#endif /* UFS_UFML_EXTERN_H_ */
