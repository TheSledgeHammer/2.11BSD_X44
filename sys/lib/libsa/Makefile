#	$NetBSD: Makefile,v 1.13 1996/10/02 16:19:51 ws Exp $

LIB=	sa

SA_INCLUDE_NET?= no			# Netboot via TFTP, NFS (No NFS Support)
SA_USE_LOADFILE?= no		# Generic executable loading support
SA_ENABLE_BIENDIAN?= no		# Biendian support
							# (currently only for loadfile_elf*)
# stand routines
SRCS+=	alloc.c bcopy.c bzero.c errno.c exit.c getfile.c \ 
		gets.c memcmp.c memcpy.c memmove.c memset.c panic.c \
		printf.c snprintf.c strdup.c strspn.c strtol.c twiddle.c \
		zalloc_malloc.c zalloc.c
		
#SRCS+= 	uuid_from_string.c uuid_to_string.c 

# io routines
SRCS+=	close.c closeall.c dev.c ls.c lseek.c open.c read.c stat.c \
		write.c

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

# /usr/stand/boot support
INCSYMLINKS=																	\
	sys/lib/libsa/bootp.h			/usr/include/stand/libsa/bootp.h 			\
	sys/lib/libsa/bootparam.h		/usr/include/stand/libsa/bootparam.h 		\
	sys/lib/libsa/byteorder.h		/usr/include/stand/libsa/byteorder.h 		\				
	sys/lib/libsa/environment.h		/usr/include/stand/libsa/environment.h 		\
	sys/lib/libsa/iodesc.h			/usr/include/stand/libsa/iodesc.h 			\ 
	sys/lib/libsa/loadfile.h		/usr/include/stand/libsa/loadfile.h 		\
	sys/lib/libsa/net.h				/usr/include/stand/libsa/net.h 				\
	sys/lib/libsa/rpc.h				/usr/include/stand/libsa/rpc.h 				\ 
	sys/lib/libsa/rpcv2.h			/usr/include/stand/libsa/rpcv2.h 			\
	sys/lib/libsa/saerrno.h			/usr/include/stand/libsa/saerrno.h 			\
	sys/lib/libsa/saio.h			/usr/include/stand/libsa/saio.h 			\
	sys/lib/libsa/saioctl.h			/usr/include/stand/libsa/saioctl.h 			\
	sys/lib/libsa/stand.h			/usr/include/stand/libsa/stand.h 			\
	sys/lib/libsa/ufs.h				/usr/include/stand/libsa/ufs.h 				\
	sys/lib/libsa/zalloc_defs.h		/usr/include/stand/libsa/zalloc_defs.h 		\
	sys/lib/libsa/zalloc_mem.h		/usr/include/stand/libsa/zalloc_mem.h 		\
	sys/lib/libsa/zalloc_protos.h	/usr/include/stand/libsa/zalloc_protos.h 	\

.include <bsd.kinc.mk>