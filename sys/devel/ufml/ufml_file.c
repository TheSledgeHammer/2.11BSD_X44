/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef SYS_DEVEL_UFML_C_
#define SYS_DEVEL_UFML_C_


struct fs_ops ufml_fsops = {
	"ufml"
};

struct file {
	off_t					f_seekp;				/* seek pointer */
	struct ufml_metadata	*f_fs;
	struct ufml_node 		*f_ip;
	daddr_t					f_buf_blkno;			/* block number of data block */
};

int
read_lower(ino_t inumber, struct open_file *f)
{
	struct file *fp = (struct file *)f->f_fsdata;
	struct ufml_node *node = fp->f_fs;
	char *buf;
	size_t rsize;
	int rc;

	if (node == NULL) {
		panic("node == NULL");
	}

	switch(node->ufml_meta->ufml_filesystem) {
	case UFML_FFS:
		register struct inode *ufs = UFMLTOUFS(node->ufml_vnode);
		register struct fs *ffs = UFMLTOFFS(node->ufml_vnode);
		buf = malloc(ffs->fs_bsize);
		twiddle();
		rc = (f->f_dev->dv_strategy)(f->f_devdata, F_READ, fsbtodb(ffs, ino_to_fsba(ffs, inumber)), ffs->fs_bsize, buf, &rsize);
		if (rc) {
			goto out;
		}
		if (rsize != ffs->fs_bsize) {
			rc = EIO;
			goto out;
		}
		{
			if(ufs->i_ump->um_fstype == UFS1) {
				struct ufs1_dinode *dp = (struct ufs1_dinode *)buf;
				DIP(ufs, dp[ino_to_fsbo(ffs, inumber)]);
			} else {
				struct ufs2_dinode *dp = (struct ufs2_dinode *)buf;
				DIP(ufs, dp[ino_to_fsbo(ffs, inumber)]);
			}
		}
		break;

	case UFML_LFS:
		register struct lfs *lfs = UFMLTOLFS(node->ufml_vnode);
		buf = malloc(lfs->lfs_bsize);
		twiddle();
		rc = (f->f_dev->dv_strategy)(f->f_devdata, F_READ, fsbtodb(lfs, lblkno(lfs, inumber)), lfs->lfs_bsize, buf, &rsize);
		if (rc) {
			goto out;
		}
		if (rsize != lfs->lfs_bsize) {
			rc = EIO;
			goto out;
		}
		break;
	}

out:
	free(buf);
	return (rc);
}

int
ufml_open(path, f)
	const char *path;
	struct open_file *f;
{
	struct file *fp;

	fp = malloc(sizeof(struct file));
	bzero(fp, sizeof(struct file));
	f->f_fsdata = (void *)fp;
}

int
ufml_close(f)
	struct open_file *f;
{

}

int
ufml_read(f, start, size, resid)
	struct open_file *f;
	char *start;
	u_int size;
	u_int *resid;	/* out */
{

}

int
ufml_write(f, start, size, resid)
	struct open_file *f;
	char *start;
	u_int size;
	u_int *resid;	/* out */
{
	return (EROFS);
}

off_t
ufml_seek(f, offset, where)
	struct open_file *f;
	off_t offset;
	int where;
{

}

int
ufml_stat(f, sb)
	struct open_file *f;
	struct stat *sb;
{

}

#endif /* SYS_DEVEL_UFML_C_ */
