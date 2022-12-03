# UFS211 (2.11BSD's Original Filesystem):
TODO:
- Finish vfsops.c & vnops.c compiler fixes.
- Mounting/Unmounting of ufs211.
- vfsops.c statfs. Calculation of certain stats.
- Macro's in ufs211_fs.h need revision.
- Code tidy-up.
	- example: inode: i_flags.
	
Potential bugs:
- Where routines use to use the old bread(dev, blkno) and getblk(dev, blkno).
  The process of updating them to the vfs version may have caused bugs in those ufs211 routines.

Missing ufs211 vnops:
- valloc
- vfree