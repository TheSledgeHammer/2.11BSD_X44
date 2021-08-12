# UFS211 (2.11BSD's Original Filesystem):
Need Touching up:
- ufs211_quota: 
	(original quota code was scattered throughout kernel)
	- needs alot of work. 
	- missing quota related code
	
Potential bugs:
- Below functions in some cases used the old bread(dev, blkno) and getblk(dev, blkno).
Updating them to the vfs version may have caused bugs in those ufs211 routines.

Double Check:
vnops
- ufs211_update
- ufs211_fsync
- ufs211_truncate

Missing ufs211 vnops:
- valloc
- vfree