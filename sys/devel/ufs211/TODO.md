# UFS211 (2.11BSD's Original Filesystem):
Need Touching up:
- ufs211_quota: 
	(original quota code was scattered throughout kernel)
	- needs alot of work. 
	- missing quota related code
	
To Complete:
- ufs211_vfsops

	
Missing ufs211 vnops:
- lease_check
- fsync
- seek
- blkatoff
- valloc
- vfree
- bwrite
- update
- truncate
