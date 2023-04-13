# Advanced Volume Manager (AdvVM):

## General:
- Remove Hardcoded limits implemented from Vinum
	- see how LVM2 does it
- Implement device configuration in advvm.c

## Device Mapper (aka dm):
- Setup interfaces and api's between the dm and advvm.
- Update the internal infrasture for improved interoperability between both the dm and advvm.
- add target: delay, flakey, mirror, snapshot & stripe
- device-mapper.c: merge with advvm.c

## Volumes:


## Domains:


## Filesets:
- Fix fileset's tag & file directory associative hash.

## Virtual_Disks: (advvm_vdisk.c/.h)
- To implement routines & functions
