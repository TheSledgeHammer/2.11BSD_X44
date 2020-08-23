# $FreeBSD$

#.include <src.opts.mk>

S.yes+=					libsa

S.${MK_DLOADER}+=		dloader
S.yes+=					man

.include <bsd.arch.inc.mk>

S.${MK_EFI}+=			efi
S.${MK_LOADER_UBOOT}+=	uboot

.if exists(${.CURDIR}/${MACHINE}/.)
S.yes+=         ${MACHINE}
.endif

# Build the actual subdir list from S.yes, adding in the 32-bit
# variant if necessary.
.for _x in ${S.yes}
SUBDIR+=${_x}
.endfor

.include <bsd.subdir.mk>