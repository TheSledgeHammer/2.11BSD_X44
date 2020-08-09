A list of what needs work in no order. To obtain a successful build.
NOTE: Don't hesitate to add to this list. :)

TODO:
General:
- Cross-Compiler
	- Build Toolchain
- Makefiles
- Bug Fixes

User:
- Everything (except lib & share):
	- Contains various Tidbits

Kernel (/sys):
arch:
- i386:
	- conf

dev:
- isa
- ic

devel: (planned)
- Code planned for future integration

Of Interest Todo:
- ifdef INET: remanents of 2.11BSD's networking stack overlay (keep in place for now)
	- Needs Updating to include later Networking capabilities


Missing in various source files (mostly sys/dev)
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
