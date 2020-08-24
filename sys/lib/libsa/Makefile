#	$NetBSD: Makefile,v 1.13 1996/10/02 16:19:51 ws Exp $

LIB=	sa

SA_INCLUDE_NET?= no			# Netboot via TFTP, NFS (No NFS Support)
SA_USE_LOADFILE?= no		# Generic executable loading support
SA_ENABLE_BIENDIAN?= no		# Biendian support
							# (currently only for loadfile_elf*)
# stand routines
SRCS+=	alloc.c bcopy.c bzero.c errno.c exit.c getfile.c \ 
		gets.c memcmp.c memcpy.c memmove.c memset.c panic.c \
		printf.c snprintf.c strerror.c strchr.c twiddle.c

# io routines
SRCS+=	close.c dev.c disklabel.c ls.c \
		lseek.c open.c read.c stat.c write.c

.if (${SA_USE_LOADFILE} == "yes")
SRCS+=	loadfile.c loadfile_ecoff.c loadfile_elf32.c loadfile_elf64.c lookup_elf32.c loadfile_xcoff32.c loadfile_xcoff64.c
.if (${MACHINE_CPU} != "mips")
SRCS+=	loadfile_aout.c
.endif
.endif

# boot filesystems
SRCS+=	ufs.c

.if (${SA_INCLUDE_NET} == "yes")
# network routines
SRCS+=	net.c netif.c rpc.c

# network info services:
SRCS+=	bootp.c bootparam.c
.endif

.if (${SA_ENABLE_BIENDIAN} == "yes")
CPPFLAGS+=	-DLIBSA_BIENDIAN_SUPPORT
SRCS+=		byteorder.c
.endif

.include <bsd.lib.mk>