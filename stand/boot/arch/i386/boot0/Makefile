# $FreeBSD$

.include "${.CURDIR}/../Makefile.inc" 

PROG=		boot0
STRIP=
BINMODE=    ${NOBINMODE}
SRCS=		${PROG}.S

# The default set of flags compiled into boot0.  This enables update (writing
# the modified boot0 back to disk after running so that the selection made is
# saved), packet mode (detect and use the BIOS EDD extensions if we try to
# boot past the 1024 cylinder liimt), and booting from all valid slices.
BOOT_BOOT0_FLAGS?=	0xf

# The number of timer ticks to wait for a keypress before assuming the default
# selection.  Since there are 18.2 ticks per second, the default value of
# 0xb6 (182d) corresponds to 10 seconds.
BOOT_BOOT0_TICKS?=	0xb6

# The base address that we the boot0 code to to run it.  Don't change this
# unless you are glutton for punishment.
BOOT_BOOT0_ORG?=	0x600

CFLAGS+=-DFLAGS=${BOOT_BOOT0_FLAGS} -DTICKS=${BOOT_BOOT0_TICKS}

LDFLAGS+=${LDFLAGS_BIN}

.include <bsd.prog.mk>
