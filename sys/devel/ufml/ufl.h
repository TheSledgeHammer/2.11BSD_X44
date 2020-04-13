/*
 * ufl.h
 *
 *  Created on: 30 Jan 2020
 *      Author: marti
 */

#ifndef SYS_UFS_UFLFS_UFL_H_
#define SYS_UFS_UFLFS_UFL_H_

struct ufl_dir {

	char 				*ufl_elem; 		/* path element */
	u_long 				ufl_entry;
	u_long 				ufl_gen;
	u_long 				ufl_mentry;
	u_long 				ufl_mgen;
	u_int64_t 			ufl_size;
	u_int64_t 			ufl_qid;

	u_int32_t 			ufl_uid;		/* File owner. */
	u_int32_t 			ufl_gid;		/* File group. */
	u_int32_t 			ufl_mid;		/* Last Modified by. */
	int32_t				ufl_atime;		/* Last access time. */
	int32_t				ufl_atimensec;	/* Last access time. */
	int32_t				ufl_mtime;		/* Last modified time. */
	int32_t				ufl_mtimensec;	/* Last modified time. */
	int32_t				ufl_ctime;		/* Last inode change time. */
	int32_t				ufl_ctimensec;	/* Last inode change time. */

	int 				ufl_qidspace;
	u_int64_t 			ufl_qidoffset;
	u_int64_t 			ufl_qidmax;
};

struct ufl_fs {
	char 				name[128];
	struct dirhash 		*hash;
	struct ufl_file 	*root;
	int 				mode;
	u_int32_t 			bsize;		/* file system block size */
	u_int64_t 			qid;
};

struct ufl_file {
	struct ufl_file 	*parent;
	struct ufl_file 	*sibling;
	struct ufl_file 	*children;

	u_int32_t 			ref;
	u_int32_t 			partial;
	u_int32_t 			removed;
	u_int32_t 			dirty;
	u_int32_t 			boffset;

	struct ufl_fs  		*fs;
	struct ufl_dir 		*dir;

	struct lfs 			*source;
	struct lfs 			*msource;

	int 				mode;
	u_int64_t 			qidoffset;
};

struct ufl_metaentry {
	unsigned char 		*p;
	u_short 			size;
};

struct ufl_mblock {
	int 				maxsize;	/* size of block */
	int 				size;		/* size used */
	int 				free;		/* free space within used size */
	int 				maxindex;	/* entries allocated for table */
	int 				nindex;		/* amount of table used */
	int 				unbotch;
	u_char 				*buf;
};


#endif /* SYS_UFS_UFLFS_UFL_H_ */
