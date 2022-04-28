# HTBC (HTree Blockchain):
- Reduce hash tree to only what is needed:
	- To be simplified htbc_inode, htbc_mfs, htbc_inode_ext, htbc_searchslot
	- All of the above structures are leftovers from ext2fs, with most there 
	infrastructure relating to the ext2fs directory and block layout.

	