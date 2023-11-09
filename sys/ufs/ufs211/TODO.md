# UFS211 (2.11BSD's Original Filesystem):
TODO:
- Mounting/Unmounting of ufs211.
- vfsops.c statfs. Calculation of certain stats.
- Fix ufs211_mountfs
	
Potential bugs:
- Where routines use to use the old bread(dev, blkno) and getblk(dev, blkno).
  The process of updating them to the vfs version may have caused bugs in those ufs211 routines.

