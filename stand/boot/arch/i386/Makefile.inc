.include <bsd.loader.mk>

LOADER_ADDRESS?=	0x200000
LDFLAGS+=	    	-nostdlib
LDFLAGS.lld+=		-Wl,--no-rosegment
MK_PIE:=	    	no

# BTX components
BTXDIR=			${BOOTARCH}/i386/btx
BTXBTXDIR=		${BTXDIR}/btx
BTXLDRDIR=		${BTXDIR}/btxldr
BTXLIBDIR=		${BTXDIR}/lib

BTXLDR=			${BTXLDRDIR}
BTXKERN=		${BTXBTXDIR}
BTXCRT=			${BTXLIBDIR}/crt0.o

CFLAGS+=		-I${BTXLIB}

# compact binary with no padding between text, data, bss
LDSCRIPT=	    ${BOOTARCH}/i386/boot.ldscript
LDFLAGS_ORG=	-Wl,--defsym,ORG=${ORG},-T,${LDSCRIPT}
LDFLAGS_BIN=	-e start ${LDFLAGS_ORG} -Wl,-N,-S,--oformat,binary
LD_FLAGS_BIN=	-static -N --gc-sections

