# UFML FS (UFS / FFS / MFS / LFS):

A Modular Filesystem Layer For Content Address Based Storage.
Inspired from Plan 9's Fossil & Venti Filesystems.

UFML: 
- LOFS/NULLFS Based
- BSD Native:
	- UFML Upper layer:
		- Features: Format Support (To Implement)
			- Archive: tar, cpio, ar
			- Encryption: blowfish, camellia, serpent, sha1, sha2, twofish
			- Compression: bzip2, gzip, lzip, lzma, xz
			- Snapshots
	- UFML Lower Layer: (To Support)
		- UFS
		- FFS
		- MFS
		- LFS
		
- Third-Party:
	- UFML Upper layer:
		- Archive: vac		(Required to support Fossil and Venti)
	- UFML Lower Layer:
		- HAMMER1/2? 	 	(DragonflyBSD)
		- PUFFS/FUSE?  	(NetBSD/FreeBSD/OpenBSD/DragonflyBSD)
		- FOSSIL			(Plan9/Inferno)
		- VENTI				(Plan9/Inferno)
	
Design Objectives of UFML:
- A modular filesystem layer that can provide additional features to UFS, FFS, MFS & LFS.
- Allow the interaction between one or more of the supported filesystems. Similar to how Fossil & Venti work on Plan 9.
