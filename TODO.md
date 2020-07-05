TODO:
- Build Toolchain (Cross-Compile): NetBSD's
	- Attempt build on NetBSD (Can't build on Linux without a Toolchain)

- arch:
	- i386:
		- consinit.c (blank out for now)
		- machdep.c: cpu_reboot, cpu_reset
		- vm_machdep.c: u->u_procp->p_p0br: (no reference in 4.4BSD-Lite2)
			- 4.3BSD Reno/ 4.4BSD Remanent: once in struct proc. Obsolete?? 

miscfs:
- LOFS: 4.4BSD: Provides a functioning stack filesystem
	- Merge best parts with NULLFS for reimplementation LOFS2

Kern:
- kern_xxx.c: reboot: needs fixing
- kern_physio.c: Incomplete: getphysbuf, putphysbuf (NetBSD 1.3)

VM:
- Copy: Devel vm_map without amap to vm

Of Interest Todo:
- ifdef INET: remanents of 2.11BSD's networking stack overlay (keep in place for now)
	- Used in later BSD's
	- Update use cases
- syscallargs.h: Clean up: Large number of args unused.

To Add:
include <net/if_atm.h>
include <net/if_dl.h>
include <net/if_fddi.h>
include <net/if_media.h>
include <net/if_types.h>
include <netinet/if_atm.h>
include <netinet/if_inarp.h>
include <netatalk/at.h>
include <netccitt/x25.h>
include <netccitt/pk.h>
include <netccitt/pk_var.h>
include <netccitt/pk_extern.h>
include <netnatm/natm.h>

Revise: Makefiles & mk (folder: /share/mk)
- Use of NetBSD's 1.3 "new" build-chain. (incompatible? GCC)
- Move to OpenBSD's build-chain (Higher Success)

- Makefiles
	- Dev (Devices)
	- Conf: Includes "To Add"
	- GENERIC, files


	