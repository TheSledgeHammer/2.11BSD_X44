TODO:

Kern:
- subr_xxx.c (enodev, enxio, enoioctl, enosys, eopnotsupp, einval, nullop)
- sys_kern.c (used? or unused?)
- sys_net (ubadr_t)
- sys_process.c (ptrace)
- uipc_syscalls.c

Ufs:
- Add UFS
- WABL (NetBSD)

i386: (4.4BSD-Lite2)
- Doesn't contain a bootloader/ bootrom
- Only i386 relavent code to load the kernel 

Bootloader: (FreeBSD 3.0)
- Easy to Understand. Includes BTX Loader
- Provides basic bootloader for i386 & others in Future
- Easier to merge with existing 2.11/4.4BSD code
- Maintain a.out, coff & elf support

Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
