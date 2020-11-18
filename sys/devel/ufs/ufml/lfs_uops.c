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

/* To Be moved into LFS when ready */

#include <sys/user.h>
#include <sys/ufs/ufs/inode.h>

#include "devel/ufs/ufml/ufml.h"
#include "devel/ufs/ufml/ufml_meta.h"
#include "devel/ufs/ufml/ufml_extern.h"
#include "devel/ufs/ufml/ufml_ops.h"

/* LFS's UFML-based vector operations */
int
lfs_archive(ap)
	struct uop_archive_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_extract(ap)
	struct uop_extract_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);
	return (0);
}

int
lfs_compress(ap)
	struct uop_compress_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_decompress(ap)
	struct uop_decompress_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_encrypt(ap)
	struct uop_encrypt_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_decrypt(ap)
	struct uop_decrypt_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_snapshot_write(ap)
	struct uop_snapshot_write_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_snapshot_read(ap)
	struct uop_snapshot_read_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_snapshot_delete(ap)
	struct uop_snapshot_delete_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

int
lfs_snapshot_commit(ap)
	struct uop_snapshot_commit_args *ap;
{
	struct inode *ip = UFMLTOLFS(ap->a_vp);

	return (0);
}

struct ufmlops lfsuops = {
	.uop_archive 			=	lfs_archive,			/* archive */
	.uop_extract 			=	lfs_extract,			/* extract */
	.uop_compress 			=	lfs_compress,			/* compress */
	.uop_decompress 		=	lfs_decompress,			/* decompress */
	.uop_encrypt 			=	lfs_encrypt,			/* encrypt */
	.uop_decrypt 			=	lfs_decrypt,			/* decrypt */
	.uop_snapshot_write 	= 	lfs_snapshot_write,		/* snapshot_write */
	.uop_snapshot_read 		= 	lfs_snapshot_read,		/* snapshot_read */
	.uop_snapshot_delete 	= 	lfs_snapshot_delete,	/* snapshot_delete */
	.uop_snapshot_commit 	= 	lfs_snapshot_commit,	/* snapshot_commit */
	(struct ufmlops *)NULL 	= 	(int(*)())NULL
};
