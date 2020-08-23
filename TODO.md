A list of what needs work in no order. To obtain a successful build.
NOTE: Don't hesitate to add to this list. :)

#TODO:
#General:
- Cross-Compiler
	- Build Toolchain
- Makefiles:
	- Use .mk for definitions
- Bug Fixes/ missing critical content

#usr/:
##lib:
- csu:
	- elf relocation:
		- see below: i386
		
##libexec:
- ld.elf_so:
	- rtld
		
#usr/sys/:
##conf:
- check

##kern:
- swapmap & coremap: 
	- Check if setup? (if not setup)
	
##arch:
- i386/x86: (Merged under i386)
	- conf
	- cpu_info
		- cpu_setup: basics implemented
	- pae: implemented but not fully integrated
		- missing in machdep.c & locore.s
	- devices: 
		- see below: dev

##devel: (planned)
- Code planned for future integration

##dev:(Derived code mostly from NetBSD 1.4)
- isa: 
	- bloated: contains unused code, generally for other platforms
- ic:
	- bloated: contains unused code, generally for other platforms

##lib:
- libkern:
	- add Makefile.inc
	
##net:
Of Interest Todo:
- 2.11BSD's networking stack
	- Needs Updating to support: i.e. ipv6, firewall/packet filter

##stand:
- boot:
	- efi support

Missing in various source files (mostly /sys/dev)
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
