# UFS211 (2.11BSD's Original Filesystem):
Need Touching up:
- ufs211_quota: needs alot of work
	- needs code specifically from quota_kern.c & quota_sys.c 
	- alot of overlaping code with ufs quota's
	- May be a potential case for the merger of boilerplate code.
	
Potential bugs:
- Where routines use to use the old bread(dev, blkno) and getblk(dev, blkno).
  The process of updating them to the vfs version may have caused bugs in those ufs211 routines.

Missing ufs211 vnops:
- valloc
- vfree