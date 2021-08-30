# UFML FS (UFS / FFS / MFS / LFS):

A Modular Filesystem Layer For Content Address Based Storage.
Inspired from Plan 9's Fossil & Venti Filesystems.

UFML:
- LOFS/NULLFS Based
- UFML Upper layer:
	- Features: Format Support (To Implement)
		- Archive: tar, cpio, ar
		- Checksums:
		- Encryption: twofish
		- Compression: bzip2, gzip, lzip, lzma, xz
		- Snapshots: 
- UFML Lower Layer: (To Support)
	- UFS
	- FFS
	- MFS
	- LFS 
	
Design Objectives of UFML:
- A modular filesystem layer that can provide additional features to UFS, FFS, MFS & LFS.
- Allow the interaction between one or more of the supported filesystems. Similar to how Fossil & Venti work on Plan 9.
