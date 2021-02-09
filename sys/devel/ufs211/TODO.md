# UFS211 (2.11BSD's Original Filesystem):
Need Touching up:
- update buflists
- ufs211_quota: missing other quota related code
- ufs211_mount: quota_kern.c refs		
- ufs211_lookup: void nchinval(dev) & void nchinit() are replaced with vnode equivalents

Remove: pdp11 mapin & mapout refs from following
- ufs211_alloc
- ufs211_bmap:
- ufs211_disksubr:
- ufs211_inode: 
- ufs211_lookup
- ufs211_mount
- ufs211_vfsops

To Complete:
- ufs211_vnops
- ufs211_vfsops

