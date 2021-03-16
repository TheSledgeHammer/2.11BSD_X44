# UFS211 (2.11BSD's Original Filesystem):
Need Touching up:
- update:
	- buflists to be inline with other filesystems
- ufs211_inode: 
	- ihash
- ufs211_quota: 
	(original quota code was scattered throughout kernel)
	- needs alot of work. 
	- missing quota related code
	
- ufs211_mount/disksubr: review needed!
	- is the method needed for ufs211 to work
	OR
	- is it an old method that:
		 A) can be reused elsewhere
		 B) is deprecated
	
- ufs211_lookup: 
	- vnode equivalents of nchinval(dev) & nchinit()

To Complete:
- ufs211_vnops
- ufs211_vfsops

	


