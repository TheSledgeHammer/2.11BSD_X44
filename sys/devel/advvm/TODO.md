# Advanced Volume Manager (AdvVM):

## General:
- Remove Hardcoded limits implemented from Vinum
	- see how LVM2 does it
- Implement device configuration in advvm.c

- Storage space allocation updates:
	- Extents could be better utilized for suballocation and storage i.e. domains and filesets.
	- The extents could really replace the block structure.
	- 

## Device Mapper (aka dm):
- Setup interfaces and api's between the dm and advvm.
- Update the internal infrasture for improved interoperability between both the dm and advvm.
- add target: delay, flakey, mirror, snapshot & stripe
- device-mapper.c: merge with advvm.c

## Volumes:
- Block and Labels need tweaking

## Domains:
- Domains need to be initialized first!

## Filesets:
- 

## Virtual_Disks: (advvm_vdisk.c/.h)
- To implement routines & functions
