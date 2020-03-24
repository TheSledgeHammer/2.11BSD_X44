# $FreeBSD$

.include <bsd.init.mk>

LIB=			i386

SRCS=	bcd.c bio.c biosacpi.c biosdisk.c biosmem.c biospnp.c \
		biospci.c biossmap.c bootinfo.c bootinfo32.c \
		comconsole.c devicename.c relocater_tramp.S \
		i386_copy.c i386_module.c nullconsole.c \
		time.c vidconsole.c spinconsole.c
		

BOOT_COMCONSOLE_PORT?= 0x3f8
CFLAGS+=	-DCOMPORT=${BOOT_COMCONSOLE_PORT}

BOOT_COMCONSOLE_SPEED?= 9600
CFLAGS+=	-DCOMSPEED=${BOOT_COMCONSOLE_SPEED}

.ifdef(BOOT_BIOSDISK_DEBUG)
# Make the disk code more talkative
CFLAGS+= -DDISK_DEBUG
.endif

# terminal emulation
CFLAGS.vidconsole.c+= -I${SRCTOP}/sys/teken
CFLAGS.teken.c+= -I${SRCTOP}/sys/teken

# XXX: make alloca() useable
CFLAGS+= -Dalloca=__builtin_alloca

CFLAGS+=	-I${BOOTSRC}/ficl -I${BOOTSRC}/ficl/i386 \
			-I${LDRSRC} -I${BOOTSRC}/i386/common \
			-I${SYSDIR}/contrib/dev/acpica/include

# Handle FreeBSD specific %b and %D printf format specifiers
CFLAGS+= ${FORMAT_EXTENSIONS}

.include <bsd.lib.mk>