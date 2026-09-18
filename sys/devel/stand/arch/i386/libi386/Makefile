# $FreeBSD$

.include <bsd.init.mk>
.include "${.CURDIR}/../Makefile.inc" 

LIB=	i386

SRCS=	bio.c bioscd.c biosdisk.c biosmem.c biossmap.c 	\
		bootexec.c bootinfo.c bootload.c comconsole.c 	\
		devicename.c i386_autoload.c i386_copy.c 		\
		i386_reboot.c nullconsole.c pread.c 			\
		relocater_tramp.S _setjmp.S smbios.c 			\
		spinconsole.c time.c vidconsole.c
		
BOOT_COMCONSOLE_PORT?= 0x3f8
CPPFLAGS+=	-DCOMPORT=${BOOT_COMCONSOLE_PORT}

BOOT_COMCONSOLE_SPEED?= 9600
CPPFLAGS+=	-DCOMSPEED=${BOOT_COMCONSOLE_SPEED}

.ifdef(BOOT_BIOSDISK_DEBUG)
# Make the disk code more talkative
CPPFLAGS+= -DDISK_DEBUG
.endif

.if !defined(BOOT_HIDE_SERIAL_NUMBERS)
# Export serial numbers, UUID, and asset tag from loader.
CPPFLAGS+= -DSMBIOS_SERIAL_NUMBERS
.if defined(BOOT_LITTLE_ENDIAN_UUID)
# Use little-endian UUID format as defined in SMBIOS 2.6.
CPPFLAGS+= -DSMBIOS_LITTLE_ENDIAN_UUID
.elif defined(BOOT_NETWORK_ENDIAN_UUID)
# Use network-endian UUID format for backward compatibility.
CPPFLAGS+= -DSMBIOS_NETWORK_ENDIAN_UUID
.endif
.endif

# Include simple terminal emulation (cons25-compatible)
CPPFLAGS+= 	-DTERM_EMU

# XXX: make alloca() useable
CPPFLAGS+= 	-Dalloca=__builtin_alloca

#CPPFLAGS+=	-I${COMMONSRC} -I${BOOTSRC}/i386/common 
			#-I${SYSDIR}/dev/acpica/include

.include "${.CURDIR}/../../../../common/Makefile.common" 

.include <bsd.lib.mk>