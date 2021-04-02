# Advanced Volume Manager (AdvVM):

## General:
- Remove Hardcoded limits implemented from Vinum
	- see how LVM2 does it
- Memory Pool Allocation of Volumes, Domains & Filesets
	- Must be able to Expand or Shrink
	- see how LVM2 does it
- Filesystem Mapping: (Implemented in Userspace?)
- Generic Operations for each type (aka Volumes, Domains & Filesets):
	- i.e. create, destroy, add, delete... etc
	
- Virtual Disk Structure

## Volumes:
- Volume list:
	- currently head of volume list is in domains
	- insertion, deletion & travesal of volumes: incomplete (lack of functionality/integration between a domain & volume)

## Domains:
- Volume & Fileset interaction
	- retrieve respective lists

## Filesets:
- Bitmaps/Hashmaps for Tag Directory & File Directory
- Fileset list:
	- Same issue as for Volumes. 
		- A solution for volumes should work with Filesets too.
